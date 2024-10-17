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


#include "src/jazz_elements/block.h"

#ifdef CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_KIND
#define INCLUDED_JAZZ_ELEMENTS_KIND


namespace jazz_elements
{

/** \brief Kind: A type definition for Jazz Blocks and Tuples.

Kind objects contain the metadata only. A Tuple is a data object of a Kind. Kinds define more complex types than (raw) Blocks, even if
they are blocks. E.g., A Block can store a video of a fixed shape (image only or soundtrack only). A Kind can store both and have
**dimensions** defining things like: image_width, image_height, number_of_frames and, possibly, a subtitle track as another vector of
strings.

It is a block with special attributes to store an array of Tensor. A Kind is a single Block! A kind has **dimensions** which are integer
variables that are used to define variable shapes.

Technically, a Kind is a Block of type CELL_TYPE_KIND_ITEM. Each item contains the Tensor metadata.

Since kinds keep only metadata, the space, unlike in Tuples, is uninterrupted as in a normal block: (header, vector of CELL_TYPE_KIND_ITEM,
attribute keys, StringBuffer).

The StringBuffer contains the item names and dimension names.

Creating Kinds
--------------

More advanced Kind functionalities (including creating kinds) can be done by Containers. This object, has a minimum of functionality to
build by parts: new_kind(), add_item() to do the basic building. It also has function to check the content and validate a Kind.

Also, kinds should define, these attributes, but that is left to the Container:

- BLOCK_ATTRIB_BLOCKTYPE as the const "Kind"
- BLOCK_ATTRIB_SOURCE as the location where the definition of the Kind can be found. Kind names are global.

*/
class Kind : public Block {

	public:

		/** Get the name for an item of a Kind by index without checking index range.

			\param idx The index of the item.

			\return A pointer to where the (zero ended) string is stored in the Block or nullptr for an invalid index.

			NOTE: Use the pointer as read-only (more than one cell may point to the same value) and never try to free it.
		*/
		inline char *item_name(int idx)	{
			if (idx < 0 || idx >= size)
				return nullptr;

			return reinterpret_cast<char *>(&p_string_buffer()->buffer[tensor.cell_item[idx].name]);
		}

		/** Get the index for an item of a Kind by name.

			\param name The name of the item.

			\return A invalid index or -1 for "not found".
		*/
		inline int index(pChar name) {
			for (int idx = 0; idx < size; idx++) {
				if (strcmp(reinterpret_cast<char *>(&p_string_buffer()->buffer[tensor.cell_item[idx].name]), name) == 0)
					return idx;
			}
			return -1;
		}

		/** Pushes the Kind's dimension names into an std::set.
		*/
		inline void dimensions(Dimensions &dims) {
			for (int i = 0; i < size; i++) {
				ItemHeader *p_it_hea = &tensor.cell_item[i];

				for (int j = 0; j < p_it_hea->rank; j++) {
					int k = p_it_hea->dim[j];

					if (k < 0) {
						char *pt = (&p_string_buffer()->buffer[-k]);

						dims.insert(std::string(pt));
					}
				}
			}
		}

		/** Initializes a Kind object (step 1): Initializes the space.

			\param num_items The number of items the Kind will have. This call must be followed by one add_item() for each of them.
			\param num_bytes The size in bytes allocated. Should be enough for all names, dimensions and attributes + ItemHeaders.
			\param att		 The attributes for the Kind. Set "as is", without adding BLOCK_ATTRIB_BLOCKTYPE or anything.

			\return			 False on error (insufficient alloc size for a very conservative minimum).
		*/
		inline bool new_kind(int		   num_items,
							 int		   num_bytes,
			   				 AttributeMap *att = nullptr) {

			if (num_items < 1 || num_items > MAX_ITEMS_IN_KIND)
				return false;

			int rq_sz = sizeof(BlockHeader) + sizeof(StringBuffer) + num_items*sizeof(ItemHeader) + 2*num_items;

			if (att != nullptr)
				rq_sz += 2*att->size();

			if (num_bytes < rq_sz)
				return false;

			memset(&cell_type, 0, num_bytes);

			cell_type	 = CELL_TYPE_KIND_ITEM;
			rank		 = 1;
			range.dim[0] = 1;
			size		 = num_items;
			total_bytes	 = num_bytes;

			set_attributes(att);

			return true;
		}

		/** Initializes a Kind object (step 2): Adds each of the items.

			\param idx		 The index of the items to add. Must be in range [0..num_items-1] of the previous new_kind() call.
			\param p_name	 The name of the item.
			\param p_dim	 The shape of the item. Rank will be set automatically on the first zero.
			\param cell_type The cell type of the item.
			\param p_dims	 Names for the dimensions. See below.

			\return			 False on error (insufficient alloc space, wrong shape, wrong dimension names).

		How dimensions are defined.
		---------------------------

		Dimensions are placeholders in the tensor shapes that contain an integer variable such as "width" instead of a constant. They are
		defined by placing a unique negative number for each dimension in the tensor and giving it a name inside the dims map. E.g.,
		if an image has shape [-1, -2, 3] and dims[-1] == "width" and dims[-2] == "height", The kind will store these variable dimensions
		together with their names.
		*/
		inline bool add_item(int		   idx,
							 char const   *p_name,
							 int		  *p_dim,
							 int		   cell_type,
							 AttributeMap *p_dims) {

			if (idx < 0 || idx >= size)
				return false;

			ItemHeader *p_it_hea = &tensor.cell_item[idx];

			int rank = 6, j, k;

			pStringBuffer psb = p_string_buffer();

			for (int i = MAX_TENSOR_RANK - 1; i >= 0; i--) {
				j = p_dim[i];
				p_it_hea->dim[i] = j;
				if (j == 0) {
					rank = i;

					if (!i)
						return false;

				} else if (j < 0) {
					if (p_dims == nullptr)
						return false;

					AttributeMap::iterator it = p_dims->find(j);
					if (it == p_dims->end())
						return false;

					k = get_string_offset(psb, it->second);

					if (k <= STRING_EMPTY)
						return false;

					p_it_hea->dim[i] = -k;
				}
			}

			p_it_hea->name = get_string_offset(psb, p_name);

			if (p_it_hea->name <= STRING_EMPTY)
				return false;

			p_it_hea->cell_type = cell_type;
			p_it_hea->rank		= rank;

			return true;
		}

		int audit();
};
typedef Kind *pKind;

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_KIND
