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


#include "src/jazz_core/core.h"


namespace jazz_core
{

using namespace jazz_elements;

/*	-----------------------------------------------
	 Core : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** \brief Creates a Core service without starting it.

	\param a_logger		A pointer to the logger.
	\param a_config		A pointer to the configuration.
	\param a_channels	A pointer to an initialized Channels Container.
	\param a_volatile	A pointer to an initialized Volatile Container.
	\param a_persisted	A pointer to an initialized Persisted Container.
*/
Core::Core(pLogger a_logger,
		   pConfigFile a_config,
		   pChannels a_channels,
		   pVolatile a_volatile,
		   pPersisted a_persisted) : BaseAPI(a_logger, a_config, a_channels, a_volatile, a_persisted) {}
									//  data_spaces((pBaseAPI) this),
									//  fields((pBaseAPI) this),
									//  bop(a_logger, a_config) {}


Core::~Core() { destroy_container(); }


/** Return object ID.

	\return A string identifying the object that is especially useful to track uplifts and versions.
*/
pChar const Core::id() {
    static char arr[] = "Core from Jazz-" JAZZ_VERSION;
    return arr;
}


/** Starts the Core service

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode Core::start() {

	int ret = BaseAPI::start();	// This initializes the one-shot functionality.

	if (ret != SERVICE_NO_ERROR)
		return ret;

//TODO: Implement Core::start()

	return SERVICE_NO_ERROR;
}


/** Shuts down the Core Service

	\return SERVICE_NO_ERROR if successful, an error code otherwise.
*/
StatusCode Core::shut_down() {

//TODO: Implement Core::shut_down()

	return BaseAPI::shut_down();
}


/** Add the base names for this ModelsAPI.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

*/
void Core::base_names(BaseNames &base_names) {

	base_names["onnx"]	  = this;	// Run from object
}


// Core Container interface
// ------------------------


StatusCode Core::new_entity(pChar p_where) {

//TODO: Implement Core::new_entity() (1)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::new_entity(Locator &where) {

//TODO: Implement Core::new_entity() (2)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::put(pChar p_where, pBlock p_block, int mode) {

//TODO: Implement Core::put() (1)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::put(Locator &where, pBlock p_block, int mode) {

//TODO: Implement Core::put() (2)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::remove(pChar p_where) {

//TODO: Implement Core::remove() (1)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::remove(Locator &where) {

//TODO: Implement Core::remove() (2)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::header(StaticBlockHeader &hea, pChar p_what) {

//TODO: Implement Core::header() (1)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::header(pTransaction &p_txn, pChar p_what) {

//TODO: Implement Core::header() (2)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::header(StaticBlockHeader &hea, Locator &what) {

//TODO: Implement Core::header() (3)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::header(pTransaction &p_txn, Locator &what) {

//TODO: Implement Core::header() (4)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::get(pTransaction &p_txn, pChar p_what) {

//TODO: Implement Core::get() (1)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::get(pTransaction &p_txn, pChar p_what, pBlock p_row_filter) {

//TODO: Implement Core::get() (2)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::get(pTransaction &p_txn, pChar p_what, pChar name) {

//TODO: Implement Core::get() (3)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::get(pTransaction &p_txn, Locator &what) {

//TODO: Implement Core::get() (4)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::get(pTransaction &p_txn, Locator &what, pBlock p_row_filter) {

//TODO: Implement Core::get() (5)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::get(pTransaction &p_txn, Locator &what, pChar name) {

//TODO: Implement Core::get() (6)

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::exec(pTransaction &p_txn, Locator &function, pTuple p_args) {

//TODO: Implement Core::exec()

	return SERVICE_NOT_IMPLEMENTED;
}


StatusCode Core::modify(Locator &function, pTuple p_args) {

//TODO: Implement Core::modify()

	return SERVICE_NOT_IMPLEMENTED;
}

#ifdef CATCH_TEST

Core COR(&LOGGER, &CONFIG, &CHN, &VOL, &PER);

#endif

} // namespace jazz_core

#if defined CATCH_TEST
#include "src/jazz_core/tests/test_core.ctest"
#endif
