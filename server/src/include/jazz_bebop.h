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


#ifndef INCLUDED_JAZZ_BEBOP
#define INCLUDED_JAZZ_BEBOP


/** \brief This namespace contains everything necessary to compile Bebop and the parents of the objects in Models.

Bebop is the language used manage the high level abstractions that are converted into ONNX Snippets and executed in the Core.
Basic ONNX functionality such as running or reverse engineering ONNX pipelines created by any other way (pytorch, etc.) is
implemented in jazz_core. Bebop is an abstraction to support composition, introspection, lensing, operators, etc. that also
sets the groundwork for jazz_models.
*/


#include "src/jazz_bebop/space.h"
#include "src/jazz_bebop/namespace.h"
#include "src/jazz_bebop/bebop.h"


#endif // ifndef INCLUDED_JAZZ_BEBOP
