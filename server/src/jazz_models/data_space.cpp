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


#include "src/jazz_models/data_space.h"


namespace jazz_models
{

// /*	-----------------------------------------------
// 	 ColSelection : I m p l e m e n t a t i o n
// --------------------------------------------------- */

// ColSelection::ColSelection(pChar query, pSpace p_space) {

// 	stdName sn;
// 	pChar p = (pChar) &sn.name;
// 	int i, l = 0;
// 	bool word_in = false, new_line = false;

// 	while (true) {
// 		switch (char c = *query++){
// 		case ' ':
// 		case '\t':
// 			if (new_line)
// 				return;

// 			if (l > 0)
// 				word_in = true;

// 			continue;

// 		case '\n':
// 			new_line = true;

// 			continue;

// 		case ',':
// 			if (new_line)
// 				return;

// 		case 0:
// 			p[l] = 0;

// 			i = p_space->col_index(&sn.name);
// 			if (i < 0)
// 				return;

// 			for (int j = 0; j < index.size(); j++)
// 				if (index[j] == i)
// 					return;

// 			name.push_back(sn);
// 			index.push_back(i);

// 			if (c == 0) {
// 				is_valid = l > 0;
// 				return;
// 			}

// 			word_in = false;
// 			l = 0;

// 			continue;

// 		default:
// 			if ((word_in) || (new_line) || (l >= sizeof(Name) - 1))
// 				return;

// 			p[l++] = c;
// 		}
// 	}
// }


// /** \brief ColSelection: Restart the iterator.

// 	\return true if successful, false failed or was already pointing to the first element.
// */
// bool ColSelection::restart() {
// 	if (!is_valid || (current_col == 0))
// 		return false;

// 	current_col = 0;

// 	return true;
// }


// /** \brief ColSelection: Get the next column index.

// 	\return The next column index or -i if the iteration is exhausted.
// */
// int ColSelection::next_index() {

// 	if (is_valid && (current_col < index.size()))
// 		return index[current_col++];

// 	return -1;
// }


// /** \brief ColSelection: Get the next column name.

// 	\return The next column name or nullptr if the iteration is exhausted.
// */
// pName ColSelection::next_name() {

// 	if (is_valid && (current_col < name.size()))
// 		return &name[current_col++].name;

// 	return nullptr;
// }

// /*	-----------------------------------------------
// 	 Space : I m p l e m e n t a t i o n
// --------------------------------------------------- */

// /** \brief Bop: Create the Space.

// 	\param api	  A pointer to a BaseAPI that provides access to containers.
// 	\param a_name The name of the Space.
// */
// Space::Space(pBaseAPI api, pName a_name) : Service(api->p_log, api->p_conf) {

// 	memcpy(&name, a_name, sizeof(Name));
// 	p_api = api;
// 	casters = {};
// }


// /** Starts the Space service

// 	\return SERVICE_NO_ERROR if successful, an error code otherwise.
// */
// StatusCode Space::start() {

// 	String s;

// 	if (!get_conf_key("SPACE_STORAGE_BASE", s)) {
// 		log(LOG_ERROR, "Config key SPACE_STORAGE_BASE not found in Space::start");

// 		return SERVICE_ERROR_BAD_CONFIG;
// 	}

// 	if ((s.length() < 2) || (s.length() >= SHORT_NAME_SIZE) || (p_api->base_server[TenBitsAtAddress(s.c_str())] == nullptr)) {
// 		log(LOG_ERROR, "Config key SPACE_STORAGE_BASE is not a valid base in Space::start");

// 		return SERVICE_ERROR_BAD_CONFIG;
// 	}

// 	strcpy(storage_base, s.c_str());

// 	return SERVICE_NO_ERROR;
// }


// /** Shuts down the Space Service

// 	\return SERVICE_NO_ERROR if successful, an error code otherwise.
// */
// StatusCode Space::shut_down() {

// 	return SERVICE_NO_ERROR;
// }

// ///> The name of the DataSpaces class required by Spaces.
// Name NAME_CLASS_DATASPACES = "DataSpaces";

// /*	-----------------------------------------------
// 	 DataSpaces : I m p l e m e n t a t i o n
// --------------------------------------------------- */

// /** \brief Bop: Start the DataSpaces.

// 	\param api	 A pointer to a BaseAPI that provides access to containers.
// */
// DataSpaces::DataSpaces(pBaseAPI api) : Space(api, &NAME_CLASS_DATASPACES) {

// 	p_api = api;
// }


// /** Starts the DataSpaces service

// 	\return SERVICE_NO_ERROR if successful, an error code otherwise.
// */
// StatusCode DataSpaces::start() {

// 	String s;

// 	if (!get_conf_key("DATASPACES_STORAGE_ENTITY", s)) {
// 		log(LOG_ERROR, "Config key DATASPACES_STORAGE_ENTITY not found in DataSpaces::start");

// 		return SERVICE_ERROR_BAD_CONFIG;
// 	}

// 	if ((s.length() < 1) || (s.length() >= sizeof(Name))) {
// 		log(LOG_ERROR, "Config key DATASPACES_STORAGE_ENTITY is not a valid base in DataSpaces::start");

// 		return SERVICE_ERROR_BAD_CONFIG;
// 	}

// 	strcpy(storage_ent, s.c_str());

// 	return true;
// }


// /** Return object ID.

// 	\return A string identifying the object that is especially useful to track uplifts and versions.
// */
// pChar const DataSpaces::id() {
//     static char arr[] = "DataSpaces from Jazz-" JAZZ_VERSION;
//     return arr;
// }

/*	-----------------------------------------------
	 DataSpace : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** \brief DataSpace: Constructor.

	\param p_owner A pointer to the BaseAPI that owns the DataSpace.
*/
DataSpace::DataSpace(pBaseAPI p_owner) : Space(p_owner) {

//TODO: Implement constructor.

}


/** \brief DataSpace: Destructor.
*/
DataSpace::~DataSpace() {

//TODO: Implement destructor.

}

} // namespace jazz_models

#if defined CATCH_TEST
#include "src/jazz_models/tests/test_data_space.ctest"
#endif
