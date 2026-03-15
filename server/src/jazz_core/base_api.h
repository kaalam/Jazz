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


#include "src/include/jazz_elements.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_BASE_API
#define INCLUDED_JAZZ_BEBOP_BASE_API


/** \brief A language to access any container by base using locators.

	BaseAPI is a Container and a parent of: Core, ModelsAPI and API.

	\see Container, Core, ModelsAPI and API
*/
namespace jazz_core
{

using namespace jazz_elements;

#define SIZE_OF_BASE_ENT_KEY	(sizeof(Locator) - sizeof(pExtraLocator))	///< Used to convert ApiQueryState -> Locator

#define RESULT_BUFFER_SIZE				  4096	///< The "result" item size in a Tuple used in a modify() call.
#define SIZE_BUFFER_REMOTE_CALL			  2048	///< The size of the buffer in which URLS for remote calls are built.

#define BASE_API_GET						 3	///< This is numerically equivalent to HTTP_GET in api.h http predicate GET
#define BASE_API_PUT						 4	///< This is numerically equivalent to HTTP_PUT in api.h http predicate PUT
#define BASE_API_DELETE						 5	///< This is numerically equivalent to HTTP_DELETE in api.h http predicate DELETE

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
#define PSTATE_INFO_SWITCH					10	///< The final switch inside or after a key: END
#define PSTATE_BASE_SWITCH					11	///< Found # while reading a base (PSTATE_IN_BASE)
#define PSTATE_ENT_SWITCH					12	///< Found # while reading an entity (PSTATE_IN_ENTITY)
#define PSTATE_KEY_SWITCH					13	///< The final switch inside or after a key: END, =, ., :, [, [&, (, or (&
#define PSTATE_FAILED						98	///< Set by the parser on any error (possibly in the r_value too)
#define PSTATE_COMPLETE_OK					99	///< Set by the parser on complete success

// BaseAPI.move_const() return values

#define RET_MV_CONST_FAILED					-1	///< return value for move_const() failed.
#define RET_MV_CONST_NOTHING				 0	///< return value for move_const() normal moving.
#define RET_MV_CONST_NEW_ENTITY				 1	///< return value for move_const() there is a ";.new" ending, otherwise parses ok.


/** \brief A buffer to keep the state while parsing/executing a query
*/
struct ApiQueryState {
	int	state;									///< The parser state from PSTATE_INITIAL to PSTATE_COMPLETE_OK
	int apply;									///< APPLY_NOTHING, APPLY_NAME, APPLY_URL, APPLY_FUNCTION, .. APPLY_ASSIGN_CONST

	char l_node	[NAME_SIZE];					///< An optional name of a Jazz node that is either the only one or the left of assignment.
	char r_node	[NAME_SIZE];					///< An additional optional name of a Jazz node for the right part in an assignment.
	char base	[SHORT_NAME_SIZE];				///< A Locator compatible base for the l_value.
	char entity	[NAME_SIZE];					///< A Locator compatible entity for the l_value.
	char key	[NAME_SIZE];					///< A Locator compatible key for the l_value.
	char name	[NAME_SIZE];					///< A possible item name
	char url	[MAX_FILE_OR_URL_SIZE];			///< The endpoint (an URL, file name, folder name, bash script)

	Locator r_value;							///< Parsed //r_base/r_entity/r_key
	Locator rr_value;							///< Parsed //r_base/r_entity/r_key(//rr_base/rr_entity/rr_key)
};


/** \brief BaseAPI: The parent of API and Core.

This class is a Container itself and a gateway to all the containers in jazz_elements. It implements all the fundamental API syntax. The
final jazz_main::API class just handles its connection via http. Normally, jazz_main::API is uplifted for security reasons.

*/
class BaseAPI : public Container {

	public:

		BaseAPI(pLogger a_logger, pConfigFile a_config, pChannels a_channels, pVolatile a_volatile, pPersisted a_persisted);
	   ~BaseAPI();

		virtual pChar const id();

		StatusCode start	();
		StatusCode shut_down();

		// Parsing methods

		bool parse (ApiQueryState  &q_state,
					pChar			p_url,
					int				method,
					bool			recurse = false);

		bool block_from_const	(pTransaction  &p_txn,
								 pChar			p_const,
								 bool			make_tuple = false);

		// API Container interface

		virtual StatusCode header  (StaticBlockHeader &hea,
									ApiQueryState	  &what);
		virtual StatusCode get	   (pTransaction	  &p_txn,
									ApiQueryState	  &what);
		virtual StatusCode put	   (ApiQueryState	  &where,
									pBlock			   p_block,
									int				   mode = WRITE_AS_BASE_DEFAULT);
		virtual StatusCode remove  (ApiQueryState	  &what);

		// Access to the individual Containers

		/** Get the Channels container.

			\return A pointer to the Channels container.
		*/
		inline pChannels get_channels() {
			return p_channels;
		}

		/** Get the Volatile container.

			\return A pointer to the Volatile container.
		*/
		inline pVolatile get_volatile() {
			return p_volatile;
		}

		/** Get the Persisted container.

			\return A pointer to the Persisted container.
		*/
		inline pPersisted get_persisted() {
			return p_persisted;
		}

		// The base_server is public to support objects using this to see what Containers are available.

		TenBitPtrLUT base_server;	///< A LUT to convert TenBitsAtAddress(base) into a pContainer.

#ifndef CATCH_TEST
	protected:
#endif

		bool parse_locator (Locator &loc,
							pChar	 p_url);

		/** Copy the string "as-is" (without percent-decoding) a string into a buffer.

			\param p_buff	 A buffer to store the result. This first char must be a zero on call or it will not write anything, just
							 check for the return code and return. (This is a trick to avoid overriding the buffer in forward call. This
							 function is an internal part of parse().)
			\param buff_size The size of the output buffer (ending zero included).
			\param p_url	 The input string.
			\param p_base	 (optional) Prefix with a base to be prefixed

			\return			 RET_MV_CONST_FAILED on error (buffer sizes or start and final characters), RET_MV_CONST_NOTHING normal moving
							 or RET_MV_CONST_NEW_ENTITY there is a ";.new" ending and no errors.

		Note: This replaces expand_url_encoded() since the string passed to parse() is already %-decoded by libmicrohttpd.
		*/
		inline int move_const(pChar p_buff, int buff_size, pChar p_url, pChar p_base = nullptr) {

			if (*(p_url++) != '&')
				return RET_MV_CONST_FAILED;

			int url_len = strlen(p_url) - 1;

			pChar p_end = p_url + url_len;

			int ret = RET_MV_CONST_NOTHING;

			if (*p_end == 'w') {
				if ((*(--p_end) != 'e') || (*(--p_end) != 'n') || (*(--p_end) != '.') || (*(--p_end) != ';'))
					return RET_MV_CONST_FAILED;

				ret		 = RET_MV_CONST_NEW_ENTITY;
				url_len -= 4;
			}

			if (*p_end != ';' && *p_end != ']' && *p_end != ')')
				return RET_MV_CONST_FAILED;

			if (*p_buff != 0)
				return ret;

			if (p_base != nullptr) {
				if (buff_size < 4 + SHORT_NAME_SIZE)
					return RET_MV_CONST_FAILED;

				*(p_buff++) = '/';
				*(p_buff++) = '/';

				buff_size -= 2;

				for (int i = 0; i < SHORT_NAME_SIZE; i++) {
					char c = *(p_base++);

					buff_size--;

					if (c == 0) {
						*(p_buff++) = '/';
						break;
					}
					*(p_buff++) = c;
				}
			}

			if (url_len >= buff_size)
				return RET_MV_CONST_FAILED;

			memcpy(p_buff, p_url, url_len);

			p_buff += url_len;

			*(p_buff) = 0;

			return ret;
		}

		/** This is an internal part of get() made independent to keep the function less crowded.

			\param p_txn		A pointer to the transaction that will be used to store the result.
			\param q_state		The structure containing the parts of the url successfully parsed.

			\return				SERVICE_NO_ERROR if successful, or an error code.

		Context: This in any possible assignment in which the right part is NOT a remote call. Functionally, it is similar to
		get_left_local(), but since it is the right of an assignment, arguments are stored at a different place and also, apply
		code are in range APPLY_ASSIGN_NOTHING..APPLY_ASSIGN_CONST instead of APPLY_NOTHING..APPLY_TEXT.
		It returns the final block as it will be returned with a new_block() interface.
		*/
		inline StatusCode get_right_local(pTransaction &p_txn, ApiQueryState &q_state) {

			pTransaction p_aux;
			pContainer	 p_container, p_aux_cont;

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
					Name ent = {"result"};
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
					Name ent = {"result"};
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

		/** This is an internal part of get() made independent to keep the function less crowded.

			\param p_txn		A pointer to the transaction that will be used to store the result.
			\param q_state		The structure containing the parts of the url successfully parsed.

			\return				SERVICE_NO_ERROR if successful, or an error code.

		Context: This in any possible assignment in which the right part is a remote call. Since q_state.url does not have
				 a valid url, itt is necessary to reconstruct it. We do have the constants if any.
		It returns the final block as it will be returned to the user with a new_block() interface.
		*/
		inline StatusCode get_right_remote(pTransaction &p_txn, ApiQueryState &q_state) {

			char buffer_2k[SIZE_BUFFER_REMOTE_CALL];

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

		/** This is an internal part of get() made independent to keep the function less crowded.

			\param p_txn		A pointer to the transaction that will be used to store the result.
			\param q_state		The structure containing the parts of the url successfully parsed.

			\return				SERVICE_NO_ERROR if successful, or an error code.

		Context: This is called when q_state.apply is APPLY_NOTHING ... APPLY_TEXT and there is no forwarding.
		It returns the final block as it will be returned to the user with a new_block() interface.
		*/
		inline StatusCode get_left_local(pTransaction &p_txn, ApiQueryState &q_state) {

			Locator		 loc;
			pTransaction p_aux;
			pContainer	 p_container, p_aux_cont;

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
					Name ent = {"result"};
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
					Name ent = {"result"};
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

		/** This is an internal part of put() made independent to keep the function less crowded.

			\param q_state		The structure containing the parts of the url successfully parsed.
			\param p_block		The block to be stored.

			\return				SERVICE_NO_ERROR if successful, or an error code.

		Context: This in any possible assignment in which we could successfully solve the right part and have a valid p_block.
		We have to do a put call and return whatever status code happened.
		*/
		inline StatusCode put_left_local(ApiQueryState &q_state, pBlock p_block) {

			pContainer p_container = (pContainer) base_server[TenBitsAtAddress(q_state.base)];

			if (p_container == nullptr)
				return SERVICE_ERROR_WRONG_BASE;

			Locator where;

			memcpy(&where, &q_state.base, SIZE_OF_BASE_ENT_KEY);

			return p_container->put(where, p_block);
		}

		pChannels	p_channels;		///< The Channels container
		pVolatile	p_volatile;		///< The Volatile container
		pPersisted	p_persisted;	///< The Persisted container
};
typedef BaseAPI *pBaseAPI;			///< A pointer to a BaseAPI

#ifdef CATCH_TEST

// Instancing BaseAPI
// ------------------

extern BaseAPI	BAPI;

#endif

} // namespace jazz_core

#endif // ifndef INCLUDED_JAZZ_BEBOP_BASE_API
