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


#include <map>

#include <microhttpd.h>
#include <curl/curl.h>

#include "src/jazz_elements/container.h"

#ifdef CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_CHANNEL
#define INCLUDED_JAZZ_ELEMENTS_CHANNEL


namespace jazz_elements
{

#define BASE_BASH_10BIT				0x022		//< First 10 bits of base "bash"
#define BASE_FILE_10BIT				0x126		//< First 10 bits of base "file"
#define BASE_HTTP_10BIT				0x288		//< First 10 bits of base "http"
#define BASE_0_MQ_10BIT				0x1b0		//< First 10 bits of base "0-mq"


#define MAX_FILE_OR_URL_SIZE		1712		///< Used inside an ExtraLocator, it makes the structure 2 Kbytes.

/// HttpQueryState apply values (on state == PSTATE_COMPLETE_OK)

#define APPLY_NOTHING					 0		///< Just an l_value with {///node}//base/entity or {///node}//base/entity/key
#define APPLY_NAME						 1		///< {///node}////base/entity/key:name (Select an item form a Tuple by name)
#define APPLY_URL						 2		///< {///node}////base& any_url_encoded_url ; (A call to http or file)
#define APPLY_FUNCTION					 3		///< {///node}//base/entity/key(//r_base/r_entity/r_key) (A function call with a block.)
#define APPLY_FUNCT_CONST				 4		///< {///node}////base/entity/key(& any_url_encoded_const) (A function call with a const.)
#define APPLY_FILTER					 5		///< {///node}//base/entity/key[//r_base/r_entity/r_key] (A filter using a a block.)
#define APPLY_FILT_CONST				 6		///< {///node}//base/entity/key[& any_url_encoded_const] (A filter using a a const.)
#define APPLY_RAW						 7		///< {///node}//base/entity/key.raw (Serialize text to raw.)
#define APPLY_TEXT						 8		///< {///node}//base/entity/key.text (Serialize raw to text.)
#define APPLY_ASSIGN_NOTHING			 9		///< {///node}//base/entity/key=//r_base/r_entity/r_key (Assign block to block.)
#define APPLY_ASSIGN_NAME				10		///< {///node}//base/entity/key=//r_base/r_entity/r_key:name (Tuple item -> block)
#define APPLY_ASSIGN_URL				11		///< {///node}//base/entity/key=//r_base/r_entity& any_url_encoded_url ; (Assign using url.)
#define APPLY_ASSIGN_FUNCTION			12		///< {///node}//base/entity/key=//r_base/r_entity/r_key(//t_base/t_entity/t_key)
#define APPLY_ASSIGN_FUNCT_CONST		13		///< {///node}//base/entity/key=//r_base/r_entity/r_key(& any_url_encoded_const)
#define APPLY_ASSIGN_FILTER				14		///< {///node}//base/entity/key=//r_base/r_entity/r_key[//t_base/t_entity/t_key]
#define APPLY_ASSIGN_FILT_CONST			15		///< {///node}//base/entity/key=//r_base/r_entity/r_key[& any_url_encoded_const]
#define APPLY_ASSIGN_RAW				16		///< {///node}//base/entity/key=//r_base/r_entity/r_key.raw
#define APPLY_ASSIGN_TEXT				17		///< {///node}//base/entity/key=//r_base/r_entity/r_key.text
#define APPLY_ASSIGN_CONST				18		///< {///node}//base/entity/key=& any_url_encoded_const ; (Assign const to block.)
#define APPLY_NEW_ENTITY				19		///< {///node}//base/entity.new (Create a new entity)
#define APPLY_GET_ATTRIBUTE				20		///< {///node}//base/entity/key.attribute(123) (read attribute 123 with HTTP_GET)
#define APPLY_SET_ATTRIBUTE				21		///< {///node}////base/entity/key.attribute(46)=& url_encoded ; (set attrib. with HTTP_GET)
#define APPLY_JAZZ_INFO					22		///< /// Show the server info.


/// A map for defining http config names
typedef std::map<int, std::string>	MapIS;


/// A structure holding connections.
typedef std::map<std::string, Index> ConnMap;


/// A structure to hold a single pipeline
struct Socket {
	char endpoint[120];		///< The endpoint at which the socket is connected.
	void *requester;		///< The (connected) zmq socket
};


/// A structure to share with the libcurl get callback.
typedef std::vector<uint8_t> GetBuffer;
typedef GetBuffer *pGetBuffer;


/// A structure keep state inside a put callback.
struct PutBuffer {
	uint64_t to_send;		///< Number of bytes to be sent.
	uint8_t *p_base;		///< The pointer (updated after each call) to the data.
};
typedef PutBuffer *pPutBuffer;


/// A structure holding pipeline.
typedef std::map<std::string, Socket> PipeMap;


/// A map for defining http config ports
typedef std::map<int, int>	MapII;


/*! \brief A proper type for specifying http status codes

Before libmicrohttpd (somewhere between > 0.9.66-1 and <= 0.9.72-2) changed MHD_Result to an enum, MHD_Result was (improperly) used
to define HTTP responses. That ended-up badly on newer versions, since it was passed to a MHD_queue_response() and stopped working
as it became an enum.

This triggered the need, for clarity reasons only, to introduce a new type, MHD_StatusCode to refer to **HTTP responses**.

See: https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml

*/
typedef unsigned int MHD_StatusCode;

extern size_t get_callback(char *ptr, size_t size, size_t nmemb, void *container);
extern size_t put_callback(char *ptr, size_t size, size_t nmemb, void *container);
extern size_t dev_null(char *_ignore, size_t size, size_t nmemb, void *_ignore_2);

/** \brief Channels: A Container doing block transactions across media (files, folders, shell, http urls and zeroMQ servers)

NOTES: 1. This is the only container that does not have a native interface. Since urls and file names can be very long the easy interface
is all you need, there is no as_locator() parsing.
2. The container itself does not store (almost) anything. Of course, files and folders are permanent storage and the http call may be
stored by some remote server.
3. Channels support four bases: "0-mq", "bash", "file" and "http". The reference for each base is this docstring.
4. copy() copies across any enabled channels as long as the corresponding get() and put() are correct.

"0-mq" Reference
----------------

The 0-mq implementation in jazz_elements is only a zeroMQ client, not a server. The only operation it supports is a translate() call.
For a translate() call to succeed, you must first have created a pipeline using a put call with a string. E.g,
put("//0-mq/pipeline/speech2text") with a block containing a string like "tcp://localhost:5555" creates a pipeline named speech2text.
Outside Jazz, you set up a zeroMQ server that listens to tcp://localhost:5555 and does speech to text, expecting, say 32-bit int vectors
as input and returning a buffer of char as output. In your translate call you must provide a tuple with two Tensors with item names "input"
and "result". The translate() call will send the raw tensor of the item "input" to the server and write whatever the server answers into
the tensor named "result", just overriding the tensor without any dimension change.
This operation expects the tensor to be binary (i.e., no variable length strings) and their shapes and types known by both parts.

In terms of the Jazz server API, this is a function call: either GET "//0-mq/speech2text/(//lmdb/stuff/my_tensor)" or
GET "//0-mq/speech2text/(&[1,2,3];)" the argument can be anything in Persisted, Volatile, even a file or an //http get or a (%-encoded)
constant as in the second example.

When using translate() as the method of Channel, you should omit the "pipeline" part, just translate(p_tuple, "//0-mq/speech2text");

Besides this, get("//0-mq/pipeline/speech2text") will return just a block with "tcp://localhost:5555" and
remove("//0-mq/pipeline/speech2text") will destroy the pipeline. Any other call using "0-mq" returns SERVICE_ERROR_NOT_APPLICABLE.

"0-mq" operation must be enabled via configuration by setting ENABLE_ZEROMQ_CLIENT to something non-zero.

"bash" Reference
----------------

This is also a translate() call, the difference is you don't create the pipeline, it always exists and is called "//bash/exec". The Tuple
is an array of byte, both ways "input" and "result". If the size of the "result" buffer is too small for the answer it will be filled up to
the available size and something will be lost. The answer includes whatever a popen("bash script.sh") writes to stdout / stderr (where
script.sh is the content of the "input" tensor).
"bash" operation must be enabled via configuration by setting ENABLE_BASH_EXEC to something non-zero. There is no security check: it can be
used for pushing AI creations to github or kill the server with //bash/exec(& jazz%20stop ;)

"file" Reference
----------------

This read/writes/deletes to the filesystem. Since the API does not use locators, there is no hardcoded name restriction. Via the http
server, just use the URL (&...;). Remember to %-encode whatever http expects to be encoded. E.g., get("//file/&whatever%20you%20want;").
Note that "//file/" is a mandatory prefix, therefore "//file/aa" is "aa" and //file//aa" is "/aa".
get() gets files as arrays of byte and folders as an Index serialized as a Tuple (the keys are file names and the values either "file" or
"folder"). put() writes either Jazz blocks with all the metadata (if mode == WRITE_EVERYTHING) of just the content of the tensor
(if mode == WRITE_TENSOR_DATA).
WRITE_ONLY_IF_EXISTS and WRITE_ONLY_IF_NOT_EXISTS work as expected. remove() deletes whatever matches the path either a file or a folder
(with anything inside it).
new_entity() creates a new folder.

"file" operation must be enabled via configuration by setting ENABLE_FILE_LEVEL to 0 (disable everything), 1 (just read), 2 (cannot
override == no remove and WRITE_ONLY_IF_NOT_EXISTS) or 3 (enable everything).

"http" Reference
----------------

Besides being an http server, Jazz is also an http client. The simplest mode of operation is just forwarding get(), put() and delete()
calls that are intended for other nodes in a Jazz cluster. This is done at the top API level by just adding a node name. E.g.,
get("///node_x//lmdb/things/this") will forward the call to the node_x (if anything is well configured see JAZZ_NODE_NAME_.., etc.)
and return the result just as if is was a local call. At the class level, this is done by forward_get(), forward_put() and forward_del().
You can also send simple GET, PUT and DELETE http calls to random urls by either using the get(), put() and remove() or using the Jazz http
server API GET "//http&https://google.com;"

The most advanced way to do it is creating a connection (similar to a "0-mq" pipeline) by put()-ing an Tuple to: //http/connection/a_name
The tuple has two items named "key" and "value" that are vectors of string of the same size (like the ones returned by new_block(8)).
The key must have a the mandatory "URL" and optionally: "CURLOPT_USERNAME", "CURLOPT_USERPWD", "CURLOPT_COOKIEFILE" and "CURLOPT_COOKIEJAR".
See https://curl.se/libcurl/c/CURLOPT_USERNAME.html and https://everything.curl.dev/libcurl-http/cookies. Once the connection exists, you
can get(), put() and remove() to just its name (without the word connection). I.e, get(txn, "//http/a_name") or
get(txn, "//http/a_name/args") will send the http GET to connection[URL] + "args". Same for put() and remove().

If you remove("//http/connection/a_name"), you destroy the connection. get("//http/connection/a_name") returns an Index serialized as a
Tuple with all the connection parameters.

"http" operation must be enabled via configuration by setting ENABLE_HTTP_CLIENT to something non-zero.
*/
class Channels : public Container {

	public:

		Channels(pLogger	 a_logger,
				 pConfigFile a_config);
	   ~Channels();

		virtual pChar const id();

		StatusCode	   start	   ();
		StatusCode	   shut_down   ();

		virtual StatusCode get		 (pTransaction		&p_txn,
									  pChar				 p_what);
		virtual StatusCode get		 (pTransaction		&p_txn,
									  pChar				 p_what,
									  pBlock			 p_row_filter);
		virtual StatusCode get		 (pTransaction		&p_txn,
									  pChar				 p_what,
									  pChar				 name);
		virtual StatusCode locate	 (Locator			&location,
									  pChar				 p_what);
		virtual StatusCode header	 (StaticBlockHeader &hea,
									  pChar				 p_what);
		virtual StatusCode header	 (pTransaction		&p_txn,
									  pChar				 p_what);
		virtual StatusCode put		 (pChar				 p_where,
									  pBlock			 p_block,
									  int				 mode = WRITE_AS_BASE_DEFAULT);
		virtual StatusCode new_entity(pChar				 p_where);
		virtual StatusCode remove	 (pChar				 p_where);
		virtual StatusCode copy		 (pChar				 p_where,
									  pChar				 p_what);
		virtual StatusCode modify    (Locator			&function,
									  pTuple			 p_args);
		MHD_StatusCode forward_get	 (pTransaction		&p_txn,
									  Name				 node,
									  pChar				 p_url);
		MHD_StatusCode forward_put	 (Name				 node,
									  pChar				 p_url,
									  pBlock			 p_block,
									  int				 mode = WRITE_AS_BASE_DEFAULT);
		MHD_StatusCode forward_del	 (Name				 node,
									  pChar				 p_url);

		// Support for container names in the API .base_names()

		void base_names(BaseNames &base_names);

		// Public config variables

		MapIS jazz_node_name = {};
		MapIS jazz_node_ip   = {};
		MapII jazz_node_port = {};

		bool search_my_node_index	= false;
		int  jazz_node_my_index		= -1;
		int  jazz_node_cluster_size =  0;

		std::string filesystem_root = {};

#ifndef CATCH_TEST
	protected:
#endif

		/** \brief The most low level get function.

			\param p_txn A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						 Transaction inside the Container. The caller can only use it read-only and **must** destroy_transaction()
						 it when done.
			\param url	 The url to be got.
			\param p_idx Additional curl_easy_setopt() options passed in an Index.

			\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

		*/
		inline StatusCode curl_get(pTransaction &p_txn, const char *url, Index *p_idx = nullptr) {
			CURL *curl;
			CURLcode c_ret;

			curl = curl_easy_init();
			if (!curl)
				return SERVICE_ERROR_NOT_READY;

			GetBuffer buff = {};

			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &buff);

			if (p_idx != nullptr) {
				Index:: iterator it;
				if ((it = p_idx->find("CURLOPT_USERNAME")) != p_idx->end())
					curl_easy_setopt(curl, CURLOPT_USERNAME, it->second.c_str());

				if ((it = p_idx->find("CURLOPT_USERPWD")) != p_idx->end())
					curl_easy_setopt(curl, CURLOPT_USERPWD, it->second.c_str());

				if ((it = p_idx->find("CURLOPT_COOKIEFILE")) != p_idx->end())
					curl_easy_setopt(curl, CURLOPT_COOKIEFILE, it->second.c_str());

				if ((it = p_idx->find("CURLOPT_COOKIEJAR")) != p_idx->end())
					curl_easy_setopt(curl, CURLOPT_COOKIEJAR, it->second.c_str());
			}
			c_ret = curl_easy_perform(curl);

			uint64_t response_code;

			if (c_ret == CURLE_OK)
    			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

			curl_easy_cleanup(curl);

			switch (c_ret) {
			case CURLE_OK:
				break;
			case CURLE_REMOTE_ACCESS_DENIED:
			case CURLE_AUTH_ERROR:
				return SERVICE_ERROR_READ_FORBIDDEN;
			case CURLE_REMOTE_FILE_NOT_FOUND:
				return SERVICE_ERROR_BLOCK_NOT_FOUND;
			default:
				return SERVICE_ERROR_IO_ERROR;
			}

			switch (response_code) {
			case MHD_HTTP_OK:
			case MHD_HTTP_CREATED:
			case MHD_HTTP_ACCEPTED:
				break;
			case MHD_HTTP_NOT_FOUND:
			case MHD_HTTP_GONE:
				return SERVICE_ERROR_BLOCK_NOT_FOUND;
			case MHD_HTTP_BAD_REQUEST:
				return SERVICE_ERROR_WRONG_ARGUMENTS;
			case MHD_HTTP_UNAUTHORIZED:
			case MHD_HTTP_PAYMENT_REQUIRED:
			case MHD_HTTP_FORBIDDEN:
			case MHD_HTTP_METHOD_NOT_ALLOWED:
			case MHD_HTTP_NOT_ACCEPTABLE:
			case MHD_HTTP_PROXY_AUTHENTICATION_REQUIRED:
			case MHD_HTTP_TOO_MANY_REQUESTS:
				return SERVICE_ERROR_READ_FORBIDDEN;
			case MHD_HTTP_INTERNAL_SERVER_ERROR ... MHD_HTTP_LOOP_DETECTED:
				return SERVICE_ERROR_MISC_SERVER;
			default:
				return SERVICE_ERROR_IO_ERROR;
			}
			size_t buf_size = buff.size();
			if (buf_size > MAX_BLOCK_SIZE)
				return SERVICE_ERROR_BLOCK_TOO_BIG;

			buff.push_back(0);

			return unwrap_received(p_txn, (pBlock) buff.data(), buf_size);
		}


		/** \brief The most low level put function.

			\param url	 The url to put to.
			\param p_blk The Block to be sent by Channels.
			\param mode  WRITE_TENSOR_DATA, WRITE_C_STR or WRITE_EVERYTHING
			\param p_idx Additional curl_easy_setopt() options passed in an Index.

			\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

		*/
		inline StatusCode curl_put(const char *url, pBlock p_blk, int mode = WRITE_AS_STRING | WRITE_AS_FULL_BLOCK, Index *p_idx = nullptr) {
			CURL *curl;
			CURLcode c_ret;

			curl = curl_easy_init();
			if (!curl)
				return SERVICE_ERROR_NOT_READY;

			PutBuffer put_buff;

			if ((mode & WRITE_AS_ANY_WRITE) == 0)
				mode = WRITE_AS_STRING | WRITE_AS_FULL_BLOCK;

			if ((mode & WRITE_AS_STRING) && (	(p_blk->cell_type == CELL_TYPE_STRING && p_blk->size == 1)
											 || (p_blk->cell_type == CELL_TYPE_BYTE	  && p_blk->rank == 1))) {
				if (p_blk->cell_type == CELL_TYPE_STRING) {
					put_buff.p_base  = (uint8_t *) p_blk->get_string(0);
					put_buff.to_send = strlen((const char *) put_buff.p_base);
				} else {
					put_buff.p_base  = &p_blk->tensor.cell_byte[0];
					put_buff.to_send = strnlen((const char *) put_buff.p_base, p_blk->size);
				}
			} else if ((mode & WRITE_AS_CONTENT) && ((p_blk->cell_type & 0xf0) == 0)) {
				put_buff.to_send = p_blk->size*(p_blk->cell_type & 0xff);
				put_buff.p_base  = &p_blk->tensor.cell_byte[0];
			} else if ((mode & WRITE_AS_FULL_BLOCK) && (p_blk->cell_type != CELL_TYPE_INDEX)) {
				put_buff.to_send = p_blk->total_bytes;
				put_buff.p_base  = (uint8_t *) p_blk;
			} else
				return SERVICE_ERROR_WRONG_ARGUMENTS;

			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dev_null);
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, put_callback);
			curl_easy_setopt(curl, CURLOPT_READDATA, (void *) &put_buff);
			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
 			curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) put_buff.to_send);

			if (p_idx != nullptr) {
				Index:: iterator it;
				if ((it = p_idx->find("CURLOPT_USERNAME")) != p_idx->end())
					curl_easy_setopt(curl, CURLOPT_USERNAME, it->second.c_str());

				if ((it = p_idx->find("CURLOPT_USERPWD")) != p_idx->end())
					curl_easy_setopt(curl, CURLOPT_USERPWD, it->second.c_str());

				if ((it = p_idx->find("CURLOPT_COOKIEFILE")) != p_idx->end())
					curl_easy_setopt(curl, CURLOPT_COOKIEFILE, it->second.c_str());

				if ((it = p_idx->find("CURLOPT_COOKIEJAR")) != p_idx->end())
					curl_easy_setopt(curl, CURLOPT_COOKIEJAR, it->second.c_str());
			}
			c_ret = curl_easy_perform(curl);

			uint64_t response_code;

			if (c_ret == CURLE_OK)
    			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

			curl_easy_cleanup(curl);

			switch (c_ret) {
			case CURLE_OK:
				break;
			case CURLE_REMOTE_ACCESS_DENIED:
			case CURLE_AUTH_ERROR:
				return SERVICE_ERROR_WRITE_FORBIDDEN;
			case CURLE_REMOTE_FILE_NOT_FOUND:
				return SERVICE_ERROR_BLOCK_NOT_FOUND;
			default:
			return SERVICE_ERROR_IO_ERROR;
			}

			switch (response_code) {
			case MHD_HTTP_OK:
			case MHD_HTTP_CREATED:
			case MHD_HTTP_ACCEPTED:
				return SERVICE_NO_ERROR;
			case MHD_HTTP_NOT_FOUND:
			case MHD_HTTP_GONE:
				return SERVICE_ERROR_BLOCK_NOT_FOUND;
			case MHD_HTTP_BAD_REQUEST:
				return SERVICE_ERROR_WRONG_ARGUMENTS;
			case MHD_HTTP_UNAUTHORIZED:
			case MHD_HTTP_PAYMENT_REQUIRED:
			case MHD_HTTP_FORBIDDEN:
			case MHD_HTTP_METHOD_NOT_ALLOWED:
			case MHD_HTTP_NOT_ACCEPTABLE:
			case MHD_HTTP_PROXY_AUTHENTICATION_REQUIRED:
			case MHD_HTTP_TOO_MANY_REQUESTS:
				return SERVICE_ERROR_WRITE_FORBIDDEN;
			case MHD_HTTP_INTERNAL_SERVER_ERROR ... MHD_HTTP_LOOP_DETECTED:
				return SERVICE_ERROR_MISC_SERVER;
			}
			return SERVICE_ERROR_IO_ERROR;
		}


		/** \brief The most low level remove function.

			\param url	 The url to put to.
			\param p_idx Additional curl_easy_setopt() options passed in an Index.

			\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

		*/
		inline StatusCode curl_remove(const char *url, Index *p_idx = nullptr) {
			CURL *curl;
			CURLcode c_ret;

			curl = curl_easy_init();
			if (!curl)
				return SERVICE_ERROR_NOT_READY;

			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dev_null);

			if (p_idx != nullptr) {
				Index:: iterator it;
				if ((it = p_idx->find("CURLOPT_USERNAME")) != p_idx->end())
					curl_easy_setopt(curl, CURLOPT_USERNAME, it->second.c_str());

				if ((it = p_idx->find("CURLOPT_USERPWD")) != p_idx->end())
					curl_easy_setopt(curl, CURLOPT_USERPWD, it->second.c_str());

				if ((it = p_idx->find("CURLOPT_COOKIEFILE")) != p_idx->end())
					curl_easy_setopt(curl, CURLOPT_COOKIEFILE, it->second.c_str());

				if ((it = p_idx->find("CURLOPT_COOKIEJAR")) != p_idx->end())
					curl_easy_setopt(curl, CURLOPT_COOKIEJAR, it->second.c_str());
			}
			c_ret = curl_easy_perform(curl);

			uint64_t response_code;

			if (c_ret == CURLE_OK)
    			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

			curl_easy_cleanup(curl);

			switch (c_ret) {
			case CURLE_OK:
				break;
			case CURLE_REMOTE_ACCESS_DENIED:
			case CURLE_AUTH_ERROR:
				return SERVICE_ERROR_WRITE_FORBIDDEN;

			case CURLE_REMOTE_FILE_NOT_FOUND:
				return SERVICE_ERROR_BLOCK_NOT_FOUND;
			default:
				return SERVICE_ERROR_IO_ERROR;
			}

			switch (response_code) {
			case MHD_HTTP_OK:
			case MHD_HTTP_CREATED:
			case MHD_HTTP_ACCEPTED:
				return SERVICE_NO_ERROR;
			case MHD_HTTP_NOT_FOUND:
			case MHD_HTTP_GONE:
				return SERVICE_ERROR_BLOCK_NOT_FOUND;
			case MHD_HTTP_BAD_REQUEST:
				return SERVICE_ERROR_WRONG_ARGUMENTS;
			case MHD_HTTP_UNAUTHORIZED:
			case MHD_HTTP_PAYMENT_REQUIRED:
			case MHD_HTTP_FORBIDDEN:
			case MHD_HTTP_METHOD_NOT_ALLOWED:
			case MHD_HTTP_NOT_ACCEPTABLE:
			case MHD_HTTP_PROXY_AUTHENTICATION_REQUIRED:
			case MHD_HTTP_TOO_MANY_REQUESTS:
				return SERVICE_ERROR_WRITE_FORBIDDEN;
			case MHD_HTTP_INTERNAL_SERVER_ERROR ... MHD_HTTP_LOOP_DETECTED:
				return SERVICE_ERROR_MISC_SERVER;
			}
			return SERVICE_ERROR_IO_ERROR;
		}

#ifndef CATCH_TEST
	private:
#endif
		char HEX[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

		inline bool compose_url(pChar p_dest, pChar p_node, pChar p_url, int buff_size) {
			int nix;

			for (MapIS::iterator it = jazz_node_name.begin(); true; ++it) {
				if (it == jazz_node_name.end())
					return false;

				if (it->second == p_node) {
					nix = it->first;
					break;
				}
			}
			int ret = snprintf(p_dest, buff_size, "http://%s:%i", jazz_node_ip[nix].c_str(), jazz_node_port[nix]);

			if (ret < 0 || ret >= buff_size)
				return false;

			p_dest	  += ret;
			buff_size -= ret;

			while (buff_size > 0) {
				u_int8_t cursor = *(p_url++);
				switch (cursor) {
				case 0:
					*p_dest = 0;
					return true;
				case '!' ... '$':
				case '&' ... '/':
				case '0' ... '9':
				case ':':
				case ';':
				case '=':
				case '?':
				case '@':
				case 'A' ... 'Z':
				case '[':
				case ']':
				case '_':
				case 'a' ... 'z':
				case '~':
					*(p_dest++) = cursor;
					buff_size--;
					break;
				default:
					if (buff_size < 4)
						return false;
					*(p_dest++) = '%';
					*(p_dest++) = HEX[cursor >> 4];
					*(p_dest++) = HEX[cursor & 0x0f];
					buff_size -= 3;
				}
			}
			return false;
		}

		int can_curl = false, curl_ok = false;
		int can_zmq  = false, zmq_ok  = false;
		int can_bash = false;
		int file_lev = 0;

		PipeMap	pipes	= {};
		ConnMap connect = {};

		void *zmq_context = nullptr;
};
typedef Channels *pChannels;


#ifdef CATCH_TEST

// Instancing Channels
// -------------------

extern Channels CHN;

#endif

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_CHANNEL
