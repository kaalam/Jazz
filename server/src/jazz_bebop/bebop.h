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


#include "src/jazz_bebop/casters.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_BEBOP
#define INCLUDED_JAZZ_BEBOP_BEBOP


//TODO: All this Fields idea is obsolete. The Fields server is just a Space, but the function is still needed.
//The Field (API) - Since A Snippet is immutable, the Field creates a new one that replaces the old one.


// /** \brief Fields: A Space for Snippets.

// A Fields is Space interface to the base `index` of the Volatile Container. The entity is the Fields and the key is the name of the snippet.

// A field is a namespace inside Fields.
// */
// class Fields : public Space {

// 	public:

// 		Fields(pBaseAPI api);

// 		virtual StatusCode start();

// 		virtual pChar const id();

// 	private:

// 		Name		storage_ent;	///< The name of the storage entity (Typically an lmdb database with the metadata of all SemSpaces).
// 		pVolatile	p_volatile;		///< The Volatile container
// 		pPersisted	p_persisted;	///< The Persisted container
// 		pBaseAPI	p_api;			///< A pointer to the BaseAPI that provides access to containers.
// };
// typedef Fields *pFields;			///< A pointer to a Fields


/** \brief The Bop compiler
*/

namespace jazz_bebop
{

/** \brief Bebop: The compiler.


*/
class Bebop : public BaseAPI {

	public:

		Bebop(pLogger a_logger, pConfigFile a_config, pChannels a_channels, pVolatile a_volatile, pPersisted a_persisted, pCore a_core);
	   ~Bebop();

		virtual pChar const id();

		StatusCode start	();
		StatusCode shut_down();

		void base_names(BaseNames &base_names);
};
typedef Bebop *pBebop;		///< A pointer to a Bebop compiler

} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_BEBOP
