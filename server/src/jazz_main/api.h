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


#include <map>

#include "src/include/jazz_models.h"

#ifdef CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_MAIN_API
#define INCLUDED_JAZZ_MAIN_API


#define MHD_PLATFORM_H					// Following recommendation in: 1.5 Including the microhttpd.h header


#include <microhttpd.h>

#if MHD_VERSION < 0x00097000
/*! \brief Replacement type for the future (libmicrohttpd-dev 0.9.72-2)

In a version somewhere between > 0.9.66-1 and <= 0.9.72-2 libmicrohttpd changed the return type of int MHD_YES and MHD_NO to a:

enum MHD_Result {MHD_NO = 0, MHD_YES = 1};

*/
typedef int MHD_Result;
#endif


/*! \brief The http API, instancing and building the server.

	This small namespace is about the server running and putting everything together. Unlike jazz_elements, jazz_bebop and jazz_model
	it is not intended to build other applications than the server.
*/
namespace jazz_main
{

//TODO: Create interface for ACTOR model (as described in develop/rfc2/jazz_actor_model.html)

using namespace jazz_elements;
using namespace jazz_bebop;
using namespace jazz_model;

#define SIZE_OF_BASE_ENT_KEY	(sizeof(Locator) - sizeof(pExtraLocator))	///< Used to convert HttpQueryState -> Locator

#define MAX_RECURSE_LEVEL_ON_STATICS		16	///< The max directory recursion depth for load_statics()
#define RESULT_BUFFER_SIZE				  4096	///< The "result" item size in a Tuple used in a modify() call.

#define RET_MV_CONST_FAILED					-1	///< return value for move_const() failed.
#define RET_MV_CONST_NOTHING				 0	///< return value for move_const() normal moving.
#define RET_MV_CONST_NEW_ENTITY				 1	///< return value for move_const() there is a ";.new" ending, otherwise parses ok.

// Values of http_put(sequence)
#define	SEQUENCE_FIRST_CALL					 0	///< First call, no pTransaction was yet assigned (data must be stored)
#define	SEQUENCE_INCREMENT_CALL				 1	///< Any number of these calls (including none) allocate bigger and keep storing
#define	SEQUENCE_FINAL_CALL					 2	///< Last call, no more data this time, do the magic and return a status code

/// Http methods

#define HTTP_NOTUSED						 0	///< Rogue value to fill the LUTs
#define HTTP_OPTIONS						 1	///< http predicate OPTIONS
#define HTTP_HEAD							 2	///< http predicate HEAD
#define HTTP_GET							 3	///< http predicate GET
#define HTTP_PUT							 4	///< http predicate PUT
#define HTTP_DELETE							 5	///< http predicate DELETE

/// Parser state values

#define PSTATE_INITIAL						 0	///< Begin parsing, assumes already seen / to avoid unnecessary initial counting
#define PSTATE_DONE_NODE					 1	///< Equivalent to PSTATE_INITIAL, except node was done and it cannot happen again
#define PSTATE_NODE0						 2	///< Already seen ///
#define PSTATE_IN_NODE						 3	///< Name starts after /// + letter, stays with valid char
#define PSTATE_BASE0						 4	///< Already seen //
#define PSTATE_IN_BASE						 5	///< Name starts after // + letter, stays with valid char
#define PSTATE_ENTITY0						 6	///< Already seen / after reading a base
#define PSTATE_IN_ENTITY					 7	///< Name starts after / + letter, stays with valid char
#define PSTATE_KEY0							 8	///< Already seen / after reading an entity
#define PSTATE_IN_KEY						 9	///< Name starts after / + letter, stays with valid char
#define PSTATE_INFO_SWITCH					10	///< The final switch inside or after a key: END, =, ., :, [, [&, (, or (&
#define PSTATE_BASE_SWITCH					11	///< Found # while reading a base
#define PSTATE_ENT_SWITCH					12	///< Found # while reading a base
#define PSTATE_KEY_SWITCH					13	///< The final switch inside or after a key: END, =, ., :, [, [&, (, or (&
#define PSTATE_FAILED						98	///< Set by the parser on any error (possibly in the r_value too)
#define PSTATE_COMPLETE_OK					99	///< Set by the parser on complete success

/** \brief A buffer to keep the state while parsing/executing a query
*/
struct HttpQueryState {
	int	state;									///< The parser state from PSTATE_INITIAL to PSTATE_COMPLETE_OK
	int apply;									///< APPLY_NOTHING, APPLY_NAME, APPLY_URL, APPLY_FUNCTION, .. APPLY_ASSIGN_CONST

	char l_node	[NAME_SIZE];					///< An optional name of a Jazz node that is either the only one or the left of assignment.
	char r_node	[NAME_SIZE];					///< An additional optional name of a Jazz node for the right part in an assignment.
	char base	[SHORT_NAME_SIZE];				///< A Locator compatible base for the l_value.
	char entity	[NAME_SIZE];					///< A Locator compatible entity for the l_value.
	char key	[NAME_SIZE];					///< A Locator compatible key for the l_value.
	char name	[NAME_SIZE];					///< A possible item name
	char url	[MAX_FILE_OR_URL_SIZE];			///< The endpoint (an URL, file name, folder name, bash script)

	Locator r_value, rr_value;					///< Parsed //r_base/r_entity/r_key, //r_base/r_entity/r_key(//rr_base/rr_entity/rr_key)
};

extern TenBitPtrLUT base_server;

typedef struct MHD_Response *pMHD_Response;
typedef struct MHD_Connection *pMHD_Connection;

// ConfigFile and Logger shared by all services

extern ConfigFile  CONFIG;
extern Logger	   LOGGER;


extern MHD_Result http_request_callback(void *cls,
										struct MHD_Connection *connection,
										const char *url,
										const char *method,
										const char *version,
										const char *upload_data,
										size_t *upload_data_size,
										void **con_cls);


/** \brief Api: A Service to manage the REST API.

This service parses and executes http queries. It is aware and redistributes to all the appropriate services. It is called directly by
the http callback http_request_callback().

*/
class Api : public Container {

	public:

		Api(pLogger		a_logger,
			pConfigFile	a_config,
			pChannels	a_channels,
			pVolatile	a_volatile,
			pPersisted	a_persisted,
			pCore		a_core,
			pModel		a_model);
	   ~Api();

		virtual pChar const id();

		StatusCode start	();
		StatusCode shut_down();

		// parsing methods

		bool parse					   (HttpQueryState &q_state,
										pChar			p_url,
										int				method,
										bool			recurse = false);

		MHD_StatusCode get_static	   (pMHD_Response  &response,
										pChar			p_url,
										bool			get_it = true);

		// deliver http error pages

		MHD_Result return_error_message(pMHD_Connection	connection,
										pChar			p_url,
										int				http_status);

		// Specific execution methods

		MHD_StatusCode http_put		   (pChar			p_upload,
										size_t			size,
										HttpQueryState &q_state,
										int				sequence);
		MHD_StatusCode http_delete	   (HttpQueryState &q_state);
		MHD_StatusCode http_get		   (pMHD_Response  &response,
										HttpQueryState &q_state);

#ifndef CATCH_TEST
	private:
#endif

		bool find_myself		();
		StatusCode load_statics	(pChar			p_base_path,
								 pChar			p_relative_path,
								 int			rec_level);
		bool expand_url_encoded	(pChar			p_buff,
								 int			buff_size,
								 pChar			p_url);
		int move_const			(pChar			p_buff,
								 int			buff_size,
								 pChar			p_url,
								 pChar			p_base = nullptr);
		bool parse_locator		(Locator	   &loc,
								 pChar			p_url);
		bool block_from_const	(pTransaction  &p_txn,
								 pChar			p_const,
								 bool			make_tuple = false);


		/** This is an internal part of http_get() made independent to keep the function less crowded.

		Context: This is called when q_state.apply is APPLY_NOTHING ... APPLY_TEXT and there is no forwarding.
		It returns the final block as it will be returned to the user with a new_block() interface.
		*/
		inline StatusCode get_left_local(pTransaction &p_txn, HttpQueryState &q_state) {

			Locator		 loc;
			pTransaction p_aux;
			pContainer	 p_container, p_aux_cont;
			Name		 ent;

			p_container = (pContainer) base_server[TenBitsAtAddress(q_state.base)];

			if (p_container == nullptr)
				return SERVICE_ERROR_WRONG_BASE;

			switch (q_state.apply) {
			case APPLY_NOTHING:
				memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);
				return p_container->get(p_txn, loc);

			case APPLY_NAME:
				memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);
				return p_container->get(p_txn, loc, q_state.name);

			case APPLY_URL:
				return p_container->get(p_txn, (pChar) q_state.url);

			case APPLY_FUNCTION:
				p_aux_cont = (pContainer) base_server[TenBitsAtAddress(q_state.r_value.base)];

				if (p_aux_cont == nullptr)
					return SERVICE_ERROR_WRONG_BASE;

				if (p_aux_cont->get(p_aux, q_state.r_value) != SERVICE_NO_ERROR)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);
				if (q_state.key[0] == 0) {
					if (p_container->modify(loc, (pTuple) p_aux->p_block) != SERVICE_NO_ERROR) {
						p_aux_cont->destroy_transaction(p_aux);

						return SERVICE_ERROR_IO_ERROR;
					}
					strcpy(ent, "result");
					if (new_block(p_txn, (pTuple) p_aux->p_block, ent) != SERVICE_NO_ERROR) {
						p_aux_cont->destroy_transaction(p_aux);

						return SERVICE_ERROR_IO_ERROR;
					}
				} else {
					if (p_container->exec(p_txn, loc, (pTuple) p_aux->p_block) != SERVICE_NO_ERROR) {
						p_aux_cont->destroy_transaction(p_aux);

						return SERVICE_ERROR_IO_ERROR;
					}
				}
				p_aux_cont->destroy_transaction(p_aux);

				return SERVICE_NO_ERROR;

			case APPLY_FUNCT_CONST:
				if (!block_from_const(p_aux, q_state.url, q_state.key[0] == 0))
					return SERVICE_ERROR_WRONG_ARGUMENTS;

				memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);
				if (q_state.key[0] == 0) {
					if (p_container->modify(loc, (pTuple) p_aux->p_block) != SERVICE_NO_ERROR) {
						destroy_transaction(p_aux);

						return SERVICE_ERROR_IO_ERROR;
					}
					strcpy(ent, "result");
					if (new_block(p_txn, (pTuple) p_aux->p_block, ent) != SERVICE_NO_ERROR) {
						destroy_transaction(p_aux);

						return SERVICE_ERROR_IO_ERROR;
					}
				} else {
					if (p_container->exec(p_txn, loc, (pTuple) p_aux->p_block) != SERVICE_NO_ERROR) {
						destroy_transaction(p_aux);

						return SERVICE_ERROR_IO_ERROR;
					}
				}
				destroy_transaction(p_aux);

				return SERVICE_NO_ERROR;

			case APPLY_FILTER:
				p_aux_cont = (pContainer) base_server[TenBitsAtAddress(q_state.r_value.base)];

				if (p_aux_cont == nullptr)
					return SERVICE_ERROR_WRONG_BASE;

				if (p_aux_cont->get(p_aux, q_state.r_value) != SERVICE_NO_ERROR)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);
				if (p_container->get(p_txn, loc, p_aux->p_block) != SERVICE_NO_ERROR) {
					p_aux_cont->destroy_transaction(p_aux);

					return SERVICE_ERROR_IO_ERROR;
				}
				p_aux_cont->destroy_transaction(p_aux);

				return SERVICE_NO_ERROR;

			case APPLY_FILT_CONST:
				if (!block_from_const(p_aux, q_state.url))
					return SERVICE_ERROR_WRONG_ARGUMENTS;

				memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);
				if (p_container->get(p_txn, loc, p_aux->p_block) != SERVICE_NO_ERROR) {
					destroy_transaction(p_aux);

					return SERVICE_ERROR_IO_ERROR;
				}
				destroy_transaction(p_aux);

				return SERVICE_NO_ERROR;

			case APPLY_RAW:
				memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);
				if (p_container->get(p_aux, loc) != SERVICE_NO_ERROR)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				if (new_block(p_txn, p_aux->p_block, CELL_TYPE_UNDEFINED) != SERVICE_NO_ERROR) {
					p_container->destroy_transaction(p_aux);

					return SERVICE_ERROR_IO_ERROR;
				}
				p_container->destroy_transaction(p_aux);

				return SERVICE_NO_ERROR;

			case APPLY_TEXT:
				memcpy(&loc, &q_state.base, SIZE_OF_BASE_ENT_KEY);
				if (p_container->get(p_aux, loc) != SERVICE_NO_ERROR)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				if (new_block(p_txn, p_aux->p_block) != SERVICE_NO_ERROR) {
					p_container->destroy_transaction(p_aux);

					return SERVICE_ERROR_IO_ERROR;
				}
				p_container->destroy_transaction(p_aux);

				return SERVICE_NO_ERROR;
			}
			return SERVICE_ERROR_MISC_SERVER;
		}

		/** This is an internal part of http_get() made independent to keep the function less crowded.

		Context: This in any possible assignment in which the right part is a remote call. Since q_state.url does not have
				 a valid url, itt is necessary to reconstruct it. We do have the constants if any.
		It returns the final block as it will be returned to the user with a new_block() interface.
		*/
		inline StatusCode get_right_remote(pTransaction &p_txn, HttpQueryState &q_state) {

			char buffer_2k[2048];

			switch (q_state.apply) {
			case APPLY_ASSIGN_NOTHING:
				sprintf(buffer_2k, "//%s/%s/%s", q_state.r_value.base, q_state.r_value.entity, q_state.r_value.key);
				break;
			case APPLY_ASSIGN_NAME:
				sprintf(buffer_2k, "//%s/%s/%s:%s", q_state.r_value.base, q_state.r_value.entity, q_state.r_value.key, q_state.name);
				break;
			case APPLY_ASSIGN_URL:
				sprintf(buffer_2k, "//%s&%s;", q_state.r_value.base, q_state.url);
				break;
			case APPLY_ASSIGN_FUNCTION:
				sprintf(buffer_2k, "//%s/%s/%s(//%s/%s/%s)", q_state.r_value.base,  q_state.r_value.entity,  q_state.r_value.key,
															 q_state.rr_value.base, q_state.rr_value.entity, q_state.rr_value.key);
				break;
			case APPLY_ASSIGN_FUNCT_CONST:
				sprintf(buffer_2k, "//%s/%s/%s(&%s)", q_state.r_value.base, q_state.r_value.entity, q_state.r_value.key, q_state.url);
				break;
			case APPLY_ASSIGN_FILTER:
				sprintf(buffer_2k, "//%s/%s/%s[//%s/%s/%s]", q_state.r_value.base,  q_state.r_value.entity,  q_state.r_value.key,
															 q_state.rr_value.base, q_state.rr_value.entity, q_state.rr_value.key);
				break;
			case APPLY_ASSIGN_FILT_CONST:
				sprintf(buffer_2k, "//%s/%s/%s[&%s]", q_state.r_value.base, q_state.r_value.entity, q_state.r_value.key, q_state.url);
				break;
			case APPLY_ASSIGN_RAW:
				sprintf(buffer_2k, "//%s/%s/%s.raw", q_state.r_value.base, q_state.r_value.entity, q_state.r_value.key);
				break;
			case APPLY_ASSIGN_TEXT:
				sprintf(buffer_2k, "//%s/%s/%s.text", q_state.r_value.base, q_state.r_value.entity, q_state.r_value.key);
				break;
			default:
				return SERVICE_ERROR_WRONG_ARGUMENTS;
			}
			return p_channels->forward_get(p_txn, q_state.r_node, buffer_2k);
		}

		/** This is an internal part of http_get() made independent to keep the function less crowded.

		Context: This in any possible assignment in which the right part is NOT remote call. Functionally, it is similar to
		get_left_local(), but since it is the right of an assignment, arguments are stored at a different place and also, apply
		code are in range APPLY_ASSIGN_NOTHING..APPLY_ASSIGN_CONST instead of APPLY_NOTHING..APPLY_TEXT.
		It returns the final block as it will be returned to the user with a new_block() interface.
		*/
		inline StatusCode get_right_local(pTransaction &p_txn, HttpQueryState &q_state) {

			pTransaction p_aux;
			pContainer	 p_container, p_aux_cont;
			Name		 ent;

			if (q_state.apply == APPLY_ASSIGN_CONST)
				return block_from_const(p_txn, q_state.url) ? SERVICE_NO_ERROR : SERVICE_ERROR_WRONG_ARGUMENTS;

			p_container = (pContainer) base_server[TenBitsAtAddress(q_state.r_value.base)];

			if (p_container == nullptr)
				return SERVICE_ERROR_WRONG_BASE;

			switch (q_state.apply) {
			case APPLY_ASSIGN_NOTHING:
				return p_container->get(p_txn, q_state.r_value);

			case APPLY_ASSIGN_NAME:
				return p_container->get(p_txn, q_state.r_value, q_state.name);

			case APPLY_ASSIGN_URL:
				return p_container->get(p_txn, (pChar) q_state.url);

			case APPLY_ASSIGN_FUNCTION:
				p_aux_cont = (pContainer) base_server[TenBitsAtAddress(q_state.rr_value.base)];

				if (p_aux_cont == nullptr)
					return SERVICE_ERROR_WRONG_BASE;

				if (p_aux_cont->get(p_aux, q_state.rr_value) != SERVICE_NO_ERROR)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				if (q_state.r_value.key[0] == 0) {
					if (p_container->modify(q_state.r_value, (pTuple) p_aux->p_block) != SERVICE_NO_ERROR) {
						p_aux_cont->destroy_transaction(p_aux);

						return SERVICE_ERROR_IO_ERROR;
					}
					strcpy(ent, "result");
					if (new_block(p_txn, (pTuple) p_aux->p_block, ent) != SERVICE_NO_ERROR) {
						p_aux_cont->destroy_transaction(p_aux);

						return SERVICE_ERROR_IO_ERROR;
					}
				} else {
					if (p_container->exec(p_txn, q_state.r_value, (pTuple) p_aux->p_block) != SERVICE_NO_ERROR) {
						p_aux_cont->destroy_transaction(p_aux);

						return SERVICE_ERROR_IO_ERROR;
					}
				}
				p_aux_cont->destroy_transaction(p_aux);

				return SERVICE_NO_ERROR;

			case APPLY_ASSIGN_FUNCT_CONST:
				if (!block_from_const(p_aux, q_state.url, q_state.r_value.key[0] == 0))
					return SERVICE_ERROR_WRONG_ARGUMENTS;

				if (q_state.r_value.key[0] == 0) {
					if (p_container->modify(q_state.r_value, (pTuple) p_aux->p_block) != SERVICE_NO_ERROR) {
						destroy_transaction(p_aux);

						return SERVICE_ERROR_IO_ERROR;
					}
					strcpy(ent, "result");
					if (new_block(p_txn, (pTuple) p_aux->p_block, ent) != SERVICE_NO_ERROR) {
						destroy_transaction(p_aux);

						return SERVICE_ERROR_IO_ERROR;
					}
				} else {
					if (p_container->exec(p_txn, q_state.r_value, (pTuple) p_aux->p_block) != SERVICE_NO_ERROR) {
						destroy_transaction(p_aux);

						return SERVICE_ERROR_IO_ERROR;
					}
				}
				destroy_transaction(p_aux);

				return SERVICE_NO_ERROR;

			case APPLY_ASSIGN_FILTER:
				p_aux_cont = (pContainer) base_server[TenBitsAtAddress(q_state.rr_value.base)];

				if (p_aux_cont == nullptr)
					return SERVICE_ERROR_WRONG_BASE;

				if (p_aux_cont->get(p_aux, q_state.rr_value) != SERVICE_NO_ERROR)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				if (p_container->get(p_txn, q_state.r_value, p_aux->p_block) != SERVICE_NO_ERROR) {
					p_aux_cont->destroy_transaction(p_aux);

					return SERVICE_ERROR_IO_ERROR;
				}
				p_aux_cont->destroy_transaction(p_aux);

				return SERVICE_NO_ERROR;

			case APPLY_ASSIGN_FILT_CONST:
				if (!block_from_const(p_aux, q_state.url))
					return SERVICE_ERROR_WRONG_ARGUMENTS;

				if (p_container->get(p_txn, q_state.r_value, p_aux->p_block) != SERVICE_NO_ERROR) {
					destroy_transaction(p_aux);

					return SERVICE_ERROR_IO_ERROR;
				}
				destroy_transaction(p_aux);

				return SERVICE_NO_ERROR;

			case APPLY_ASSIGN_RAW:
				if (p_container->get(p_aux, q_state.r_value) != SERVICE_NO_ERROR)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				if (new_block(p_txn, p_aux->p_block, CELL_TYPE_UNDEFINED) != SERVICE_NO_ERROR) {
					p_container->destroy_transaction(p_aux);

					return SERVICE_ERROR_IO_ERROR;
				}
				p_container->destroy_transaction(p_aux);

				return SERVICE_NO_ERROR;

			case APPLY_ASSIGN_TEXT:
				if (p_container->get(p_aux, q_state.r_value) != SERVICE_NO_ERROR)
					return SERVICE_ERROR_BLOCK_NOT_FOUND;

				if (new_block(p_txn, p_aux->p_block, (pChar) nullptr, true) != SERVICE_NO_ERROR) {
					p_container->destroy_transaction(p_aux);

					return SERVICE_ERROR_IO_ERROR;
				}
				p_container->destroy_transaction(p_aux);

				return SERVICE_NO_ERROR;
			}
			return SERVICE_ERROR_MISC_SERVER;
		}

		/** This is an internal part of http_get() made independent to keep the function less crowded.

		Context: This in any possible assignment in which we could successfully solve the right part and have a valid p_block.
		We have to do a put call and return whatever status code happened.
		*/
		inline StatusCode put_left_local(HttpQueryState &q_state, pBlock p_block) {

			pContainer p_container = (pContainer) base_server[TenBitsAtAddress(q_state.base)];

			if (p_container == nullptr)
				return SERVICE_ERROR_WRONG_BASE;

			Locator where;

			memcpy(&where, &q_state.base, SIZE_OF_BASE_ENT_KEY);

			return p_container->put(where, p_block);
		}

		pChannels	p_channels;
		pVolatile	p_volatile;
		pPersisted	p_persisted;
		pCore		p_core;
		pModel		p_model;

		Index		www;
		int			remove_statics;
};

#ifdef CATCH_TEST

// Instancing Api
// --------------

extern Api	  TT_API;

#endif

} // namespace jazz_main

#endif // ifndef INCLUDED_JAZZ_MAIN_API
