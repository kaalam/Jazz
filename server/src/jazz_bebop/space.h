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


#include "src/include/jazz_core.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_SPACE
#define INCLUDED_JAZZ_BEBOP_SPACE


namespace jazz_bebop
{

using namespace jazz_elements;
using namespace jazz_core;


class Space;					///< Forward definition of Space
typedef Space *pSpace;			///< A pointer to a Space


/** \brief Space: The Bebop object that belongs to a hierarchy.

Space: Extends Snippet with:

    - A parent defining a hierarchical structure
    - An address (a locator)
    - A kind
    - Casting mechanisms across kinds
    - operators: is (=), get (<-), key (.), inside ({..}), within ([..]), call ((..)) and after (|>)
    - properties: keys (a dictionary to children and settings), src (the source code)

Spaces are a hierarchy, since they have a parent and children. Both are store in the keys property.

*/
class Space : public Snippet {

	public:

	Space(pBaseAPI p_owner);
	~Space();

	std::map<stdName, std::string> keys;	///< An attribute that is a map of names to other Spaces or strings.

};

} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_SPACE
