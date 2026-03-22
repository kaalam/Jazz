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


#include "src/jazz_models/concept.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_MODELS_DATA_SPACE
#define INCLUDED_JAZZ_MODELS_DATA_SPACE


//TODO: Make DataSpaces just an interface with the language and an internal container. A different one for each storage type.


/** \brief The DataSpace and its utilities.

This is an essential part of Bop to abstract data storage supporting dataframes, possibly sharded
and lazy-loaded. It also provides indexing which can select rows, keys of find nearest neighbors.
*/

namespace jazz_models
{

/** \brief RowSelection: An iterator over the rows of a Space.

	This is an abstract class that provides the interface of an iterator over rows. The where() method of a Space returns a RowSelection
	descendant that is compatible with this interface.

	\see Space

*/
// class RowSelection {

// 	/** \brief The constructor for a RowSelection.

// 		\param query	A query string that is understood by the descendant. In Bop, this is the content of a WHERE clause with a syntax
// 					    that depends on how the Space is indexed (Time, categorical, Embedding storage, ...).
// 		\param p_space	The Space that created the object is required because the descendant may need to call .get_index_data() or other
// 						methods.
// 	*/
// 	RowSelection(pChar query, pSpace p_space) {}

// 	/** \brief Restart the iterator.

// 		\return True if the iterator was successfully restarted, false if it is not possible.
// 	*/
// 	virtual bool restart() {
// 		return false;
// 	}

// 	/** \brief Get the next row.

// 		\return The next row number or SPACE_ROW_STOP_ITERATOR if the iteration is exhausted.
// 	*/
// 	virtual RowNumber next() {
// 		return SPACE_ROW_STOP_ITERATOR;
// 	}

// 	bool is_valid = false;	///< True if the iterator was created by a successful query.
// };
// typedef RowSelection *pRowSelection;	///< A pointer to a RowSelection


/** \brief ColSelection: A selection of columns from a Space.

	The interface is consistent with RowSelection, but unlike RowSelection the parent class is probably all you need.

	\see Space

*/
// class ColSelection {

// 	public:

// 		/** \brief The constructor for a ColSelection.

// 			The constructor will parse the query and create a list of column names and indices. These will not change during the lifetime
// 			of the object.

// 			\param query	By default, a list of comma separated column names. A descendant may define a different interface.
// 							In Bop, this is the content of a SELECT clause.
// 			\param p_space	The Space that created the object is required to call col_index().
// 		*/
// 		ColSelection(pChar query, pSpace p_space);

// 		virtual bool restart();

// 		virtual int	  next_index();
// 		virtual pName next_name();

// 		bool is_valid = false;	///< True if the iterator was created by a successful query.

// #ifndef CATCH_TEST
// 	private:
// #endif

// 		int current_col = 0;			///< The index of the current column (the next to be retrieved).
// 		stdNames name = {};				///< The list of column names in the selection.
// 		std::vector<int>  index = {};	///< The list of column indices in the selection.
// };
// typedef ColSelection *pColSelection;	///< A pointer to a ColSelection


#define DATASPACE_INDEX_ROW_NUM		1	///< The space is indexed by row number.
#define DATASPACE_INDEX_KEY			2	///< The space is indexed by key.
#define DATASPACE_INDEX_EMBEDDING	3	///< The space is indexed by partial row embedding.

// class Space : public Service {

// 	public:

// 	// Service interface

// 		Space(pBaseAPI api, pName a_name);

// 		virtual StatusCode start	();
// 		virtual StatusCode shut_down();

// 	// Persistence interface

// 		/** \brief Load the metadata of the Space.

// 			This method must be implemented by descendants. It will load a record of metadata from `//storage_base/storage_ent/name`.

// 			\return SERVICE_NO_ERROR if successful, an error code otherwise.
// 		*/
// 		virtual StatusCode load_meta() {
// 			return SERVICE_NOT_IMPLEMENTED;
// 		}

// 		/** \brief Save the metadata of the Space.

// 			This method must be implemented by descendants. It will store a record of metadata into `//storage_base/storage_ent/name`.

// 			\return SERVICE_NO_ERROR if successful, an error code otherwise.
// 		*/
// 		virtual StatusCode save_meta() {
// 			return SERVICE_NOT_IMPLEMENTED;
// 		}

// 	// Internal interface for ColSelection to have access to data by rows.

// 		/** \brief Return the number of rows in the Space.

// 			\return The number of rows in the Space or SPACE_NOT_A_ROW if the Space is invalid.
// 		*/
// 		virtual RowNumber num_rows() {
// 			return SPACE_NOT_A_ROW;
// 		}

// 		/** \brief Get a pointer to the data of the index of a given row.

// 			\param row	The row number.

// 			\return A pointer to the index or nullptr if there is no index. Examples include a vector for an embedding, of a string for
// 					a key-based index and a TimePoint for a time-based index.
// 		*/
// 		virtual void* get_index_data(RowNumber row) {
// 			return nullptr;
// 		}

// 	// Space interface

// 		/** \brief Get the number of columns in the Space.

// 			\return The number of columns in the Space.
// 		*/
// 		virtual int num_cols() {
// 			return 0;
// 		}

// 		/** \brief Get the name of a column.

// 			\param col The column index.

// 			\return The name of the column or nullptr if the index is out of range.
// 		*/
// 		virtual pName col_name(int col) {
// 			return nullptr;
// 		}

// 		/** \brief Get the index of a column.

// 			\param name The name of the column.

// 			\return The index of the column or -1 if the column is not found.
// 		*/
// 		virtual int col_index(pName name) {
// 			return -1;
// 		}

// 		/** \brief Get the location of a cell as a Locator

// 			\param row	 The row number.
// 			\param col	 The column index.
// 			\param index If the block contains multiple rows, which is the index of `row` in the block.

// 			\return A pointer to a Locator object that can be used to access the cell or nullptr if the cell is not found.
// 		*/
// 		virtual pLocator locator(RowNumber row, int col, int &index) {
// 			return nullptr;
// 		}

// 		/** \brief Get a RowSelection from a query.

// 			\param query A query string that is understood by the descendant. In Bop, this is the content of a WHERE clause with a syntax
// 						 that depends on how the Space is indexed (Time, categorical, Embedding storage, ...).

// 			\return A RowSelection object that can be used to iterate over the rows that match the query.
// 		*/
// 		virtual pRowSelection where(pChar query) {
// 			return nullptr;
// 		}

// 		/** \brief Get a ColSelection from a query.

// 			\param query By default, a list of comma separated column names. Spaces that use descendants of ColSelection may define a
// 						 different interface.
// 			\return A ColSelection object that can be used to iterate over the selected columns. If the query is invalid, the object
// 					will be invalid and return no columns. This is intentional since nullptr in a get_row() call means "all columns".
// 					An invalid query produces a selection of "no columns".
// 		*/
// 		virtual pColSelection select(pChar query) {
// 			return new ColSelection(query, this);
// 		}

// 		/** \brief Get the appropriate Caster from the AS clause of a query.

// 			\param query The name of the Caster to get.

// 			\return A pointer to the Caster or nullptr if the Caster is not found.
// 		*/
// 		virtual pCaster as(pChar query) {
// 			stdName name(query);
// 			Casters::iterator it = casters.find(name);
// 			if (it == casters.end())
// 				return nullptr;

// 			return it->second;
// 		}

// 		/** \brief Register a Caster descendant to make it available in queries.

// 			\param cast The Caster to register.

// 			\return True if the Caster was successfully registered, false if a Caster with the same name is already registered.
// 		*/
// 		virtual bool register_caster(pCaster cast) {
// 			Casters::iterator it = casters.find(cast->name);
// 			if (it != casters.end())
// 				return false;

// 			casters[cast->name] = cast;
// 			return true;
// 		}

// 		/** \brief Get a row from the Space as a Tuple

// 			\param p_txn A transaction that will be used to get the result. It must be destroyed with:
// 						 p_txn->p_owner->destroy_transaction(p_txn)
// 			\param row	 The row number as obtained from a RowSelection.next() call.
// 			\param cols	 Pointer to a ColSelection to specify columns to be retrieved. If nullptr, all columns are retrieved.
// 			\param cast	 Pointer to a Caster to convert the result. If nullptr, no conversion is done.

// 			\return SERVICE_NO_ERROR if successful, an error code otherwise.
// 		*/
// 		virtual StatusCode get_row(pTransaction	&p_txn, RowNumber row, pColSelection cols = nullptr, pCaster cast = nullptr) {
// 			return SERVICE_NOT_IMPLEMENTED;
// 		}

// #ifndef CATCH_TEST
// 	protected:
// #endif

// 		char storage_base [SHORT_NAME_SIZE];	///< The base name of the storage container.
// 		Name	 name;							///< The name of the Space.
// 		pBaseAPI p_api;							///< A pointer to the BaseAPI that provides access to containers.
// 		Casters	 casters;						///< A map of all the available Casters.
// };


// /** \brief DataSpaces: The data space.

// */
// class DataSpaces : public Space {

// 	public:

// 		DataSpaces(pBaseAPI api);

// 		virtual StatusCode start();
// 		virtual pChar const id();

// 		// Space interface

// 		// virtual StatusCode load_meta();
// 		// virtual StatusCode save_meta();
// 		// virtual RowNumber num_rows();
// 		// virtual void* get_index_data(RowNumber row);
// 		// virtual int num_cols();
// 		// virtual pName col_name(int col);
// 		// virtual int col_index(pName name);
// 		// virtual pLocator locator(RowNumber row, int col, int &index);
// 		// virtual pRowSelection where(pChar query);
// 		// virtual StatusCode get_row(pTransaction	&p_txn, RowNumber row, pColSelection cols = nullptr, pCaster cast = nullptr);

// 		// DataSpaces-ETL interface

// 	private:

// 		pBaseAPI p_api;				///< A pointer to the BaseAPI that provides access to containers.
// 		Name storage_ent;			///< The name of the storage entity (Typically an lmdb database with the metadata of all DataSpaces).
// };

/** \brief DataSpace
*/
class DataSpace : public Space {

	public:

		DataSpace(pBaseAPI p_owner);
		~DataSpace();

		std::map<stdName, std::string> grid;	///< An attribute that is a map of names to other Spaces or strings.

};
typedef DataSpace *pDataSpace;		///< A pointer to a DataSpace


} // namespace jazz_models

#endif // ifndef INCLUDED_JAZZ_MODELS_DATA_SPACE
