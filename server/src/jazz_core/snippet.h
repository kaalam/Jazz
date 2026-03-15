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


#include "src/jazz_core/opcodes.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_SNIPPET
#define INCLUDED_JAZZ_BEBOP_SNIPPET


/** \brief A Concept ancestor that contains both the source and the object code

A Snippet is an object that can do both forward (compile) and reverse engineering (decompile) between compilable Bop and an onnx file.
It supports a number of serializations to and from Jazz Blocks. It forms the minimal unit of what can be run.

*/

//TODO: Synchronize http://localhost:8888/jazz_reference/api_ref_core.html (Erase this only when Core is done.)

namespace jazz_core
{

/** The kind of a Snippet
*/
const char KIND_SNIPPET[]	= "{\"object\" : BYTE[obj_size], \"body\" : STRING[body_len], \"calls\" : STRING[calls_len]," \
							  "\"input\" : STRING[input_len], \"output\" : STRING[output_len], \"reads\" : STRING[reads_len]," \
							  "\"source\" : STRING[source_len], \"writes\" : STRING[writes_len]}";

/** The empty Snippet
*/
const char EMPTY_SNIPPET[]	= "(\"object\" : [], \"body\" : [NA], \"calls\" : [NA], \"input\" : [NA], \"output\" : [NA]," \
							  "\"reads\" : [NA], \"source\" : [NA], \"writes\" : [NA])";

/** The empty Snippet
*/
const char SNIPPET_VERSION[]				= "snp1";	///< The content of the attribute BLOCK_ATTRIB_SNIPVERS

const char SNIPSTATE_EMPTY_SNIPPET[]		= "____";	///< The snippet is empty
const char SNIPSTATE_SOURCE_AVAILABLE[]		= "s___";	///< The source code is available
const char SNIPSTATE_SOURCE_PREPROCESSED[]	= "sx__";	///< The source code has been preprocessed successfully
const char SNIPSTATE_SOURCE_COMPILED[]		= "sxo_";	///< The source code has been compiled successfully
const char SNIPSTATE_OBJECT_AVAILABLE[]		= "__o_";	///< The snippet is created from an onnx object
const char SNIPSTATE_OBJECT_PREPROCESSED[]	= "_xo_";	///< The snippet (from onnx) has been reverse engineered successfully
const char SNIPSTATE_CAN_RUN[]				= "___a";	///< The global state is neither error nor empty state() >= SNIPSTATE_CAN_RUN
const char SNIPSTATE_CAN_RUN_OBJECT[]		= "_xor";	///< The snippet (from onnx) is ready to run (or has run before)
const char SNIPSTATE_CAN_RUN_SOURCE[]		= "sxor";	///< The snippet (from source) is ready to run (or has run before)
const char SNIPSTATE_IS_RUNNING_OBJECT[]	= "_xoi";	///< The snippet is currently running (from onnx)
const char SNIPSTATE_IS_RUNNING_SOURCE[]	= "sxoi";	///< The snippet is currently running (from source)
const char SNIPSTATE_FAILED_SRC_PREPROC[] 	= "sX_!";	///< The source code preprocessing failed
const char SNIPSTATE_FAILED_SRC_COMPILE[] 	= "sxO!";	///< The source code compilation failed
const char SNIPSTATE_FAILED_OBJ_PREPROC[] 	= "_Xo!";	///< The snippet (from onnx) reverse engineering failed
const char SNIPSTATE_FAILED_RUN_OBJECT[]	= "_xo!";	///< The snippet failed to run (from onnx)
const char SNIPSTATE_FAILED_RUN_SOURCE[]	= "sxo!";	///< The snippet failed to run (from source)

#define MASK_SNIPSTATE_GENERAL	0xff000000				///< The mask for the general state
#define MASK_SNIPSTATE_OBJECT	0x00ff0000				///< The mask for the object state
#define MASK_SNIPSTATE_INTER	0x0000ff00				///< The mask for the intermediate state
#define MASK_SNIPSTATE_SOURCE	0x000000ff				///< The mask for the source state
#define SNIPSTATE_UNDEFINED		0x7fffffff				///< An error state returned when there is no attribute

#define SNIP_INDEX_OBJECT		0						///< The index of the object code in a Snippet
#define SNIP_INDEX_BODY			1						///< The index of the body code in a Snippet
#define SNIP_INDEX_CALLS		2						///< The index of the calls code in a Snippet
#define SNIP_INDEX_INPUT		3						///< The index of the input code in a Snippet
#define SNIP_INDEX_OUTPUT		4						///< The index of the output code in a Snippet
#define SNIP_INDEX_READS		5						///< The index of the reads code in a Snippet
#define SNIP_INDEX_SOURCE		6						///< The index of the source code in a Snippet
#define SNIP_INDEX_WRITES		7						///< The index of the writes code in a Snippet

#define BLOCK_ATTRIB_SNIPSTATE	(BLOCK_ATTRIB_BASE_BOP + 1)		///< The compilation/running state of a Snippet.
#define BLOCK_ATTRIB_SNIPVERS	(BLOCK_ATTRIB_BASE_BOP + 2)		///< The Snippet interface version.

/** \brief A SnippetText is a vector of strings

	A std::vector of String to hold the source and intermediate code of a Snippet. This is used to make compilation
and code generation easier to interface in c++.

The caller owns the object (initially empty) and is responsible for freeing it when no longer needed.
*/
typedef std::vector<String> SnippetText;
typedef SnippetText *pSnippetText;				///< A pointer to a SnippetText


/** \brief Snippet: A code snippet and the ancestor of Concept.

A Snippet is a Tuple with items and some attributes. The items are sources and compiled onnx code. The attributes identify it as a Snippet
and manage state ().

Attributes
----------

A snippet always defines two attributes:

BLOCK_ATTRIB_SNIPVERS	: The version of the Snippet '1' reserved for future use or extension in general (Concepts).
BLOCK_ATTRIB_SNIPSTATE	: A four character from SNIPSTATE_EMPTY_SNIPPET to SNIPSTATE_FAILED_RUN_SOURCE

The string is made by binary sorted ASCII characters with 4 fields: general, object, intermediate (body, input, output, reads, writes,
calls) and source. The characters are (lower is problematic: ! (33) is irrecoverable error, 'A'..'Z' (65..90) are
explanations for error states, _ (93) is empty, 'a'..'z' (97..122) are valid states). The order of the fields is general goes in the
top bits when represented as an integer. state() >= SNIPSTATE_CAN_RUN matches any can_run or is_running,
state() < SNIPSTATE_EMPTY_SNIPPET matches any error.

You can also mask using MASK_SNIPSTATE_GENERAL .. MASK_SNIPSTATE_SOURCE to get the state of a specific part of the Snippet. The indices
are defines as SNIP_INDEX_OBJECT .. SNIP_INDEX_WRITES.

Kind
----

A Valid snippet satisfies the Kind defined as KIND_SNIPPET

The items are:

\verbatim
source, input, output, reads, writes, calls, body, object
------  -----------------------------------------  ------
   |                        |                         +--> The Onnx object code
   |                        +----------------------------> The intermediate (preprocessed) source code
   +-----------------------------------------------------> The original source code (If available)
\endverbatim

Item indices: Since the order of the blocks in a tuple are fixed, you can retrieve the items both by name of by index.

Interface
---------

The Snippet object provides an interface to access the text parts as a SnippetText and also returns its state as an
integer and its onnx object as a pointer.

Since Snippets are immutable, the logic of adding blocks to the tuple is managed by the Fields object.

*/
class Snippet : public Tuple {

	public:

		using Tuple::get_block;

		int	  get_state();
		bool  get_block(int idx, SnippetText &snip_text);
		bool  get_block(pChar name, SnippetText &snip_text);
		int	  object_size();
		void* get_object();
};
typedef Snippet *pSnippet;		///< A pointer to a Snippet

} // namespace jazz_core

#endif // ifndef INCLUDED_JAZZ_BEBOP_SNIPPET
