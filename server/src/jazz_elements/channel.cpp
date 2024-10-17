/* Jazz (c) 2018-2024 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#include <sys/stat.h>
#include <filesystem>

#include <zmq.h>
#include <microhttpd.h>


#include "src/jazz_elements/channel.h"


namespace jazz_elements
{

/** \brief A callback for libCURL GET.

	(see https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html)
*/
size_t get_callback(char *ptr, size_t size, size_t nmemb, void *container) {	// cppcheck-suppress unusedFunction
	size = size*nmemb;

	if (size) {
		size_t ix = pGetBuffer(container)->size();

		pGetBuffer(container)->resize(ix + size);

		uint8_t *p_dest = pGetBuffer(container)->data();

		memcpy(p_dest, ptr, size);
	}

	return size;
}


/** \brief A callback for libCURL GET to ignore all the blocks sent by the server in PUT and DELETE calls.

	(see https://curl.haxx.se/libcurl/c/CURLOPT_WRITEFUNCTION.html)
*/
size_t dev_null(char *_ignore, size_t size, size_t nmemb, void *_ignore_2) {	// cppcheck-suppress unusedFunction
	size = size*nmemb;

	return size;
}


/** \brief A callback for libCURL PUT.

	(see https://curl.haxx.se/libcurl/c/CURLOPT_READFUNCTION.html)
*/
size_t put_callback(char *ptr, size_t size, size_t nmemb, void *container) {	// cppcheck-suppress unusedFunction
	size = size*nmemb;

	if (size) {
		size = std::min(size, pPutBuffer(container)->to_send);
		memcpy(ptr, pPutBuffer(container)->p_base, size);

		pPutBuffer(container)->p_base  += size;
		pPutBuffer(container)->to_send -= size;
	}

	return size;
}

/*	-----------------------------------------------
	 Channels : I m p l e m e n t a t i o n
--------------------------------------------------- */

Channels::Channels(pLogger a_logger, pConfigFile a_config) : Container(a_logger, a_config) {}


Channels::~Channels() { destroy_container(); }


/** Return object ID.

	\return A string identifying the object that is especially useful to track uplifts and versions.
*/
pChar const Channels::id() {
    static char arr[] = "Channels from Jazz-" JAZZ_VERSION;
    return arr;
}


/** Reads config variables and sets jazz_node_* public variables.
*/
StatusCode Channels::start() {

	int ret = Container::start();	// This initializes the one-shot functionality.

	if (ret != SERVICE_NO_ERROR)
		return ret;

	std::string my_name;

	if (!get_conf_key("JAZZ_NODE_MY_NAME", my_name)) {
		log(LOG_ERROR, "Channels::start() failed to find JAZZ_NODE_MY_NAME");

		return EXIT_FAILURE;
	}
	search_my_node_index = my_name == "";	// Note that /// starts a remark resulting in the value being empty.
	if (search_my_node_index)
		my_name = "localhost";

	jazz_node_cluster_size =  0;
	jazz_node_my_index	   = -1;
	int min_port = 999999;
	int max_port = 0;

	std::string s;
	char key[40];

	for (int i = 1;; i++) {
		sprintf(key, "JAZZ_NODE_NAME_%i", i);

		if (!get_conf_key(key, s))
			break;

		jazz_node_name[i] = s;

		if (my_name == s)
			jazz_node_my_index = i;

		sprintf(key, "JAZZ_NODE_IP_%i", i);

		if (!get_conf_key(key, s)) {
			log_printf(LOG_ERROR, "Channels::start() failed to find %s", key);

			return EXIT_FAILURE;
		}

		jazz_node_ip[i] = s;

		sprintf(key, "JAZZ_NODE_PORT_%i", i);

		int port;

		if (!get_conf_key(key, port)) {
			log_printf(LOG_ERROR, "Channels::start() failed to find %s", key);

			return EXIT_FAILURE;
		}
		min_port = std::min(port, min_port);
		max_port = std::max(port, max_port);

		jazz_node_port[i] = port;

		jazz_node_cluster_size++;
	}

	if (jazz_node_my_index < 0) {
		log(LOG_ERROR, "Channels::start() failed to find JAZZ_NODE_MY_NAME in JAZZ_NODE_NAME_*");

		return EXIT_FAILURE;
	}

	if (search_my_node_index && min_port != max_port) {
		log(LOG_ERROR, "Channels::start() failed. Automatic JAZZ_NODE_MY_NAME detection requires all ports being identical.");

		return EXIT_FAILURE;
	}

	if (!get_conf_key("ENABLE_ZEROMQ_CLIENT", can_zmq)) {
		log(LOG_ERROR, "Channels::start() failed to find ENABLE_ZEROMQ_CLIENT");

		return EXIT_FAILURE;
	}

	if (!get_conf_key("ENABLE_HTTP_CLIENT", can_curl)) {
		log(LOG_ERROR, "Channels::start() failed to find ENABLE_HTTP_CLIENT");

		return EXIT_FAILURE;
	}

	if (!get_conf_key("ENABLE_BASH_EXEC", can_bash)) {
		log(LOG_ERROR, "Channels::start() failed to find ENABLE_BASH_EXEC");

		return EXIT_FAILURE;
	}

	if (!get_conf_key("ENABLE_FILE_LEVEL", file_lev)) {
		log(LOG_ERROR, "Channels::start() failed to find ENABLE_FILE_LEVEL");

		return EXIT_FAILURE;
	}

	if (!curl_ok)
		curl_ok = can_curl && curl_global_init(CURL_GLOBAL_SSL) == CURLE_OK;

	if (!zmq_ok)
		zmq_ok = can_zmq && ((zmq_context = zmq_ctx_new()) != nullptr);

	return SERVICE_NO_ERROR;
}


/** Shuts down the Persisted Service
*/
StatusCode Channels::shut_down() {

	if (curl_ok) {
		curl_global_cleanup();

		curl_ok = false;
	}

	if (zmq_ok) {
		for (PipeMap::iterator it = pipes.begin(); it != pipes.end(); ++it)
			zmq_close(it->second.requester);

		pipes.clear();

		if (zmq_context != nullptr)
			zmq_ctx_destroy(zmq_context);

		zmq_context	= nullptr;
		zmq_ok		= false;
	}

	for (ConnMap::iterator it = connect.begin(); it != connect.end(); ++it) {
		it->second.clear();
	}
	connect.clear();

	return Container::shut_down();	// Closes the one-shot functionality.
}


/** Native (Channels) interface **complete Block** retrieval.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container. The data read from the endpoint will be stored as a rank == 1 CELL_TYPE_BYTE
					for all bases except "bash" shell output is returned as a rank == 1 CELL_TYPE_STRING
	\param p_what	The endpoint.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.

**NOTE**: See the description of Channels for reference.
*/
StatusCode Channels::get(pTransaction &p_txn, pChar p_what) {

	if ((*p_what++ != '/') || (*p_what++ != '/') || (*p_what == 0))
		return SERVICE_ERROR_WRONG_ARGUMENTS;

	int base = TenBitsAtAddress(p_what);

	switch (base) {
	case BASE_FILE_10BIT: {
		if (file_lev < 1)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		p_what += 4;
		if (*p_what++ != '/')
			return SERVICE_ERROR_WRONG_BASE;

	    struct stat p_stat;
    	int ret = stat(p_what, &p_stat);
		if (ret != 0)
			return SERVICE_ERROR_BLOCK_NOT_FOUND;

		if (S_ISDIR(p_stat.st_mode)) {
			Index idx = {};

			DIR *dir;
			if ((dir = opendir(p_what)) == nullptr)
				return SERVICE_ERROR_IO_ERROR;

			struct dirent *ent;

			while ((ent = readdir(dir)) != nullptr) {	// cppcheck-suppress readdirCalled ; cppcheck is wrong! readdir_r is deprecated and
														// readdir() (3) is thread safe.
				switch (ent->d_type) {
				case DT_REG:
					idx[ent->d_name] = "file";
					break;
				case DT_DIR:
					if (ent->d_name[0] != '.')
						idx[ent->d_name] = "folder";
				}
			}
	  		closedir(dir);

			return new_block(p_txn, idx);
		}

		if (S_ISREG(p_stat.st_mode)) {
			if (p_stat.st_size > MAX_BLOCK_SIZE)
				return SERVICE_ERROR_BLOCK_TOO_BIG;

			int dim[MAX_TENSOR_RANK] = {(int) p_stat.st_size, 0};

			ret = new_block(p_txn, CELL_TYPE_BYTE, dim, FILL_NEW_DONT_FILL);

			if (ret != SERVICE_NO_ERROR)
				return ret;

			bool read_ok = false;
			FILE *fp;
			fp = fopen(p_what, "rb");
			if (fp != nullptr) {
				read_ok = fread(&p_txn->p_block->tensor.cell_byte, 1, p_stat.st_size, fp) == p_stat.st_size;

				fclose(fp);
			}

			if (!read_ok) {
				destroy_transaction(p_txn);

				return SERVICE_ERROR_IO_ERROR;
			}
			return SERVICE_NO_ERROR;
		}}
		return SERVICE_ERROR_IO_ERROR;

	case BASE_HTTP_10BIT: {
		if (!curl_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		p_what += 4;
		if (*p_what++ != '/')
			return SERVICE_ERROR_WRONG_BASE;

		ConnMap::iterator it;

		if (strncmp(p_what, "connection/", 11) == 0) {
			p_what += 11;

			it = connect.find(p_what);

			if (it == connect.end())
				return SERVICE_ERROR_ENTITY_NOT_FOUND;

			return new_block(p_txn, it->second);
		}
		pChar pt = strchr(p_what, '/');
		if (pt == nullptr)
			it = connect.find(p_what);
		else {
			*pt = 0;
			it  = connect.find(p_what);
			*pt = '/';
		}
		if (it != connect.end()) {
			std::string url = it->second["URL"];

			if (pt != nullptr)
				url += ++pt;

			if (it->second.size() > 1)
				return curl_get(p_txn, url.c_str(), &it->second);
			else
				return curl_get(p_txn, url.c_str());
		}
		return curl_get(p_txn, p_what); }

	case BASE_0_MQ_10BIT:
		if (!zmq_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		p_what += 4;
		if (*p_what++ != '/')
			return SERVICE_ERROR_WRONG_BASE;

		if (strncmp(p_what, "pipeline/", 9) != 0)
			return SERVICE_ERROR_WRONG_ARGUMENTS;

		p_what += 9;
		PipeMap::iterator it = pipes.find(p_what);

		if (it == pipes.end())
			return SERVICE_ERROR_ENTITY_NOT_FOUND;

		return new_block(p_txn, CELL_TYPE_STRING, nullptr, FILL_WITH_TEXTFILE, 0, it->second.endpoint);
	}

	return SERVICE_ERROR_WRONG_BASE;
}


/** Easy Channels interface **get(2)** is not applicable.

**NOTE**: This always returns SERVICE_ERROR_NOT_APPLICABLE. Channels do not contain anything, just use get(1) instead.
*/
StatusCode Channels::get(pTransaction &p_txn, pChar p_what, pBlock p_row_filter) {

	return SERVICE_ERROR_NOT_APPLICABLE;
}


/** Easy Channels interface **get(3)** is not applicable.

**NOTE**: This always returns SERVICE_ERROR_NOT_APPLICABLE. Channels do not contain anything, just use get(1) instead.
*/
StatusCode Channels::get(pTransaction &p_txn, pChar p_what, pChar name) {

	return SERVICE_ERROR_NOT_APPLICABLE;
}


/** Easy Channels interface **locate** is not applicable.

**NOTE**: This always returns SERVICE_ERROR_NOT_APPLICABLE. Channels do not contain anything, just use get() instead.
*/
StatusCode Channels::locate(Locator &location, pChar p_what) {

	return SERVICE_ERROR_NOT_APPLICABLE;
}


/** Easy Channels interface **header** is not applicable.

**NOTE**: This always returns SERVICE_ERROR_NOT_APPLICABLE. Channels do not contain anything, just use get() instead.
*/
StatusCode Channels::header(StaticBlockHeader &hea, pChar p_what) {

	return SERVICE_ERROR_NOT_APPLICABLE;
}



/** Easy Channels interface **header** is not applicable.

**NOTE**: This always returns SERVICE_ERROR_NOT_APPLICABLE. Channels do not contain anything, just use get() instead.
*/
StatusCode Channels::header(pTransaction &p_txn, pChar p_what) {

	return SERVICE_ERROR_NOT_APPLICABLE;
}


/** Easy Channels interface for **Block writing**

	\param p_where	The destination endpoint.
	\param p_block	The Block to be written/sent by Channels.
	\param mode		Some writing restriction that depends on the base. WRITE_ONLY_IF_EXISTS and WRITE_ONLY_IF_NOT_EXISTS can only be used
					on **file**. WRITE_TENSOR_DATA should be used as default, otherwise the metadata will also be written/sent.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: See the description of Channels for reference.
*/
StatusCode Channels::put(pChar p_where, pBlock p_block, int mode) {

	if ((*p_where++ != '/') || (*p_where++ != '/') || (*p_where == 0))
		return SERVICE_ERROR_WRONG_ARGUMENTS;

	int base = TenBitsAtAddress(p_where);

	switch (base) {
	case BASE_FILE_10BIT: {
		if (file_lev < 2)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		if ((mode & WRITE_AS_ANY_WRITE) == 0)
			mode = WRITE_AS_CONTENT;

		p_where += 4;
		if (*p_where++ != '/')
			return SERVICE_ERROR_WRONG_BASE;

		if (((mode & (WRITE_ONLY_IF_EXISTS | WRITE_ONLY_IF_NOT_EXISTS)) != 0) || (file_lev == 2)) {
		    struct stat p_stat;
			bool exists = stat(p_where, &p_stat) == 0;
			if ((exists && (mode & WRITE_ONLY_IF_NOT_EXISTS)) || (!exists && (mode & WRITE_ONLY_IF_EXISTS)))
				return SERVICE_ERROR_WRITE_FORBIDDEN;
			if (exists && (file_lev == 2))
				return SERVICE_ERROR_BASE_FORBIDDEN;
		}

		pChar p_buff;
		int	  size;

		if ((mode & WRITE_AS_STRING) && (	(p_block->cell_type == CELL_TYPE_STRING && p_block->size == 1)
										 || (p_block->cell_type == CELL_TYPE_BYTE	&& p_block->rank == 1))) {
			if (p_block->cell_type == CELL_TYPE_STRING) {
				p_buff = p_block->get_string(0);
				size   = strlen(p_buff);
			} else {
				p_buff = reinterpret_cast<pChar>(&p_block->tensor.cell_byte[0]);
				size   = strnlen(p_buff, p_block->size);
			}
		} else if ((mode & WRITE_AS_CONTENT) && ((p_block->cell_type & 0xf0) == 0)) {
			size   = p_block->size*(p_block->cell_type & 0xff);
			p_buff = reinterpret_cast<pChar>(&p_block->tensor.cell_byte[0]);
		} else if ((mode & WRITE_AS_FULL_BLOCK) && (p_block->cell_type != CELL_TYPE_INDEX)) {
			size   = p_block->total_bytes;
			p_buff = reinterpret_cast<pChar>(p_block);
		} else
			return SERVICE_ERROR_WRITE_FORBIDDEN;

		FILE *fp;
		fp = fopen(p_where, "wb");
		if (fp == nullptr)
			return SERVICE_ERROR_IO_ERROR;

		size_t w_len = fwrite(p_buff, 1, size, fp);
		fclose(fp);

		if (w_len != size)
			return SERVICE_ERROR_IO_ERROR;}

		return SERVICE_NO_ERROR;

	case BASE_HTTP_10BIT: {
		if (!curl_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		if ((mode & WRITE_AS_ANY_WRITE) == 0)
			mode = WRITE_AS_STRING | WRITE_AS_FULL_BLOCK;

		p_where += 4;
		if (*p_where++ != '/')
			return SERVICE_ERROR_WRONG_BASE;

		ConnMap::iterator it;

		if (strncmp(p_where, "connection/", 11) == 0) {
			p_where += 11;

			it = connect.find(p_where);

			if (it != connect.end())
				return SERVICE_ERROR_WRITE_FORBIDDEN;

			int n_rows;
			pTuple p_tup;
			pBlock p_key, p_val;

			if (	p_block->cell_type != CELL_TYPE_TUPLE_ITEM || p_block->size != 2
				|| (p_tup = (pTuple) p_block)->index((pChar) "key") != 0 || p_tup->index((pChar) "value") != 1
				|| (p_key = p_tup->get_block(0))->cell_type != CELL_TYPE_STRING || p_key->rank != 1
				|| (p_val = p_tup->get_block(1))->cell_type != CELL_TYPE_STRING || p_val->rank != 1
				|| (n_rows = p_key->size) != p_val->size)
				return SERVICE_ERROR_WRONG_ARGUMENTS;

			Index idx = {};

			for (int i = 0; i < n_rows; i++) {
				idx[p_key->get_string(i)] = p_val->get_string(i);
			}

			if (idx.find("URL") == idx.end())
				return SERVICE_ERROR_WRONG_ARGUMENTS;

			connect[p_where] = idx;

			return SERVICE_NO_ERROR;
		}
		pChar pt = strchr(p_where, '/');
		if (pt == nullptr)
			it = connect.find(p_where);
		else {
			*pt = 0;
			it  = connect.find(p_where);
			*pt = '/';
		}
		if (it != connect.end()) {
			std::string url = it->second["URL"];

			if (pt != nullptr)
				url += ++pt;

			if (it->second.size() > 1)
				return curl_put(url.c_str(), p_block, mode, &it->second);
			else
				return curl_put(url.c_str(), p_block, mode);
		}
		return curl_put(p_where, p_block, mode); }

	case BASE_0_MQ_10BIT:
		if (!zmq_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		if ((mode & WRITE_AS_ANY_WRITE) == 0)
			mode = WRITE_AS_FULL_BLOCK;

		p_where += 4;
		if (*p_where++ != '/')
			return SERVICE_ERROR_WRONG_BASE;

		if (strncmp(p_where, "pipeline/", 9) != 0)
			return SERVICE_ERROR_WRONG_ARGUMENTS;

		p_where += 9;

		if (pipes.find(p_where) != pipes.end())
			return SERVICE_ERROR_WRITE_FORBIDDEN;

		if (p_block->cell_type != CELL_TYPE_STRING || p_block->size != 1)
			return SERVICE_ERROR_WRONG_ARGUMENTS;

		Socket sock;
		strncpy(sock.endpoint, p_block->get_string(0), sizeof(sock.endpoint));

		if (   (zmq_context == nullptr) || ((sock.requester = zmq_socket(zmq_context, ZMQ_REQ)) == nullptr)
			|| (zmq_connect(sock.requester, sock.endpoint) != 0)) {
			if (sock.requester != nullptr)
				zmq_close(sock.requester);

			return SERVICE_ERROR_IO_ERROR;
		}

		pipes[p_where] = sock;

		return SERVICE_NO_ERROR;
	}

	return SERVICE_ERROR_WRONG_BASE;
}


/** Easy Channels interface for **creating folders**

	\param p_where The path to the folder to be created via mkdir()

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: This is only used with //file.
*/
StatusCode Channels::new_entity(pChar p_where) {

	if ((*p_where++ != '/') || (*p_where++ != '/') || (*p_where == 0))
		return SERVICE_ERROR_WRONG_ARGUMENTS;

	int base = TenBitsAtAddress(p_where);

	if (base == BASE_FILE_10BIT) {
		if (file_lev < 2)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		p_where += 4;
		if (*p_where++ != '/')
			return SERVICE_ERROR_WRONG_BASE;

		if (file_lev == 2) {
		    struct stat p_stat;
			if (stat(p_where, &p_stat) == 0)
				return SERVICE_ERROR_BASE_FORBIDDEN;
		}
		if (mkdir(p_where, 0700) == 0)
			return SERVICE_NO_ERROR;

		return SERVICE_ERROR_IO_ERROR;
	}

	return SERVICE_ERROR_WRONG_BASE;
}


/** Easy Channels interface for **deleting**:

	\param p_where Some endpoint to be deleted.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: See the description of Channels for reference.
*/
StatusCode Channels::remove(pChar p_where) {

	if ((*p_where++ != '/') || (*p_where++ != '/') || (*p_where == 0))
		return SERVICE_ERROR_WRONG_ARGUMENTS;

	int base = TenBitsAtAddress(p_where);

	switch (base) {
	case BASE_FILE_10BIT:
		if (file_lev < 3)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		p_where += 4;
		if (*p_where++ != '/')
			return SERVICE_ERROR_WRONG_BASE;

	    struct stat p_stat;
		if (stat(p_where, &p_stat) != 0)
			return SERVICE_ERROR_BLOCK_NOT_FOUND;

		if (S_ISREG(p_stat.st_mode)) {
			if (std::remove(p_where) != 0)
				return SERVICE_ERROR_IO_ERROR;

			return SERVICE_NO_ERROR;
		}
		if (S_ISDIR(p_stat.st_mode)) {
			if (std::filesystem::remove_all(p_where) < 1)	// Returns the number of items removed
				return SERVICE_ERROR_IO_ERROR;

			return SERVICE_NO_ERROR;
		}
		return SERVICE_ERROR_IO_ERROR;

	case BASE_HTTP_10BIT: {
		if (!curl_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		p_where += 4;
		if (*p_where++ != '/')
			return SERVICE_ERROR_WRONG_BASE;

		ConnMap::iterator it;

		if (strncmp(p_where, "connection/", 11) == 0) {
			p_where += 11;

			it = connect.find(p_where);

			if (it == connect.end())
				return SERVICE_ERROR_ENTITY_NOT_FOUND;

			connect.erase(it);

			return SERVICE_NO_ERROR;
		}
		pChar pt = strchr(p_where, '/');
		if (pt == nullptr)
			it = connect.find(p_where);
		else {
			*pt = 0;
			it  = connect.find(p_where);
			*pt = '/';
		}
		if (it != connect.end()) {
			std::string url = it->second["URL"];

			if (pt != nullptr)
				url += ++pt;

			if (it->second.size() > 1)
				return curl_remove(url.c_str(), &it->second);
			else
				return curl_remove(url.c_str());
		}
		return curl_remove(p_where); }

	case BASE_0_MQ_10BIT:
		if (!zmq_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		p_where += 4;
		if (*p_where++ != '/')
			return SERVICE_ERROR_WRONG_BASE;

		if (strncmp(p_where, "pipeline/", 9) != 0)
			return SERVICE_ERROR_WRONG_ARGUMENTS;

		p_where += 9;
		PipeMap::iterator it = pipes.find(p_where);

		if (it == pipes.end())
			return SERVICE_ERROR_ENTITY_NOT_FOUND;

		zmq_close(it->second.requester);

		pipes.erase(it);

		return SERVICE_NO_ERROR;
	}

	return SERVICE_ERROR_WRONG_BASE;
}


/** Easy Channels interface for **Block copying** (possibly across bases but inside the Channels).

	\param p_where Some destination endpoint.
	\param p_what  Some source endpoint.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: This is just a get() and a put(). See the description of Channels for reference.
*/
StatusCode Channels::copy(pChar p_where, pChar p_what) {

	pTransaction p_txn;

	int ret = get(p_txn, p_what);

	if (ret != SERVICE_NO_ERROR)
		return ret;

	int mode;

	switch (TenBitsAtAddress(p_what + 2)) {
	case BASE_FILE_10BIT:
		mode = WRITE_AS_CONTENT;
		break;
	case BASE_HTTP_10BIT:
		mode = WRITE_AS_STRING | WRITE_AS_FULL_BLOCK;
		break;
	default:
		mode = WRITE_AS_FULL_BLOCK;
	}

	ret = put(p_where, p_txn->p_block, mode);

	destroy_transaction(p_txn);

	return ret;
}


/** The function call interface for **modify**: In jazz_elements, this is only implemented in Channels.

	\param function	Some description of a service. In general base/entity/key. In Channels the key must be empty and the entity is
					the pipeline.
	\param p_args	In Channels: A Tuple with two items, "input" with the data passed to the service and "result" with the data returned.
					The result will be overridden in-place without any allocation.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

This is what most frameworks would call predict(), something that takes any tensor as an input and returns another tensor. In channels,
it just gives support to some other service doing that connected via zeroMQ or bash. Outside jazz_elements, the services use this
to run their own models.
*/
StatusCode Channels::modify(Locator &function, pTuple p_args) {

	if (   p_args->cell_type != CELL_TYPE_TUPLE_ITEM || p_args->size != 2
		|| p_args->index((pChar) "input") != 0 || p_args->index((pChar) "result") != 1)
		return SERVICE_ERROR_WRONG_ARGUMENTS;

	int base = TenBitsAtAddress(function.base);

	switch (base) {
	case BASE_BASH_10BIT: {
		if (!can_bash)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		if (   p_args->get_block(0)->cell_type != CELL_TYPE_BYTE || p_args->get_block(0)->rank != 1
			|| p_args->get_block(1)->cell_type != CELL_TYPE_BYTE || p_args->get_block(1)->rank != 1)
			return SERVICE_ERROR_WRONG_ARGUMENTS;

		int size_input  = p_args->get_block(0)->size;
		int size_result = p_args->get_block(1)->size;

		pChar p_input  = (pChar) &p_args->get_block(0)->tensor.cell_byte[0];
		pChar p_result = (pChar) &p_args->get_block(1)->tensor.cell_byte[0];

		char script[] = "/tmp/jzz-srcXXXXXX";
		int fd = mkstemp(script);

		if (fd < 0)
			return SERVICE_ERROR_IO_ERROR;

		if (write(fd, p_input, size_input) != size_input) {
			close(fd);
			return SERVICE_ERROR_IO_ERROR;
		}
		close(fd);

		FILE *fp;
		char buffer[128];

		sprintf(buffer, "bash %s", script);

		fp = popen(buffer, "r");

		if (fp == nullptr)
			return SERVICE_ERROR_IO_ERROR;

		while (fgets(buffer, 128, fp) != nullptr) {
			int ll = strlen(buffer);
			if (ll < size_result) {
				if (ll > 0) {
					memcpy(p_result, buffer, ll);
					p_result	+= ll;
					size_result	-= ll;
				}
			} else {
				ll = size_result - 1;
				memcpy(p_result, buffer, ll);
				p_result	+= ll;
				size_result	-= ll;
				break;
			}
		}
		memset(p_result, 0, size_result);

		int ret = pclose(fp);

		if (ret == 0)
			return SERVICE_NO_ERROR; }

		return SERVICE_ERROR_IO_ERROR;

	case BASE_0_MQ_10BIT:
		if (!zmq_ok)
			return SERVICE_ERROR_BASE_FORBIDDEN;

		switch (p_args->get_block(0)->cell_type) {
		case CELL_TYPE_BYTE:
		case CELL_TYPE_BYTE_BOOLEAN:
		case CELL_TYPE_INTEGER:
		case CELL_TYPE_FACTOR:
		case CELL_TYPE_GRADE:
		case CELL_TYPE_BOOLEAN:
		case CELL_TYPE_SINGLE:
		case CELL_TYPE_LONG_INTEGER:
		case CELL_TYPE_TIME:
		case CELL_TYPE_DOUBLE:
			break;
		default:
			return SERVICE_ERROR_WRONG_ARGUMENTS;
		}
		switch (p_args->get_block(1)->cell_type) {
		case CELL_TYPE_BYTE:
		case CELL_TYPE_BYTE_BOOLEAN:
		case CELL_TYPE_INTEGER:
		case CELL_TYPE_FACTOR:
		case CELL_TYPE_GRADE:
		case CELL_TYPE_BOOLEAN:
		case CELL_TYPE_SINGLE:
		case CELL_TYPE_LONG_INTEGER:
		case CELL_TYPE_TIME:
		case CELL_TYPE_DOUBLE:
			break;
		default:
			return SERVICE_ERROR_WRONG_ARGUMENTS;
		}
		PipeMap::iterator it = pipes.find(function.entity);

		if (it == pipes.end())
			return SERVICE_ERROR_ENTITY_NOT_FOUND;

		int size_input  = p_args->get_block(0)->size;
		int size_result = p_args->get_block(1)->size;

		pChar p_input  = (pChar) &p_args->get_block(0)->tensor.cell_byte[0];
		pChar p_result = (pChar) &p_args->get_block(1)->tensor.cell_byte[0];

		memset(p_result, 0, size_result);

		if (zmq_send(it->second.requester, p_input, size_input, 0) < 0)
			return SERVICE_ERROR_IO_ERROR;

		if (zmq_recv(it->second.requester, p_result, size_result, 0) < 0)
			return SERVICE_ERROR_IO_ERROR;

		return SERVICE_NO_ERROR;
	}

	return SERVICE_ERROR_WRONG_BASE;
}


/** Add the base names for this Channels.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

*/
void Channels::base_names(BaseNames &base_names) {

	base_names["bash"] = this;		// Runs shell scripts
	base_names["file"] = this;		// Returns arrays of bytes with attributes for files, IndexIS for folders.
	base_names["http"] = this;		// libCURL
	base_names["0-mq"] = this;		// zeroMQ (client)
}


/** Forwards an HTTP_GET call to another node in the Jazz cluster.

	\param p_txn  A pTransaction owned by Channels. It must be destroy_transaction()-ed after successful use.
	\param node	  The name of the endpoint node. It must be found in the cluster config.
	\param p_url  The unparsed url (server excluded) the remote Jazz server can serve.

	\return		  MHD_HTTP_OK on success, or some valid http status error code.
*/
MHD_StatusCode Channels::forward_get(pTransaction &p_txn, Name node, pChar p_url) {

	char buffer[1024];

	if (!curl_ok)
		return SERVICE_ERROR_BASE_FORBIDDEN;

	if (!compose_url(buffer, (pChar) node, p_url, sizeof(buffer)))
		return SERVICE_ERROR_UNKNOWN_JAZZNODE;

	return curl_get(p_txn, buffer);
}


/** Forwards an HTTP_PUT call to another node in the Jazz cluster.

	\param node		The name of the endpoint node. It must be found in the cluster config.
	\param p_url	The unparsed url (server excluded) the remote Jazz server can serve.
	\param p_block	A block to be put (owned by the caller).
	\param mode		WRITE_TENSOR_DATA, WRITE_C_STR or WRITE_EVERYTHING (see curl_put).

	\return			MHD_HTTP_CREATED on success, or some valid http status error code.
*/
MHD_StatusCode Channels::forward_put(Name node, pChar p_url, pBlock p_block, int mode) {

	char buffer[1024];

	if (!curl_ok)
		return SERVICE_ERROR_BASE_FORBIDDEN;

	if (!compose_url(buffer, (pChar) node, p_url, sizeof(buffer)))
		return SERVICE_ERROR_UNKNOWN_JAZZNODE;

	if (p_block->hash64 == 0)
		p_block->close_block();

	return curl_put(buffer, p_block, mode);
}


/** Forwards an HTTP_DELETE call to another node in the Jazz cluster.

	\param node	  The name of the endpoint node. It must be found in the cluster config.
	\param p_url  The unparsed url (server excluded) the remote Jazz server can serve.

	\return		  MHD_HTTP_OK on success, or some valid http status error code.
*/
MHD_StatusCode Channels::forward_del(Name node, pChar p_url) {

	char buffer[1024];

	if (!curl_ok)
		return SERVICE_ERROR_BASE_FORBIDDEN;

	if (!compose_url(buffer, (pChar) node, p_url, sizeof(buffer)))
		return SERVICE_ERROR_UNKNOWN_JAZZNODE;

	return curl_remove(buffer);
}

#ifdef CATCH_TEST

Channels CHN(&LOGGER, &CONFIG);

#endif

} // namespace jazz_elements

#ifdef CATCH_TEST
#include "src/jazz_elements/tests/test_channel.ctest"
#endif
