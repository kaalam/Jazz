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


#define CATCH_CONFIG_MAIN		//This tells Catch2 to provide a main() - has no effect when CATCH_TEST is not defined

#include "src/jazz_main/main.h"


#if !defined CATCH_TEST

using namespace std;
using namespace jazz_main;
using namespace jazz_bebop;
using namespace jazz_models;


/** Display the Jazz logo message automatically appending JAZZ_VERSION.
 */
void show_credits() {
	cout << "\x20 888888" << endl
		 << "\x20 \x20 `88b" << endl
		 << " \x20 \x20 888" << endl
		 << " \x20 \x20 888\x20 8888b.\x20 88888888 88888888" << endl
		 << " \x20 \x20 888 \x20 \x20 `88b\x20 \x20 d88P \x20 \x20 d88P" << endl
		 << " \x20 \x20 888 .d888888 \x20 d88P \x20 \x20 d88P" << endl
		 << " \x20 \x20 88P 888\x20 888\x20 d88P \x20 \x20 d88P" << endl
		 << " \x20 \x20 888 `Y888888 88888888 88888888" << endl
		 << " \x20 .d88P" << endl
		 << " .d88P'" << endl
		 << "888P'" << endl << endl
		 << " \x20 \x20 (c)" << JAZZ_YEARS << " - kaalam.ai - The Authors of Jazz" << endl << endl
		 << " \x20 \x20This product includes software developed at BBVA" << endl
		 << " \x20 \x20 Licensed under the Apache License, Version 2.0" << endl
		 << " \x20 \x20 \x20 http://www.apache.org/licenses/LICENSE-2.0" << endl
		 << " \x20 \x20 \x20 \x20 \x20version: " << JAZZ_VERSION << " (" << LINUX_PLATFORM << ")" << endl
#ifdef DEBUG
		 << endl
		 << " \x20 \x20 \x20 \x20 \x20 \x20 *** D E B U G - B U I L D ***" << endl
#endif
		 << endl;
}


/** Explain usage of the command line interface to stdout.
 */
void show_usage() {
	cout << "\x20 usage: jazz <config> start | stop | status" << endl << endl

		 << " <config>: A configuration file for the server in case of command start." << endl
		 << "\x20 \x20 \x20 \x20 \x20\x20 by default, Jazz will try to load: " << JAZZ_DEFAULT_CONFIG_PATH << endl
		 << "\x20 start\x20 : Start the jazz server." << endl
		 << "\x20 stop \x20 : Stop the jazz server." << endl
		 << "\x20 status : Just check if server is running." << endl;
}


/** Parse the command line argument (start, stop, ...) into a numeric constant, CMD_START, CMD_STOP, ...

	\param arg The argument as typed.
	\return the numeric constant and CMD_HELP when not known.
*/
int parse_command(const char *arg) {
	if (!strcmp("start",  arg)) return CMD_START;
	if (!strcmp("stop",	  arg)) return CMD_STOP;
	if (!strcmp("status", arg)) return CMD_STATUS;

	return CMD_HELP;
}


/** Entry point.

\param argc (Linux) number of arguments, counting the name of the executable.
\param argv (Linux) each argument, argv[0] being the name of the executable.

\return EXIT_FAILURE or EXIT_SUCCESS

When the command is "start":
	if running already, message + EXIT_FAILURE
	if configuration declared, but does no exist, message + EXIT_FAILURE
	else try to start the server calling main_server_start()

When the command is "stop":
	if not running already, message + EXIT_FAILURE
	else send a SIGTERM to the server and wait to check if it closes
		if the server closes before 20 seconds, message + EXIT_SUCCESS
		else message + EXIT_FAILURE

When the command is "status":
	if not running already, message + EXIT_FAILURE
	else message + EXIT_SUCCESS

When the command is anything else, too many or too few:
	show help + EXIT_FAILURE

 */
int main(int argc, char* argv[]) {
	int cmd = (argc < 2 || argc > 3) ? CMD_HELP : parse_command(argv[argc - 1]);

	if (cmd == CMD_HELP || argc == 3 && cmd != CMD_START) {
		show_credits();
		show_usage();

		exit(EXIT_FAILURE);
	}

#ifdef DEBUG
	string proc_name("./djazz");

	pid_t jzzPID;
#else
	string proc_name("./jazz");

	pid_t jzzPID = jazz_elements::FindProcessIdByName(proc_name.c_str());

	if (!jzzPID)
		proc_name = "/etc/jazz-server/jazz";
#endif

	jzzPID = jazz_elements::FindProcessIdByName(proc_name.c_str());

	if (!jzzPID) {
		if (cmd != CMD_START) {
			cout << "The process \"" << proc_name << "\" is not running." << endl;

			exit(EXIT_FAILURE);
		}

		if (argc == 3) {
			if (!jazz_elements::FileExists(argv[1])) {
				cout << "The file " << argv[1] << " does not exist." << endl;

				exit(EXIT_FAILURE);
			}
			if (!CONFIG.load_config(argv[1])) {
				cout << "The configuration file " << argv[1] << " could not be parsed." << endl;

				exit(EXIT_FAILURE);
			}
			cout << endl
				 << "**NOTE:** The configuration file " << argv[1] << " has been loaded." << endl
				 << "---------" << endl << endl;
		}

		show_credits();

		if (!start_service(&CHANNELS)) {
			exit(EXIT_FAILURE);
		}

		if (!start_service(&VOLATILE)) {
			stop_service(&CHANNELS);

			exit(EXIT_FAILURE);
		}

		if (!start_service(&PERSISTED)) {
			stop_service(&VOLATILE);
			stop_service(&CHANNELS);

			exit(EXIT_FAILURE);
		}

		if (!start_service(&CORE)) {
			stop_service(&PERSISTED);
			stop_service(&VOLATILE);
			stop_service(&CHANNELS);

			exit(EXIT_FAILURE);
		}

		if (!start_service(&MODEL)) {
			stop_service(&CORE);
			stop_service(&PERSISTED);
			stop_service(&VOLATILE);
			stop_service(&CHANNELS);

			exit(EXIT_FAILURE);
		}

		if (!start_service(&API)) {
			stop_service(&MODEL);
			stop_service(&CORE);
			stop_service(&PERSISTED);
			stop_service(&VOLATILE);
			stop_service(&CHANNELS);

			exit(EXIT_FAILURE);
		}

		init_http_callback();
		int ret_code = HTTP.start(&signalHandler_SIGTERM, Jazz_MHD_Daemon, &http_request_callback, CHANNELS);

		if (ret_code != EXIT_SUCCESS) {
			stop_service(&HTTP);
			stop_service(&API);
			stop_service(&MODEL);
			stop_service(&CORE);
			stop_service(&PERSISTED);
			stop_service(&VOLATILE);
			stop_service(&CHANNELS);
		}

		exit(ret_code);

	} else {

		if (cmd == CMD_START) {
			cout << "The process \"" << proc_name << "\" is already running with pid = " << jzzPID << "." << endl;

			exit(EXIT_FAILURE);
		}

		if (cmd == CMD_STOP) {
			kill(jzzPID, SIGTERM);

			cout << "Break signal was sent to process \"" << proc_name << "\" running with pid = " << jzzPID << "." << endl << endl;
			for (int t = 0; t < 200; t++) {
				struct timespec ts;

				ts.tv_sec  = 1;
				ts.tv_nsec = 0;
				nanosleep(&ts, NULL);

				if (!jazz_elements::FindProcessIdByName(proc_name.c_str())) {
					cout << "The process \"" << proc_name << "\" was stopped." << endl;

					exit(EXIT_SUCCESS);
				}
			}

			cout << "Waiting for \"" << proc_name << "\" to stop timed out." << endl;

			exit(EXIT_FAILURE);
		}

// (cmd == CMD_STATUS)

		cout << "The process \"" << proc_name << "\" is running with pid = " << jzzPID << "." << endl;

		exit(EXIT_SUCCESS);
	}
};

#endif
