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


#include "src/jazz_core/snippet.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_CORE
#define INCLUDED_JAZZ_BEBOP_CORE


/** \brief Core: The execution unit is now a wrapper around onnx-runtime.

A core is not a service, it is stored in a Bop API.
*/

namespace jazz_core
{

using namespace jazz_elements;


/** \brief Core: The execution unit is now a wrapper around onnx-runtime.

This class:

  * Manages dataspaces.
  * Manages fields.
  * Manages life cycle of Snippets: create from source, from object, update, delete.
  * Compiles or decompiles Snippets.
  * Manages onnx runtime sessions.
  * Runs bop objects.

API Container interface:
------------------------

This is the only interface this class has. Everything, is done using this interface, even when this object is used by a ModelsAPI.

Everything here can be called from the API + destroy_transaction() (You don't override destroy_transaction() since you don't
override new_transaction()). Of course, not everything must be supported, but since it could be called by some possibly nonsensical
API query, it is better to reject whatever is not supported with a proper error code.

The class must support all forms of: `new_entity`, `put`, `remove`, `header`, `get`, `exec`, `modify`.

Unlike in other containers, the override is based on only on types, using the locate() mechanism is optional.
Unlike in other containers, copy() is not called from the API. copy() is just an internal function to copy inside a Container in the
most efficient way.

*/
class Core : public BaseAPI {

//TODO: Synchronize http://localhost:8888/jazz_reference/api_ref_core.html (Erase this only when Core is done.)
//TODO: Use SNIPPET_STORAGE_ENTITY
//TODO: Use ONNXRT_MAX_NUM_SESSIONS

	public:

		Core(pLogger a_logger, pConfigFile a_config, pChannels a_channels, pVolatile a_volatile, pPersisted a_persisted);
	   ~Core();

		virtual pChar const id();

		StatusCode start	();
		StatusCode shut_down();

		void base_names(BaseNames &base_names);

		// API Container interface: (see docstring)

		virtual StatusCode new_entity  (pChar				p_where);
		virtual StatusCode new_entity  (Locator			   &where);
		virtual StatusCode put		   (pChar				p_where,
										pBlock				p_block,
										int					mode = WRITE_AS_BASE_DEFAULT);
		virtual StatusCode put		   (Locator			   &where,
										pBlock				p_block,
										int					mode = WRITE_AS_BASE_DEFAULT);
		virtual StatusCode remove	   (pChar				p_where);
		virtual StatusCode remove	   (Locator			   &where);
		virtual StatusCode header	   (StaticBlockHeader  &hea,
										pChar				p_what);
		virtual StatusCode header	   (pTransaction	   &p_txn,
										pChar				p_what);
		virtual StatusCode header	   (StaticBlockHeader  &hea,
										Locator			   &what);
		virtual StatusCode header	   (pTransaction	   &p_txn,
										Locator			   &what);
		virtual StatusCode get		   (pTransaction	   &p_txn,
										pChar				p_what);
		virtual StatusCode get		   (pTransaction	   &p_txn,
										pChar				p_what,
										pBlock				p_row_filter);
		virtual StatusCode get		   (pTransaction	   &p_txn,
										pChar				p_what,
										pChar				name);
		virtual StatusCode get		   (pTransaction	   &p_txn,
										Locator			   &what);
		virtual StatusCode get		   (pTransaction	   &p_txn,
										Locator			   &what,
										pBlock				p_row_filter);
		virtual StatusCode get		   (pTransaction	   &p_txn,
							  			Locator			   &what,
							  			pChar				name);
		virtual StatusCode exec		   (pTransaction	   &p_txn,
										Locator			   &function,
										pTuple				p_args);
		virtual StatusCode modify	   (Locator			   &function,
										pTuple				p_args);

	private:

		// DataSpaces data_spaces;		///< The data spaces.
		// Fields	   fields;			///< The fields.
		// Bop		   bop;				///< The Bop compiler.
};
typedef Core *pCore;		///< A pointer to a Core


#ifdef CATCH_TEST

// Instancing Core
// -----------------

extern Core COR;

#endif

} // namespace jazz_core

#endif // ifndef INCLUDED_JAZZ_BEBOP_CORE
