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


#include "src/jazz_models/models_api.h"


namespace jazz_models
{

using namespace jazz_elements;

/*	-----------------------------------------------
	 ModelsAPI : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** Create the ModelsAPI without starting it.

	\param a_logger	A pointer to the logger.
	\param a_config	A pointer to the configuration.
	\param a_channels A pointer to the ChannelsAPI.
	\param a_volatile A pointer to the VolatileAPI.
	\param a_persisted A pointer to the PersistedAPI.
	\param a_core A pointer to the CoreAPI.
	\param a_bebop A pointer to the BebopAPI.
*/
ModelsAPI::ModelsAPI(pLogger	 a_logger,
					 pConfigFile a_config,
					 pChannels	 a_channels,
					 pVolatile	 a_volatile,
					 pPersisted	 a_persisted,
					 pCore		 a_core,
					 pBebop		 a_bebop) : BaseAPI(a_logger, a_config, a_channels, a_volatile, a_persisted) {}


ModelsAPI::~ModelsAPI() { destroy_container(); }


/** Return object ID.

	\return A string identifying the object that is especially useful to track uplifts and versions.
*/
pChar const ModelsAPI::id() {
    static char arr[] = "ModelsAPI from Jazz-" JAZZ_VERSION;
    return arr;
}


/** Starts the ModelsAPI service

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode ModelsAPI::start() {

	int ret = BaseAPI::start();	// This initializes the one-shot functionality.

	if (ret != SERVICE_NO_ERROR)
		return ret;

//TODO: Implement ModelsAPI::start()

	return SERVICE_NO_ERROR;
}


/** Shuts down the ModelsAPI Service

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode ModelsAPI::shut_down() {

//TODO: Implement ModelsAPI::shut_down()

	return BaseAPI::shut_down();
}


/** Add the base names for this ModelsAPI.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

*/
void ModelsAPI::base_names(BaseNames &base_names) {

	base_names["brane"]	  = this;	// The Brane model
	base_names["resolve"] = this;	// The default resolver (in case there are more than one models)
}


// API Container interface
// -----------------------


/** "API" interface **complete Block** retrieval. This uses a parse()d what and is the only BasePI + descendants GET method.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param what		Some successfully parse()d ApiQueryState that also distinguishes API interface from Container interface.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

It should support the range from APPLY_NOTHING to APPLY_TEXT. This includes function calls APPLY_FUNCTION and APPLY_FUNCT_CONST,
but also APPLY_FILTER and APPLY_FILT_CONST to select from the result of a function call. Also, APPLY_URL is very convenient for
passing text as an argument to a function. APPLY_NOTHING can return some metadata about the model including a list of endpoints.
APPLY_NAME can define specifics of an endpoint. APPLY_RAW and APPLY_TEXT can be used to select the favorite serialization format
of the result. Therefore, the function interface should be considered as the whole range and not just APPLY_FUNCTION.
*/
StatusCode ModelsAPI::get(pTransaction &p_txn, ApiQueryState &what) {

//TODO: Implement ModelsAPI::get

	return SERVICE_NOT_IMPLEMENTED;

}

#ifdef CATCH_TEST

ModelsAPI MDL(&LOGGER, &CONFIG, &CHN, &VOL, &PER, &COR);

#endif

} // namespace jazz_models

#if defined CATCH_TEST
#include "src/jazz_models/tests/test_models_api.ctest"
#endif
