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


#include "src/include/jazz_platform.h"


#include <map>
#include <set>
#include <chrono>
#include <stdint.h>
#include <string>


#ifdef CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_TYPES
#define INCLUDED_JAZZ_ELEMENTS_TYPES


/*! \brief The namespace for Jazz Utils, Blocks, Kinds, Tuples, Containers, etc.

	This namespace includes utilities, types, constants and structures used by Block, all different block-based data and code structures
	(Tuple, Kind and Field). All the Services used to allocate/store and communicate blocks (Channels, Volatile, Persisted, ..).
*/
namespace jazz_elements
{

#define SHORT_NAME_SIZE			 8				///< Size of a node name or a base name (ending 0 included)
#define NAME_SIZE				32				///< Size of a Name (ending 0 included)
#define NAME_LENGTH				NAME_SIZE - 1	///< Maximum length of a Name.name
#define ONE_MB					(1024*1024)		///< Is used in log_printf() of error/warning messages

/** Number of elements preallocated in thread-specific buffers. This number can be modified down (but not up) via the configuration keys:
MHD_THREAD_POOL_SIZE and BEBOP_NUM_CORES. As expected, MHD_THREAD_POOL_SIZE also defines the thread pool size allocated in libmicrohttpd.

There is a specific document on threads called "On the Jazz Actor model (multithreading reference)" in the documentation
at <host>/develop/rfc2/jazz_actor_model.html
*/
#define JAZZ_MAX_NUM_THREADS	64

#define MAX_TENSOR_RANK			6			///< Maximum rank = 6, E.g. a 2D array of raw videos (row, column, frame, x, y, color)
#define MAX_CHECKS_4_MATCH		25			///< Maximum number of tries to match in get_string_offset() before setting stop_check_4_match
#define MAX_ITEMS_IN_KIND		64			///< The number of items merged into a kind or tuple.

/// Different values for Block.cell_type
#define CELL_TYPE__

#define CELL_TYPE_UNDEFINED		0x000		///< A cell_type value to be set by a text parser

// 8 bit cell types
#define CELL_TYPE_BYTE			0x001		///< A tensor of unsigned 8-bit binaries. NA is not defined for this type
#define CELL_TYPE_BYTE_BOOLEAN	0x101		///< A tensor 8-bit booleans: 0, 1, BYTE_BOOLEAN_NA = NA

// 32 bit cell types
#define CELL_TYPE_INTEGER		0x004		///< A tensor of 32-bit signed integers. NA is INTEGER_NA
#define CELL_TYPE_FACTOR		0x104		///< A tensor of 32-bit unsorted categorical. NA is INTEGER_NA
#define CELL_TYPE_GRADE			0x204		///< A tensor of 32-bit sorted categorical. NA is INTEGER_NA
#define CELL_TYPE_BOOLEAN		0x304		///< A tensor of 32-bit booleans: 0, 1, BOOLEAN_NA = NA
#define CELL_TYPE_SINGLE		0x404		///< A tensor of IEEE 754 32-bit float (aka single). NA is SINGLE_NA
#define CELL_TYPE_STRING		0x504		///< A tensor or 32-bit offsets to immutable strings or STRING_NA or STRING_EMPTY

// 64 bit cell types
#define CELL_TYPE_LONG_INTEGER	0x008		///< A tensor of 64-bit signed integers. NA is LONG_INTEGER_NA
#define CELL_TYPE_TIME			0x108		///< A tensor of 64-bit TimePoint. NA is TIME_POINT_NA
#define CELL_TYPE_DOUBLE		0x208		///< A vector of floating point numbers. Binary compatible with an R REALSXP (vector of numeric)

// 40 byte cell types
#define CELL_TYPE_TUPLE_ITEM	0x028		///< A vector of ItemHeader (in a Tuple)
#define CELL_TYPE_KIND_ITEM		0x128		///< A vector of ItemHeader (in a Kind)

// 48 byte cell types
#define CELL_TYPE_INDEX			0x030		///< An Index (accessed via a pBlockHeader instead of a pBlock)

// NA values or empty string values for all cell_type values
#define BYTE_BOOLEAN_NA			0x0ff		///< NA for 8-bit boolean is binary 0xff. Type does not exist in R.
#define BOOLEAN_NA				0x0ff		///< NA for a 32-bit boolean is binary 0xff. This is R compatible.
#define INTEGER_NA				INT_MIN		///< NA for a 32-bit integer. This is R compatible.
#define SINGLE_NA				F_NA		///< NA for a float is the value returned by nanf(). Type does not exist in R.
#define STRING_NA				0			///< NA for a string coded as CELL_TYPE_STRING
#define STRING_EMPTY			1			///< An empty string coded as CELL_TYPE_STRING
#define LONG_INTEGER_NA			LLONG_MIN	///< NA for a 64-bit integer. Type does not exist in R.
#define TIME_POINT_NA			0			///< NA for a CELL_TYPE_TIME is a 64-bit zero. Type does not exist in R.
#define DOUBLE_NA				R_NA		///< NA for a double. This is R compatible.

#define SINGLE_NA_UINT32		F_NA_uint32	///< An unsigned int32 version to .cell_uint[] ==
#define DOUBLE_NA_UINT64		R_NA_uint64	///< An unsigned int64 version to .cell_ulongint[] ==

/// Possible return values of Kind.kind_audit() and Tuple.tuple_audit()

#define MIXED_TYPE_

#define MIXED_TYPE_INVALID		0			///< The kind fails audit (wrong or duplicate item names, wrong dimensions, ...).
#define MIXED_TYPE_KIND		 	1			///< This Block is a valid Kind.
#define MIXED_TYPE_TUPLE		2			///< This Block is a valid Tuple.

/// Attribute types for Block descendants

#define BLOCK_ATTRIB_

#define BLOCK_ATTRIB_BLOCKTYPE	1			///< The fundamental block type: Tensor, Kind or Tuple. Can also be extended.
#define BLOCK_ATTRIB_SOURCE		2			///< The location of the source set by Channels, also Source of a Kind in Persisted
#define BLOCK_ATTRIB_DEST		3			///< The location of the destination. Less frequent, but may help Channels for a PUT.
#define BLOCK_ATTRIB_MIMETYPE	4			///< The mime type (can also be some proprietary file spec. E.g., "Adobe PhotoShop Image")
#define BLOCK_ATTRIB_URL		5			///< A url for the server to expose the file by.
#define BLOCK_ATTRIB_LANGUAGE	6			///< An http language identifier that will be returned in an API GET call.

/// Values for argument set_has_NA of close_block()
#define SET_HAS_NA_

#define SET_HAS_NA_FALSE		0			///< Set to false without checking
#define SET_HAS_NA_TRUE			1			///< Set to true without checking
#define SET_HAS_NA_AUTO			2			///< Check if there are and set accordingly (slowest option when closing, best later)


typedef std::chrono::steady_clock::time_point TimePoint;	///< A time point stored as 8 bytes


/** \brief A short identifier used in Blocks, Containers and API

Names are used in may contexts including: identifying a Container in an API query, a Container inside another Container, a Block in a
Container, a field in a Tuple or a Kind and some API query arguments (e.g., as_json).

Names are vanilla ASCII NAME_LENGTH long string starting with a letter and containing just letters, numbers and the underscore.
They can be validated using the function valid_name() or the regex REGEX_VALIDATE_NAME.
*/
typedef char Name[NAME_SIZE];
typedef char *pChar;


/** \brief The dimension of a tensor.

	The structure is declared as a union to make filter operation more elegant.
*/
struct TensorDim {
	int dim[MAX_TENSOR_RANK];	///< Dimensions for the Tensor. The product of all * (cell_type & 0xff) < 2 Gb
};


/// Header for an item (of a Kind or Tuple)
struct ItemHeader {
	int	cell_type;				///< The type for the cells in the item. See CELL_TYPE_*
	int name;					///< The name of this item as an offset in StringBuffer.
	int	rank;					///< The number of dimensions
	int	dim[MAX_TENSOR_RANK];	///< Dimensions for the Tensor. For Kind: negative numbers are dimension names as -offset in StringBuffer.
	union {
		int data_start;			///< The data start of this tensor as an offset of &BlockHeader.tensor. (If is is a Tuple.)
		int item_size;			///< During parsing of text blocks, this field is named item_size to temporarily hold the item size.
	};
};


/// A tensor of cell size 1, 4, 8 or sizeof(BlockHeader)
union Tensor {
	uint8_t	   cell_byte[0];		///< Cell size for CELL_TYPE_BYTE
	bool	   cell_bool[0];		///< .. CELL_TYPE_BYTE_BOOLEAN
	int		   cell_int[0];			///< .. CELL_TYPE_INTEGER, CELL_TYPE_FACTOR, CELL_TYPE_GRADE, CELL_TYPE_BOOLEAN and CELL_TYPE_STRING
	uint32_t   cell_uint[0];		///< .. CELL_TYPE_SINGLE or CELL_TYPE_BOOLEAN as 32 bit unsigned
	float	   cell_single[0];		///< .. CELL_TYPE_SINGLE
	long long  cell_longint[0];		///< .. CELL_TYPE_LONG_INTEGER
	time_t	   cell_time[0];		///< .. CELL_TYPE_TIME
	uint64_t   cell_ulongint[0];	///< .. CELL_TYPE_DOUBLE or CELL_TYPE_TIME as 64 bit unsigned
	double	   cell_double[0];		///< .. CELL_TYPE_DOUBLE
	ItemHeader cell_item[0];		///< .. An array of BlockHeader used by Kinds and Tuples
};


typedef std::set<std::string> Dimensions;				///< An set::set with the dimension names returned by kind.dimensions()


typedef std::map<std::string, std::string>	Index;		///< An Index kept in RAM by Volatile implemented as an stdlib map (string, string)


/// Header for a Movable Block (Tensor, Kind or Tuple) or a Dynamic Block (Index)
struct BlockHeader {
	int	cell_type;						///< The type for the cells in the tensor. See CELL_TYPE_*
	int size;							///< The total number of cells in the tensor
	TimePoint created;					///< Timestamp when the block was created
	union {
		struct {
			int	rank;					///< The number of dimensions
			TensorDim range;			///< The dimensions of the tensor in terms of ranges (Max. size is 2 Gb.)
			int num_attributes;			///< Number of elements in the JazzAttributesMap
			int total_bytes;			///< Total size of the block everything included
			bool has_NA;				///< If true, at least one value in the tensor is a NA and block requires NA-aware arithmetic
			uint64_t hash64;			///< Hash of everything but the header

			Tensor tensor;				///< A tensor for type cell_type and dimensions set by Block.set_dimensions()

		};
		Index index;					///< Any kind of Index
	};
};
typedef BlockHeader	*pBlockHeader;


/// A Binary Compatible BlockHeader without Index (and therefore constructors/destructors)
struct StaticBlockHeader {
	int	cell_type;						///< The type for the cells in the tensor. See CELL_TYPE_*
	int size;							///< The total number of cells in the tensor
	TimePoint created;					///< Timestamp when the block was created
			int	rank;					///< The number of dimensions
			TensorDim range;			///< The dimensions of the tensor in terms of ranges (Max. size is 2 Gb.)
			int num_attributes;			///< Number of elements in the JazzAttributesMap
			int total_bytes;			///< Total size of the block everything included
			bool has_NA;				///< If true, at least one value in the tensor is a NA and block requires NA-aware arithmetic
			uint64_t hash64;			///< Hash of everything but the header

			Tensor tensor;				///< A tensor for type cell_type and dimensions set by Block.set_dimensions()
};
typedef StaticBlockHeader *pStaticBlockHeader;


/// Structure at the end of a Block, initially created with init_string_buffer()
struct StringBuffer {
	bool stop_check_4_match;	///< When the StringBuffer is small, try to match existing indices of the same string to save RAM
	bool alloc_failed;			///< A previous call to get_string_offset() failed to alloc space for a string
	int	 last_idx;				///< The index to the first free space after the last stored string
	int	 buffer_size;			///< The size in bytes of buffer[]
	char buffer[];				///< The buffer where strings are stored starting with two zeroes for STRING_NA & STRING_EMPTY
};
typedef StringBuffer *pStringBuffer;


extern float  F_NA;		///< NaN in single
extern double R_NA;		///< NaN in double (binary R-compatible)

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_TYPES
