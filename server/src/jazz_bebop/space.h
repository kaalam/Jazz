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

typedef uint64_t RowNumber;		///< A row number in a Space
typedef Name	 ColumnName;	///< A column name in a Space

#define SPACE_NOT_A_ROW				0xffffFFFFffffFFFE	///< A row number that is not valid used for initialization.
#define SPACE_ROW_STOP_ITERATOR		0xffffFFFFffffFFFF	///< A row number returned by the iterator when it is done.


class Space;					///< Forward definition of Space
typedef Space *pSpace;			///< A pointer to a Space


class Space : public Tuple {

	public:

	// Service interface

};

//TODO: Clarify how spaces build a tree.
//TODO: Clarify why we need singular/plural like collections of, because we want to have just one service for all the spaces at a level.
//TODO: Think that in Bop, a space is something that accepts then [] and the '.' the latter being just a property of what is on its left:
		// (E.g.: this.is.just.a.hierarchy.defining.some.path.on.a.tree)
//TODO: Is row the appropriate abstraction? Is a space somehow always tabular? A row is a key

/** \brief Space: The abstract space.

	This is the abstract parent of DataSpaces, Fields/SemSpaces and Snippet/Concept. A Space is an abstraction over many blocks that
	provides:

	- An abstraction in the form of rows and columns.
	- A mechanism to load and update its own metadata in a persisted way.

	Through inheritance, it provides such things as:

	- Sharding and replication across a cluster.
	- A mechanism to load, update, invalidate blocks. This supports continuous update like in time series.
	- Indexing by time, key, embedding, etc.

	The class Space is mostly empty. It provides the parent virtual interface and the parents of all the auxiliary classes used
	to access data.

	\see DataSpaces, Fields, jazz_models::SemSpaces, Snippet, jazz_models::Concept
*/

} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_SPACE
