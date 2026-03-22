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

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_SEM_SPACE
#define INCLUDED_JAZZ_BEBOP_SEM_SPACE


/** \brief Semantic space: SemSpace class.

*/

namespace jazz_models
{

/** \brief SemSpace: A Fields for Concepts.
*/
class SemSpace : public DataSpace {

	public:

	public:

		SemSpace(pBaseAPI p_owner);
		~SemSpace();

		std::map<stdName, std::string> self;	///< An attribute that is a map of names to other Spaces or strings.

		// SemSpace(pBaseAPI api);

		// virtual StatusCode start();

		// virtual pChar const id();

		// SemSpace-ETL interface

//TODO: Define the SemSpace-ETL interface. This is understood by the Bop compiler and is used by ETL maintenance software that writes
//		special Bop scripts to populate/update/destroy the SemSpace.

//TODO: Define the SemSpace interface.

	private:

		Name storage_ent;			///< The name of the storage entity (Typically an lmdb database with the metadata of all SemSpace).
};
typedef SemSpace *pSemSpace;		///< A pointer to a SemSpace

} // namespace jazz_models

#endif // ifndef INCLUDED_JAZZ_BEBOP_SEM_SPACE
