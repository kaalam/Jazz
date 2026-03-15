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


#include "src/jazz_bebop/bebop.h"


//TODO: All this Fields idea is obsolete. The Fields server is just a Space, but the function is still needed.
//TODO: Remove FIELD_STORAGE_ENTITY from the config as it no longer exists.

// ///> The name of the Fields class required by Spaces.
// Name NAME_CLASS_FIELDS = "Fields";

// /*	-----------------------------------------------
// 	 Fields : I m p l e m e n t a t i o n
// --------------------------------------------------- */

// /** \brief Bop: Start the Fields.

// 	\param api	 A pointer to a BaseAPI that provides access to containers.
// */
// Fields::Fields(pBaseAPI api) : Space(api, &NAME_CLASS_FIELDS) {
// 	p_api = api;
// }


// /** Starts the Fields service

// 	\return SERVICE_NO_ERROR if successful, an error code otherwise.
// */
// StatusCode Fields::start() {

// 	int ret = Fields::start();

// 	if (ret != SERVICE_NO_ERROR)
// 		return ret;

// 	String s;

// 	if (!get_conf_key("FIELD_STORAGE_ENTITY", s)) {
// 		log(LOG_ERROR, "Config key FIELD_STORAGE_ENTITY not found in Fields::start");

// 		return SERVICE_ERROR_BAD_CONFIG;
// 	}

// 	if ((s.length() < 1) || (s.length() >= sizeof(Name))) {
// 		log(LOG_ERROR, "Config key FIELD_STORAGE_ENTITY is not a valid base in Fields::start");

// 		return SERVICE_ERROR_BAD_CONFIG;
// 	}

// 	strcpy(storage_ent, s.c_str());

// 	p_volatile	= p_api->get_volatile();
// 	p_persisted	= p_api->get_persisted();

// 	if (p_volatile == nullptr || p_persisted == nullptr) {
// 		log(LOG_ERROR, "Volatile or Persisted container not valid in BaseAPI detected in Fields::start");

// 		return SERVICE_ERROR_WRONG_ARGUMENTS;
// 	}

// 	return true;
// }


// /** Return object ID.

// 	\return A string identifying the object that is especially useful to track uplifts and versions.
// */
// pChar const Fields::id() {
//     static char arr[] = "Fields from Jazz-" JAZZ_VERSION;
//     return arr;
// }


// #ifdef CATCH_TEST

// Fields FIELDS(&BAPI);

// #endif


namespace jazz_bebop
{

/*	-----------------------------------------------
	 Bop : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** \brief Bop: Start the compiler.

	\param a_logger		A pointer to the logger.
	\param a_config		A pointer to the configuration.
	\param a_channels	A pointer to the Channels container.
	\param a_volatile	A pointer to the Volatile container.
	\param a_persisted	A pointer to the Persisted container.
	\param a_core		A pointer to the Core container.
*/
Bebop::Bebop(pLogger a_logger, pConfigFile a_config, pChannels a_channels, pVolatile a_volatile, pPersisted a_persisted, pCore a_core)
	: BaseAPI(a_logger, a_config, a_channels, a_volatile, a_persisted) {}

Bebop::~Bebop() {}

/** Return object ID.

	\return A string identifying the object that is especially useful to track uplifts and versions.
*/
pChar const Bebop::id() {
    static char arr[] = "Bebop from Jazz-" JAZZ_VERSION;
    return arr;
}


/** Starts the ModelsAPI service

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode Bebop::start() {

	int ret = BaseAPI::start();	// This initializes the one-shot functionality.

	if (ret != SERVICE_NO_ERROR)
		return ret;

//TODO: Implement Bebop::start()

	return SERVICE_NO_ERROR;
}


/** Shuts down the ModelsAPI Service

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode Bebop::shut_down() {

//TODO: Implement Bebop::shut_down()

	return BaseAPI::shut_down();
}


/** Add the base names for this Bebop.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

*/
void Bebop::base_names(BaseNames &base_names) {

	base_names["bop"]	  = this;	// The bop-25 compiler
	base_names["bebop"]	  = this;	// The bop-25 compiler
	base_names["compile"] = this;	// The default compiler (in case there are more than one languages)
}




} // namespace jazz_bebop

#if defined CATCH_TEST
#include "src/jazz_bebop/tests/test_bebop.ctest"
#endif
