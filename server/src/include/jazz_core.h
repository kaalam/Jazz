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


#include "src/include/jazz_elements.h"


#ifndef INCLUDED_JAZZ_CORE
#define INCLUDED_JAZZ_CORE


/** \brief The namespace with anything required for ONNX runtime execution running inside Jazz: BaseAPI, Snippet and Core.

This is the namespace is above (includes) jazz_elements and allows access to any Container in jazz_elements, via a BaseAPI.
It separates the execution from the compilation which happen in the namespace jazz_bebop.
*/


#include "src/jazz_core/base_api.h"
#include "src/jazz_core/std_wrap.h"
#include "src/jazz_core/opcodes.h"
#include "src/jazz_core/snippet.h"
#include "src/jazz_core/core.h"


#endif // ifndef INCLUDED_JAZZ_CORE
