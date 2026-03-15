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


#include "src/jazz_models/model.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_MODELS_MODELS_API
#define INCLUDED_JAZZ_MODELS_MODELS_API


/** \brief ModelsAPI: The model API.
*/

namespace jazz_models
{

/** \brief ModelsAPI: The model API.

	This class holds an object that can execute via an .exec() method in a similar way as a Core, but using a model as a resolver.
*/
class ModelsAPI : public BaseAPI {

//TODO: Synchronize http://localhost:8888/jazz_reference/api_ref_model.html#models-in-modelsapi (Erase this only when Core is done.)

	public:

		ModelsAPI(pLogger	  a_logger,
				  pConfigFile a_config,
				  pChannels	  a_channels,
				  pVolatile	  a_volatile,
				  pPersisted  a_persisted,
				  pCore		  a_core,
				  pBebop	  a_bebop);
	   ~ModelsAPI();

		virtual pChar const id();

		StatusCode start	();
		StatusCode shut_down();

		void base_names(BaseNames &base_names);

		// API Container interface

		virtual StatusCode get (pTransaction  &p_txn,
								ApiQueryState &what);
};
typedef ModelsAPI *pModelsAPI;	///< A pointer to a ModelsAPI


#ifdef CATCH_TEST

// Instancing ModelsAPI
// --------------------

extern ModelsAPI MDL;

#endif

} // namespace jazz_models

#endif // ifndef INCLUDED_JAZZ_MODELS_MODELS_API
