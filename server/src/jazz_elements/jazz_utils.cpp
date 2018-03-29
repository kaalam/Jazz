/* Jazz (c) 2018 kaalam.ai (The Authors of Jazz), using (under the same license):

   BBVA - Jazz: A lightweight analytical web server for data-driven applications.

   Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

  This product includes software developed at

   BBVA (https://www.bbva.com/)

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

//TODO: Move all the utils and break the logger in functions.


#include <map>
#include <string.h>

#include <unistd.h>
#include <dirent.h>

#include <iostream>
#include <fstream>


#include "src/jazz_elements/jazz_utils.h"


namespace jazz_utils {


/** Count the number of bytes required by an utf-8 string of len characters.

\param buff The string (not necessarily null-terminated)
\param len	The number of characters in the string

\return		The numebr of bytes in the string.

The RFC http://www.ietf.org/rfc/rfc3629.txt says:

Char. number range	|		 UTF-8 octet sequence
(hexadecimal)	|			   (binary)
--------------------+---------------------------------------------
0000 0000-0000 007F | 0xxxxxxx
0000 0080-0000 07FF | 110xxxxx 10xxxxxx
0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
int CountBytesFromUtf8(char *buff, int len)
{
	int bytes = 0;

	while (len > 0)	{
		len--;
		bytes++;
		char lb = *buff++;

		if ((lb & 0xE0) == 0xC0) {			// 110x xxxx
			buff++;
			bytes++;
		}
		else if ((lb & 0xF0) == 0xE0) {		// 1110 xxxx
			buff  += 2;
			bytes += 2;
		}
		else if ((lb & 0xF8 ) == 0xF0) {	// 1111 0xxx
			buff  += 3;
			bytes += 3;
		}
	}

	return bytes;
}


/** Expand escaped strings at runtime.

Public Domain by Jerry Coffin.

Interprets a string in a manner similar to that the compiler does string literals in a program.	 All escape sequences are
longer than their translated equivalant, so the string is translated in place and either remains the same length or becomes shorter.

April 17 tests built and compliant with:

\\a	 07	   Alert (Beep, Bell) (added in C89)[1]
\\b	 08	   Backspace
\\f	 0C	   Formfeed
\\n	 0A	   Newline (Line Feed); see notes below
\\r	 0D	   Carriage Return
\\t	 09	   Horizontal Tab
\\v	 0B	   Vertical Tab
\\\\ 5C	   Backslash
\\'	 27	   Single quotation mark
\\"	 22	   Double quotation mark
\\?	 3F	   Question mark (used to avoid trigraphs)
\\nnn	   The byte whose numerical value is given by nnn interpreted as an octal number
\\xhh	   The byte whose numerical value is given by hh… interpreted as a hexadecimal number

https://en.wikipedia.org/wiki/Escape_sequences_in_C
*/
char *ExpandEscapeSequences(char *buff)
{
	char        *pt	 = buff;
	size_t		 len = strlen(buff);
	unsigned int num, any;

	while (nullptr != (pt = strchr(pt, '\\'))) {
		int numlen = 1;
		switch (pt[1]) {
		case 'a':
			*pt = '\a';
			break;

		case 'b':
			*pt = '\b';
			break;

		case 'f':
			*pt = '\f';
			break;

		case 'n':
			*pt = '\n';
			break;

		case 'r':
			*pt = '\r';
			break;

		case 't':
			*pt = '\t';
			break;

		case 'v':
			*pt = '\v';
			break;

		case '\\':
			break;

		case '\'':
			*pt = '\'';
			break;

		case '\"':
			*pt = '\"';
			break;

		case '\?':
			*pt = '\?';
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			if (pt[2] && pt[3] && sscanf(&pt[1], "%3o", &num) == 1 && sscanf(&pt[2], "%3o", &any) == 1 && sscanf(&pt[3], "%3o", &any) == 1) {
				numlen = 3;
				*pt	 = (char) num;
			} else {
				numlen = 1;
				*pt = pt[1];
			}
			break;

		case 'x':
			if (pt[2] && pt[3] && sscanf(&pt[2], "%2x", &num) == 1 && sscanf(&pt[3], "%2x", &any) == 1) {
				numlen = 3;
				*pt	 = (char) num;
			} else {
				numlen = 1;
				*pt = pt[1];
			}
			break;

		default:
			*pt = pt[1];
		}

		int siz = pt - buff + numlen;
		pt++;
		memmove(pt, pt + numlen, len - siz);
	}

	return buff;
}


/** Find the pid of a process given its name.

	It does NOT find itself as it is intended to find other processes of the same name.

	\param name The name of the process as typed on the console. In case parameters were given to start the process,
	it is just the left part before the first space.

	\return the pid of the process if found, 0 if not.
 */
pid_t FindProcessIdByName(const char *name)
{
	DIR			  *dir;
	struct dirent *ent;
	char		  *endptr;
	char		   buf[512];

	if (!(dir = opendir("/proc"))) {
		perror("can't open /proc");

		return 0;
	}

	pid_t pid_self = getpid();

	while((ent = readdir(dir)) != nullptr) {
		// if endptr is not null, the directory is not entirely numeric, so ignore it
		long lpid = strtol(ent->d_name, &endptr, 10);

		if (*endptr != '\0') {
			continue;
		}

		// try to open the cmdline file
		snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
		FILE* fp = fopen(buf, "r");

		if (fp) {
			if (fgets(buf, sizeof(buf), fp) != nullptr) {
				// check the first token in the file, the program name
				char* first = strtok(buf, " ");

				// cout << first << endl;
				if (!strcmp(first, name) && (pid_t) lpid != pid_self) {
					fclose(fp);
					closedir(dir);

					return (pid_t) lpid;
				}
			}
			fclose(fp);
		}
	}

	closedir(dir);

	return 0;
}


#define MURMUR_SEED	  76493		///< Just a 5 digit prime

/** MurmurHash2, 64-bit versions, by Austin Appleby

	(from https://sites.google.com/site/murmurhash/)

	All code is released to the public domain. For business purposes, Murmurhash is
	under the MIT license.

	The same caveats as 32-bit MurmurHash2 apply here - beware of alignment
	and endian-ness issues if used across multiple platforms.

	// 64-bit hash for 64-bit platforms
*/
uint64_t MurmurHash64A(const void *key, int len)
{
	const uint64_t m = 0xc6a4a7935bd1e995;
	const int	   r = 47;

	uint64_t h = MURMUR_SEED ^ (len*m);

	const uint64_t *data = (const uint64_t *) key;
	const uint64_t *end  = data + (len/8);

	while(data != end) {
		uint64_t k = *data++;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const unsigned char *data2 = (const unsigned char*) data;

	switch(len & 7)	{
	case 7: h ^= uint64_t(data2[6]) << 48;
	case 6: h ^= uint64_t(data2[5]) << 40;
	case 5: h ^= uint64_t(data2[4]) << 32;
	case 4: h ^= uint64_t(data2[3]) << 24;
	case 3: h ^= uint64_t(data2[2]) << 16;
	case 2: h ^= uint64_t(data2[1]) << 8;
	case 1: h ^= uint64_t(data2[0]);

		h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}


/** Remove space and tab for a string.

	\param s Input string
	\return String without space or tab.
*/
std::string RemoveSpaceOrTab(std::string s)
{
	for(int i = s.length() - 1; i >= 0; i--) if(s[i] == ' ' || s[i] == '\t') s.erase(i, 1);

	return s;
}


/** Load a configuration from a file.

	Configuration is stored in: map<string, string> config which is private and read using the function get_key().

	\param input_file_name The input file name containing a configuration

	\return check num_keys
*/
JazzConfigFile::JazzConfigFile(const char *input_file_name)
{
	std::ifstream fh (input_file_name);

	if (!fh.is_open()) return;

	std::string ln, key, val;

	while (!fh.eof())
	{
		getline(fh, ln);

		size_t p;
		p = ln.find("//");

		if (p != std::string::npos) ln.erase(p, ln.length());

		p = ln.find("=");

		if (p != std::string::npos)
		{
			key = RemoveSpaceOrTab(ln.substr(0, p - 1));
			val = RemoveSpaceOrTab(ln.substr(p + 1, ln.length()));

			std::cout << "config_put(\"" << key << "\", \"" << val << "\");" << std::endl;
			config[key] = val;
		}
	}
	fh.close();
}


/** Get the number of known configuration keys.

	\return	 The number of configuration keys read from the file when constructing the object. Zero means some failure.
*/
int JazzConfigFile::num_keys ()
{
	return config.size();
}


/** Get the value for an existing configuration key.

	\param key	 The configuration key to be searched.
	\param value Value to be returned only when the function returns true.
	\return		 True when the key exists and can be returned with the specific (overloaded) type.
*/
bool JazzConfigFile::get_key(const char *key, int &value)
{
	std::string keys (key);

	try	{
		std::string val = config[keys];
		std::string::size_type extra;

		int i = stoi(val, &extra);

		if (extra != val.length()) return false;

		value = i;

		return true;
	}

	catch (...) {
		return false;
	}
}


/** Get the value for an existing configuration key.

	\param key	 The configuration key to be searched.
	\param value Value to be returned only when the function returns true.
	\return		 True when the key exists and can be returned with the specific (overloaded) type.
*/
bool JazzConfigFile::get_key(const char *key, double &value)
{
	std::string keys (key);

	try {
		std::string val = config[keys];
		std::string::size_type extra;

		double d = stod(val, &extra);

		if (extra != val.length()) return false;

		value = d;

		return true;
	}

	catch (...) {
		return false;
	}
}


/** Get the value for an existing configuration key.

	\param key	 The configuration key to be searched.
	\param value Value to be returned only when the function returns true.
	\return		 True when the key exists and can be returned with the specific (overloaded) type.
*/
bool JazzConfigFile::get_key(const char *key, std::string &value)
{
	std::string keys (key);

	std::string s = config[keys];

	if (!s.length()) return false;

	value = s;

	return true;
}


/** DEBUG ONLY function: Set a config key manually.

	\param key	The configuration key to be set.
	\param val	New value of the key as a string (also valid for int and double if the string can be converted).
*/
void JazzConfigFile::debug_put(const std::string key, const std::string val)
{
	config[key] = val;
}

} // namespace jazz_utils


#if defined CATCH_TEST
#include "src/jazz_elements/tests/test_utils.ctest"
#endif
