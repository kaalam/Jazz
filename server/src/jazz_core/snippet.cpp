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


#include "src/jazz_core/snippet.h"


namespace jazz_core
{

/*	-----------------------------------------------
	 Snippet : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** Returns the state of the Snippet as an integer.

	Returns the value of the attribute BLOCK_ATTRIB_SNIPSTATE (only the first 4 chars) as an integer.

	\see Snippet

	\return The state of the Snippet as an integer.

*/
int Snippet::get_state() {
	pChar pt = get_attribute(BLOCK_ATTRIB_SNIPSTATE);

	if (pt == nullptr)
		return SNIPSTATE_UNDEFINED;

	return reinterpret_cast<int*>(pt)[0];
}


/** Get a specific block (whose type is a tensor of strings) into a SnippetText.

	\param idx			The index of the block.
	\param snip_text	The SnippetText to fill. The caller must provide an empty object and is responsible of
						disposing it. If not empty, the content will be appended.

	\return True if the block was found and either is is empty or snip_text was filled.

*/
bool Snippet::get_block(int idx, SnippetText &snip_text) {

	pBlock pt = get_block(idx);

	if ((pt == nullptr) || (pt->cell_type != CELL_TYPE_STRING))
		return false;

	for (int i = 0; i < pt->size; i++) {
		if (pt->tensor.cell_int[i] != STRING_NA) {
			String s = pt->get_string(i);
			snip_text.push_back(s);
		}
	}

	return true;
}


/** Get a specific block (whose type is a tensor of strings) into a SnippetText.

	\param name			The name of the block.
	\param snip_text	The SnippetText to fill. The caller must provide an empty object and is responsible of
						disposing it. If not empty, the content will be appended.

	\return True if the block was found and either is is empty or snip_text was filled.

*/
bool Snippet::get_block(pChar name, SnippetText &snip_text) {

	int idx = index(name);

	if (idx < 0)
		return false;

	return get_block(idx, snip_text);
}


/** Get the size of the onnx object block.

	\return The size of the object block or 0 if the block is not found.
*/
int	Snippet::object_size() {

	pBlock pt = get_block(SNIP_INDEX_OBJECT);

	if (pt)
		return pt->size;

	return 0;
}


/** Get the onnx object block.

	This does not copy or allocate memory, it returns a pointer inside the Snippet object which will be valid
	as long as the Snippet object is valid. Do not try to free it.

	\return A pointer to the onnx object block or nullptr if the block is not found.
*/
void* Snippet::get_object() {

	pBlock pt = get_block(SNIP_INDEX_OBJECT);

	if (pt && pt->size > 0)
		return &pt->tensor.cell_byte[0];

	return nullptr;
}

} // namespace jazz_core

#if defined CATCH_TEST
#include "src/jazz_core/tests/test_snippet.ctest"
#endif
