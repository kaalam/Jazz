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

	This small namespace is about the server running and putting everything together. Unlike jazz_elements, jazz_bebop and jazz_models
	it is not intended to build other applications than the server.
*/
namespace jazz_main
{

//TODO: Create interface for ACTOR model (as described in develop/rfc2/jazz_actor_model.html)

using namespace jazz_elements;
using namespace jazz_bebop;
using namespace jazz_models;

#define MAX_RECURSE_LEVEL_ON_STATICS		16	///< The max directory recursion depth for load_statics()

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

typedef struct MHD_Response *pMHD_Response;		///< Pointer to a MHD_Response
typedef struct MHD_Connection *pMHD_Connection;	///< Pointer to a MHD_Connection

extern MHD_Result http_request_callback(void *cls,
										struct MHD_Connection *connection,
										const char *url,
										const char *method,
										const char *version,
										const char *upload_data,
										size_t *upload_data_size,
										void **con_cls);


/** \brief API: A Service to manage the REST API.

This service parses and executes http queries. It is aware and redistributes to all the appropriate services. It is called directly by
the http callback http_request_callback().

*/
class API : public BaseAPI {

	public:

		API(pLogger		a_logger,
			pConfigFile	a_config,
			pChannels	a_channels,
			pVolatile	a_volatile,
			pPersisted	a_persisted,
			pCore		a_core,
			pBebop		a_bebop,
			pModelsAPI	a_model);
	   ~API();

		virtual pChar const id();

		StatusCode start	();
		StatusCode shut_down();

		// parsing methods

		MHD_StatusCode get_static	   (pMHD_Response  &response,
										pChar			p_url,
										bool			get_it = true);

		// deliver http error pages

		MHD_Result return_error_message(pMHD_Connection	connection,
										pChar			p_url,
										int				http_status);

		// Specific execution methods

		MHD_StatusCode http_put	   (pChar			p_upload,
									size_t			size,
									ApiQueryState  &q_state,
									int				sequence);
		MHD_StatusCode http_delete (ApiQueryState  &q_state);
		MHD_StatusCode http_get	   (pMHD_Response  &response,
									ApiQueryState  &q_state);

#ifndef CATCH_TEST
	private:
#endif

		StatusCode load_statics	(pChar	p_base_path,
								 pChar	p_relative_path,
								 int	rec_level);
		bool expand_url_encoded	(pChar	p_buff,
								 int	buff_size,
								 pChar	p_url);

		pCore		p_core;			///< The Core
		pBebop		p_bebop;		///< The Bebop compiler
		pModelsAPI	p_model;		///< The ModelsAPI

		Index		www;			///< A map from url to locators to serve static files
		int			remove_statics;	///< A flag to remove the statics from persistence on shutdown configured by REMOVE_STATICS_ON_CLOSE
};

#ifdef CATCH_TEST

// Instancing API
// --------------

extern API	  TT_API;

#endif

} // namespace jazz_main

#endif // ifndef INCLUDED_JAZZ_MAIN_API
