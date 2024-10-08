/* Jazz (c) 2018-2024 kaalam.ai (The Authors of Jazz), using (under the same license):

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


#include "src/jazz_main/server.h"

#ifdef CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_MAIN_INSTANCES
#define INCLUDED_JAZZ_MAIN_INSTANCES


#ifndef CATCH_TEST

// Uplifted containers: The declarations cannot go inside the namespace jazz_main.
// -------------------

#include "src/uplifted/uplifted_instances.h"

#endif


namespace jazz_main
{

using namespace jazz_elements;
using namespace jazz_bebop;
using namespace jazz_models;

/*	-----------------------------
	  I n s t a n t i a t i n g
--------------------------------- */

#ifndef CATCH_TEST

// Block containers:

extern Channels	 CHANNELS;		// The container channeling blocks.
extern Volatile	 VOLATILE;		// The container allocating volatile blocks.
extern Persisted PERSISTED;		// The container allocating persisted blocks.

// Code execution:

extern Core CORE;				// The container running Bop code.

// Http server:

extern HttpServer HTTP;			// The server

// SIGTERM Callback and http server daemon:

extern pMHD_Daemon Jazz_MHD_Daemon;
void signalHandler_SIGTERM(int signum);

#endif


/** \brief The initialization of the global variables for http_request_callback().
*/
void init_http_callback();

// Utils for starting and stopping Services:

/** \brief A little utility to start services writing output to the console.

	It also logs errors directly to the LOGGER instance.

	\param service		The address of the Service being started.

	\return	True if the service started ok.

*/
bool start_service(pService service);


/** \brief A little utility to stop services writing output to the console.

	It also logs errors directly to the LOGGER instance.

	\param service		The address of the Service being stopped.

	\return	True if the service stopped ok.

*/
bool stop_service(pService service);

} // namespace jazz_main

#endif // ifndef INCLUDED_JAZZ_MAIN_INSTANCES
