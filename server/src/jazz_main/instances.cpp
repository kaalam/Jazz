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


#include "src/jazz_main/instances.h"


#ifndef CATCH_TEST

// Uplifted containers: The implementation must go outside the namespace jazz_main, but uses instances from jazz_main
// -------------------

using namespace jazz_main;

#include "src/uplifted/uplifted_instances.cpp"

#endif


namespace jazz_main
{
	using namespace std;

/*	-----------------------------
	  I n s t a n t i a t i n g
--------------------------------- */

#ifndef CATCH_TEST

ConfigFile	CONFIG(JAZZ_DEFAULT_CONFIG_PATH);
Logger		LOGGER(CONFIG, "LOGGER_PATH");

// Services

Channels	CHANNELS (&LOGGER, &CONFIG);
Volatile	VOLATILE (&LOGGER, &CONFIG);
Persisted	PERSISTED(&LOGGER, &CONFIG);

// Code execution:

Core		CORE(&LOGGER, &CONFIG);

// Http server:

HttpServer	HTTP(&LOGGER, &CONFIG);

#endif

// Callbacks

pMHD_Daemon	Jazz_MHD_Daemon;


/*	---------------------------------------------
	 M A I N   H T T P	 E N T R Y	 P O I N T S
------------------------------------------------- */

#define MHD_HTTP_ANYERROR true

#ifndef CATCH_TEST
#ifdef DEBUG
MHD_Result print_out_key(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) {
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - conn (key:value) : %s:%.40s", key, value);

	return MHD_YES;
}
#endif
#endif


/// Indices inside state (anything else is a pTransaction of a PUT call).
#define	STATE_NEW_CALL			0		///< Default state: connection open for any call
#define	STATE_NOT_ACCEPTABLE	1		///< Data upload failed, query execution failed locating targets. Returns MHD_HTTP_NOT_ACCEPTABLE
#define	STATE_BAD_REQUEST		2		///< PUT query is call malformed. Returns MHD_HTTP_BAD_REQUEST.

int callback_state [3];

char response_put_ok[]			= "0";
char response_put_fail[]		= "1";

TenBitIntLUT http_methods;				///< A LUT to convert argument const char *method int an integer code.
TenBitPtrLUT base_server;				///< A LUT to convert argument const char *method int an integer code.

#ifndef CATCH_TEST

/** Callback function for MHD. See: https://www.gnu.org/software/libmicrohttpd/tutorial.html

	Jazz does not use post processor callbacks linked with MHD_create_post_processor().
	Jazz does not use request completed callbacks linked with MHD_start_daemon().

	The only callback functions are:

		1. This (http_request_callback): The full operational blocks and instrumental API.
		2. http_apc_callback():			 An IP based (firewall like) that can filter based on called IPs.

	This function is multithreaded with the default configuration settings. (Other settings are untested for the moment.)

	The internal operation of the callback function is subject to change and the remarks in the source code are the description of it.
*/
MHD_Result http_request_callback(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
								 const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {

	// Step 1: First opportunity to end the connection before uploading or getting. Not used. We initialize con_cls for the next call.

	if (*con_cls == NULL) {
		*con_cls = &callback_state[STATE_NEW_CALL];

		return MHD_YES;
	}

	// Step 2 : Continue uploads in progress, checking all possible error conditions.

	int http_method = http_methods[TenBitsAtAddress(method)];

	HttpQueryState q_state;
	q_state.state = PSTATE_INITIAL;

	struct MHD_Response *response = nullptr;

	if ((uintptr_t) *con_cls < (uintptr_t) &callback_state || (uintptr_t) *con_cls > (uintptr_t) &callback_state[2]) {
		if (http_method != HTTP_PUT || !API.parse(q_state, (pChar) url, HTTP_PUT)) {
			LOGGER.log(LOG_MISS, "http_request_callback(): Trying to continue state_upload_in_progress, but API.parse() failed.");

			return MHD_NO;
		}
		q_state.rr_value.p_extra = (pExtraLocator) *con_cls;

		int sequence = (*upload_data_size == 0) ? SEQUENCE_FINAL_CALL : SEQUENCE_INCREMENT_CALL;

		switch (API.http_put((pChar) upload_data, *upload_data_size, q_state, sequence)) {
		case MHD_HTTP_CREATED:
			goto create_response_answer_put_ok;

		case MHD_HTTP_OK:
			goto continue_in_put_ok;
		}
		goto continue_in_put_not_acceptable;
	}

	// Step 3 : Get rid of failed uploads without doing anything.

	if (*con_cls == &callback_state[STATE_NOT_ACCEPTABLE]) {
		if (*upload_data_size == 0)
			goto create_response_answer_put_not_acceptable;

		return MHD_YES;
	}

	if (*con_cls == &callback_state[STATE_BAD_REQUEST]) {
		if (*upload_data_size == 0)
			goto create_response_answer_put_bad_request;

		return MHD_YES;
	}

	// Step 4 : This point is reached just once per http petition. Parse the query, returns errors and web pages, continue to API.

#ifdef DEBUG
	LOGGER.log_printf(LOG_DEBUG, "+----------------------------------+----------------------------+");
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - cls \x20 \x20 \x20 \x20 \x20 \x20 \x20: %p", cls);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - connection \x20 \x20 \x20 : %p", connection);
	MHD_get_connection_values(connection, MHD_HEADER_KIND, &print_out_key, NULL);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - url \x20 \x20 \x20 \x20 \x20 \x20 \x20: %s", url);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - method \x20 \x20 \x20 \x20 \x20 : %s", method);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - version \x20 \x20 \x20 \x20 \x20: %s", version);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - upload_data \x20 \x20 \x20: %p", upload_data);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - upload_data_size : %d", *upload_data_size);
	LOGGER.log_printf(LOG_DEBUG, "| HTTP callback - *con_cls \x20 \x20 \x20 \x20 : %p", *con_cls);
	LOGGER.log_printf(LOG_DEBUG, "+----------------------------------+----------------------------+");
#endif

	MHD_StatusCode status;

	switch (http_method) {
	case HTTP_NOTUSED:
		goto create_response_answer_put_bad_request;

	case HTTP_OPTIONS: {	// Shield variable "allow" initialization to support the goto logic.

			std::string allow;

			if (url[0] != '/' || url[1] != '/') {
				if (API.get_static(response, (pChar) url, false) == MHD_HTTP_OK)
					allow = "HEAD,GET,";

				allow = allow + "OPTIONS";
			} else {
				if (API.parse(q_state, (pChar) url, HTTP_GET))
					allow = "HEAD,GET,";

				if (API.parse(q_state, (pChar) url, HTTP_PUT))
					allow = allow + "PUT,";

				if (API.parse(q_state, (pChar) url, HTTP_DELETE))
					allow = allow + "DELETE,";

				allow = allow + "OPTIONS";
			}

			response = MHD_create_response_from_buffer(1, response_put_ok, MHD_RESPMEM_PERSISTENT);

			MHD_add_response_header(response, MHD_HTTP_HEADER_SERVER, "Jazz " JAZZ_VERSION " - " LINUX_PLATFORM);
			MHD_add_response_header(response, MHD_HTTP_HEADER_ALLOW, allow.c_str());
		}

		status = MHD_HTTP_NO_CONTENT;

		goto answer_status;

	case HTTP_HEAD:
	case HTTP_GET:
		if (url[0] != '/' || url[1] != '/') {

			if ((status = API.get_static(response, (pChar) url)) != MHD_HTTP_OK)
				return API.return_error_message(connection, (pChar) url, status);

			goto answer_status;

		} else if (!API.parse(q_state, (pChar) url, HTTP_GET))
			return API.return_error_message(connection, (pChar) url, MHD_HTTP_BAD_REQUEST);

		break;

	default:
		if (url[0] != '/' || url[1] != '/')
			return API.return_error_message(connection, (pChar) url, MHD_HTTP_METHOD_NOT_ALLOWED);

		else if (!API.parse(q_state, (pChar) url, http_method)) {

			if (http_method == HTTP_PUT)
				goto continue_in_put_bad_request;

			return API.return_error_message(connection, (pChar) url, MHD_HTTP_BAD_REQUEST);
		}
	}

	// Step 5 : This is the core. This point is only reached by correct API queries for the first (or only) time.

	switch (http_method) {
	case HTTP_PUT:
		status = API.http_put((pChar) upload_data, *upload_data_size, q_state, SEQUENCE_FIRST_CALL);

		break;

	case HTTP_DELETE:
		status = API.http_delete(q_state);

		break;

	default:

		status = API.http_get(response, q_state);
	}

	// Step 6 : The core finished, just distribute the answer as appropriate.

	if (http_method == HTTP_PUT) {
		if (status == MHD_HTTP_OK) {
			if (*upload_data_size) goto continue_in_put_ok;
			else 				   goto create_response_answer_put_ok;
		} else {
			if (*upload_data_size) goto continue_in_put_not_acceptable;
			else				   goto create_response_answer_put_not_acceptable;
		}
	}

	if (status != MHD_HTTP_OK)
		return API.return_error_message(connection, (pChar) url, status);

	if (http_method == HTTP_DELETE)
		response = MHD_create_response_from_buffer(1, response_put_ok, MHD_RESPMEM_PERSISTENT);

	MHD_Result ret;

answer_status:

	ret = MHD_queue_response(connection, status, response);

	MHD_destroy_response(response);

	return ret;

create_response_answer_put_ok:

	response = MHD_create_response_from_buffer(1, response_put_ok, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response(connection, MHD_HTTP_CREATED, response);

	MHD_destroy_response(response);

	return ret;

create_response_answer_put_not_acceptable:

	response = MHD_create_response_from_buffer(1, response_put_fail, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response(connection, MHD_HTTP_NOT_ACCEPTABLE, response);

	MHD_destroy_response(response);

	return ret;

create_response_answer_put_bad_request:

	response = MHD_create_response_from_buffer(1, response_put_fail, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);

	MHD_destroy_response(response);

	return ret;

continue_in_put_not_acceptable:

	if (*upload_data_size) {
		*upload_data_size = 0;
		*con_cls		  = &callback_state[STATE_NOT_ACCEPTABLE];

		return MHD_YES;
	}

	return MHD_NO;

continue_in_put_bad_request:

	if (*upload_data_size) {
		*upload_data_size = 0;
		*con_cls		  = &callback_state[STATE_BAD_REQUEST];

		return MHD_YES;
	}

	return MHD_NO;

continue_in_put_ok:

	if (*upload_data_size) {
		*upload_data_size = 0;
		*con_cls		  = q_state.rr_value.p_extra;

		return MHD_YES;
	}

	return MHD_NO;
}


void init_http_callback() {
	for (int i = 0; i < 1024; i++)
		http_methods[i] = HTTP_NOTUSED;

	http_methods[TenBitsAtAddress("OPTIONS")] = HTTP_OPTIONS;
	http_methods[TenBitsAtAddress("HEAD")]	  = HTTP_HEAD;
	http_methods[TenBitsAtAddress("GET")]	  = HTTP_GET;
	http_methods[TenBitsAtAddress("PUT")]	  = HTTP_PUT;
	http_methods[TenBitsAtAddress("DELETE")]  = HTTP_DELETE;
}


bool start_service(pService service) {
	cout << "Starting " << service->id() << " ... ";

	if (service->start() != SERVICE_NO_ERROR) {
		cout << "FAILED!" << endl;
		LOGGER.log_printf(LOG_ERROR, "Errors occurred starting %s.", service->id());

		return false;
	} else {
		cout << "ok" << endl;
		LOGGER.log_printf(LOG_INFO, "Service %s started Ok.", service->id());

		return true;
	}
}


bool stop_service(pService service) {
	cout << "Stopping " << service->id() << " ... ";

	if (service->shut_down() != SERVICE_NO_ERROR) {
		cout << "FAILED!" << endl;
		LOGGER.log_printf(LOG_ERROR, "Errors occurred stopping %s.", service->id());

		return false;
	} else {
		cout << "ok" << endl;
		LOGGER.log_printf(LOG_INFO, "Service %s stopped Ok.", service->id());

		return true;
	}
}

#endif


/** Capture SIGTERM. This callback procedure stops a running server.

	See main_server_start() for details on the server's start/stop.
*/
void signalHandler_SIGTERM(int signum) {
	cout << "Interrupt signal (" << signum << ") received." << endl;

	cout << "Closing the http server ... ok." << endl;

	MHD_stop_daemon(Jazz_MHD_Daemon);

	bool stop_ok = true;

#ifndef CATCH_TEST

	if (!stop_service(&HTTP))	   stop_ok = false;

	if (!stop_service(&API))	   stop_ok = false;

	if (!stop_service(&MODEL))	   stop_ok = false;

	if (!stop_service(&CORE))	   stop_ok = false;

	if (!stop_service(&PERSISTED)) stop_ok = false;
	if (!stop_service(&VOLATILE))  stop_ok = false;
	if (!stop_service(&CHANNELS))  stop_ok = false;

#endif

	if (stop_ok) exit(EXIT_SUCCESS); else exit(EXIT_FAILURE);
}

} // namespace jazz_main
