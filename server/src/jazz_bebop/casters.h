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


#include "src/jazz_bebop/objects.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_CASTERS
#define INCLUDED_JAZZ_BEBOP_CASTERS


/** \brief One liner.

//TODO: Write this!
*/

namespace jazz_bebop
{

using namespace jazz_core;

/** \brief Caster: An optional converter of the output.

	This is a function that converts the result returned by a Space.get_row(). It can be a conversion from text to tokens or vice versa,
from wave to aperiodic, a sentence embedding, an image format converter, etc. Something deterministic. Is is identified by a name and
used in a query with the AS keyword.

	The collection of all the available Caster descendants is managed by a Casters object.
*/
class Caster {
	public:

	/** \brief The constructor for a Caster.

		\param api	A BaseAPI. It is used to create new blocks.
	*/
	Caster(pBaseAPI api) {}

	/** \brief Convert a block doing whatever the caster does.

		\param p_txn	The transaction that contains the new block.
		\param p_block	The block to convert.

		\return SERVICE_NO_ERROR if successful, an error code otherwise.
	*/
	virtual StatusCode get(pTransaction	&p_txn, pBlock p_block) {
		return SERVICE_NOT_IMPLEMENTED;
	}

	stdName name;		///< The name of the Caster.
};
typedef Caster *pCaster;						///< A pointer to a Caster
typedef std::map<stdName, pCaster> Casters;		///< A map of Caster pointers
typedef Casters *pCasters;						///< A pointer to a Casters


} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_CASTERS
