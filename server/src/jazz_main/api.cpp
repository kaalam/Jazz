/* Jazz (c) 2018-2026 kaalam.ai (The Authors of Jazz), using (under the same license):

	1. Biomodelling - The AATBlockQueue class (c) Jacques Basaldúa, 2009-2012 licensed
	  exclusively for the use in the Jazz server software.

	  Copyright 2009-2012 Jacques Basaldúa

	2. BBVA - Jazz: A lightweight analytical web server for data-driven applications.

      Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

      This product includes software developed at

      BBVA (https://www.bbva.com/)

	3. LMDB, Copyright 2011-2017 Howard Chu, Symas Corp. All rights reserved.

	  Licensed under http://www.OpenLDAP.org/license.html


	  Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	  http://www.apache.org/licenses/LICENSE-2.0

	  Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/


#include <sys/utsname.h>


#include "src/jazz_main/api.h"


namespace jazz_main
{

/*	-----------------------------------------------
	 API : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** Constructor for the API service.

	\param a_logger		A pointer to the Logger object.
	\param a_config		A pointer to the ConfigFile object.
	\param a_channels	A pointer to the Channels object.
	\param a_volatile	A pointer to the Volatile object.
	\param a_persisted	A pointer to the Persisted object.
	\param a_core		A pointer to the Core object.
	\param a_bebop		A pointer to the Bebop object.
	\param a_model		A pointer to the ModelsAPI object.
*/
API::API(pLogger	 a_logger,
		 pConfigFile a_config,
		 pChannels	 a_channels,
		 pVolatile	 a_volatile,
		 pPersisted	 a_persisted,
		 pCore		 a_core,
		 pBebop		 a_bebop,
		 pModelsAPI	 a_model) : BaseAPI(a_logger, a_config, a_channels, a_volatile, a_persisted) {

	p_core	= a_core;
	p_bebop	= a_bebop;
	p_model	= a_model;

	www	 = {};
}


API::~API() { destroy_container(); }


/** Return object ID.

	\return A string identifying the object that is especially useful to track uplifts and versions.
*/
pChar const API::id() {
    static char arr[] = "API from Jazz-" JAZZ_VERSION;
    return arr;
}


/** Starts the API service

	\return		SERVICE_NO_ERROR if successful, an error code otherwise.

	Configuration-wise the API has just two keys:

	- STATIC_HTML_AT_START: which defines a path to a tree of static objects that should be uploaded on start.
	- REMOVE_STATICS_ON_CLOSE: removes the whole database Persisted //static when this service closes.

	Besides that, this function initializes global (and object) variables used by the parser (mostly CharLUT).
*/
StatusCode API::start() {

	int ret = BaseAPI::start();	// This initializes the one-shot functionality.

	if (ret != SERVICE_NO_ERROR)
		return ret;

	BaseNames base = {};

	p_core->base_names(base);
	p_model->base_names(base);

	for (BaseNames::iterator it = base.begin(); it != base.end(); ++it) {
		int tt = TenBitsAtAddress(it->first.c_str());

		if (base_server[tt] != nullptr) {
			log_printf(LOG_ERROR, "API::start(): Base name conflict with \"%s\"", it->first.c_str());

			return SERVICE_ERROR_STARTING;
		}
		base_server[tt] = it->second;
	}

	String statics_path;

	if (get_conf_key("STATIC_HTML_AT_START", statics_path)) {
		ret = load_statics((pChar) statics_path.c_str(), (pChar) "/", 0);

		if (ret != SERVICE_NO_ERROR) {
			log_printf(LOG_ERROR, "API::start(): load_statics() failed loading \"%s\"", statics_path.c_str());

			return ret;
		}
	}

	if (!get_conf_key("REMOVE_STATICS_ON_CLOSE", remove_statics))
		remove_statics = false;

	return SERVICE_NO_ERROR;
}


/** Shuts down the Persisted Service

	\return		SERVICE_NO_ERROR if successful, an error code otherwise.

	Configuration-wise the API has just one key:

	- REMOVE_STATICS_ON_CLOSE: removes the whole database Persisted //static when this service closes.
*/
StatusCode API::shut_down() {

	StatusCode err;

	if (remove_statics) {
		Locator loc = {"lmdb", "www"};
		for (Index::iterator it = www.begin(); it != www.end(); ++it) {
			strcpy(loc.key, it->second.c_str());
			if ((err = p_persisted->remove(loc)) != SERVICE_NO_ERROR)
				log_printf(LOG_MISS, "API::shut_down(): Persisted.remove(//lmdb/www/%s) returned %d", it->second.c_str(), err);
		}
	}

	www.clear();

	return BaseAPI::shut_down();	// Closes the one-shot functionality.
}


/** Check a non-API url into and return the static object related with it.

	\param response	A valid (or error) MHD_Response pointer with the resource, status, mime, etc.
	\param p_url	The http url (that has already been checked not to start with //)
	\param get_it	If true (default), it actually gets it as a response, otherwise it just check if it exists.

	\return			Some error code or SERVICE_NO_ERROR if successful.

*/
MHD_StatusCode API::get_static(pMHD_Response &response, pChar p_url, bool get_it) {

	Index::iterator it = www.find(String(p_url));

	if (it == www.end())
		return MHD_HTTP_NOT_FOUND;

	Locator loc = {"lmdb", "www"};

	strcpy(loc.key, it->second.c_str());

	pTransaction p_txn;
	if (p_persisted->get(p_txn, loc) != SERVICE_NO_ERROR)
		return MHD_HTTP_BAD_GATEWAY;

	if (p_txn->p_block->cell_type == CELL_TYPE_STRING && p_txn->p_block->size == 1) {
		pChar p_str = p_txn->p_block->get_string(0);
		int size = strlen(p_str);

		response = MHD_create_response_from_buffer(size, p_str, MHD_RESPMEM_MUST_COPY);
	} else {
		int size = (p_txn->p_block->cell_type & 0xff)*p_txn->p_block->size;

		response = MHD_create_response_from_buffer(size, &p_txn->p_block->tensor, MHD_RESPMEM_MUST_COPY);
	}

	pChar p_att;
	if ((p_att = p_txn->p_block->get_attribute(BLOCK_ATTRIB_MIMETYPE)) != nullptr)
		MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, p_att);

	if ((p_att = p_txn->p_block->get_attribute(BLOCK_ATTRIB_LANGUAGE)) != nullptr)
		MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_LANGUAGE, p_att);

	p_persisted->destroy_transaction(p_txn);

	return MHD_HTTP_OK;
}


/** Finish a query by delivering the appropriate message page.

	\param connection  The MHD connection passed to the callback function. (Needed for MHD_queue_response()ing the response.)
	\param p_url	   The url that failed.
	\param http_status The http status error (e.g., MHD_HTTP_NOT_FOUND)

	\return			   A valid answer for an MHD callback. It is an integer generated by MHD_queue_response() and returned by the callback.

	This function searches for a persistence block named ("www", "httpERR_%d") where %d is the code in decimal and serves it as an answer.
*/
MHD_Result API::return_error_message(pMHD_Connection connection, pChar p_url, int http_status) {

	char answer[2048];

	sprintf(answer,
			"<html><head><style type=\"text/css\">"
			"*{transition: all 0.6s;}"
			"html {height: 100%%;}"
			"body{font-family: 'Lato', sans-serif; color: #081040; margin: 0;}"
			"#main{display: table; width: 100%%; height: 100vh; text-align: center;}"
			".fof{display: table-cell;}"
			".fof h1{font-size: 50px; display: inline-block; padding-right: 12px; animation: type .5s alternate infinite;}"
			"@keyframes type{from{box-shadow: inset -4px 0px 0px #102080;}to{box-shadow: inset -4px 0px 0px transparent;}}"
			"</style></head><body>"
			"<div style=\"background-color:#fffcf8;padding: 12px 30px 6px 30px;\"><h2>Http error on : %.140s</h2></div>"
			"<hr/>"
			"<div id=\"main\" style=\"background-color:#f0f8ff;\"><div class=\"fof\"><br/><br/><h1>Error %d</h1>"
			"<hr style=\"height:2px; width:35%%; border-width:0; color:red; background-color:red\">"
			"</div></div></body></html>",
			p_url,
			http_status);

	struct MHD_Response *response = MHD_create_response_from_buffer(strlen(answer), answer, MHD_RESPMEM_MUST_COPY);

	MHD_Result ret = MHD_queue_response(connection, http_status, response);

	MHD_destroy_response(response);

	return ret;
}


/**	 Execute a put block using some one-shot block as an intermediate buffer.

	\param p_upload	A pointer to the data uploaded with the http PUT call.
	\param size		The size of the data uploaded with the http PUT call.
	\param q_state	The structure containing the parts of the url successfully parsed.
	\param sequence SEQUENCE_FIRST_CALL, SEQUENCE_INCREMENT_CALL or SEQUENCE_FINAL_CALL. (See below)

	\return			MHD_HTTP_CREATED if SEQUENCE_FINAL_CALL is successful, MHD_HTTP_OK if any othe call is successful, or any HTTP error
					status code.

	This function is **only** called after a successful parse() of an HTTP_PUT query. It is not private because it is called for the
callback, but it is not intended for any other context.

Internals
---------

It supports any successful HTTP_PUT syntax, that is:

APPLY_NOTHING: With or without node, mandatory base, entity and key.
APPLY_RAW & APPLY_TEXT: With or without node, mandatory base, entity and key.
APPLY_URL: With or without node and just a base.

In all cases, calls with a node (it can only be l_node) q_state.url contains exactly what has to be forwarded.

Call logic:
-----------

SEQUENCE_FIRST_CALL comes first and is mandatory. On success, the functions keeps the data in a block stored in a pTransaction in
q_state.rr_value.p_extra (that pointer will be returned in successive call of the same PUT query).
SEQUENCE_INCREMENT_CALL may or may not come, if it does, it must allocate bigger blocks and store more data in the same pTransaction.
SEQUENCE_FINAL_CALL is called just once, it must destroy the pTransaction when done
*/
MHD_StatusCode API::http_put(pChar p_upload, size_t size, ApiQueryState &q_state, int sequence) {

	if (q_state.state != PSTATE_COMPLETE_OK)
		return MHD_HTTP_BAD_REQUEST;

	pTransaction p_txn = (pTransaction) q_state.rr_value.p_extra;

	switch (sequence) {
	case SEQUENCE_FIRST_CALL: {
		if (size == 0)
			return MHD_HTTP_OK;

		int dim[MAX_TENSOR_RANK] = {0, 0, 0, 0, 0, 0};

		dim[0] = size;

		if (new_block(p_txn, CELL_TYPE_BYTE, &dim[0], FILL_NEW_DONT_FILL) !=  SERVICE_NO_ERROR)
			return MHD_HTTP_INSUFFICIENT_STORAGE;

		memcpy(&p_txn->p_block->tensor.cell_byte[0], p_upload, size);

		q_state.rr_value.p_extra = (pExtraLocator) p_txn; }

		return MHD_HTTP_OK;

	case SEQUENCE_INCREMENT_CALL: {
		int dim[MAX_TENSOR_RANK] = {0, 0, 0, 0, 0, 0};

		int prev_size = p_txn->p_block->size;

		dim[0] = prev_size + size;

		pTransaction p_aux;

		if (new_block(p_aux, CELL_TYPE_BYTE, &dim[0], FILL_NEW_DONT_FILL) !=  SERVICE_NO_ERROR) {
			destroy_transaction(p_txn);

			return MHD_HTTP_INSUFFICIENT_STORAGE;
		}
		memcpy(&p_aux->p_block->tensor.cell_byte[0], &p_txn->p_block->tensor.cell_byte[0], prev_size);
		memcpy(&p_aux->p_block->tensor.cell_byte[prev_size], p_upload, size);

		std::swap(p_txn->p_block, p_aux->p_block);

		destroy_transaction(p_aux); }

		return MHD_HTTP_OK;
	}

	if (unwrap_received(p_txn) != SERVICE_NO_ERROR)
		return MHD_HTTP_INSUFFICIENT_STORAGE;

	switch (put(q_state, p_txn->p_block)) {
	case SERVICE_NO_ERROR:
		destroy_transaction(p_txn);
		return MHD_HTTP_CREATED;

	case SERVICE_ERROR_WRONG_BASE:
		destroy_transaction(p_txn);
		return MHD_HTTP_SERVICE_UNAVAILABLE;

	default:
		destroy_transaction(p_txn);
		return MHD_HTTP_BAD_GATEWAY;
	}
}


/**	 Execute an http DELETE of a block using the block API.

	\param q_state The structure containing the parts of the url successfully parsed.

	\return			  true if successful, log(LOG_MISS, "further details") for errors.

	This function is **only** called after a successful parse() of an HTTP_DELETE query. It is not private because it is called for the
callback, but it is not intended for any other context.

Internals
---------

It supports any successful HTTP_PUT syntax, that is:

APPLY_NOTHING: With or without node, mandatory base and entity, with of without a key.
APPLY_URL: With or without node and just a base.

In all cases, calls with a node (it can only be l_node) q_state.url contains exactly what has to be forwarded.

*/
MHD_StatusCode API::http_delete(ApiQueryState &q_state) {

	if (q_state.state != PSTATE_COMPLETE_OK)
		return MHD_HTTP_BAD_REQUEST;

	switch (remove(q_state)) {
	case SERVICE_NO_ERROR:
		return MHD_HTTP_OK;

	case SERVICE_ERROR_WRONG_BASE:
		return MHD_HTTP_SERVICE_UNAVAILABLE;

	default:
		return MHD_HTTP_NOT_FOUND;
	}
}


/** Execute a get block using the instrumental API.

	\param response	A valid (or error) MHD_Response pointer with the resource. It will only be used on success.
	\param q_state	The structure containing the parts of the url successfully parsed.

	\return			MHD_HTTP_OK if successful, or a valid http status error.

	This function is **only** called after a successful parse() of HTTP_GET and HTTP_HEAD queries. It is not private because it is called
for the callback, but it is not intended for any other context.

Internals
---------

It supports, basically everything, which is, all apply in many versions:

APPLY_NOTHING, APPLY_NAME, APPLY_URL, APPLY_FUNCTION, APPLY_FUNCT_CONST, APPLY_FILTER, APPLY_FILT_CONST, APPLY_RAW, APPLY_TEXT,
APPLY_ASSIGN_NOTHING, APPLY_ASSIGN_NAME, APPLY_ASSIGN_URL, APPLY_ASSIGN_FUNCTION, APPLY_ASSIGN_FUNCT_CONST, APPLY_ASSIGN_FILTER,
APPLY_ASSIGN_FILT_CONST, APPLY_ASSIGN_RAW, APPLY_ASSIGN_TEXT, APPLY_ASSIGN_CONST, APPLY_NEW_ENTITY, APPLY_GET_ATTRIBUTE,
APPLY_SET_ATTRIBUTE and APPLY_JAZZ_INFO

To simplify, this top level function decomposes the logic into smaller parts.

*/
MHD_StatusCode API::http_get(pMHD_Response &response, ApiQueryState &q_state) {

	if (q_state.state != PSTATE_COMPLETE_OK)
		return MHD_HTTP_BAD_REQUEST;

	pTransaction p_txn;
	pChar		 p_str;

	switch (q_state.apply) {
	case APPLY_NOTHING ... APPLY_TEXT: {
		pBaseAPI p_base_api = (pBaseAPI) base_server[TenBitsAtAddress(q_state.base)];
		p_base_api = (p_base_api == p_core || p_base_api == p_model) ? p_base_api : this;

		switch (p_base_api->get(p_txn, q_state)) {
		case SERVICE_NO_ERROR:
			// This condition is required by http. The BaseAPI::get() can return an index block.
			if (p_txn->p_block->cell_type == CELL_TYPE_INDEX) {
				p_txn->p_owner->destroy_transaction(p_txn);

				return MHD_HTTP_BAD_REQUEST;
			}
			break;

		case SERVICE_ERROR_WRONG_ARGUMENTS:
			return MHD_HTTP_BAD_REQUEST;
		default:
			return MHD_HTTP_NOT_FOUND;
		}
		// This is the "auto-magic" conversion into string from blocks of string with one element.
		if (p_txn->p_block->cell_type == CELL_TYPE_STRING && p_txn->p_block->size == 1 && p_txn->p_block->num_attributes == 0) {
			p_str = p_txn->p_block->get_string(0);
			response = MHD_create_response_from_buffer(strlen(p_str), p_str, MHD_RESPMEM_MUST_COPY);
		} else {
			if (q_state.apply == APPLY_TEXT)
				response = MHD_create_response_from_buffer(p_txn->p_block->size - 1, &p_txn->p_block->tensor, MHD_RESPMEM_MUST_COPY);
			else {
				if (p_txn->p_block->hash64 == 0)
					p_txn->p_block->close_block();

				response = MHD_create_response_from_buffer(p_txn->p_block->total_bytes, p_txn->p_block, MHD_RESPMEM_MUST_COPY);
			}
		}
		p_txn->p_owner->destroy_transaction(p_txn);

		return MHD_HTTP_OK; }

	case APPLY_ASSIGN_NOTHING ... APPLY_NEW_ENTITY:
	case APPLY_SET_ATTRIBUTE:
		switch (get(p_txn, q_state)) {
		case SERVICE_NO_ERROR:
			break;
		case SERVICE_ERROR_WRONG_BASE:
			return MHD_HTTP_SERVICE_UNAVAILABLE;
		default:
			return MHD_HTTP_BAD_REQUEST;
		}
		response = MHD_create_response_from_buffer(1, (char *) "0", MHD_RESPMEM_PERSISTENT);	// cppcheck-suppress cstyleCast

		if (q_state.apply == APPLY_SET_ATTRIBUTE
			&& q_state.r_value.attribute == BLOCK_ATTRIB_URL
			&& strcmp(q_state.base, "lmdb") == 0
			&& strcmp(q_state.entity, "www") == 0)
				www[q_state.url] = q_state.key;

		return MHD_HTTP_OK;

	case APPLY_GET_ATTRIBUTE:
		switch (get(p_txn, q_state)) {
		case SERVICE_NO_ERROR:
			break;
		case SERVICE_ERROR_WRONG_BASE:
			return MHD_HTTP_SERVICE_UNAVAILABLE;
		default:
			return MHD_HTTP_NOT_FOUND;
		}
		if (q_state.l_node[0] != 0)
			if (p_txn->p_block->cell_type == CELL_TYPE_STRING && p_txn->p_block->size == 1 && p_txn->p_block->num_attributes == 0) {
				p_str	 = p_txn->p_block->get_string(0);
				response = MHD_create_response_from_buffer(strlen(p_str), p_str, MHD_RESPMEM_MUST_COPY);
			} else {
				int size = (p_txn->p_block->cell_type & 0xff)*p_txn->p_block->size;
				response = MHD_create_response_from_buffer(size, &p_txn->p_block->tensor, MHD_RESPMEM_MUST_COPY);
			}
		else {
			p_str = p_txn->p_block->get_attribute(q_state.r_value.attribute);

			if (p_str == nullptr) {
				p_txn->p_owner->destroy_transaction(p_txn);

				return MHD_HTTP_NOT_FOUND;
			}
		}
		response = MHD_create_response_from_buffer(strlen(p_str), p_str, MHD_RESPMEM_MUST_COPY);

		MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain; charset=utf-8");

		p_txn->p_owner->destroy_transaction(p_txn);

		return MHD_HTTP_OK;

	case APPLY_JAZZ_INFO:
#ifdef DEBUG
		String st("DEBUG");
#else
		String st("RELEASE");
#endif
		struct utsname unn;
		uname(&unn);

		char buffer_1k[1024];

		int my_idx	 = p_channels->jazz_node_my_index;
		int my_port	 = p_channels->jazz_node_port[my_idx];
		int nn_nodes = p_channels->jazz_node_cluster_size;

		String my_name = p_channels->jazz_node_name[my_idx];
		String my_ip   = p_channels->jazz_node_ip[my_idx];

		sprintf(buffer_1k, "Jazz\n\n version : %s\n build   : %s\n artifact: %s\n jazznode: %s (%s:%d) (%d of %d)\n "
				"sysname : %s\n hostname: %s\n kernel\x20 : %s\n sysvers : %s\n machine : %s",
				JAZZ_VERSION, st.c_str(), LINUX_PLATFORM, my_name.c_str(), my_ip.c_str(), my_port, my_idx, nn_nodes,
				unn.sysname, unn.nodename, unn.release, unn.version, unn.machine);

		response = MHD_create_response_from_buffer(strlen(buffer_1k), buffer_1k, MHD_RESPMEM_MUST_COPY);

		MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain; charset=utf-8");

		return MHD_HTTP_OK;
	}
	return MHD_HTTP_BAD_REQUEST;
}


/** Push a copy of all the files in the path (searched recursively) to the Persisted database "static" and index their names
to be found by get_static().

It also assigns attributes:
- BLOCK_ATTRIB_URL == same relative path after the root path.
- BLOCK_ATTRIB_MIMETYPE guessed from the file extension (html, js, png, etc.)
- BLOCK_ATTRIB_LANGUAGE == en-us

	\param p_base_path		The path to the tree of webpage statics.
	\param p_relative_path	The relative path that becomes the url with a file name added, starting with /.
	\param rec_level		The level of recursion (from 0 to MAX_RECURSE_LEVEL_ON_STATICS)

	\return		Some error code or SERVICE_NO_ERROR if successful.
*/
StatusCode API::load_statics(pChar p_base_path, pChar p_relative_path, int rec_level) {

	if (!p_persisted->is_running()) {
		log(LOG_MISS, "API::load_statics(): Skipped because Persistence is not running.");

		return SERVICE_NO_ERROR;
	}

	if (rec_level > MAX_RECURSE_LEVEL_ON_STATICS)
		return SERVICE_ERROR_TOO_DEEP;

	DIR *dir;
	char root_dir[1024];
	sprintf(root_dir, "%s%s", p_base_path, p_relative_path);

	if ((dir = opendir(root_dir)) != nullptr) {

		if (rec_level == 0 && !p_persisted->dbi_exists((pChar) "www")) {
			Locator loc = {"lmdb", "www", ""};
			int ret;
			if ((ret = p_persisted->new_entity(loc)) != SERVICE_NO_ERROR) {
				log(LOG_ERROR, "API::load_statics(): Failed to create www database.");

		  		closedir(dir);

				return ret;
			}
		}

		struct dirent *ent;

		int		 file_num = 1;
		uint64_t dir_hash = MurmurHash64A(&root_dir, strlen(root_dir));

		while ((ent = readdir(dir)) != nullptr) {	// cppcheck-suppress readdirCalled ; cppcheck is wrong! readdir_r is deprecated and
													// readdir() (3) is thread safe.
			if (ent->d_type == DT_REG) {
				char fn[2048];						// Compile warning: "output between 1 and 1279 bytes into a destination of size 1024"
				int ret = snprintf(fn, 1024, "//file/%s%s", root_dir, ent->d_name);

				if (ret < 0 || ret >= 1024) {
					log(LOG_ERROR, "API::load_statics(): File path/name too long.");

			  		closedir(dir);

					return SERVICE_ERROR_NO_MEM;
				}

				pTransaction p_base, p_txn;

				ret = p_channels->get(p_base, (pChar) &fn);

				if (ret != SERVICE_NO_ERROR) {
					log(LOG_ERROR, "API::load_statics(): p_channels->get() failed.");

			  		closedir(dir);

					return ret;
				}

				AttributeMap atts;
				p_base->p_block->get_attributes(&atts);

				sprintf(fn, "%s%s", p_relative_path, ent->d_name);

				atts[BLOCK_ATTRIB_URL]		= fn;
				atts[BLOCK_ATTRIB_LANGUAGE] = "en-us";

				pChar p_ext = strrchr(fn, '.');

				char mime_type[40] = {"application/octet-stream"};

				if (p_ext != nullptr) {
					if (strcmp(p_ext, ".css") == 0)
						strcpy(mime_type, "text/css");
					else if (strcmp(p_ext, ".gif") == 0)
						strcpy(mime_type, "image/gif");
					else if (strcmp(p_ext, ".htm") == 0 || strcmp(p_ext, ".html") == 0)
						strcpy(mime_type, "text/html");
					else if (strcmp(p_ext, ".ico") == 0)
						strcpy(mime_type, "image/x-icon");
					else if (strcmp(p_ext, ".jpg") == 0 || strcmp(p_ext, ".jpeg") == 0)
						strcpy(mime_type, "image/jpeg");
					else if (strcmp(p_ext, ".js") == 0)
						strcpy(mime_type, "application/javascript");
					else if (strcmp(p_ext, ".json") == 0)
						strcpy(mime_type, "application/json");
					else if (strcmp(p_ext, ".md") == 0 || strcmp(p_ext, ".txt") == 0)
						strcpy(mime_type, "text/plain; charset=utf-8");
					else if (strcmp(p_ext, ".mp4") == 0)
						strcpy(mime_type, "video/mp4");
					else if (strcmp(p_ext, ".otf") == 0)
						strcpy(mime_type, "font/otf");
					else if (strcmp(p_ext, ".pdf") == 0)
						strcpy(mime_type, "application/pdf");
					else if (strcmp(p_ext, ".png") == 0)
						strcpy(mime_type, "image/png");
					else if (strcmp(p_ext, ".ttf") == 0)
						strcpy(mime_type, "font/ttf");
					else if (strcmp(p_ext, ".wav") == 0)
						strcpy(mime_type, "audio/wav");
					else if (strcmp(p_ext, ".xml") == 0)
						strcpy(mime_type, "application/xml");
				}
				atts[BLOCK_ATTRIB_MIMETYPE] = mime_type;

				if (new_block(p_txn, p_base->p_block, (pBlock) nullptr, &atts) != SERVICE_NO_ERROR) {
					log(LOG_ERROR, "API::load_statics(): new_block() with attributes failed.");

					p_channels->destroy_transaction(p_base);

			  		closedir(dir);

					return SERVICE_ERROR_NO_MEM;
				}
				p_channels->destroy_transaction(p_base);

				Locator loc = {"lmdb", "www", ""};

				sprintf(loc.key, "blk%lx_%d", dir_hash, file_num++);

				ret = p_persisted->put(loc, p_txn->p_block);

				if (ret == SERVICE_NO_ERROR)
					www[p_txn->p_block->get_attribute(BLOCK_ATTRIB_URL)] = loc.key;

				destroy_transaction(p_txn);

				if (ret != SERVICE_NO_ERROR) {
					log(LOG_ERROR, "API::load_statics(): p_persisted->put() failed.");

			  		closedir(dir);

					return ret;
				}
				log_printf(LOG_INFO, "www static %s loaded as %s", mime_type, fn);
			} else if (ent->d_type == DT_DIR && ent->d_name[0] != '.') {
				char next_relative_path[1024];
				int ret = snprintf(next_relative_path, 1024, "%s%s/", p_relative_path, ent->d_name);

				if (ret < 0 || ret >= 1024) {
					log(LOG_ERROR, "API::load_statics(): nested path too long.");

			  		closedir(dir);

					return SERVICE_ERROR_NO_MEM;
				}

				ret = load_statics(p_base_path, (pChar) &next_relative_path, rec_level + 1);

				if (ret != SERVICE_NO_ERROR) {
			  		closedir(dir);

					return ret;
				}
			}
		}
  		closedir(dir);
	}

	return SERVICE_NO_ERROR;
}


/** Copy while percent-decoding a string into a buffer. Only RFC 3986 section 2.3 and RFC 3986 section 2.2 characters accepted.

	\param p_buff	 A buffer to store the result.
	\param buff_size The size of the output buffer (ending zero included).
	\param p_url	 The input string.

	\return			'true' if successful. Possible errors are wrong %-syntax, wrong char range or output buffer too small.

See https://en.wikipedia.org/wiki/Percent-encoding This is utf-8 compatible, utf-8 chars are just percent encoded one byte at a time.
*/
bool API::expand_url_encoded(pChar p_buff, int buff_size, pChar p_url) {

	if (*(p_url++) != '&')
		return false;

	pChar p_end = p_url;

	p_end += strlen(p_url) - 1;

	if (*p_end != ';' && *p_end != ']' && *p_end != ')')
		return false;

	while (buff_size-- > 0) {
		if (p_url == p_end) {
			*(p_buff) = 0;

			return true;
		}
		switch (char ch = *(p_url++)) {
		case '!' ... '$':
		case '&' ... '/':
		case '0' ... '9':
		case ':':
		case ';':
		case '=':
		case '?':
		case '@':
		case 'A' ... 'Z':
		case '[':
		case ']':
		case '_':
		case 'a' ... 'z':
		case '~':
			*(p_buff++) = ch;
			break;

		case '%': {
			if (p_url == p_end)
				return false;

			int xhi = from_hex(*(p_url++));

			if (p_url == p_end)
				return false;

			int xlo = from_hex(*(p_url++));

			*(p_buff++) = (xhi << 4) + xlo;

			break;
		}
		default:
			return false;
		}
	}

	return false;
}


#ifdef CATCH_TEST

API	TT_API(&LOGGER, &CONFIG, &CHN, &VOL, &PER, &COR, &BOP, &MDL);

#endif

} // namespace jazz_main

#ifdef CATCH_TEST
#include "src/jazz_main/tests/test_api.ctest"
#endif
