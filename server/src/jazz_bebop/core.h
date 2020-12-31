/* Jazz (c) 2018-2021 kaalam.ai (The Authors of Jazz), using (under the same license):

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


// #include <stl_whatever>

#include "src/include/jazz_elements.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_CORE
#define INCLUDED_JAZZ_BEBOP_CORE


/*! \brief Bebop, cores, kernel and extending the language.

	This namespace includes anything related with running and implementing Bebop code.

	All together is instanced in the server as the BOP (of Jazz).
*/
namespace jazz_bebop
{

using namespace jazz_elements;


/**< \brief Bebop: A Service to manage running cores and code bases.

*/
class Bebop : public Service {

	public:

		Bebop (pLogger	  a_logger,
				  pConfigFile a_config);

		Service_ErrorCode start		();
		Service_ErrorCode shut_down	(bool restarting_service = false);

};

} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_CORE
