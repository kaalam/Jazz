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


#include "src/jazz_core/base_api.h"


namespace jazz_core
{

/*	-----------------------------------------------
	 Parser grammar definition
--------------------------------------------------- */

#define REX_SLASH				"[/]"
#define REX_NAME_FIRST			"[a-zA-Z0-9~]"
#define REX_NAME_ANY			"[a-zA-Z0-9\\-_~$]"
#define REX_BASE_SWITCH			"[&]"
#define REX_INFO_SWITCH			"[\\x00]"
#define REX_ENT_SWITCH			"[\\x00\\.\\]\\)]"
#define REX_KEY_SWITCH			"[\\x00\\.:=\\[\\(\\]\\)]"

#define MAX_NUM_PSTATES			14		///< Maximum number of non error states the parser can be in
#define NUM_STATE_TRANSITIONS	20		///< Maximum number of state transitions in the parsing grammar. Applies to const only.

/** A vector of StateTransition. This only runs once, on construction of the API object, initializes the LUTs from a sequence of
StateTransition constants in the source of api.cpp.
*/
typedef ParseStateTransition ParseStateTransitions[NUM_STATE_TRANSITIONS];

/** The parser logic defined in terms of transitions between states.
*/
ParseStateTransitions state_tr = {
	{PSTATE_INITIAL,	PSTATE_BASE0,		REX_SLASH},

	{PSTATE_BASE0,		PSTATE_NODE0,		REX_SLASH},
	{PSTATE_BASE0,		PSTATE_IN_BASE,		REX_NAME_FIRST},

	{PSTATE_NODE0,		PSTATE_IN_NODE,		REX_NAME_FIRST},
	{PSTATE_NODE0,		PSTATE_INFO_SWITCH,	REX_INFO_SWITCH},

	{PSTATE_IN_BASE,	PSTATE_IN_BASE,		REX_NAME_ANY},
	{PSTATE_IN_BASE,	PSTATE_ENTITY0,		REX_SLASH},
	{PSTATE_IN_BASE,	PSTATE_BASE_SWITCH,	REX_BASE_SWITCH},

	{PSTATE_IN_NODE,	PSTATE_IN_NODE,		REX_NAME_ANY},
	{PSTATE_IN_NODE,	PSTATE_DONE_NODE,	REX_SLASH},

	{PSTATE_DONE_NODE,	PSTATE_BASE0,		REX_SLASH},

	{PSTATE_ENTITY0,	PSTATE_IN_ENTITY,	REX_NAME_FIRST},

	{PSTATE_IN_ENTITY,	PSTATE_IN_ENTITY,	REX_NAME_ANY},
	{PSTATE_IN_ENTITY,	PSTATE_KEY0,		REX_SLASH},
	{PSTATE_IN_ENTITY,	PSTATE_ENT_SWITCH,	REX_ENT_SWITCH},

	{PSTATE_KEY0,		PSTATE_IN_KEY,		REX_NAME_FIRST},
	{PSTATE_KEY0,		PSTATE_KEY_SWITCH,	REX_KEY_SWITCH},

	{PSTATE_IN_KEY,		PSTATE_IN_KEY,		REX_NAME_ANY},
	{PSTATE_IN_KEY,		PSTATE_KEY_SWITCH,	REX_KEY_SWITCH},

	{MAX_NUM_PSTATES}
};

/** The parser logic defined as a LUT (initialized by compile_next_state_LUT()).
*/
ParseNextStateLUT parser_state_switch[MAX_NUM_PSTATES];

/*	-----------------------------------------------
	 BaseAPI : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** \brief Creates a BaseAPI service without starting it.

	\param a_logger		A pointer to the logger.
	\param a_config		A pointer to the configuration.
	\param a_channels	A pointer to an initialized Channels Container.
	\param a_volatile	A pointer to an initialized Volatile Container.
	\param a_persisted	A pointer to an initialized Persisted Container.

*/
BaseAPI::BaseAPI(pLogger a_logger,
				 pConfigFile a_config,
				 pChannels a_channels,
				 pVolatile a_volatile,
				 pPersisted a_persisted) : Container(a_logger, a_config) {

	p_channels	= a_channels;
	p_volatile	= a_volatile;
	p_persisted	= a_persisted;

	compile_next_state_LUT(parser_state_switch, MAX_NUM_PSTATES, state_tr);
}

BaseAPI::~BaseAPI() { destroy_container(); }


/** Return object ID.

	\return A string identifying the object that is especially useful to track uplifts and versions.
*/
pChar const BaseAPI::id() {
    static char arr[] = "BaseAPI from Jazz-" JAZZ_VERSION;
    return arr;
}


/** Starts the BaseAPI service

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode BaseAPI::start() {

	int ret = Container::start();	// This initializes the one-shot functionality.

	if (ret != SERVICE_NO_ERROR)
		return ret;

	memset(base_server, 0, sizeof(base_server));

	BaseNames base = {};

	p_channels->base_names(base);
	p_volatile->base_names(base);
	p_persisted->base_names(base);

	for (BaseNames::iterator it = base.begin(); it != base.end(); ++it) {
		int tt = TenBitsAtAddress(it->first.c_str());

		if (base_server[tt] != nullptr) {
			log_printf(LOG_ERROR, "BaseAPI::start(): Base name conflict with \"%s\"", it->first.c_str());

			return SERVICE_ERROR_STARTING;
		}
		base_server[tt] = it->second;
	}

	return SERVICE_NO_ERROR;
}


/** Shuts down the BaseAPI Service

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode BaseAPI::shut_down() {

	return destroy_container();
}


/** Parse an API url into an APIParseBuffer for later execution.

	\param q_state	A structure with the parts the url successfully parsed ready to be executed.
	\param p_url	The http url (that has already been checked to start with //)
	\param method	The http method in [HTTP_NOTUSED .. BASE_API_DELETE]
	\param recurse	True in an assignment while processing the r_value

	\return			Some error code or SERVICE_NO_ERROR if successful.

	When parse() is successful, the content of the APIParseBuffer **must** be executed by a call (that depends on the method) and will
unlock() all the intermediate blocks.

method | call executed by
-------|-----------------
BASE_API_GET, HTTP_HEAD | API.http_get()
BASE_API_PUT | API.http_put()
BASE_API_DELETE | API.http_delete()

*/
bool BaseAPI::parse(ApiQueryState &q_state, pChar p_url, int method, bool recurse) {

	int buf_size;
	pChar p_out;

	if (!recurse) {
		q_state.l_node[0] = 0;
		q_state.r_node[0] = 0;
		q_state.name[0]	  = 0;
	}
	q_state.url[0] = 0;
	q_state.apply  = APPLY_NOTHING;
	q_state.state  = PSTATE_INITIAL;

	p_url++;	// parse() is only called after checking the trailing //, this skips the first / to set state to PSTATE_INITIAL

	while (true) {
		unsigned char cursor;

		cursor = *(p_url++);
		q_state.state = parser_state_switch[q_state.state].next[cursor];
		if ((q_state.state == PSTATE_KEY_SWITCH) && (cursor == '.') && (p_url[1] <= '9') && (p_url[1] >= '0'))
			q_state.state = PSTATE_IN_KEY;

		switch (q_state.state) {
		case PSTATE_NODE0:
			if (recurse)
				p_out = (pChar) &q_state.r_node;
			else
				p_out = (pChar) &q_state.l_node;

			buf_size = NAME_SIZE - 1;

			break;

		case PSTATE_BASE0:
			if (recurse)
				p_out = (pChar) &q_state.r_value.base;
			else
				p_out = (pChar) &q_state.base;

			buf_size = SHORT_NAME_SIZE - 1;

			break;

		case PSTATE_ENTITY0:
			if (recurse)
				p_out = (pChar) &q_state.r_value.entity;
			else
				p_out = (pChar) &q_state.entity;

			buf_size = NAME_SIZE - 1;

			break;

		case PSTATE_KEY0:
			if (recurse)
				p_out = (pChar) &q_state.r_value.key;
			else
				p_out = (pChar) &q_state.key;

			buf_size = NAME_SIZE - 1;

			break;

		case PSTATE_IN_NODE:
		case PSTATE_IN_BASE:
		case PSTATE_IN_ENTITY:
		case PSTATE_IN_KEY:
			if (buf_size-- == 0) {
				q_state.state = PSTATE_FAILED;

				return false;
			}
			*(p_out++) = cursor;
			*(p_out)   = 0;

			break;

		case PSTATE_INFO_SWITCH:
			q_state.base[0]	  = 0;
			q_state.entity[0] = 0;
			q_state.key[0]	  = 0;

			if (method == BASE_API_GET) {
				q_state.apply = APPLY_JAZZ_INFO;
				q_state.state = PSTATE_COMPLETE_OK;

				return true;
			}
			q_state.state = PSTATE_FAILED;

			return false;

		case PSTATE_BASE_SWITCH:
			int mc;
			if (recurse)
				mc = move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url - 1, q_state.r_value.base);
			else
				mc = move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url - 1, q_state.base);

			if (mc == RET_MV_CONST_FAILED) {
				q_state.state = PSTATE_FAILED;

				return false;
			}
			q_state.apply = mc == RET_MV_CONST_NOTHING ? APPLY_URL : APPLY_NEW_ENTITY;
			q_state.state = PSTATE_COMPLETE_OK;
			if (recurse) {
				q_state.r_value.entity[0] = 0;
				q_state.r_value.key[0]	  = 0;
			} else {
				q_state.entity[0] = 0;
				q_state.key[0]	  = 0;
			}
			return true;

		case PSTATE_ENT_SWITCH:
			if (cursor == 0 && method != BASE_API_DELETE) {
				q_state.state = PSTATE_FAILED;

				return false;
			}
			q_state.state = PSTATE_COMPLETE_OK;
			if (recurse)
				q_state.r_value.key[0] = 0;
			else
				q_state.key[0] = 0;

			if (cursor != '.')
				return true;

			if (method != BASE_API_GET || strcmp("new", p_url) != 0) {
				q_state.state = PSTATE_FAILED;

				return false;
			}
			q_state.apply = APPLY_NEW_ENTITY;

			return true;

		case PSTATE_KEY_SWITCH:
			q_state.state = PSTATE_FAILED;

			if (p_out == (pChar) q_state.key || p_out == (pChar) q_state.r_value.key) {
				if (cursor != '(')
					return false;

				if (recurse)
					q_state.r_value.key[0] = 0;
				else
					q_state.key[0] = 0;

				if (method != BASE_API_GET)
					return false;

				if (*p_url == '&' && move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url) == RET_MV_CONST_NOTHING)
					q_state.apply = APPLY_FUNCT_CONST;
				else if (	*p_url == '/'
						 && ((recurse && parse_locator(q_state.rr_value, p_url)) || (!recurse && parse_locator(q_state.r_value, p_url))))
					q_state.apply = APPLY_FUNCTION;
				else
					return false;

				q_state.state = PSTATE_COMPLETE_OK;

				return true;
			}

			switch (cursor) {
			case 0:
				q_state.state = PSTATE_COMPLETE_OK;

				return true;

			case '.':
				if (method != BASE_API_GET && method != BASE_API_PUT)
					return false;

				if (strcmp("raw", p_url) == 0) {
					q_state.state = PSTATE_COMPLETE_OK;
					q_state.apply = APPLY_RAW;

					return true;
				}
				if (strcmp("text", p_url) == 0) {
					q_state.state = PSTATE_COMPLETE_OK;
					q_state.apply = APPLY_TEXT;

					return true;
				}
				if (method != BASE_API_GET)
					return false;

				if (strcmp("new", p_url) == 0) {
					q_state.state = PSTATE_COMPLETE_OK;
					q_state.apply = APPLY_NEW_ENTITY;

					return true;
				}
				if (strncmp(p_url, "attribute(", 10) != 0)
					return false;

				p_url += 10;

				int i_len;

				if (sscanf(p_url, "%i%n", &q_state.r_value.attribute, &i_len) != 1)
					return false;

				p_url += i_len;

				if (*(p_url++) != ')')
					return false;

				switch (*(p_url++)) {
				case 0:
					q_state.state = PSTATE_COMPLETE_OK;
					q_state.apply = APPLY_GET_ATTRIBUTE;

					return true;

				case '=':
					if (*p_url == '&' && move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url) == RET_MV_CONST_NOTHING) {
						q_state.state = PSTATE_COMPLETE_OK;
						q_state.apply = APPLY_SET_ATTRIBUTE;

						return true;
					}
				}
				return false;

			case ':':
				if (method != BASE_API_GET || strlen(p_url) >= NAME_SIZE)
					return false;

				strcpy(q_state.name, p_url);
				q_state.apply = APPLY_NAME;
				q_state.state = PSTATE_COMPLETE_OK;

				return true;

			case '=':
				if (recurse || (method != BASE_API_GET))
					return false;

				if (*p_url == '&' && move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url) == RET_MV_CONST_NOTHING) {
					q_state.apply = APPLY_ASSIGN_CONST;
					q_state.state = PSTATE_COMPLETE_OK;

					return true;
				}
				if ((*p_url != '/') || !parse(q_state, p_url, BASE_API_GET, true))
					return false;

				switch (q_state.apply) {
				case APPLY_NOTHING:
					q_state.apply = APPLY_ASSIGN_NOTHING;
					return true;
				case APPLY_NAME:
					q_state.apply = APPLY_ASSIGN_NAME;
					return true;
				case APPLY_URL:
					q_state.apply = APPLY_ASSIGN_URL;
					return true;
				case APPLY_FUNCTION:
					q_state.apply = APPLY_ASSIGN_FUNCTION;
					return true;
				case APPLY_FUNCT_CONST:
					q_state.apply = APPLY_ASSIGN_FUNCT_CONST;
					return true;
				case APPLY_FILTER:
					q_state.apply = APPLY_ASSIGN_FILTER;
					return true;
				case APPLY_FILT_CONST:
					q_state.apply = APPLY_ASSIGN_FILT_CONST;
					return true;
				case APPLY_RAW:
					q_state.apply = APPLY_ASSIGN_RAW;
					return true;
				case APPLY_TEXT:
					q_state.apply = APPLY_ASSIGN_TEXT;
					return true;
				}
				q_state.state = PSTATE_FAILED;

				return false;

			case '[':
				if (method != BASE_API_GET)
					return false;

				if (*p_url == '&' && move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url) == RET_MV_CONST_NOTHING)
					q_state.apply = APPLY_FILT_CONST;
				else if (	*p_url == '/'
						 && ((recurse && parse_locator(q_state.rr_value, p_url)) || (!recurse && parse_locator(q_state.r_value, p_url))))

					q_state.apply = APPLY_FILTER;
				else
					return false;

				q_state.state = PSTATE_COMPLETE_OK;

				return true;

			case '(':
				if (method != BASE_API_GET)
					return false;

				if (*p_url == '&' && move_const((pChar) &q_state.url, MAX_FILE_OR_URL_SIZE, p_url) == RET_MV_CONST_NOTHING)
					q_state.apply = APPLY_FUNCT_CONST;
				else if (	*p_url == '/'
						 && ((recurse && parse_locator(q_state.rr_value, p_url)) || (!recurse && parse_locator(q_state.r_value, p_url))))
					q_state.apply = APPLY_FUNCTION;
				else
					return false;

				q_state.state = PSTATE_COMPLETE_OK;

				return true;

			default:
				q_state.state = PSTATE_FAILED;

				return false;
			}
			break;

		case PSTATE_DONE_NODE:
			if (!recurse) {
				q_state.url[0] = '/';
				strcpy((pChar) &q_state.url[1], p_url);
			}
			break;

		default:
			q_state.state = PSTATE_FAILED;

			return false;
		}
	}
}


/** Parse a simple //base/entity/key string (Used inside the main API.parse()).

	\param loc	 A Locator to store the result (that will be left in undetermined on error).
	\param p_url The input string.

	\return		 `true` if successful.
*/
bool BaseAPI::parse_locator(Locator &loc, pChar p_url) {

	int buf_size, state = PSTATE_INITIAL;
	pChar p_out;

	loc.p_extra = nullptr;

	p_url++;	// parse_locator() is only called after checking the trailing /, this skips the first / to set state to PSTATE_INITIAL

	while (true) {
		unsigned char cursor;

		cursor = *(p_url++);
		state = parser_state_switch[state].next[cursor];

		switch (state) {
		case PSTATE_BASE0:
			p_out	 = (pChar) &loc.base;
			buf_size = SHORT_NAME_SIZE - 1;

			break;

		case PSTATE_ENTITY0:
			p_out	 = (pChar) &loc.entity;
			buf_size = NAME_SIZE - 1;

			break;

		case PSTATE_KEY0:
			p_out	 = (pChar) &loc.key;
			buf_size = NAME_SIZE - 1;

			break;

		case PSTATE_IN_BASE:
		case PSTATE_IN_ENTITY:
		case PSTATE_IN_KEY:
			if (buf_size-- == 0)
				return false;

			*(p_out++) = cursor;
			*(p_out)   = 0;

			break;

		case PSTATE_ENT_SWITCH:
			loc.key[0] = 0;

		case PSTATE_KEY_SWITCH:
			if (p_out == (pChar) loc.key)
				return false;

			switch (cursor) {
			case 0:
				return true;
			case ')':
			case ']':
				return *(p_url) == 0;
			}
			return false;

		default:
			return false;
		}
	}
}


// API Container interface
// -----------------------


/** "API" interface **metadata of a Block** retrieval. This uses a parse()d what and is the only BasePI + descendants GET method.

	\param hea	A StaticBlockHeader structure that will receive the metadata.
	\param what	Some successfully parse()d ApiQueryState that also distinguishes API interface from Container interface.

	\return	SERVICE_NO_ERROR on success (and a valid hea), or some negative value (error).

This is a faster, not involving RAM allocation version of the other form of header. For a tensor, is will be the only thing you need, but
for a Kind or a Tuple, you probably want the types of all its items and need to pass a pTransaction to hold the data.

NOTE: It only supports APPLY_NOTHING and APPLY_NAME. It is used by Core to check argument types of Tensors or Tuple items.
*/
StatusCode BaseAPI::header(StaticBlockHeader &hea, ApiQueryState &what) {

	switch (what.apply) {
	case APPLY_NOTHING:
	case APPLY_NAME:
		break;
	default:
		return SERVICE_ERROR_WRONG_ARGUMENTS;
	}

	if (what.l_node[0] != 0) {
		pTransaction p_txn;
		int ret = p_channels->forward_get(p_txn, what.l_node, what.url);
		if (ret == SERVICE_NO_ERROR) {
			memcpy(&hea, p_txn->p_block, sizeof(StaticBlockHeader));
			p_channels->destroy_transaction(p_txn);
		}
		return ret;
	}

	pContainer p_container = (pContainer) base_server[TenBitsAtAddress(what.base)];

	if (p_container == nullptr)
		return SERVICE_ERROR_WRONG_BASE;

	Locator loc;
	memcpy(&loc, &what.base, SIZE_OF_BASE_ENT_KEY);

	if (what.apply == APPLY_NOTHING)
		return p_container->header(hea, loc);

	pTransaction p_txn;
	int ret = p_container->get(p_txn, loc, what.name);
	if (ret == SERVICE_NO_ERROR) {
		memcpy(&hea, p_txn->p_block, sizeof(StaticBlockHeader));
		p_container->destroy_transaction(p_txn);
	}
	return ret;
}


/** "API" interface **complete Block** retrieval. This uses a parse()d what and is the only BasePI + descendants GET method.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param what		Some successfully parse()d ApiQueryState that also distinguishes API interface from Container interface.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

In the BaseAPI class, this implements all the apply cases from APPLY_NOTHING to APPLY_SET_ATTRIBUTE (APPLY_JAZZ_INFO is http only).
What the aseAPI class does is forwarding the request to the right container (if the base is found, returning SERVICE_ERROR_WRONG_BASE
if not).

In the descendants (Core and ModelsAPI) it should support the range from APPLY_NOTHING to APPLY_TEXT. This includes function calls
APPLY_FUNCTION and APPLY_FUNCT_CONST, but also APPLY_FILTER and APPLY_FILT_CONST to select from the result of a function call.
Also, APPLY_URL is very convenient for passing text as an argument to a function. APPLY_NOTHING can return some metadata about
the model including a list of endpoints. APPLY_NAME can define specifics of an endpoint. APPLY_RAW and APPLY_TEXT can be used to
select the favorite serialization format of the result. Therefore, the function interface should be considered as the whole range
and not just APPLY_FUNCTION.
*/
StatusCode BaseAPI::get(pTransaction &p_txn, ApiQueryState &what) {

	int	ret;

	p_txn = nullptr;

	switch (what.apply) {
	case APPLY_NOTHING ... APPLY_TEXT:
		if (what.l_node[0] != 0)
			return p_channels->forward_get(p_txn, what.l_node, what.url);

		return get_left_local(p_txn, what);

	case APPLY_ASSIGN_NOTHING ... APPLY_ASSIGN_TEXT:
		if (what.r_node[0] != 0)
			ret = get_right_remote(p_txn, what);
		else
			ret = get_right_local(p_txn, what);

		if (ret != SERVICE_NO_ERROR)
			return ret;

		if (what.l_node[0] != 0) {
			char buffer_2k[SIZE_BUFFER_REMOTE_CALL];

			sprintf(buffer_2k, "//%s/%s/%s", what.base, what.entity, what.key);

			ret = p_channels->forward_put(what.l_node, buffer_2k, p_txn->p_block);
		} else
			ret = put_left_local(what, p_txn->p_block);

		destroy_transaction(p_txn);

		return ret;

	case APPLY_ASSIGN_CONST:
		if (what.l_node[0] != 0) {
			ret = p_channels->forward_get(p_txn, what.l_node, what.url);

			if (ret != SERVICE_NO_ERROR)
				return ret;
		} else {
			ret = get_right_local(p_txn, what);

			if (ret != SERVICE_NO_ERROR)
				return ret;

			ret = put_left_local(what, p_txn->p_block);
		}
		destroy_transaction(p_txn);

		return ret;

	case APPLY_NEW_ENTITY:
		if (what.l_node[0] != 0) {
			ret = p_channels->forward_get(p_txn, what.l_node, what.url);
			if (ret == SERVICE_NO_ERROR)
				p_channels->destroy_transaction(p_txn); }
		else {
			Locator	   loc;
			pContainer p_container = (pContainer) base_server[TenBitsAtAddress(what.base)];

			if (p_container == nullptr)
				return SERVICE_ERROR_WRONG_BASE;

			if (what.url[0] == 0) {
				memcpy(&loc, &what.base, SIZE_OF_BASE_ENT_KEY);

				ret = p_container->new_entity(loc);
			} else
				ret = p_container->new_entity((pChar) what.url);
		}
		return ret;

	case APPLY_GET_ATTRIBUTE: {
		if (what.l_node[0] != 0)
			return p_channels->forward_get(p_txn, what.l_node, what.url);

		Locator	   loc;
		pContainer p_container = (pContainer) base_server[TenBitsAtAddress(what.base)];

		if (p_container == nullptr)
			return SERVICE_ERROR_WRONG_BASE;

		memcpy(&loc, &what.base, SIZE_OF_BASE_ENT_KEY);

		return p_container->get(p_txn, loc); }

	case APPLY_SET_ATTRIBUTE:
		if (what.l_node[0] != 0)
			return p_channels->forward_get(p_txn, what.l_node, what.url);

		Locator	   loc;
		pContainer p_container = (pContainer) base_server[TenBitsAtAddress(what.base)];

		if (p_container == nullptr)
			return SERVICE_ERROR_WRONG_BASE;

		memcpy(&loc, &what.base, SIZE_OF_BASE_ENT_KEY);

		pTransaction p_aux;
		if ((ret = p_container->get(p_aux, loc)) != SERVICE_NO_ERROR)
			return ret;

		AttributeMap atts;
		p_aux->p_block->get_attributes(&atts);

		atts[what.r_value.attribute] = what.url;

		ret = new_block(p_txn, p_aux->p_block, (pBlock) nullptr, &atts);
		p_container->destroy_transaction(p_aux);

		if (ret != SERVICE_NO_ERROR)
			return ret;

		ret = p_container->put(loc, p_txn->p_block);

		destroy_transaction(p_txn);

		return ret;
	}

	return SERVICE_ERROR_WRONG_ARGUMENTS;
}


/** "API" interface for **Block storing**: This uses a parse()d `where` and is the only BasePI + descendants PUT method.

	\param where	Some successfully parse()d ApiQueryState that also distinguishes API interface from Container interface.
	\param p_block	A block to be stored. Notice it is a block, not a Transaction. If necessary, the Container will make a copy, write to
					disc, PUT it via http, etc. The container does not own the pointer in any way.
	\param mode		Some writing restriction, either WRITE_ONLY_IF_EXISTS or WRITE_ONLY_IF_NOT_EXISTS. WRITE_TENSOR_DATA returns
					the error SERVICE_ERROR_WRONG_ARGUMENTS. (See NOTE below.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

NOTE: The http API does not use mode, but everything in jazz_elements does. Especially, anything in Channels that uses http as a client
makes intensive use of WRITE_AS_STRING, WRITE_AS_CONTENT, ... Also, it is nice that Persisted supports WRITE_ONLY_IF_EXISTS and
WRITE_ONLY_IF_NOT_EXISTS to support things like one-time initialization or preventing undesired creation of new variables. Therefore,
think twice before completely removing mode even if the http API does not use it. At Bebop level and model level, it can be used.

NOTE: From an API perspective, put() only supports: APPLY_NOTHING, APPLY_RAW, APPLY_TEXT and APPLY_URL (both local and remote).
*/
StatusCode BaseAPI::put(ApiQueryState &where, pBlock p_block, int mode) {

	if (where.l_node[0] != 0)
		return p_channels->forward_put(where.l_node, where.url, p_block);

	pContainer p_container = (pContainer) base_server[TenBitsAtAddress(where.base)];

	if (p_container == nullptr)
		return SERVICE_ERROR_WRONG_BASE;

	int			 ret;
	Locator		 loc;
	pTransaction p_aux;

	switch (where.apply) {
	case APPLY_NOTHING:
		memcpy(&loc, &where.base, SIZE_OF_BASE_ENT_KEY);

		return p_container->put(loc, p_block);

	case APPLY_RAW:
		ret = new_block(p_aux, p_block, CELL_TYPE_UNDEFINED);

		if (ret != SERVICE_NO_ERROR)
			return ret;

		memcpy(&loc, &where.base, SIZE_OF_BASE_ENT_KEY);

		ret = p_container->put(loc, p_aux->p_block);

		destroy_transaction(p_aux);

		return ret;

	case APPLY_TEXT:
		ret = new_block(p_aux, p_block, nullptr, true);

		if (ret != SERVICE_NO_ERROR)
			return ret;

		memcpy(&loc, &where.base, SIZE_OF_BASE_ENT_KEY);

		ret = p_container->put(loc, p_aux->p_block);

		destroy_transaction(p_aux);

		return ret;

	case APPLY_URL:
		return p_container->put(where.url, p_block);

	default:
		return SERVICE_ERROR_WRONG_ARGUMENTS;
	}
}


/** The "API" interface: This uses a parse()d `what` and

	\param what Some successfully parse()d ApiQueryState that also distinguishes API interface from Container interface.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

Internals
---------

It supports any successful HTTP_PUT syntax, that is:

APPLY_NOTHING: With or without node, mandatory base and entity, with of without a key.
APPLY_URL: With or without node and just a base.

In all cases, calls with a node (it can only be l_node) `what.url` contains exactly what has to be forwarded.
*/
StatusCode BaseAPI::remove(ApiQueryState &what) {

	if (what.l_node[0] != 0)
		return p_channels->forward_del(what.l_node, what.url);

	pContainer p_container = (pContainer) base_server[TenBitsAtAddress(what.base)];

	if (p_container == nullptr)
		return SERVICE_ERROR_WRONG_BASE;

	Locator loc;

	switch (what.apply) {
	case APPLY_NOTHING:
		memcpy(&loc, &what.base, SIZE_OF_BASE_ENT_KEY);

		return p_container->remove(loc);
	case APPLY_URL:
		return p_container->remove((pChar) what.url);

	default:
		return SERVICE_ERROR_WRONG_ARGUMENTS;
	}
}

// Protected methods
// -----------------

/** Creates a block from a constant read in the URL.

	\param p_txn	  A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					  Transaction inside the Container. The caller can only use it read-only and **must** destroy_transaction() when done.
	\param p_const	  The constant.
	\param make_tuple Insert it into a Tuple with "input" and "result".

	\return			'true' if successful.
*/
bool BaseAPI::block_from_const(pTransaction &p_txn, pChar p_const, bool make_tuple) {

	p_txn = nullptr;

	pTransaction p_text, p_tensor, p_result;

	int dim[MAX_TENSOR_RANK] = {0};

	if (make_tuple) {
		int size = strlen(p_const);
		dim[0] = size;
		dim[1] = 0;

		if (new_block(p_text, CELL_TYPE_BYTE, &dim[0], FILL_NEW_DONT_FILL) != SERVICE_NO_ERROR)
			return false;

		memcpy(&p_text->p_block->tensor, p_const, size);
	} else {
		if (new_block(p_text, CELL_TYPE_STRING, nullptr, FILL_WITH_TEXTFILE, 0, (pChar) p_const, 0) != SERVICE_NO_ERROR)
			return false;
	}

	if (new_block(p_tensor, p_text->p_block, CELL_TYPE_UNDEFINED) == SERVICE_NO_ERROR)
		destroy_transaction(p_text);
	else
		p_tensor = p_text;

	if (!make_tuple) {
		p_txn = p_tensor;

		return true;
	}

	dim[0] = RESULT_BUFFER_SIZE;
	dim[1] = 0;
	if (new_block(p_result, CELL_TYPE_BYTE, &dim[0], FILL_NEW_WITH_ZERO) !=  SERVICE_NO_ERROR) {
		destroy_transaction(p_tensor);

		return false;
	}

	StaticBlockHeader hea[2];
	Name			  names[2]	 = {"input", "result"};
	pBlock			  p_block[2] = {p_tensor->p_block, p_result->p_block};

	memcpy(&hea[0], p_block[0], sizeof(StaticBlockHeader));
	memcpy(&hea[1], p_block[1], sizeof(StaticBlockHeader));

	p_block[0]->get_dimensions(hea[0].range.dim);
	p_block[1]->get_dimensions(hea[1].range.dim);

	int ret = new_block(p_txn, 2, hea, names, p_block);

	destroy_transaction(p_tensor);
	destroy_transaction(p_result);

	return ret == SERVICE_NO_ERROR;
}


#ifdef CATCH_TEST

BaseAPI BAPI(&LOGGER, &CONFIG, &CHN, &VOL, &PER);

#endif

} // namespace jazz_core

#if defined CATCH_TEST
#include "src/jazz_core/tests/test_base_api.ctest"
#endif
