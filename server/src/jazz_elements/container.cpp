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


#include "src/jazz_elements/container.h"


namespace jazz_elements
{

/*	-------------------------------------------------------------
	 Global const (to avoid local initialization at each call)
---------------------------------------------------------------- */

char NA [8]				 = NA_AS_TEXT;
char ESCAPE_LOW_ASCII[8] = {"abtnvfr"};
char DEF_INT8_FMT [8]	 = {"%hhu\0"};
char DEF_INT32_FMT [8]	 = {"%i\0"};
char DEF_INT64_FMT [8]	 = {"%lli\0"};
char DEF_FLOAT32_FMT [8] = {"%.9e\0"};
char DEF_FLOAT64_FMT [8] = {"%.18e\0"};
char DEF_FLOAT_TIME [24] = {"%Y-%m-%d %H:%M:%S"};

uint32_t F_NA_uint32;	///< A binary exact copy of F_NA
uint64_t R_NA_uint64;	///< A binary exact copy of R_NA

int LOCATOR_SIZE[3] = {SHORT_NAME_SIZE - 1, NAME_SIZE - 1, NAME_SIZE - 1};

/*	--------------------------------------------------------
	 Global compile_next_state_LUT() (also used in API)
--------------------------------------------------------- */

void compile_next_state_LUT(ParseNextStateLUT lut[], int num_states, ParseStateTransition trans[]) {

	memset(lut, -1, num_states*sizeof(ParseNextStateLUT));

	int i = 0;
	while (trans[i].from != num_states) {
		ParseNextStateLUT *p_next = &lut[trans[i].from];

		std::regex  rex(trans[i].rex);
		std::string s("-");

		for (int j = 0; j < 256; j++) {
			s[0] = j;
			if (std::regex_match(s, rex)) {
#ifdef DEBUG
				if (p_next->next[j] != PSTATE_INVALID_CHAR)
					throw 1;
#endif
				p_next->next[j] = trans[i].to;
			}
		}

		i++;
	}
}

/*	-----------------------------------------------
	 Parser grammar definition
--------------------------------------------------- */

#define REX_ALL_SPACES			"[ \\t]"
#define REX_ALL_SPACES_LEFT_BR	"[ \\t\\[]"
#define REX_ALL_SPACES_RIGHT_BR	"[ \\t\\]]"
#define REX_COMMA				"[,]"
#define REX_DOUBLEQUOTE			"[\"]"
#define REX_RIGHT_BR			"[\\]]"
#define REX_BACKSLASH			"[\\\\]"
#define REX_NA_FIRST			"[N]"
#define REX_NA_ANY				"[A \\t]"
#define REX_NUMBER_FIRST		"[0-9\\-]"
#define REX_DOT_FIRST			"[\\.]"
#define REX_INT_ANY				"[0-9]"
#define REX_REAL_ANY			"[e0-9\\+\\-\\.]"
#define REX_INT_REAL_SWITCH		"[e\\.]"
#define REX_STRING_NOT_ESC		"[\\x20-\\x21\\x23-\\[\\]-\\x7e]"	// Anything from 32 to 126, except \ and "
#define REX_STRING_FIRST_ESC	"[abtnvfr\"\\\\]"					// Anything immediately after a \ (except x)
#define REX_STRING_HEX			"[0-9a-fA-F]"						// Anything immediately after a \x (twice)
#define REX_X					"[x]"								// x after a \ leads to hex
#define REX_TIME				"[0-9a-zA-Z :\\-]"					// Enforces DEF_FLOAT_TIME = "%Y-%m-%d %H:%M:%S"

#define MAX_NUM_PSTATES			27		///< Maximum number of non error states the parser can be in
#define NUM_STATE_TRANSITIONS	75		///< Maximum number of state transitions in the parsing grammar. Applies to const only.

#define PSTATE_IN_AUTO			 0		///< Parser state: Reached "[" (shape in), cell type is CELL_TYPE_UNDEFINED
#define PSTATE_IN_INT			 1		///< Parser state: Reached "[" (shape in), cell type is any integer
#define PSTATE_IN_REAL			 2		///< Parser state: Reached "[" (shape in), cell type is any float
#define PSTATE_IN_STRING		 3		///< Parser state: Reached "[" (shape in), cell type is CELL_TYPE_STRING
#define PSTATE_IN_TIME			 4		///< Parser state: Reached "[" (shape in), cell type is CELL_TYPE_TIME

#define PSTATE_NA_INT			 5		///< Parser state: Reached "[" (shape in), cell type is any integer
#define PSTATE_NA_REAL			 6		///< Parser state: Reached "[" (shape in), cell type is any float
#define PSTATE_NA_STRING		 7		///< Parser state: Reached "[" (shape in), cell type is CELL_TYPE_STRING
#define PSTATE_NA_TIME			 8		///< Parser state: Reached "[" (shape in), cell type is CELL_TYPE_TIME

#define PSTATE_CONST_AUTO		 9		///< Parser state: Parsing value, cell type is CELL_TYPE_UNDEFINED
#define PSTATE_CONST_INT		10		///< Parser state: Parsing value, cell type is any integer
#define PSTATE_CONST_REAL		11		///< Parser state: Parsing value, cell type is any float
#define PSTATE_CONST_STRING0	12		///< Parser state: Parsing value (first char == "), cell type is CELL_TYPE_STRING
#define PSTATE_CONST_STRING_E0	13		///< Parser state: Parsing value (string is escaped by \\), cell type is CELL_TYPE_STRING
#define PSTATE_CONST_STRING_E1	14		///< Parser state: Parsing value (1 hex chars pending after \\xX), cell type is CELL_TYPE_STRING
#define PSTATE_CONST_STRING_E2	15		///< Parser state: Parsing value (2 hex chars pending after \\x), cell type is CELL_TYPE_STRING
#define PSTATE_CONST_STRING_N	16		///< Parser state: Parsing value, cell type is CELL_TYPE_STRING
#define PSTATE_CONST_TIME		17		///< Parser state: Parsing value, cell type is CELL_TYPE_TIME

#define PSTATE_SEP_INT			18		///< Parser state: Reached a cell separator, cell type is any integer
#define PSTATE_SEP_REAL			19		///< Parser state: Reached a cell separator, cell type is any float
#define PSTATE_SEP_STRING		20		///< Parser state: Reached a cell separator, cell type is CELL_TYPE_STRING
#define PSTATE_SEP_TIME			21		///< Parser state: Reached a cell separator, cell type is CELL_TYPE_TIME

#define PSTATE_END_STRING		22		///< Parser state: Reached a cell separator (last char == "), cell type is CELL_TYPE_STRING

#define PSTATE_OUT_INT			23		///< Parser state: Reached "]" (shape out), cell type is any integer
#define PSTATE_OUT_REAL			24		///< Parser state: Reached "]" (shape out), cell type is any float
#define PSTATE_OUT_STRING		25		///< Parser state: Reached "]" (shape out), cell type is CELL_TYPE_STRING
#define PSTATE_OUT_TIME			26		///< Parser state: Reached "]" (shape out), cell type is CELL_TYPE_TIME

/** A vector of StateTransition.l This only runs once, when construction the API object, initializes the LUTs from a sequence of
StateTransition constants in the source of api.cpp.
*/
typedef ParseStateTransition ParseStateTransitions[NUM_STATE_TRANSITIONS];

ParseStateTransitions state_tr = {

	{PSTATE_IN_AUTO,			PSTATE_IN_AUTO,			REX_ALL_SPACES_LEFT_BR},
	{PSTATE_IN_AUTO,			PSTATE_CONST_AUTO,		REX_NUMBER_FIRST},
	{PSTATE_IN_AUTO,			PSTATE_CONST_STRING0,	REX_DOUBLEQUOTE},
	{PSTATE_IN_AUTO,			PSTATE_IN_REAL,			REX_DOT_FIRST},

	{PSTATE_IN_INT,				PSTATE_IN_INT,			REX_ALL_SPACES_LEFT_BR},
	{PSTATE_IN_INT,				PSTATE_CONST_INT,		REX_NUMBER_FIRST},
	{PSTATE_IN_INT,				PSTATE_NA_INT,			REX_NA_FIRST},

	{PSTATE_IN_REAL,			PSTATE_IN_REAL,			REX_ALL_SPACES_LEFT_BR},
	{PSTATE_IN_REAL,			PSTATE_CONST_REAL,		REX_NUMBER_FIRST},
	{PSTATE_IN_REAL,			PSTATE_NA_REAL,			REX_NA_FIRST},

	{PSTATE_IN_STRING,			PSTATE_IN_STRING,		REX_ALL_SPACES_LEFT_BR},
	{PSTATE_IN_STRING,			PSTATE_CONST_STRING0,	REX_DOUBLEQUOTE},
	{PSTATE_IN_STRING,			PSTATE_NA_STRING,		REX_NA_FIRST},

	{PSTATE_IN_TIME,			PSTATE_IN_TIME,			REX_ALL_SPACES_LEFT_BR},
	{PSTATE_IN_TIME,			PSTATE_CONST_TIME,		REX_INT_ANY},				// Enforces DEF_FLOAT_TIME = "%Y-%m-%d %H:%M:%S"
	{PSTATE_IN_TIME,			PSTATE_NA_TIME,			REX_NA_FIRST},

	{PSTATE_CONST_AUTO,			PSTATE_CONST_AUTO,		REX_INT_ANY},
	{PSTATE_CONST_AUTO,			PSTATE_CONST_REAL,		REX_INT_REAL_SWITCH},
	{PSTATE_CONST_AUTO,			PSTATE_SEP_INT,			REX_COMMA},
	{PSTATE_CONST_AUTO,			PSTATE_OUT_INT,			REX_ALL_SPACES_RIGHT_BR},

	{PSTATE_CONST_INT,			PSTATE_CONST_INT,		REX_INT_ANY},
	{PSTATE_CONST_INT,			PSTATE_SEP_INT,			REX_COMMA},
	{PSTATE_CONST_INT,			PSTATE_OUT_INT,			REX_ALL_SPACES_RIGHT_BR},

	{PSTATE_CONST_REAL,			PSTATE_CONST_REAL,		REX_REAL_ANY},
	{PSTATE_CONST_REAL,			PSTATE_SEP_REAL,		REX_COMMA},
	{PSTATE_CONST_REAL,			PSTATE_OUT_REAL,		REX_ALL_SPACES_RIGHT_BR},

	{PSTATE_CONST_STRING0,		PSTATE_CONST_STRING_E0,	REX_BACKSLASH},
	{PSTATE_CONST_STRING0,		PSTATE_CONST_STRING_N,	REX_STRING_NOT_ESC},
	{PSTATE_CONST_STRING0,		PSTATE_END_STRING,		REX_DOUBLEQUOTE},

	{PSTATE_CONST_STRING_E0,	PSTATE_CONST_STRING_E2,	REX_X},
	{PSTATE_CONST_STRING_E0,	PSTATE_CONST_STRING_N,	REX_STRING_FIRST_ESC},

	{PSTATE_CONST_STRING_E1,	PSTATE_CONST_STRING_N,	REX_STRING_HEX},

	{PSTATE_CONST_STRING_E2,	PSTATE_CONST_STRING_E1,	REX_STRING_HEX},

	{PSTATE_CONST_STRING_N,		PSTATE_CONST_STRING_N,	REX_STRING_NOT_ESC},
	{PSTATE_CONST_STRING_N,		PSTATE_CONST_STRING_E0,	REX_BACKSLASH},
	{PSTATE_CONST_STRING_N,		PSTATE_END_STRING,		REX_DOUBLEQUOTE},

	{PSTATE_END_STRING,			PSTATE_END_STRING,		REX_ALL_SPACES},
	{PSTATE_END_STRING,			PSTATE_SEP_STRING,		REX_COMMA},
	{PSTATE_END_STRING,			PSTATE_OUT_STRING,		REX_RIGHT_BR},

	{PSTATE_CONST_TIME,			PSTATE_CONST_TIME,		REX_TIME},
	{PSTATE_CONST_TIME,			PSTATE_SEP_TIME,		REX_COMMA},
	{PSTATE_CONST_TIME,			PSTATE_OUT_TIME,		REX_RIGHT_BR},

	{PSTATE_NA_INT,				PSTATE_NA_INT,			REX_NA_ANY},
	{PSTATE_NA_INT,				PSTATE_SEP_INT,			REX_COMMA},
	{PSTATE_NA_INT,				PSTATE_OUT_INT,			REX_RIGHT_BR},

	{PSTATE_NA_REAL,			PSTATE_NA_REAL,			REX_NA_ANY},
	{PSTATE_NA_REAL,			PSTATE_SEP_REAL,		REX_COMMA},
	{PSTATE_NA_REAL,			PSTATE_OUT_REAL,		REX_RIGHT_BR},

	{PSTATE_NA_STRING,			PSTATE_NA_STRING,		REX_NA_ANY},
	{PSTATE_NA_STRING,			PSTATE_SEP_STRING,		REX_COMMA},
	{PSTATE_NA_STRING,			PSTATE_OUT_STRING,		REX_RIGHT_BR},

	{PSTATE_NA_TIME,			PSTATE_NA_TIME,			REX_NA_ANY},
	{PSTATE_NA_TIME,			PSTATE_SEP_TIME,		REX_COMMA},
	{PSTATE_NA_TIME,			PSTATE_OUT_TIME,		REX_RIGHT_BR},

	{PSTATE_SEP_INT,			PSTATE_SEP_INT,			REX_ALL_SPACES},
	{PSTATE_SEP_INT,			PSTATE_CONST_INT,		REX_NUMBER_FIRST},
	{PSTATE_SEP_INT,			PSTATE_NA_INT,			REX_NA_FIRST},

	{PSTATE_SEP_REAL,			PSTATE_SEP_REAL,		REX_ALL_SPACES},
	{PSTATE_SEP_REAL,			PSTATE_CONST_REAL,		REX_NUMBER_FIRST},
	{PSTATE_SEP_REAL,			PSTATE_NA_REAL,			REX_NA_FIRST},

	{PSTATE_SEP_STRING,			PSTATE_SEP_STRING,		REX_ALL_SPACES},
	{PSTATE_SEP_STRING,			PSTATE_CONST_STRING0,	REX_DOUBLEQUOTE},
	{PSTATE_SEP_STRING,			PSTATE_NA_STRING,		REX_NA_FIRST},

	{PSTATE_SEP_TIME,			PSTATE_SEP_TIME,		REX_ALL_SPACES},
	{PSTATE_SEP_TIME,			PSTATE_CONST_TIME,		REX_INT_ANY},
	{PSTATE_SEP_TIME,			PSTATE_NA_TIME,			REX_NA_FIRST},

	{PSTATE_OUT_INT,			PSTATE_OUT_INT,			REX_ALL_SPACES_RIGHT_BR},
	{PSTATE_OUT_INT,			PSTATE_IN_INT,			REX_COMMA},

	{PSTATE_OUT_REAL,			PSTATE_OUT_REAL,		REX_ALL_SPACES_RIGHT_BR},
	{PSTATE_OUT_REAL,			PSTATE_IN_REAL,			REX_COMMA},

	{PSTATE_OUT_STRING,			PSTATE_OUT_STRING,		REX_ALL_SPACES_RIGHT_BR},
	{PSTATE_OUT_STRING,			PSTATE_IN_STRING,		REX_COMMA},

	{PSTATE_OUT_TIME,			PSTATE_OUT_TIME,		REX_ALL_SPACES_RIGHT_BR},
	{PSTATE_OUT_TIME,			PSTATE_IN_TIME,			REX_COMMA},

	{MAX_NUM_PSTATES}
};

ParseNextStateLUT parser_state_switch[MAX_NUM_PSTATES];

/*	--------------------------------------------------
	 Container : I m p l e m e n t a t i o n
--------------------------------------------------- */

Container::Container(pLogger a_logger, pConfigFile a_config) : Service(a_logger, a_config) {

	compile_next_state_LUT(parser_state_switch, MAX_NUM_PSTATES, state_tr);

	memcpy(&F_NA_uint32, &F_NA, sizeof(&F_NA));
	memcpy(&R_NA_uint64, &R_NA, sizeof(&R_NA));

	max_transactions = 0;
	alloc_bytes = warn_alloc_bytes = fail_alloc_bytes = 0;
	p_buffer = p_free = nullptr;
	_lock_ = 0;
}


Container::~Container() { destroy_container(); }


/** Reads variables from config and sets private variables accordingly.
*/
StatusCode Container::start() {

	if (!get_conf_key("ONE_SHOT_MAX_TRANSACTIONS", max_transactions)) {
		log(LOG_ERROR, "Config key ONE_SHOT_MAX_TRANSACTIONS not found in Container::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	int i = 0;

	if (!get_conf_key("ONE_SHOT_WARN_BLOCK_KBYTES", i)) {
		log(LOG_ERROR, "Config key ONE_SHOT_WARN_BLOCK_KBYTES not found in Container::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}
	warn_alloc_bytes = 1024; warn_alloc_bytes *= i;

	if (!get_conf_key("ONE_SHOT_ERROR_BLOCK_KBYTES", i)) {
		log(LOG_ERROR, "Config key ONE_SHOT_ERROR_BLOCK_KBYTES not found in Container::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}
	fail_alloc_bytes = 1024; fail_alloc_bytes *= i;

	return new_container();
}


/** Destroys everything and zeroes allocation.
*/
StatusCode Container::shut_down() {

	return destroy_container();
}


/** Enter (soft lock) a Block for reading. Many readers are allowed simultaneously, but it is incompatible with writing.

	\param p_txn		The address of the Block's Transaction.

NOTE: This is not used in API queries or normal Bop execution. In those cases, Blocks are immutable. This allows for mutable blocks
in multithreaded algorithms like MCTS.

*/
void Container::enter_read(pTransaction p_txn) {
	int retry = 0;

	while (true) {
		int32_t lock = p_txn->_lock_;

		if (lock >= 0) {
			int32_t next = lock + 1;

			if (p_txn->_lock_.compare_exchange_weak(lock, next))
				return;
		}

		if (++retry > LOCK_NUM_RETRIES_BEFORE_YIELD) {
			std::this_thread::yield();
			retry = 0;
		}
	}
}


/** Enter (hard lock) a Block for writing. No readers are allowed during writing.

	\param p_txn		The address of the Block's Transaction.

NOTE: This is not used in API queries or normal Bop execution. In those cases, Blocks are immutable. This allows for mutable blocks
in multithreaded algorithms like MCTS.

*/
void Container::enter_write(pTransaction p_txn) {
	int retry = 0;

	while (true) {
		int32_t lock = p_txn->_lock_;

		if (lock >= 0) {
			int32_t next = lock - LOCK_WEIGHT_OF_WRITE;

			if (p_txn->_lock_.compare_exchange_weak(lock, next)) {
				while (true) {
					if (p_txn->_lock_ == -LOCK_WEIGHT_OF_WRITE)
						return;
					if (++retry > LOCK_NUM_RETRIES_BEFORE_YIELD) {
						std::this_thread::yield();
						retry = 0;
					}
				}
			}
		}

		if (++retry > LOCK_NUM_RETRIES_BEFORE_YIELD) {
			std::this_thread::yield();
			retry = 0;
		}
	}
}


/** Release the soft lock of Block after reading. This is mandatory for each enter_read() call or it may result in permanent locking.

	\param p_txn		The address of the Block's Transaction.

NOTE: This is not used in API queries or normal Bop execution. In those cases, Blocks are immutable. This allows for mutable blocks
in multithreaded algorithms like MCTS.

*/
void Container::leave_read(pTransaction p_txn) {

	while (true) {
		int32_t lock = p_txn->_lock_;
		int32_t next = lock - 1;

		if (p_txn->_lock_.compare_exchange_weak(lock, next))
			return;
	}
}


/** Release the hard lock of Block after writing. This is mandatory for each enter_write() call or it may result in permanent locking.

	\param p_txn		The address of the Block's Transaction.

NOTE: This is not used in API queries or normal Bop execution. In those cases, Blocks are immutable. This allows for mutable blocks
in multithreaded algorithms like MCTS.

*/
void Container::leave_write(pTransaction p_txn) {

	while (true) {
		int32_t lock = p_txn->_lock_;
		int32_t next = lock + LOCK_WEIGHT_OF_WRITE;

		if (p_txn->_lock_.compare_exchange_weak(lock, next))
			return;
	}
}


/** Allocate a Transaction to share a block via the API.

	\param p_txn	A pointer to a valid Transaction passed by reference. On failure, it will assign nullptr to it.

	\return			SERVICE_NO_ERROR on success (and a valid p_txn), or some error.

NOTE: The idea of this method being virtual is allowing descendants to use new_block() calls to create one shot blocks that can be later
inserted into different structures.
*/
StatusCode Container::new_transaction(pTransaction &p_txn) {

	if (alloc_bytes > warn_alloc_bytes && !alloc_warning_issued) {
		log_printf(LOG_WARN, "Service Container exceeded RAM %0.2f Mb of %0.2f Mb",
				   (double) alloc_bytes/ONE_MB, (double) warn_alloc_bytes/ONE_MB);

		alloc_warning_issued = true;
	}

	lock_container();

	if (p_free == nullptr) {
		unlock_container();
		p_txn = nullptr;

		return SERVICE_ERROR_NO_MEM;
	}

	p_txn  = p_free;
	p_free = pStoredTransaction(p_free)->p_next;

	p_txn->p_block = nullptr;
	p_txn->status  = BLOCK_STATUS_EMPTY;
	p_txn->_lock_  = 0;
	p_txn->p_owner = this;

	unlock_container();

	return SERVICE_NO_ERROR;
}


/** Dealloc the Block in the p_tnx->p_block (if not null) and free the Transaction inside a Container.

	\param p_txn	A pointer to a valid Transaction passed by reference. Once finished, p_txn is set to nullptr to avoid reusing.

NOTE: The idea of this method being virtual is allowing descendants to use new_block() calls to create one shot blocks that can be later
inserted into different structures.
*/
void Container::destroy_transaction  (pTransaction &p_txn) {

	if (p_txn->p_owner == nullptr) {
		log_printf(LOG_ERROR, "Transaction %p has no p_owner", p_txn);

		return;
	}
	if (p_txn->p_owner != this) {
		p_txn->p_owner->destroy_transaction(p_txn);

		return;
	}

	enter_write(p_txn);

	if (p_txn->p_block != nullptr) {
		if (p_txn->p_block->cell_type == CELL_TYPE_INDEX) {
			p_txn->p_hea->index.~map();
			alloc_bytes -= sizeof(BlockHeader);
		} else
			alloc_bytes -= p_txn->p_block->total_bytes;

		free(p_txn->p_block);

		p_txn->p_block = nullptr;
	}

	lock_container();

	pStoredTransaction(p_txn)->p_next = (pStoredTransaction) p_free;

	p_txn->status = BLOCK_STATUS_DESTROYED;

	p_free = p_txn;
	p_txn  = nullptr;

	unlock_container();
}


/** Create a new Block (1): Create a Tensor from raw data specifying everything from scratch.

	\param p_txn			A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
							Transaction inside the Container. The caller can only use it read-only and **must** destroy_transaction()
							it when done.
	\param cell_type		The tensor cell type in [CELL_TYPE_BYTE..CELL_TYPE_DOUBLE]
	\param dim				This defines both the rank and the dimensions of the tensor. Note that, except for the first position a
							dimension of 0 and 1 is the same dim = {3, 1} is a vector of 3 elements with rank 1, exactly like {3, 0}.
							As a matter of convention, dim should always end with a 0 except when it is MAX_TENSOR_RANK long.
							For the first dimension 1 means one element and 0 means no element. Both have rank 1. The latter is the
							typical result of a selection where no row matches the condition. Blocks never have rank == 0 and zero-element
							blocks have the same rank as the block from which they were selected. When 0 rows are selected from a block
							of dim = {r, s, t} the resulting block is {0, s, t} with size == 0 and rank == 3.
							If dim == nullptr and p_text != nullptr, dim will be set to the number of lines (see eol) in p_text when
							cell_type == CELL_TYPE_STRING.
	\param fill_tensor		How to fill the tensor. The options are: FILL_NEW_DONT_FILL (don't do anything with the tensor),
							FILL_NEW_WITH_ZERO (fill with binary zero no matter what the cell_type is), FILL_NEW_WITH_NA fill with the
							appropriate NA for the cell_type).
	\param stringbuff_size	One of the possible ways to allocate space for strings is declaring this size. When this is non-zero a buffer
							will be allocated with this size plus whatever size is required by the strings in att. new_jazz_block() will
							only allocate the space and do nothing with it. The caller should assign strings with Block.set_string().
	\param p_text			The other possible way to allocate space for strings is by declaring p_text. Imagine the content of p_text
							as a text file with n = size rows that will be pushed into the tensor and the string buffer. The eol character
							separates the cells. (cell_type == CELL_TYPE_STRING & p_text != nullptr) overrides any setting in fill_tensor.
							Also, either dim should be nullptr and set automatically or its resulting size must be the same as the number
							of lines in p_text.
	\param eol				A single character that separates the cells in p_text and will not be pushed to the string buffer.
	\param att				The attributes to set when creating the block. They are immutable.

	NOTES: String buffer allocation should not be used to dynamically change attribute values. Attributes are immutable and should be
	changed	only creating a new block with new = new_jazz_block(p_from = old, att = new_att). String buffer allocation should only be
	used for cell_type == CELL_TYPE_STRING and either with stringbuff_size or with p_text (and eol).
	If stringbuff_size is used, Block.set_string() should be used afterwards. If p_text is used, the tensor is already filled and
	Block.set_string() **should not** be called after that.

	OWNERSHIP: Remember: the p_txn returned on success points inside the Container. Use it as read-only and don't forget to
	destroy_transaction() it when done.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).
*/
StatusCode Container::new_block(pTransaction &p_txn,
								int			  cell_type,
								int			 *dim,
								int			  fill_tensor,
								int			  stringbuff_size,
								const char	 *p_text,
								char		  eol,
								AttributeMap *att) {

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR)
		return ret;

	StaticBlockHeader hea;

	hea.cell_type = cell_type;

	int text_length = 0, num_lines = 0;

	if (p_text != nullptr) {
		if (cell_type != CELL_TYPE_STRING || fill_tensor != FILL_WITH_TEXTFILE) {
			destroy_transaction(p_txn);

			return SERVICE_ERROR_NEW_BLOCK_ARGS;
		}

		const char *pt = p_text;
		while (*pt) {
			if (*pt == eol) {
				if (!pt[1])
					break;

				num_lines++;
			}
			pt++;
		}
		text_length = (uintptr_t) pt - (uintptr_t) p_text - num_lines;
		num_lines++;
	}

	if (dim == nullptr) {
		if (p_text == nullptr) {
			destroy_transaction(p_txn);

			return SERVICE_ERROR_NEW_BLOCK_ARGS;
		}

		TensorDim i_dim;
		i_dim.dim[0] = num_lines;
		i_dim.dim[1] = 0;
#ifdef DEBUG				// Initialize i_dim for Valgrind.
		i_dim.dim[2] = 0;
		i_dim.dim[3] = 0;
		i_dim.dim[4] = 0;
		i_dim.dim[5] = 0;
#endif
		reinterpret_cast<pBlock>(&hea)->set_dimensions(i_dim.dim);
	} else {
		reinterpret_cast<pBlock>(&hea)->set_dimensions(dim);

		if (num_lines && (num_lines != hea.size)){
			destroy_transaction(p_txn);

			return SERVICE_ERROR_NEW_BLOCK_ARGS;
		}
	}

	hea.num_attributes = 0;

	hea.total_bytes = (uintptr_t) reinterpret_cast<pBlock>(&hea)->p_string_buffer() - (uintptr_t) (&hea) + sizeof(StringBuffer) + 4;

	if (att != nullptr && att->size() == 0)
		att = nullptr;

	if (att	!= nullptr) {
		for (AttributeMap::iterator it = att->begin(); it != att->end(); ++it) {
			int len = it->second == nullptr ? 0 : strlen(it->second);

			if (len)
				hea.total_bytes += len + 1;

			hea.num_attributes++;
		}
		hea.total_bytes += 2*hea.num_attributes*sizeof(int);
	}

	hea.total_bytes += stringbuff_size + text_length + num_lines;

	p_txn->p_block = block_malloc(hea.total_bytes);

	if (p_txn->p_block == nullptr) {
		destroy_transaction(p_txn);

		return SERVICE_ERROR_NO_MEM;
	}

#ifdef DEBUG		// Initialize everything for Valgrind.
	memset(p_txn->p_block, 0, hea.total_bytes);
#endif

	memcpy(p_txn->p_block, &hea, sizeof(BlockHeader));

	p_txn->p_block->num_attributes = 0;
	p_txn->p_block->set_attributes(att);

#ifdef DEBUG	// Initialize the RAM between the end of the tensor and the base of the attribute key vector for Valgrind.
	{
		char *pt1 = reinterpret_cast<char *>(&p_txn->p_block->tensor + (p_txn->p_block->cell_type & 0xf)*p_txn->p_block->size),
			 *pt2 = reinterpret_cast<char *>(p_txn->p_block->align64bit((uintptr_t) pt1));

		while (pt1 < pt2)
			*(pt1++) = 0;

		if (cell_type == CELL_TYPE_STRING && fill_tensor == FILL_NEW_DONT_FILL) {
			pStringBuffer psb = p_txn->p_block->p_string_buffer();

			pt1 = &psb->buffer[psb->last_idx];
			pt2 = (char *) ((uintptr_t) p_txn->p_block + p_txn->p_block->total_bytes);

			while (pt1 < pt2)
				*(pt1++) = 0;
		}
	}
#endif

	if (p_text != nullptr) {
		p_txn->p_block->has_NA = false;
		pStringBuffer psb = p_txn->p_block->p_string_buffer();

		int offset = psb->last_idx;

		char *pt_out = &psb->buffer[offset];

		int row = 1, len = 0;
		const char *pt_in = p_text;

		p_txn->p_block->tensor.cell_int[0] = offset;

		while (*pt_in) {
			offset++;
			if (*pt_in != eol) {
				*pt_out = *pt_in;
				len++;
			} else {
				if (!len)
					p_txn->p_block->tensor.cell_int[row - 1] = STRING_EMPTY;

				if (!pt_in[1])
					break;

				*pt_out = 0;

				p_txn->p_block->tensor.cell_int[row] = offset;

				len = 0;
				row++;
			}
			pt_out++;
			pt_in++;
		}
		if (!len)
			p_txn->p_block->tensor.cell_int[row - 1] = STRING_EMPTY;

		*pt_out = 0;

		psb->last_idx			= offset + (*pt_in == 0);
		psb->stop_check_4_match = true;							// Block::get_string_offset() does not support match with empty strings.
	} else {
		switch (fill_tensor) {
		case FILL_NEW_DONT_FILL:
			p_txn->p_block->has_NA = p_txn->p_block->cell_type != CELL_TYPE_BYTE;
			break;

		case FILL_NEW_WITH_ZERO:
			memset(&p_txn->p_block->tensor, 0, (p_txn->p_block->cell_type & 0xf)*p_txn->p_block->size);
			p_txn->p_block->has_NA =	  (p_txn->p_block->cell_type == CELL_TYPE_STRING)
									   || (p_txn->p_block->cell_type == CELL_TYPE_TIME);
			break;

		case FILL_NEW_WITH_NA:
			p_txn->p_block->has_NA = true;

			switch (cell_type) {
			case CELL_TYPE_BYTE_BOOLEAN:
				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_byte[i] = BOOLEAN_NA;
				break;

			case CELL_TYPE_INTEGER:
			case CELL_TYPE_FACTOR:
			case CELL_TYPE_GRADE:
				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_int[i] = INTEGER_NA;
				break;

			case CELL_TYPE_BOOLEAN:
				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_uint[i] = BOOLEAN_NA;
				break;

			case CELL_TYPE_SINGLE: {
				u_int una = reinterpret_cast<u_int*>(&SINGLE_NA)[0];

				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_uint[i] = una;
				break; }

			case CELL_TYPE_STRING:
				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_int[i] = STRING_NA;
				break;

			case CELL_TYPE_LONG_INTEGER:
				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_longint[i] = LONG_INTEGER_NA;
				break;

			case CELL_TYPE_TIME:
				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_longint[i] = TIME_POINT_NA;
				break;

			case CELL_TYPE_DOUBLE: {
				uint64_t una = reinterpret_cast<uint64_t*>(&DOUBLE_NA)[0];

				for (int i = 0; i < p_txn->p_block->size; i++) p_txn->p_block->tensor.cell_ulongint[i] = una;
				break; }

			default:
				destroy_transaction(p_txn);

				return SERVICE_ERROR_NEW_BLOCK_ARGS;		// No silent fail, JAZZ_FILL_NEW_WITH_NA is undefined for the type
			}
			break;

		default:
			destroy_transaction(p_txn);

			return SERVICE_ERROR_NEW_BLOCK_ARGS;			// No silent fail, fill_tensor is invalid
		}
	}

	p_txn->p_block->hash64 = 0;
	p_txn->status = BLOCK_STATUS_READY;

	return SERVICE_NO_ERROR;
}


/** Create a new Block (2): Create a Kind or Tuple from arrays of StaticBlockHeader, names, and, in the case of a tuple, Tensors.

	\param p_txn		A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						Transaction inside the Container. The caller can only use it read-only and **must** destroy_transaction()
						it when done.
	\param num_items	The number of items the Kind or Tuple will have.
	\param p_hea		A vector of num_items pointers to StaticBlockHeaders defining the Kind or Tuple. The shape must be defined in
						"human-readable" format, i.e., what a pBlock->get_dimensions() returns (not the internal way a block stores it).
						When creating a Kind, negative constants must be defined in dims-> and will be used to create dimensions.
	\param p_names		An array of num_items Name structures by which the items will go.
	\param p_block		The data, only for tuples. It must have the same shape as p_hea but that will not be checked. Unlike p_hea
						this has the shape stores as a real block (but it is not used) instead of "human-readable". If it is nullptr,
						a Kind will be created, otherwise a Tuple will be created and just the tensor data will be copied from here.
	\param dims			For Kinds only, the names of the dimensions. Note that p_hea must have negative values for the dimensions, just
						like when Kinds are built using Kind.new_kind() followed by Kind.add_item()
	\param att			The attributes to set when creating the block. They are immutable. To change the attributes of a Block
						use the version of new_jazz_block() with parameter p_from.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).
*/
StatusCode Container::new_block(pTransaction	   &p_txn,
								int					num_items,
								StaticBlockHeader	p_hea[],
								Name				p_names[],
								pBlock				p_block[],
								AttributeMap	   *dims,
								AttributeMap	   *att) {

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR)
		return ret;

	StaticBlockHeader hea;
	TensorDim i_dim;
	i_dim.dim[0] = num_items;
	i_dim.dim[1] = 0;
#ifdef DEBUG				// Initialize i_dim for Valgrind.
	i_dim.dim[2] = 0;
	i_dim.dim[3] = 0;
	i_dim.dim[4] = 0;
	i_dim.dim[5] = 0;
#endif

	if (p_block == nullptr)
		hea.cell_type = CELL_TYPE_KIND_ITEM;
	else
		hea.cell_type = CELL_TYPE_TUPLE_ITEM;

	reinterpret_cast<pBlock>(&hea)->set_dimensions(i_dim.dim);

	hea.num_attributes = 0;

	hea.total_bytes = (uintptr_t) reinterpret_cast<pBlock>(&hea)->p_string_buffer() - (uintptr_t) (&hea) + sizeof(StringBuffer) + 4;

	if (att != nullptr && att->size() == 0)
		att = nullptr;

	if (att	!= nullptr) {
		for (AttributeMap::iterator it = att->begin(); it != att->end(); ++it) {
			int len = it->second == nullptr ? 0 : strlen(it->second);

			if (len)
				hea.total_bytes += len + 1;

			hea.num_attributes++;
		}
		hea.total_bytes += 2*hea.num_attributes*sizeof(int);
	}

	for (int i = 0; i < num_items; i++) {
		pChar p_name = (pChar) &p_names[i];

		hea.total_bytes += strlen(p_name) + 1;

		if (p_block != nullptr)
			hea.total_bytes += p_block[i]->total_bytes + 15;	// 15 == worst case of align 128-bit
	}

	if (dims != nullptr && p_block == nullptr) {
		for (AttributeMap::iterator it = dims->begin(); it != dims->end(); ++it) {
			int len = it->second == nullptr ? 0 : strlen(it->second);

			hea.total_bytes += len + 1;
		}
	}

	p_txn->p_block = block_malloc(hea.total_bytes);

	if (p_txn->p_block == nullptr) {
		destroy_transaction(p_txn);

		return SERVICE_ERROR_NO_MEM;
	}

#ifdef DEBUG		// Initialize everything for Valgrind.
	memset(p_txn->p_block, 0, hea.total_bytes);
#endif

	if (p_block == nullptr) {
		if (reinterpret_cast<pKind>(p_txn->p_block)->new_kind(num_items, hea.total_bytes, att) != SERVICE_NO_ERROR) {
			destroy_transaction(p_txn);

			return SERVICE_ERROR_BAD_NEW_KIND;
		}

		for (int i = 0; i < num_items; i++) {
			pChar p_name = (pChar) &p_names[i];

			if (!reinterpret_cast<pKind>(p_txn->p_block)->add_item(i, p_name, p_hea[i].range.dim, p_hea[i].cell_type, dims)) {
				destroy_transaction(p_txn);

				return SERVICE_ERROR_BAD_KIND_ADD;
			}
		}
		p_txn->p_block->hash64 = 0;
		p_txn->status = BLOCK_STATUS_READY;

		return SERVICE_NO_ERROR;
	}

	ret = reinterpret_cast<pTuple>(p_txn->p_block)->new_tuple(num_items, p_block, p_names, hea.total_bytes, att);

	if (ret == SERVICE_NO_ERROR) {
		p_txn->p_block->hash64 = 0;
		p_txn->status = BLOCK_STATUS_READY;
	} else
		destroy_transaction(p_txn);

	return ret;
}


/** Create a new Block (3): Create a Tensor by selecting rows (filtering) from another Tensor.

	\param p_txn		A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						Transaction inside the Container. The caller can only use it read-only and **must** destroy_transaction()
						it when done.
	\param p_from		The block we want to filter from. The resulting block will be a subset of the rows (selection on the first
						dimension of the tensor). This can be either a tensor or a Tuple. In the case of a Tuple, all the tensors must
						have the same first dimension.
	\param p_row_filter	The block we want to use as a filter. This is either a tensor of boolean or integer that can_filter(p_from).
	\param att			The attributes to set when creating the block. They are immutable. To change the attributes of a Block
						use the version of new_jazz_block() with parameter p_from.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).
*/
StatusCode Container::new_block(pTransaction &p_txn,
								pBlock		  p_from,
						   		pBlock		  p_row_filter,
								AttributeMap *att) {

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR)
		return ret;

	if (p_from == nullptr || p_from->size < 0 || p_from->range.dim[0] < 1) {
		destroy_transaction(p_txn);

		return SERVICE_ERROR_NEW_BLOCK_ARGS;
	}

	int tensor_diff		= 0,
		old_tensor_size = p_from->size*(p_from->cell_type & 0xff),
		tensor_rows,
		bytes_per_row,
		selected_rows;

	if (p_row_filter != nullptr) {
		if (!p_row_filter->can_filter(p_from)) {
			destroy_transaction(p_txn);

			return SERVICE_ERROR_NEW_BLOCK_ARGS;
		}
		if (p_row_filter->cell_type == CELL_TYPE_BYTE_BOOLEAN) {
			selected_rows = 0;
			for (int i = 0; i < p_row_filter->size; i++)
				if (p_row_filter->tensor.cell_bool[i])
					selected_rows++;
		} else {
			selected_rows = p_row_filter->size;
		}
		if (p_from->size) {
			tensor_rows = p_from->size/p_from->range.dim[0];

			bytes_per_row		= old_tensor_size/tensor_rows;
			int new_tensor_size = selected_rows*bytes_per_row;

			old_tensor_size = (uintptr_t) p_from->align64bit(old_tensor_size);
			new_tensor_size = (uintptr_t) p_from->align64bit(new_tensor_size);

			tensor_diff = new_tensor_size - old_tensor_size;

			if (tensor_diff == 0 && !p_row_filter->is_a_filter()) {		// For consistency, check indices
				destroy_transaction(p_txn);

				return SERVICE_ERROR_NEW_BLOCK_ARGS;
			}
		}
	}

	int attrib_diff		   = 0,
		new_num_attributes = 0;

	if (att != nullptr && att->size() == 0)
		att = nullptr;

	if (att	!= nullptr) {
		int new_attrib_bytes = 0;

		for (AttributeMap::iterator it = att->begin(); it != att->end(); ++it) {
			int len = it->second == nullptr ? 0 : strlen(it->second);

			if (len)
				new_attrib_bytes += len + 1;

			new_num_attributes++;
		}

		if (!new_num_attributes) {
			destroy_transaction(p_txn);

			return SERVICE_ERROR_NEW_BLOCK_ARGS;
		}

		new_attrib_bytes += new_num_attributes*2*sizeof(int);

		int old_attrib_bytes = p_from->num_attributes*2*sizeof(int);

		if (p_from->cell_type != CELL_TYPE_STRING) {
			pStringBuffer psb = p_from->p_string_buffer();
			old_attrib_bytes += std::max(0, psb->buffer_size - 4);
		}

		attrib_diff = new_attrib_bytes - old_attrib_bytes;
	}

	int total_bytes = p_from->total_bytes + tensor_diff + attrib_diff;

	p_txn->p_block = block_malloc(total_bytes);

	if (p_txn->p_block == nullptr) {
		destroy_transaction(p_txn);

		return SERVICE_ERROR_NO_MEM;
	}

#ifdef DEBUG		// Initialize everything for Valgrind.
	memset(p_txn->p_block, 0, total_bytes);
#endif

	memcpy(p_txn->p_block, p_from, sizeof(BlockHeader));

	p_txn->p_block->total_bytes = total_bytes;

	if (tensor_diff) {
		p_txn->p_block->size = selected_rows*p_from->range.dim[0];

		u_char *p_dest = &p_txn->p_block->tensor.cell_byte[0],
			   *p_src  = &p_from->tensor.cell_byte[0];

		if (p_row_filter->cell_type == CELL_TYPE_BYTE_BOOLEAN) {
			for (int i = 0; i < p_row_filter->size; i++) {
				if (p_row_filter->tensor.cell_bool[i]) {
					memcpy(p_dest, p_src, bytes_per_row);
					p_dest = p_dest + bytes_per_row;
				}
				p_src = p_src + bytes_per_row;
			}
		} else {
			int j2 = -1;
			for (int i = 0; i < p_row_filter->size; i++) {
				int j = p_row_filter->tensor.cell_int[i];
				if (j <= j2 || j >= tensor_rows) {
					destroy_transaction(p_txn);

					return SERVICE_ERROR_NEW_BLOCK_ARGS;
				}
				memcpy(p_dest, p_src + j*bytes_per_row, bytes_per_row);
				p_dest = p_dest + bytes_per_row;
				j2 = j;
			}
		}
	} else {
		memcpy(&p_txn->p_block->tensor, &p_from->tensor, old_tensor_size);
	}

	if (att	!= nullptr)	{
		if (p_from->cell_type != CELL_TYPE_STRING) {
			p_txn->p_block->num_attributes = 0;
			p_txn->p_block->set_attributes(att);
		} else {
			p_txn->p_block->num_attributes = new_num_attributes;

			pStringBuffer p_nsb = p_txn->p_block->p_string_buffer(), p_osb = p_from->p_string_buffer();

			p_txn->p_block->init_string_buffer();

			memcpy(&p_nsb->buffer, &p_osb->buffer, p_osb->buffer_size);

			p_nsb->last_idx = p_osb->last_idx;
			p_nsb->stop_check_4_match = p_osb->stop_check_4_match;

			int i = 0;
			int *ptk = p_txn->p_block->p_attribute_keys();

			for (AttributeMap::iterator it = att->begin(); it != att->end(); ++it) {
				ptk[i] = it->first;
				ptk[i + new_num_attributes] = p_txn->p_block->get_string_offset(p_nsb, it->second);

				i++;
			}
		}
	} else {
		memcpy(p_txn->p_block->p_attribute_keys(), p_from->p_attribute_keys(), p_from->num_attributes*2*sizeof(int));

		pStringBuffer p_nsb = p_txn->p_block->p_string_buffer(), p_osb = p_from->p_string_buffer();

		memcpy(p_nsb, p_osb, p_osb->buffer_size + sizeof(StringBuffer));
	}

	p_txn->p_block->hash64 = 0;

	p_txn->status = BLOCK_STATUS_READY;

	return SERVICE_NO_ERROR;
}


/** Create a new Block (4): Create a Tensor by selecting an item from a Tuple.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container. The caller can only use it read-only and **must** destroy_transaction()
					it when done.
	\param p_from	The Tuple from which the item is selected.
	\param name		The name of the item to be selected.
	\param att		The attributes to set when creating the block. They are immutable. To change the attributes of a Block
					use the version of new_jazz_block() with parameter p_from.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

The Tuple already has a method .block() that does this, the difference is the .block() method returns a pointer inside the Tuple
while this makes a copy to a new block, possibly with attributes.
*/
StatusCode Container::new_block(pTransaction &p_txn,
								pTuple		  p_from,
						   		pChar		  name,
								AttributeMap *att) {

	if (p_from->cell_type != CELL_TYPE_TUPLE_ITEM) {
		p_txn = nullptr;

		return SERVICE_ERROR_WRONG_TYPE;
	}

	int idx = p_from->index(name);

	if (idx < 0) {
		p_txn = nullptr;

		return SERVICE_ERROR_WRONG_NAME;
	}

	pBlock p_block = p_from->get_block(idx);

	return new_block(p_txn, p_block, (pBlock) nullptr, att);	// This manages the alloc issues of att and possible strings already.
}


/** Create a new Block (5): Create a Tensor, Kind or Tuple from a Text block kept as a Tensor of CELL_TYPE_BYTE of rank == 1 or a
							Tensor of CELL_TYPE_STRING of size == 1

	\param p_txn		A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						Transaction inside the Container. The caller can only use it read-only and **must** destroy_transaction()
						it when done.
	\param p_from_text	The block containing a valid serialization of Tensor, Kind or Tuple.
	\param cell_type	The expected output type.
	\param p_as_kind	For Tuples only, the definition of the exact types of each item. The serialization contains names, but not types
						to make it JSON-like. E.g., "("temperature":[[20,21],[22,22]], "sensation":["cold", "mild"])" This can be skipped
						by passing a nullptr. In that case, the items will be guessed only as CELL_TYPE_INTEGER, CELL_TYPE_DOUBLE or
						CELL_TYPE_STRING. The serialization of a Kind includes types and does not use this.
						E.g. "{"temperature":INTEGER[num_places,2], "sensation":STRING[num_places]}"
	\param att			The attributes to set when creating the block. They are immutable. To change the attributes of a Block
						use the version of new_jazz_block() with parameter p_from.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).
*/
StatusCode Container::new_block(pTransaction &p_txn,
								pBlock		  p_from_text,
						   		int			  cell_type,
								pKind		  p_as_kind,
								AttributeMap *att) {

	pChar p_source;
	int	  source_l;
	switch (p_from_text->cell_type) {
	case CELL_TYPE_BYTE:
		if (p_from_text->rank != 1)
			return SERVICE_ERROR_NEW_BLOCK_ARGS;

		p_source = (pChar) &p_from_text->tensor.cell_byte[0];
		source_l = p_from_text->size;

		break;

	case CELL_TYPE_STRING:
		if (p_from_text->size != 1)
			return SERVICE_ERROR_NEW_BLOCK_ARGS;

		p_source = p_from_text->get_string(0);
		source_l = strlen(p_source);

		break;

	default:
		return SERVICE_ERROR_NEW_BLOCK_ARGS;
	}

	ItemHeader item_hea[MAX_ITEMS_IN_KIND];
	Name item_name[MAX_ITEMS_IN_KIND];
	int num_bytes = source_l;
	int num_items;
	pChar p_in = p_source;

	p_txn = nullptr;

	if (skip_space(p_in, num_bytes) <= 0)
		return PARSE_ERROR_UNEXPECTED_EOF;

	if (cell_type == CELL_TYPE_UNDEFINED) {
		if (*p_in == '(')
			cell_type = CELL_TYPE_TUPLE_ITEM;

		if (*p_in == '{')
			cell_type = CELL_TYPE_KIND_ITEM;
	}

	switch (cell_type) {
	case CELL_TYPE_TUPLE_ITEM: {
		if (get_char(p_in, num_bytes) != '(')
			return PARSE_ERROR_UNEXPECTED_CHAR;

		int item_idx = 0;
		while (true) {
			if (!get_item_name(p_in, num_bytes, item_name[item_idx]))
				return PARSE_ERROR_ITEM_NAME;

			int item_type;
			if (p_as_kind != nullptr) {
				if (item_idx >= p_as_kind->size)
					return PARSE_ERROR_TOO_MANY_ITEMS;

				if (strcmp(p_as_kind->item_name(item_idx), item_name[item_idx]) != 0)
					return PARSE_ERROR_ITEM_NAME_MISMATCH;

				item_type = p_as_kind->tensor.cell_item[item_idx].cell_type;
			} else
				item_type = CELL_TYPE_UNDEFINED;

			if (!get_shape_and_size(p_in, num_bytes, item_type, &item_hea[item_idx]))
				return PARSE_ERROR_TENSOR_EXPLORATION;

			if (skip_space(p_in, num_bytes) <= 0)
				return PARSE_ERROR_UNEXPECTED_EOF;

			item_idx++;

			if (item_idx >= MAX_ITEMS_IN_KIND)
				return PARSE_ERROR_TOO_MANY_ITEMS;

			char cl = get_char(p_in, num_bytes);

			if (cl == ')')
				break;

			if (cl != ',')
				return PARSE_ERROR_UNEXPECTED_CHAR;
		}

		num_items = item_idx;

		if (skip_space(p_in, num_bytes) != 0)
			return PARSE_ERROR_EXPECTED_EOF;

		break;
	}
	case CELL_TYPE_KIND_ITEM: {
		MapSI idx_dims = {};

		if (get_char(p_in, num_bytes) != '{')
			return PARSE_ERROR_UNEXPECTED_CHAR;

		int item_idx = 0;
		while (true) {
			if (!get_item_name(p_in, num_bytes, item_name[item_idx]))
				return PARSE_ERROR_ITEM_NAME;

			if (!get_type_and_shape(p_in, num_bytes, &item_hea[item_idx], idx_dims))
				return PARSE_ERROR_KIND_EXPLORATION;

			if (skip_space(p_in, num_bytes) <= 0)
				return PARSE_ERROR_UNEXPECTED_EOF;

			item_idx++;

			if (item_idx >= MAX_ITEMS_IN_KIND)
				return PARSE_ERROR_TOO_MANY_ITEMS;

			char cl = get_char(p_in, num_bytes);

			if (cl == '}')
				break;

			if (cl != ',')
				return PARSE_ERROR_UNEXPECTED_CHAR;
		}

		num_items = item_idx;

		if (skip_space(p_in, num_bytes) != 0)
			return PARSE_ERROR_EXPECTED_EOF;

		break;
	}
	default:
		if (!get_shape_and_size(p_in, num_bytes, cell_type, &item_hea[0]))
			return PARSE_ERROR_TENSOR_EXPLORATION;

		if (skip_space(p_in, num_bytes) != 0)
			return PARSE_ERROR_EXPECTED_EOF;

		if (cell_type == CELL_TYPE_UNDEFINED)
			cell_type = item_hea[0].cell_type;
	}

	num_bytes = source_l;
	p_in	  = p_source;

	skip_space(p_in, num_bytes);

	switch (cell_type) {
	case CELL_TYPE_TUPLE_ITEM: {
		pTransaction p_aux_txn[MAX_ITEMS_IN_KIND];

		get_char(p_in, num_bytes);		// '(')

		for (int i = 0; i < num_items; i++) {
			Name it_name;
			get_item_name(p_in, num_bytes, it_name);

			if (item_hea[i].cell_type == CELL_TYPE_STRING) {
				StatusCode ret = new_text_block(p_aux_txn[i], item_hea[i], p_in, num_bytes, att);

				if (ret != SERVICE_NO_ERROR) {
					for (int j = i - 1; j >= 0; j--)
						destroy_transaction(p_aux_txn[j]);

					return ret;
				}
			} else {
				int ret = new_block(p_aux_txn[i], item_hea[i].cell_type, item_hea[i].dim, FILL_NEW_DONT_FILL);

				if (ret != SERVICE_NO_ERROR) {
					for (int j = i - 1; j >= 0; j--)
						destroy_transaction(p_aux_txn[j]);

					return ret;
				}

				if (!fill_tensor(p_in, num_bytes, p_aux_txn[i]->p_block)) {
					for (int j = i; j >= 0; j--)
						destroy_transaction(p_aux_txn[j]);

					return PARSE_ERROR_TENSOR_FILLING;
				}
			}
			skip_space(p_in, num_bytes);

			if (*p_in != ')')
				get_char(p_in, num_bytes);
		}

		pBlock			  p_item_blck[MAX_ITEMS_IN_KIND];
		StaticBlockHeader p_hea		 [MAX_ITEMS_IN_KIND];

		for (int i = 0; i < num_items; i++) {
			p_item_blck[i] = p_aux_txn[i]->p_block;
			memcpy(&p_hea[i], p_item_blck[i], sizeof(StaticBlockHeader));
			p_item_blck[i]->get_dimensions(&p_hea[i].range.dim[0]);
		}

		int ret = new_block(p_txn, num_items, p_hea, item_name, p_item_blck, nullptr, att);

		for (int i = 0; i < num_items; i++)
			destroy_transaction(p_aux_txn[i]);

		return ret;
	}
	case CELL_TYPE_KIND_ITEM: {
		MapSI idx_dims = {};

		get_char(p_in, num_bytes);		// '{')

		for (int i = 0; i < num_items; i++) {
			Name it_name;
			get_item_name(p_in, num_bytes, it_name);

			get_type_and_shape(p_in, num_bytes, &item_hea[i], idx_dims);

			skip_space(p_in, num_bytes);

			if (*p_in != '}')
				get_char(p_in, num_bytes);
		}

		StaticBlockHeader hea [MAX_ITEMS_IN_KIND];

		for (int i = 0; i < num_items; i++) {
			hea[i].cell_type = item_hea[i].cell_type;
			hea[i].rank		 = item_hea[i].rank;

			memcpy(&hea[i].range, &item_hea[i].dim, sizeof(TensorDim));
		}

		if (idx_dims.size() != 0) {
			AttributeMap dims = {};

			MapSI::iterator it;

			for (it = idx_dims.begin(); it != idx_dims.end(); ++it)
				dims[it->second] = it->first.c_str();

			return new_block(p_txn, num_items, hea, item_name, nullptr, &dims, att);
		} else
			return new_block(p_txn, num_items, hea, item_name, nullptr, nullptr, att);
	}
	case CELL_TYPE_STRING:
		return new_text_block(p_txn, item_hea[0], p_in, num_bytes, att);
	}

	int ret = new_block(p_txn, cell_type, item_hea[0].dim, FILL_NEW_DONT_FILL, 0, nullptr, '\n', att);

	if (ret != SERVICE_NO_ERROR)
		return ret;

	if (!fill_tensor(p_in, num_bytes, p_txn->p_block)) {
		destroy_transaction(p_txn);

		return PARSE_ERROR_TENSOR_FILLING;
	}

	return SERVICE_NO_ERROR;
}


/** Create a new Block (6): Create a Tensor of CELL_TYPE_BYTE of rank == 1 with a text serialization of a Tensor, Kind or Tuple.

	\param p_txn		 A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						 Transaction inside the Container. The caller can only use it read-only and **must** destroy_transaction()
						 it when done.
	\param p_from_raw	 The block to be serialized
	\param p_fmt		 An optional numerical precision format specifier. In the case of Tuples, if this is used all items with numerical
						 tensors will use it. (This may imply converting integer to double depending on the specifier.)
	\param ret_as_string Return the serialization as a tensor of just one string rather than a vector of byte.
	\param att			 The attributes to set when creating the block. They are immutable. To change the attributes of a Block
						 use the version of new_jazz_block() with parameter p_from.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).
*/
StatusCode Container::new_block(pTransaction &p_txn,
								pBlock		  p_from_raw,
						   		pChar		  p_fmt,
								bool		  ret_as_string,
								AttributeMap *att) {

	int item_len[MAX_ITEMS_IN_KIND];
	int total_bytes;

	p_txn = nullptr;

	switch (p_from_raw->cell_type) {
	case CELL_TYPE_BYTE:
	case CELL_TYPE_INTEGER:
	case CELL_TYPE_FACTOR:
	case CELL_TYPE_GRADE:
	case CELL_TYPE_LONG_INTEGER:
		total_bytes = tensor_int_as_text(p_from_raw, nullptr, p_fmt);

		if (total_bytes == 0)
			return SERVICE_ERROR_BAD_BLOCK;

		break;

	case CELL_TYPE_BYTE_BOOLEAN:
	case CELL_TYPE_BOOLEAN:
		total_bytes = tensor_bool_as_text(p_from_raw, nullptr);

		if (total_bytes == 0)
			return SERVICE_ERROR_BAD_BLOCK;

		break;

	case CELL_TYPE_SINGLE:
	case CELL_TYPE_DOUBLE:
		total_bytes = tensor_float_as_text(p_from_raw, nullptr, p_fmt);

		if (total_bytes == 0)
			return SERVICE_ERROR_BAD_BLOCK;

		break;

	case CELL_TYPE_STRING:
		total_bytes = tensor_string_as_text(p_from_raw, nullptr);

		if (total_bytes == 0)
			return SERVICE_ERROR_BAD_BLOCK;

		break;

	case CELL_TYPE_TIME:
		total_bytes = tensor_time_as_text(p_from_raw, nullptr, p_fmt);

		if (total_bytes == 0)
			return SERVICE_ERROR_BAD_BLOCK;

		break;

	case CELL_TYPE_TUPLE_ITEM:
		total_bytes = tensor_tuple_as_text((pTuple) p_from_raw, nullptr, p_fmt, item_len);

		if (total_bytes == 0)
			return SERVICE_ERROR_BAD_BLOCK;

		break;

	case CELL_TYPE_KIND_ITEM:
		total_bytes = tensor_kind_as_text((pKind) p_from_raw, nullptr);

		if (total_bytes == 0)
			return SERVICE_ERROR_BAD_BLOCK;

		break;

	default:
		return SERVICE_ERROR_WRONG_TYPE;
	}

	int dim[MAX_TENSOR_RANK];

	dim[0] = total_bytes;
	dim[1] = 0;
#ifdef DEBUG				// Initialize i_dim for Valgrind.
	dim[2] = 0;
	dim[3] = 0;
	dim[4] = 0;
	dim[5] = 0;
#endif

	StatusCode ret = new_block(p_txn, CELL_TYPE_BYTE, dim, FILL_NEW_DONT_FILL, 0, nullptr, 0, att);

	if (ret != SERVICE_NO_ERROR)
		return ret;

	switch (p_from_raw->cell_type) {
	case CELL_TYPE_BYTE:
	case CELL_TYPE_INTEGER:
	case CELL_TYPE_FACTOR:
	case CELL_TYPE_GRADE:
	case CELL_TYPE_LONG_INTEGER:
		tensor_int_as_text(p_from_raw, (pChar) &p_txn->p_block->tensor, p_fmt);

		break;

	case CELL_TYPE_BYTE_BOOLEAN:
	case CELL_TYPE_BOOLEAN:
		tensor_bool_as_text(p_from_raw, (pChar) &p_txn->p_block->tensor);

		break;

	case CELL_TYPE_SINGLE:
	case CELL_TYPE_DOUBLE:
		tensor_float_as_text(p_from_raw, (pChar) &p_txn->p_block->tensor, p_fmt);

		break;

	case CELL_TYPE_STRING:
		tensor_string_as_text(p_from_raw, (pChar) &p_txn->p_block->tensor);

		break;

	case CELL_TYPE_TIME:
		tensor_time_as_text(p_from_raw, (pChar) &p_txn->p_block->tensor, p_fmt);

		break;

	case CELL_TYPE_TUPLE_ITEM:
		tensor_tuple_as_text((pTuple) p_from_raw, (pChar) &p_txn->p_block->tensor, p_fmt, item_len);

		break;

	default:
		tensor_kind_as_text((pKind) p_from_raw, (pChar) &p_txn->p_block->tensor);

	}

	if (ret_as_string) {
		pTransaction p_aux;

		if (new_block(p_aux, CELL_TYPE_STRING, nullptr, FILL_WITH_TEXTFILE, 0, (pChar) &p_txn->p_block->tensor, 0) != SERVICE_NO_ERROR) {
			destroy_transaction(p_txn);

			return SERVICE_ERROR_NO_MEM;
		}
		std::swap(p_txn->p_block, p_aux->p_block);

		destroy_transaction(p_aux);
	}

	return SERVICE_NO_ERROR;
}


/** Create a new Block (7): Create an empty Index block.

	\param p_txn		A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						Transaction inside the Container. The caller can only use it read-only and **must** destroy_transaction()
						it when done.
	\param cell_type	The type of index (from CELL_TYPE_INDEX_II to CELL_TYPE_INDEX_SS)

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Unlike all the other blocks, Tensor, Kind and Tuple, this returns a header with an std::map. Therefore, it is dynamically allocated,
by just using it. As such, it is not movable and cannot be used in any transactions other that Channels serializing it as a
Tuple(key, value). When no longer needed, it has to be destroy_transaction()-ed just like the other Blocks created with new_block() and
the Container will take care of freeing the std::map before destroying the transaction.

*/
StatusCode Container::new_block(pTransaction &p_txn, int cell_type) {

	if ((cell_type & 0xff) != CELL_TYPE_INDEX){
		p_txn = nullptr;

		return SERVICE_ERROR_WRONG_TYPE;
	}

	StatusCode ret = new_transaction(p_txn);

	if (ret != SERVICE_NO_ERROR)
		return ret;

	p_txn->p_block = block_malloc(sizeof(BlockHeader));

	if (p_txn->p_block == nullptr) {
		destroy_transaction(p_txn);

		return SERVICE_ERROR_NO_MEM;
	}

	memset(p_txn->p_hea, 0, sizeof(BlockHeader));	// Zeroing an std::map is not enough. Void constructor is still required.

	p_txn->p_hea->cell_type = cell_type;
	p_txn->p_hea->size	    = 1;

	p_txn->p_hea->index = {};

	p_txn->p_block->hash64 = 0;

	p_txn->status = BLOCK_STATUS_READY;

	return SERVICE_NO_ERROR;
}


/** Create a new Block (8): Create a Tuple of (key:STRING[length],value:STRING[length]) with the content of an Index.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container. The caller can only use it read-only and **must** destroy_transaction()
					it when done.
	\param index	An Index we want to convert into a Tuple.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

*/
StatusCode Container::new_block(pTransaction &p_txn, Index &index) {

	int num_rows = index.size();
	int bytes_key = 0, bytes_val = 0;

	for (Index::iterator it = index.begin(); it != index.end(); ++it) {
		bytes_key += it->first.length();
		bytes_val += it->second.length();
	}

	pTransaction p_key, p_val;

	int dim[MAX_TENSOR_RANK] = {num_rows, 0, 0, 0, 0, 0};

	int ret;

	if ((ret = new_block(p_key, CELL_TYPE_STRING, dim, FILL_NEW_DONT_FILL, bytes_key + 2*num_rows)) != SERVICE_NO_ERROR)
		return ret;

	if ((ret = new_block(p_val, CELL_TYPE_STRING, dim, FILL_NEW_DONT_FILL, bytes_val + 2*num_rows)) != SERVICE_NO_ERROR) {
		destroy_transaction(p_key);

		return ret;
	}
	int i = 0;
	pBlock key = p_key->p_block, val = p_val->p_block;
	for (Index::iterator it = index.begin(); it != index.end(); ++it) {
		key->set_string(i, it->first.c_str());
		val->set_string(i++, it->second.c_str());
	}

	StaticBlockHeader hea[2];
	Name			  name[2]  = {"key", "value"};
	pBlock			  block[2] = {key, val};

	memcpy(&hea[0], key, sizeof(StaticBlockHeader));
	memcpy(&hea[1], val, sizeof(StaticBlockHeader));
	hea[0].range.dim[0] = num_rows;
	hea[1].range.dim[0] = num_rows;

	ret = new_block(p_txn, 2, hea, name, block);

	destroy_transaction(p_key);
	destroy_transaction(p_val);

	return ret;
}


/** "Easy" interface **complete Block** retrieval. This parses p_what and, on success, calls the native get() equivalent.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param p_what	Some string that as_locator() can parse into a Locator. E.g. //base/entity/key

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.
*/
StatusCode Container::get(pTransaction &p_txn, pChar p_what) {
	Locator loc;
	StatusCode ret;

	p_txn = nullptr;

	if ((ret = as_locator(loc, p_what)) != SERVICE_NO_ERROR)
		return ret;

	return get(p_txn, loc);
}


/** "Easy" interface **selection of rows in a Block** retrieval. This parses p_what and, on success, calls the native get() equivalent.

	\param p_txn		A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
						Transaction inside the Container.
	\param p_what		Some string that as_locator() can parse into a Locator. E.g. //base/entity/key
	\param p_row_filter	The block we want to use as a filter. This is either a tensor of boolean or integer that can_filter(p_from).

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.
*/
StatusCode Container::get(pTransaction &p_txn, pChar p_what, pBlock p_row_filter) {
	Locator loc;
	StatusCode ret;

	p_txn = nullptr;

	if ((ret = as_locator(loc, p_what)) != SERVICE_NO_ERROR)
		return ret;

	return get(p_txn, loc, p_row_filter);
}


/** "Easy" interface **selection of a tensor in a Tuple** retrieval. This parses p_what and, on success, calls the native get() equivalent.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param p_what	Some string that as_locator() can parse into a Locator. E.g. //base/entity/key
	\param name		The name of the item to be selected.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.
*/
StatusCode Container::get(pTransaction &p_txn, pChar p_what, pChar name) {
	Locator loc;
	StatusCode ret;

	p_txn = nullptr;

	if ((ret = as_locator(loc, p_what)) != SERVICE_NO_ERROR)
		return ret;

	return get(p_txn, loc, name);
}


/** "Easy" interface : Locate a block by some relation that is understandable by the descendant.

	\param location	The solved location of the block.
	\param p_what	A valid reference to a block. E.g. //deque/ent/~first, //tree/ent/key~parent, //queue/ent/~highest

	\return	SERVICE_NO_ERROR on success (and a valid location), or some negative value (error).

The default behaviour of this is returning the same locator that is given for most Containers. Only containers like Volatile which
support block relations defined by commands will return actual //base/ent/key locators that are the result of some computation.
*/
StatusCode Container::locate(Locator &location, pChar p_what) {
	Locator what;
	StatusCode ret;

	if ((ret = as_locator(what, p_what)) != SERVICE_NO_ERROR)
		return ret;

	return locate(location, what);
}


/** "Easy" interface **metadata of a Block** retrieval. This parses p_what and, on success, calls the native header() equivalent.

	\param hea		A StaticBlockHeader structure that will receive the metadata.
	\param p_what	Some string that as_locator() can parse into a Locator. E.g. //base/entity/key

	\return	SERVICE_NO_ERROR on success (and a valid hea), or some negative value (error).

This is a faster, not involving RAM allocation version of the other form of header. For a tensor, is will be the only thing you need, but
for a Kind or a Tuple, you probably want the types of all its items and need to pass a pTransaction to hold the data.
*/
StatusCode Container::header(StaticBlockHeader &hea, pChar p_what) {
	Locator loc;
	StatusCode ret;

	if ((ret = as_locator(loc, p_what)) != SERVICE_NO_ERROR)
		return ret;

	return header(hea, loc);
}


/** "Easy" interface **metadata of a Block** retrieval. This parses p_what and, on success, calls the native header() equivalent.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param p_what	Some string that as_locator() can parse into a Locator. E.g. //base/entity/key

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Unlike its faster form, this allocates a Block and therefore, it is equivalent to a new_block() call. On success, it will return a
Transaction that belongs to the Container and must be destroy_transaction()-ed when the caller is done.

For Tensors it will allocate a block that only has the StaticBlockHeader (What you can more efficiently get from the other form.)
For Kinds, the metadata of all the items is exactly the same a .get() call returns.
For Tuples, it does what you expect: returning a Block with the metadata of all the items without the data.
*/
StatusCode Container::header(pTransaction &p_txn, pChar p_what) {
	Locator loc;
	StatusCode ret;

	p_txn = nullptr;

	if ((ret = as_locator(loc, p_what)) != SERVICE_NO_ERROR)
		return ret;

	return header(p_txn, loc);
}


/** "Easy" interface for **Block storing**: This parses p_where and, on success, calls the native put() equivalent.

	\param p_where	Some string that as_locator() can parse into a Locator. E.g. //base/entity/key
	\param p_block	A block to be stored. Notice it is a block, not a Transaction. If necessary, the Container will make a copy, write to
					disc, PUT it via http, etc. The container does not own the pointer in any way.
	\param mode		Some writing restriction that should return an error if not supported. It controls overriding or writing just the data
					as when writing to a file.

	\return	SERVICE_NO_ERROR on success or some negative value (error).
*/
StatusCode Container::put(pChar p_where, pBlock p_block, int mode) {
	Locator loc;
	StatusCode ret;

	if ((ret = as_locator(loc, p_where)) != SERVICE_NO_ERROR)
		return ret;

	return put(loc, p_block, mode);
}


/** "Easy" interface for **creating entities**: This parses p_where and, on success, calls the native new_entity() equivalent.

	\param p_where	Some string that as_locator() can parse into a Locator. E.g. //base/entity

	\return	SERVICE_NO_ERROR on success or some negative value (error).

	What an entity is, is Container and base dependent. It can be an lmdb database, a folder in a filesystem, a Volatile tree, ...
*/
StatusCode Container::new_entity(pChar p_where) {
	Locator loc;
	StatusCode ret;

	if ((ret = as_locator(loc, p_where)) != SERVICE_NO_ERROR)
		return ret;

	return new_entity(loc);
}


/** "Easy" interface for **deleting entities and blocks**: This parses p_where and, on success, calls the native header() equivalent.

	\param p_where	Some string that as_locator() can parse into a Locator. E.g. //base/entity or //base/entity/key

	\return	SERVICE_NO_ERROR on success or some negative value (error).

	What an entity is, is Container and base dependent. It can be an lmdb database, a folder in a filesystem, a Volatile tree, ...
*/
StatusCode Container::remove(pChar p_where) {
	Locator loc;
	StatusCode ret;

	if ((ret = as_locator(loc, p_where)) != SERVICE_NO_ERROR)
		return ret;

	return remove(loc);
}


/** "Easy" interface for **Block copying**: This parses p_what and p_where. On success, it calls the native copy() equivalent.

	\param p_where	Some **destination** that as_locator() can parse into a Locator. E.g. //base/entity/key
	\param p_what	Some **source** that as_locator() can parse into a Locator. E.g. //base/entity/key

	\return	SERVICE_NO_ERROR on success or some negative value (error).

**NOTE**: This does not copy blocks across Containers. A copy() call is a short way to do "get(tx, what); put(where, tx);
destroy_transaction(tx);" without the Container needing to allocate Transactions and, possibly, not even blocks. To copy blocks
across containers, you need channels.
*/
StatusCode Container::copy(pChar p_where, pChar p_what) {
	Locator where, what;
	StatusCode ret;

	if ((ret = as_locator(where, p_where)) != SERVICE_NO_ERROR)
		return ret;

	if ((ret = as_locator(what, p_what)) != SERVICE_NO_ERROR)
		return ret;

	return copy(where, what);
}


/** The function call interface for **exec**: Execute an opcode in a formal field.

	\param p_txn	A pointer to a Transaction passed by reference. If successful, the Container will return a pointer to a
					Transaction inside the Container.
	\param function	Some description of a service. In general base/entity/key. In Channels the key must be empty and the entity is
					the pipeline. In Bebop, the key is the opcode and the entity, the field, In Agents, the entity is a context.
	\param p_args	A Tuple passed as argument to the call that is not modified. This may be a pure function in Bebop or have context
					in Agency.

	\return	SERVICE_NO_ERROR on success (and a valid p_txn), or some negative value (error).

Usage-wise, this is equivalent to a new_block() call. On success, it will return a Transaction that belongs to the Container and must
be destroy_transaction()-ed when the caller is done.
*/
StatusCode Container::exec(pTransaction &p_txn, Locator &function, pTuple p_args) {

	return SERVICE_ERROR_NOT_APPLICABLE;
}


/** The function call interface for **modify**: In jazz_elements, this is only implemented in Channels.

	\param function	Some description of a service. In general base/entity/key. In Channels the key must be empty and the entity is
					the pipeline. In Bebop, the key is the opcode and the entity, the field, In Agents, the entity is a context.
	\param p_args	In Channels: A Tuple with two items, "input" with the data passed to the service and "result" with the data returned.
					The result will be overridden in-place without any allocation.

	\return	SERVICE_NO_ERROR on success or some negative value (error).

modify() is similar to exec(), but, rather than creating a new block with the result, it modifies the Tuple p_args.
*/
StatusCode Container::modify(Locator &function, pTuple p_args) {

	return SERVICE_ERROR_NOT_APPLICABLE;
}


/** The parser: A simple parser that does not support Locator.p_extra, but is enough for Volatile and Persisted.

	\param result	A Locator to contained the parsed result on success. (Undefined content on error.)
	\param p_what	Some string to be parsed. E.g. //base/entity/key

	\return	SERVICE_NO_ERROR on success or some negative value (error).

More complex Container descendants that support URLs, credentials, cookies, etc. will override this minimalistic parser.
*/
StatusCode Container::as_locator(Locator &result, pChar p_what) {

	if (p_what[0] != '/' || p_what[1] != '/')
		return SERVICE_ERROR_PARSING_NAMES;

	p_what += 2;

	int	  section = 0;
	int	  size	  = LOCATOR_SIZE[section];
	bool  written = false, number = false;
	pChar p_out	  = (pChar) &result.base;

	while (true) {
		switch (char ch = *(p_what++)) {
		case '~':
			number = section == 2;

			if (--size < 0)
				return SERVICE_ERROR_PARSING_NAMES;

			*(p_out++) = ch;
			written	   = true;

			break;

		case '.':
			if (!number)
				return SERVICE_ERROR_PARSING_NUMBERS;

		case 'a' ... 'z':
		case 'A' ... 'Z':
		case '_':
			number = false;
		case '0' ... '9':
		case '-':
			if (--size < 0)
				return SERVICE_ERROR_PARSING_NAMES;

			*(p_out++) = ch;
			written	   = true;

			break;

		case '/':
			if (!written)
				return SERVICE_ERROR_PARSING_NAMES;

			*(p_out++) = 0;

			if (++section > 2)
				return SERVICE_ERROR_PARSING_NAMES;

			if (section == 1)
				p_out = (pChar) &result.entity;
			else
				p_out = (pChar) &result.key;

			size	= LOCATOR_SIZE[section];
			written = false;

			break;

		case 0:
			if (!written)
				return SERVICE_ERROR_PARSING_NAMES;

			*(p_out++) = 0;

			if (section == 0)
				return SERVICE_ERROR_PARSING_NAMES;

			if (section == 1)
				result.key[0] = 0;

			result.p_extra = nullptr;

			return SERVICE_NO_ERROR;

		default:
			return SERVICE_ERROR_PARSING_NAMES;
		}
	}
}


/** The "native" interface: This is what really does the job and **must be implemented in the Container descendants**.

**NOTE**: The root Container class does not implement this.
*/
StatusCode Container::get(pTransaction &p_txn, Locator &what) {

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** The "native" interface: This is what really does the job and **must be implemented in the Container descendants**.

**NOTE**: The root Container class does not implement this.
*/
StatusCode Container::get(pTransaction &p_txn, Locator &what, pBlock p_row_filter) {

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** The "native" interface: This is what really does the job and **must be implemented in the Container descendants**.

**NOTE**: The root Container class does not implement this.
*/
StatusCode Container::get(pTransaction &p_txn, Locator &what, pChar name) {

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** The "native" interface: The default behavior for most Container descendants that do not support commands is just copying.

	\param location	The solved location of the block.
	\param what		A valid reference to a block. E.g. //deque/ent/~first, //tree/ent/key~parent, //queue/ent/~highest

	\return	SERVICE_NO_ERROR on success (and a valid location), or some negative value (error).

NOTE: This just copies what into location. It will not verify the existence of the block. The function of locate() is to
evaluate commands in Containers supporting them (E.g, Volatile). Otherwise, it is a trivial conversion from the easy into the native API.
*/
StatusCode Container::locate(Locator &location, Locator &what) {

	memcpy(&location, &what, sizeof(Locator));

	return SERVICE_NO_ERROR;
}


/** The "native" interface: This is what really does the job and **must be implemented in the Container descendants**.

**NOTE**: The root Container class does not implement this.
*/
StatusCode Container::header(StaticBlockHeader &hea, Locator &what) {

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** The "native" interface: This is what really does the job and **must be implemented in the Container descendants**.

**NOTE**: The root Container class does not implement this.
*/
StatusCode Container::header(pTransaction &p_txn, Locator &what) {

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** The "native" interface: This is what really does the job and **must be implemented in the Container descendants**.

**NOTE**: The root Container class does not implement this.
*/
StatusCode Container::put(Locator &where, pBlock p_block, int mode) {

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** The "native" interface: This is what really does the job and **must be implemented in the Container descendants**.

**NOTE**: The root Container class does not implement this.
*/
StatusCode Container::new_entity(Locator &where) {

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** The "native" interface: This is what really does the job and **must be implemented in the Container descendants**.

**NOTE**: The root Container class does not implement this.
*/
StatusCode Container::remove(Locator &where) {

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** The "native" interface: This is what really does the job and **must be implemented in the Container descendants**.

**NOTE**: The root Container class does not implement this.
*/
StatusCode Container::copy(Locator &where, Locator &what) {

	return SERVICE_NOT_IMPLEMENTED;		// API Only: One-shot container does not support this.
}


/** Add the base names for this Container.

	\param base_names	A BaseNames map passed by reference to which the base names of this object are added by this call.

	The root class Container does not add any base names.
*/
void Container::base_names(BaseNames &base_names) {}


/** Creates the buffers for new_transaction()/destroy_transaction()

	\return	SERVICE_NO_ERROR or SERVICE_ERROR_NO_MEM on RAM alloc failure.
*/
StatusCode Container::new_container() {

	if (p_buffer != nullptr || max_transactions <= 0) {
#ifdef CATCH_TEST
		destroy_container();
#else
		log(LOG_ERROR, "new_container() called on a running Container().");
		return SERVICE_ERROR_STARTING;
#endif
	}

	alloc_bytes = 0;

	alloc_warning_issued = false;

	_lock_ = 0;

	p_buffer = (pStoredTransaction) malloc(max_transactions*sizeof(StoredTransaction));

	if (p_buffer == nullptr)
		return SERVICE_ERROR_NO_MEM;

	p_free = p_buffer;

	pStoredTransaction pt = (pStoredTransaction) p_buffer;

	for (int i = 1; i < max_transactions; i++) {
		pt->status = BLOCK_STATUS_DESTROYED;
		pStoredTransaction l_pt = pt++;
		l_pt->p_next = pt;
	}
	pt->status = BLOCK_STATUS_DESTROYED;
	pt->p_next = nullptr;

	return SERVICE_NO_ERROR;
}


/** Destroys everything: all transactions and the buffer itself

	\return	SERVICE_NO_ERROR.
*/
StatusCode Container::destroy_container() {

	if (p_buffer != nullptr) {
		pStoredTransaction pt = (pStoredTransaction) p_buffer;

		for (int i = 0; i < max_transactions; i++) {
			if (pt->status != BLOCK_STATUS_DESTROYED) {
				pTransaction p_txn = pt;
				destroy_transaction(p_txn);
			}
			pt++;
		}
		free(p_buffer);
	}
	alloc_bytes = 0;
	p_buffer = p_free = nullptr;
	_lock_ = 0;

	return SERVICE_NO_ERROR;
}


/** Parse a source definition of an item in a Kind, possibly with dimensions.

	\param p_in			The input char stream cursor.
	\param num_bytes	The number of bytes with data above *p_in
	\param item_hea		The structure that receives the resulting cell_type, rank and shape.
	\param dims			A MapSI to retrieve the dimension names.

	\return	True on success will return a valid item_hea (otherwise ite_hea is undefined).
*/
bool Container::get_type_and_shape(pChar &p_in, int &num_bytes, ItemHeader *item_hea, MapSI &dims) {

	if (skip_space(p_in, num_bytes) < 7)
		return false;

	if (strncmp(p_in, "INTEGER", 7) == 0) {
		item_hea->cell_type = CELL_TYPE_INTEGER;
		num_bytes -= 7;
		p_in	  += 7;
	} else if (strncmp(p_in, "DOUBLE", 6) == 0) {
		item_hea->cell_type = CELL_TYPE_DOUBLE;
		num_bytes -= 6;
		p_in	  += 6;
	} else if (strncmp(p_in, "BYTE", 4) == 0) {
		item_hea->cell_type = CELL_TYPE_BYTE;
		num_bytes -= 4;
		p_in	  += 4;
	} else if (strncmp(p_in, "STRING", 6) == 0) {
		item_hea->cell_type = CELL_TYPE_STRING;
		num_bytes -= 6;
		p_in	  += 6;
	} else if (strncmp(p_in, "BOOLEAN", 7) == 0) {
		item_hea->cell_type = CELL_TYPE_BOOLEAN;
		num_bytes -= 7;
		p_in	  += 7;
	} else if (strncmp(p_in, "SINGLE", 6) == 0) {
		item_hea->cell_type = CELL_TYPE_SINGLE;
		num_bytes -= 6;
		p_in	  += 6;
	} else if (strncmp(p_in, "TIME", 4) == 0) {
		item_hea->cell_type = CELL_TYPE_TIME;
		num_bytes -= 4;
		p_in	  += 4;
	} else if (strncmp(p_in, "LONG_INTEGER", 12) == 0) {
		item_hea->cell_type = CELL_TYPE_LONG_INTEGER;
		num_bytes -= 12;
		p_in	  += 12;
	} else if (strncmp(p_in, "BYTE_BOOLEAN", 12) == 0) {
		item_hea->cell_type = CELL_TYPE_BYTE_BOOLEAN;
		num_bytes -= 12;
		p_in	  += 12;
	} else if (strncmp(p_in, "FACTOR", 6) == 0) {
		item_hea->cell_type = CELL_TYPE_FACTOR;
		num_bytes -= 6;
		p_in	  += 6;
	} else if (strncmp(p_in, "GRADE", 5) == 0) {
		item_hea->cell_type = CELL_TYPE_GRADE;
		num_bytes -= 5;
		p_in	  += 5;
	} else
		return false;

	if (skip_space(p_in, num_bytes) < 3)
		return false;

	if (get_char(p_in, num_bytes) != '[')
		return false;

	item_hea->rank = 0;

	memset(&item_hea->dim, 0, sizeof(TensorDim));

	while (true) {
		Name dim_name;
		if (skip_space(p_in, num_bytes) < 2)
			return false;

		char cl = *p_in;

		if (cl >= '0' && cl <= '9') {
			if (!sscanf_int32(p_in, num_bytes, item_hea->dim[item_hea->rank]) || item_hea->dim[item_hea->rank] < 1)
				return false;

		} else if ((cl >= 'A' && cl <= 'Z') || (cl >= 'a' && cl <= 'z')) {
			if (!get_item_name(p_in, num_bytes, dim_name, false, false))
				return false;

			if (dims.find(dim_name) == dims.end()) {
				int ix = -(dims.size() + 1);

				dims[dim_name] = ix;

				item_hea->dim[item_hea->rank] = ix;
			} else
				item_hea->dim[item_hea->rank] = dims[dim_name];
		}
		if (skip_space(p_in, num_bytes) < 1)
			return false;

		cl = get_char(p_in, num_bytes);

		if (item_hea->rank++ >= MAX_TENSOR_RANK)
			return false;

		if (cl == ']')
			return true;

		if (cl != ',')
			return false;
	}
}


/** Parse a tensor, measure it and fail on inconsistent shape

	\param p_in			The input char stream cursor.
	\param num_bytes	The number of bytes with data above *p_in
	\param cell_type	The type of the tensor being parsed (if CELL_TYPE_UNDEFINED, it will be detected)
	\param item_hea		The structure that receives the resulting cell_type, rank, shape and item_size.

	\return	True on success will return a valid item_hea (otherwise ite_hea is undefined).

In the case of CELL_TYPE_STRING, item_size will return the serialized total size of all the strings.
*/
bool Container::get_shape_and_size(pChar &p_in, int &num_bytes, int cell_type, ItemHeader *item_hea) {

	char state;

	switch (cell_type) {
	case CELL_TYPE_UNDEFINED:
		state = PSTATE_IN_AUTO;
		break;
	case CELL_TYPE_BYTE:
	case CELL_TYPE_INTEGER:
	case CELL_TYPE_FACTOR:
	case CELL_TYPE_GRADE:
	case CELL_TYPE_LONG_INTEGER:
	case CELL_TYPE_BYTE_BOOLEAN:
	case CELL_TYPE_BOOLEAN:
		state = PSTATE_IN_INT;
		break;
	case CELL_TYPE_SINGLE:
	case CELL_TYPE_DOUBLE:
		state = PSTATE_IN_REAL;
		break;
	case CELL_TYPE_STRING:
		state = PSTATE_IN_STRING;
		break;
	case CELL_TYPE_TIME:
		state = PSTATE_IN_TIME;
		break;
	default:
		return false;
	}

	item_hea->cell_type = cell_type;
	item_hea->item_size = 0;

	memset(item_hea->dim, -1, sizeof(TensorDim));		// == {-1, -1, -1, -1, -1, -1};
	TensorDim n_item = {0,  0,  0,  0,  0,  0};

	int level = -1;
	bool first_row = true;

	while (true) {
		unsigned char cursor;

		if (num_bytes == 0)
			return false;

		cursor = get_char(p_in, num_bytes);
		state  = parser_state_switch[state].next[cursor];

		switch (state) {
		case PSTATE_OUT_INT:
			if (item_hea->cell_type == CELL_TYPE_UNDEFINED)
				item_hea->cell_type = CELL_TYPE_INTEGER;

		case PSTATE_OUT_REAL:
			if (item_hea->cell_type == CELL_TYPE_UNDEFINED)
				item_hea->cell_type = CELL_TYPE_DOUBLE;

		case PSTATE_OUT_STRING:
			if (item_hea->cell_type == CELL_TYPE_UNDEFINED)
				item_hea->cell_type = CELL_TYPE_STRING;

		case PSTATE_OUT_TIME:
			if (cursor == ']') {
				n_item.dim[level]++;

				if (item_hea->dim[level] < 0)
					item_hea->dim[level] = n_item.dim[level];
				else {
					if (item_hea->dim[level] != n_item.dim[level])
						return false;
				};
				level--;

				if (level == -1) {
					for (int i = item_hea->rank; i < MAX_TENSOR_RANK; i++)
						item_hea->dim[i] = 0;

					return true;
				}
			}
			break;

		case PSTATE_IN_AUTO:
		case PSTATE_IN_INT:
		case PSTATE_IN_REAL:
		case PSTATE_IN_STRING:
		case PSTATE_IN_TIME:
			if (cursor == ',')
				n_item.dim[level]++;

			if (cursor == '[') {
				level++;
				if (first_row)
					item_hea->rank = level + 1;
				else {
					if (level >= item_hea->rank)
						return false;
				}
				if (level >= MAX_TENSOR_RANK)
					return false;

				n_item.dim[level] = 0;
			};
			break;

		case PSTATE_SEP_INT:
		case PSTATE_SEP_REAL:
		case PSTATE_SEP_STRING:
		case PSTATE_SEP_TIME:
			if (cursor == ',') {
				if (level != item_hea->rank - 1)
					return false;

				n_item.dim[level]++;
			}

			first_row = false;

			break;

		case PSTATE_CONST_STRING_E0:
			if (*p_in == 'n')
				item_hea->item_size++;

		case PSTATE_CONST_STRING_N:
			item_hea->item_size++;

			break;

		case PSTATE_CONST_STRING0:
			// if (*p_in == '"')
			// 	this_is_an_empty_string();
		case PSTATE_CONST_AUTO:
		case PSTATE_CONST_INT:
		case PSTATE_CONST_REAL:
		case PSTATE_CONST_STRING_E1:
		case PSTATE_CONST_STRING_E2:
		case PSTATE_CONST_TIME:
		case PSTATE_NA_INT:
		case PSTATE_NA_REAL:
		case PSTATE_NA_STRING:
		case PSTATE_NA_TIME:
		case PSTATE_END_STRING:
			break;

		default:
			return false;
		}
	}
}


/** Parse a tensor of CELL_TYPE_STRING by serializing it into a buffer compatible with a new_block(1).p_text

	\param p_in			The input char stream cursor.
	\param num_bytes	The number of bytes with data above *p_in
	\param p_out		A pointer to a buffer that has enough size for the serialized output.
	\param num_cells	The expected number of cell for verification.
	\param is_NA		An array to store the indices of the NA cells (to be set after the new_block() call).
	\param hasLN		An array to store the indices of the string containing \n (which is serialized) and to be deserialized later.

	\return	True on success

**Note**: Cells are separated by a \\n character (not escaped) which is not confused with a \\n inside the string which becomes escaped.
*/
bool Container::fill_text_buffer(pChar &p_in, int &num_bytes, pChar p_out, int num_cells, int is_NA[], int hasLN[]) {

	char state = PSTATE_IN_STRING;

	int level = 0, ix_NA = 0, ix_LN = 0, ix = 0;

	is_NA[0] = -1;
	hasLN[0] = -1;

	while (true) {
		unsigned char cursor;

		if (num_bytes == 0)
			return false;

		cursor = get_char(p_in, num_bytes);
		state  = parser_state_switch[state].next[cursor];

		switch (state) {
		case PSTATE_OUT_STRING:
			if (cursor == ']') {
				level--;

				if (level == 0) {
					*p_out = 0;

					return ix == num_cells;
				}
			}

			break;

		case PSTATE_NA_STRING:
			if (cursor == 'A') {
				*(p_out++) = '\n';

				is_NA[ix_NA++] = ix;
				is_NA[ix_NA]   = -1;

				ix++;
			}

			break;

		case PSTATE_IN_STRING:
			if (cursor == '[')
				level++;

			break;

		case PSTATE_CONST_STRING_E0:
			cursor = get_char(p_in, num_bytes);
			state  = parser_state_switch[state].next[cursor];

			switch (cursor) {
			case 'a':
				*(p_out++) = '\a';
				break;

			case 'b':
				*(p_out++) = '\b';
				break;

			case 't':
				*(p_out++) = '\t';
				break;

			case 'n':
				*(p_out++) = '\\';
				*(p_out++) = 'n';
				if (hasLN[0] < 0) {
					hasLN[0] = ix;
					hasLN[1] = -1;
				} else if (hasLN[ix_LN] != ix) {
					ix_LN++;
					hasLN[ix_LN]	 = ix;
					hasLN[ix_LN + 1] = -1;
				}

				break;

			case 'v':
				*(p_out++) = '\v';
				break;

			case 'f':
				*(p_out++) = '\f';
				break;

			case 'r':
				*(p_out++) = '\r';
				break;

			case '\"':
				*(p_out++) = '\"';
				break;

			case '\\':
				*(p_out++) = '\\';
				break;

			default:
				int xhi = from_hex(cursor = get_char(p_in, num_bytes));
				state	= parser_state_switch[state].next[cursor];

				int xlo = from_hex(cursor = get_char(p_in, num_bytes));
				state	= parser_state_switch[state].next[cursor];

				*(p_out++) = (xhi << 4) + xlo;

				break;
			}
			break;

		case PSTATE_CONST_STRING_N:
			*(p_out++) = cursor;

			break;

		case PSTATE_END_STRING:
			*(p_out++) = '\n';
			ix++;

			break;

		case PSTATE_CONST_STRING0:
		case PSTATE_CONST_STRING_E1:
		case PSTATE_CONST_STRING_E2:
		case PSTATE_SEP_STRING:
			break;

		default:
			return false;
		}
	}
}


/** Parse a tensor of anything other than CELL_TYPE_STRING into a binary (allocated but not initialized) Block.

	\param p_in			The input char stream cursor.
	\param num_bytes	The number of bytes with data above *p_in
	\param p_block		The allocated block. Already has anything, (cell type, shape, etc.), except the data.

	\return	True on success
*/
bool Container::fill_tensor(pChar &p_in, int &num_bytes, pBlock p_block) {

	char cell [MAX_SIZE_OF_CELL_AS_TEXT];

	pChar p_st = (pChar) &cell, p_end = (pChar) &cell + sizeof(cell) - 1;

	switch (p_block->cell_type) {
	case CELL_TYPE_BYTE: {
		char state = PSTATE_IN_INT;
		int level = 0;
		uint8_t *p_out = &p_block->tensor.cell_byte[0];

		while (true) {
			unsigned char cursor;

			if (num_bytes == 0)
				return false;

			cursor = get_char(p_in, num_bytes);
			state  = parser_state_switch[state].next[cursor];

			switch (state) {
			case PSTATE_OUT_INT:
				if (cursor == ']') {
					if (p_st != ((pChar) &cell)) {		// Ugly parenthesis required by cppcheck
						*p_st = 0;
						p_st  = (pChar) &cell;

						if (sscanf(p_st, "%hhu", p_out) != 1)
							return false;

						p_out++;
					}
					level--;

					if (level == 0)
						return p_out == &p_block->tensor.cell_byte[p_block->size];
				}
				break;

			case PSTATE_IN_INT:
				if (cursor == '[')
					level++;

				break;

			case PSTATE_SEP_INT:
				if (cursor == ',') {
					*p_st = 0;
					p_st  = (pChar) &cell;

					if (sscanf(p_st, "%hhu", p_out) != 1)
						return false;

					p_out++;
				}
				break;

			case PSTATE_CONST_INT:
				if (p_st == p_end)
					return false;

				*(p_st++) = cursor;

				break;

			default:

				return false;
			}
		}
	}
	case CELL_TYPE_INTEGER:
	case CELL_TYPE_FACTOR:
	case CELL_TYPE_GRADE: {
		char state = PSTATE_IN_INT;
		int level = 0;
		int *p_out = &p_block->tensor.cell_int[0];

		while (true) {
			unsigned char cursor;

			if (num_bytes == 0)
				return false;

			cursor = get_char(p_in, num_bytes);
			state  = parser_state_switch[state].next[cursor];

			switch (state) {
			case PSTATE_OUT_INT:
				if (cursor == ']') {
					if (level == p_block->rank && !push_int_cell(cell, p_st, p_out))
						return false;

					level--;

					if (level == 0)
						return p_out == &p_block->tensor.cell_int[p_block->size];
				}
				break;

			case PSTATE_NA_INT:
				if (cursor == 'A')
					p_st = (pChar) &cell;

				break;

			case PSTATE_IN_INT:
				if (cursor == '[')
					level++;

				break;

			case PSTATE_SEP_INT:
				if (cursor == ',')
					if (!push_int_cell(cell, p_st, p_out))
						return false;

				break;

			case PSTATE_CONST_INT:
				if (p_st == p_end)
					return false;

				*(p_st++) = cursor;

				break;

			default:

				return false;
			}
		}
	}
	case CELL_TYPE_LONG_INTEGER: {
		char state = PSTATE_IN_INT;
		int level = 0;
		long long *p_out = &p_block->tensor.cell_longint[0];

		while (true) {
			unsigned char cursor;

			if (num_bytes == 0)
				return false;

			cursor = get_char(p_in, num_bytes);
			state  = parser_state_switch[state].next[cursor];

			switch (state) {
			case PSTATE_OUT_INT:
				if (cursor == ']') {
					if (level == p_block->rank && !push_int_cell(cell, p_st, p_out))
						return false;

					level--;

					if (level == 0)
						return p_out == &p_block->tensor.cell_longint[p_block->size];
				}
				break;

			case PSTATE_NA_INT:
				if (cursor == 'A')
					p_st = (pChar) &cell;

				break;

			case PSTATE_IN_INT:
				if (cursor == '[')
					level++;

				break;

			case PSTATE_SEP_INT:
				if (cursor == ',')
					if (!push_int_cell(cell, p_st, p_out))
						return false;

				break;

			case PSTATE_CONST_INT:
				if (p_st == p_end)
					return false;

				*(p_st++) = cursor;

				break;

			default:

				return false;
			}
		}
	}
	case CELL_TYPE_BYTE_BOOLEAN: {
		char state = PSTATE_IN_INT;
		int level = 0;
		bool *p_out = &p_block->tensor.cell_bool[0];

		while (true) {
			unsigned char cursor;

			if (num_bytes == 0)
				return false;

			cursor = get_char(p_in, num_bytes);
			state  = parser_state_switch[state].next[cursor];

			switch (state) {
			case PSTATE_OUT_INT:
				if (cursor == ']') {
					if (level == p_block->rank && !push_bool_cell(cell, p_st, p_out))
						return false;

					level--;

					if (level == 0)
						return p_out == &p_block->tensor.cell_bool[p_block->size];
				}
				break;

			case PSTATE_NA_INT:
				if (cursor == 'A')
					p_st = (pChar) &cell;

				break;

			case PSTATE_IN_INT:
				if (cursor == '[')
					level++;

				break;

			case PSTATE_SEP_INT:
				if (cursor == ',')
					if (!push_bool_cell(cell, p_st, p_out))
						return false;

				break;

			case PSTATE_CONST_INT:
				if (p_st == p_end)
					return false;

				*(p_st++) = cursor;

				break;

			default:

				return false;
			}
		}
	}
	case CELL_TYPE_BOOLEAN: {
		char state = PSTATE_IN_INT;
		int level = 0;
		uint32_t *p_out = &p_block->tensor.cell_uint[0];

		while (true) {
			unsigned char cursor;

			if (num_bytes == 0)
				return false;

			cursor = get_char(p_in, num_bytes);
			state  = parser_state_switch[state].next[cursor];

			switch (state) {
			case PSTATE_OUT_INT:
				if (cursor == ']') {
					if (level == p_block->rank && !push_bool_cell(cell, p_st, p_out))
						return false;

					level--;

					if (level == 0)
						return p_out == &p_block->tensor.cell_uint[p_block->size];
				}
				break;

			case PSTATE_NA_INT:
				if (cursor == 'A')
					p_st = (pChar) &cell;

				break;

			case PSTATE_IN_INT:
				if (cursor == '[')
					level++;

				break;

			case PSTATE_SEP_INT:
				if (cursor == ',')
					if (!push_bool_cell(cell, p_st, p_out))
						return false;

				break;

			case PSTATE_CONST_INT:
				if (p_st == p_end)
					return false;

				*(p_st++) = cursor;

				break;

			default:

				return false;
			}
		}
	}
	case CELL_TYPE_SINGLE: {
		char state = PSTATE_IN_REAL;
		int level = 0;
		float *p_out = &p_block->tensor.cell_single[0];

		while (true) {
			unsigned char cursor;

			if (num_bytes == 0)
				return false;

			cursor = get_char(p_in, num_bytes);
			state  = parser_state_switch[state].next[cursor];

			switch (state) {
			case PSTATE_OUT_REAL:
				if (cursor == ']') {
					if (level == p_block->rank && !push_real_cell(cell, p_st, p_out))
						return false;

					level--;

					if (level == 0)
						return p_out == &p_block->tensor.cell_single[p_block->size];
				}
				break;

			case PSTATE_NA_REAL:
				if (cursor == 'A')
					p_st = (pChar) &cell;

				break;

			case PSTATE_IN_REAL:
				if (cursor == '[')
					level++;

				break;

			case PSTATE_SEP_REAL:
				if (cursor == ',')
					if (!push_real_cell(cell, p_st, p_out))
						return false;

				break;

			case PSTATE_CONST_REAL:
				if (p_st == p_end)
					return false;

				*(p_st++) = cursor;

				break;

			default:

				return false;
			}
		}
	}
	case CELL_TYPE_DOUBLE: {
		char state = PSTATE_IN_REAL;
		int level = 0;
		double *p_out = &p_block->tensor.cell_double[0];

		while (true) {
			unsigned char cursor;

			if (num_bytes == 0)
				return false;

			cursor = get_char(p_in, num_bytes);
			state  = parser_state_switch[state].next[cursor];

			switch (state) {
			case PSTATE_OUT_REAL:
				if (cursor == ']') {
					if (level == p_block->rank && !push_real_cell(cell, p_st, p_out))
						return false;

					level--;

					if (level == 0)
						return p_out == &p_block->tensor.cell_double[p_block->size];
				}
				break;

			case PSTATE_NA_REAL:
				if (cursor == 'A')
					p_st = (pChar) &cell;

				break;

			case PSTATE_IN_REAL:
				if (cursor == '[')
					level++;

				break;

			case PSTATE_SEP_REAL:
				if (cursor == ',')
					if (!push_real_cell(cell, p_st, p_out))
						return false;

				break;

			case PSTATE_CONST_REAL:
				if (p_st == p_end)
					return false;

				*(p_st++) = cursor;

				break;

			default:

				return false;
			}
		}
	}
	case CELL_TYPE_TIME: {
		char state = PSTATE_IN_TIME;
		int level = 0;
		time_t *p_out = &p_block->tensor.cell_time[0];

		while (true) {
			unsigned char cursor;

			if (num_bytes == 0)
				return false;

			cursor = get_char(p_in, num_bytes);
			state  = parser_state_switch[state].next[cursor];

			switch (state) {
			case PSTATE_OUT_TIME:
				if (cursor == ']') {
					if (level == p_block->rank && !push_time_cell(cell, p_st, p_out, DEF_FLOAT_TIME))
						return false;

					level--;

					if (level == 0)
						return p_out == &p_block->tensor.cell_time[p_block->size];
				}
				break;

			case PSTATE_NA_TIME:
				if (cursor == 'A')
					p_st = (pChar) &cell;

				break;

			case PSTATE_IN_TIME:
				if (cursor == '[')
					level++;

				break;

			case PSTATE_SEP_TIME:
				if (cursor == ',')
					if (!push_time_cell(cell, p_st, p_out, DEF_FLOAT_TIME))
						return false;

				break;

			case PSTATE_CONST_TIME:
				if (p_st == p_end)
					return false;

				*(p_st++) = cursor;

				break;

			default:
				return false;
			}
		}
	}
	default:
		return false;
	}
}


/** Implements the complete text block creation: fill_text_buffer()/new_block() and fixing NA and ExpandEscapeSequences()

	\param p_txn		Transaction for the new_block() call.
	\param item_hea		An ItemHeader computed by a previous call to get_type_and_shape()
	\param p_in			The input char stream cursor.
	\param num_bytes	The number of bytes with data above *p_in
	\param att			An AttributeMap for the new_block() call.

	\return	StatusCode like a new_block() call
*/
int Container::new_text_block(pTransaction &p_txn, ItemHeader &item_hea, pChar &p_in, int &num_bytes, AttributeMap *att) {

	p_txn = nullptr;

	int num_cells = item_hea.dim[0];

	for (int i = 1; i < item_hea.rank; i++)
		num_cells *= item_hea.dim[i];

	int bf_size = num_cells + item_hea.item_size + 1;
	int ix_size = (num_cells + 1)*sizeof(int);

	pChar p_txt = (pChar) malloc(bf_size);

	if (p_txt == nullptr)
		return SERVICE_ERROR_NO_MEM;

	int *p_is_NA = reinterpret_cast<int *>(malloc(ix_size));

	StatusCode ret;

	if (p_is_NA == nullptr)
		ret = SERVICE_ERROR_NO_MEM;

	else {
		int *p_hasLN = reinterpret_cast<int *>(malloc(ix_size));

		if (p_hasLN == nullptr)
			ret = SERVICE_ERROR_NO_MEM;

		else {
			if (!fill_text_buffer(p_in, num_bytes, p_txt, num_cells, p_is_NA, p_hasLN))
				ret = PARSE_ERROR_TEXT_FILLING;

			else {
				ret = new_block(p_txn, CELL_TYPE_STRING, item_hea.dim, FILL_WITH_TEXTFILE, 0, p_txt, '\n', att);

				if (ret == SERVICE_NO_ERROR) {
					for (int i = 0; i < num_cells; i++) {
						if (p_is_NA[i] < 0)
							break;

						p_txn->p_block->tensor.cell_int[p_is_NA[i]] = STRING_NA;
					}
					for (int i = 0; i < num_cells; i++) {
						if (p_hasLN[i] < 0)
							break;

						ExpandEscapeSequences(p_txn->p_block->get_string(p_hasLN[i]));
					}
				}
			}

			alloc_bytes -= ix_size;
			free(p_hasLN);
		}

		alloc_bytes -= ix_size;
		free(p_is_NA);
	}

	alloc_bytes -= bf_size;
	free(p_txt);

	return ret;
}


/** Serializes a Tensor of CELL_TYPE_BYTE, CELL_TYPE_INTEGER, CELL_TYPE_FACTOR, CELL_TYPE_GRADE or CELL_TYPE_LONG_INTEGER as a string.

	\param p_block	The raw block to be serialized as text (must have one of the types above).
	\param p_dest	Optionally, a pointer with the address to which the output is serialized. (If nullptr, only size counting is done)
	\param p_fmt	Optionally, format specifier that is understood by sprintf (default is %i)

	\return	The length in bytes required to store the output if p_dest == nullptr

The serialization includes NA identification, commas spaces an square brackets to define the shape.
*/
int Container::tensor_int_as_text(pBlock p_block, pChar p_dest, pChar p_fmt) {
	int shape[MAX_TENSOR_RANK];
	int idx[MAX_TENSOR_RANK] = {0, 0, 0, 0, 0, 0};
	int rank_1 = p_block->rank - 1;

	p_block->get_dimensions((int *) &shape);

	if (p_dest == nullptr) {
		int total_len = p_block->rank;	// Length of opening_brackets()

		switch (p_block->cell_type) {
		case CELL_TYPE_BYTE: {
			if (p_fmt == nullptr)
				p_fmt = (pChar) &DEF_INT8_FMT;

			uint8_t *p_t = &p_block->tensor.cell_byte[0];

			for (int i = 0; i < p_block->size; i++) {
				char cell [MAX_SIZE_OF_CELL_AS_TEXT];

				total_len += sprintf(cell, p_fmt, p_t[0]) + separator_len(rank_1, shape, idx);
				p_t++;
			}

			return total_len + 1;
		}
		case CELL_TYPE_INTEGER:
		case CELL_TYPE_FACTOR:
		case CELL_TYPE_GRADE: {
			if (p_fmt == nullptr)
				p_fmt = (pChar) &DEF_INT32_FMT;

			int *p_t = &p_block->tensor.cell_int[0];

			for (int i = 0; i < p_block->size; i++) {
				if (p_t[0] == INTEGER_NA)
					total_len += LENGTH_NA_AS_TEXT + separator_len(rank_1, shape, idx);

				else {
					char cell [MAX_SIZE_OF_CELL_AS_TEXT];

					total_len += sprintf(cell, p_fmt, p_t[0]) + separator_len(rank_1, shape, idx);
				}

				p_t++;
			}

			return total_len + 1;
		}
		case CELL_TYPE_LONG_INTEGER: {
			if (p_fmt == nullptr)
				p_fmt = (pChar) &DEF_INT64_FMT;

			long long *p_t = &p_block->tensor.cell_longint[0];

			for (int i = 0; i < p_block->size; i++) {
				if (p_t[0] == LONG_INTEGER_NA)
					total_len += LENGTH_NA_AS_TEXT + separator_len(rank_1, shape, idx);

				else {
					char cell [MAX_SIZE_OF_CELL_AS_TEXT];

					total_len += sprintf(cell, p_fmt, p_t[0]) + separator_len(rank_1, shape, idx);
				}

				p_t++;
			}

			return total_len + 1;
		}
		default:
			return 0;
		}
	}

	opening_brackets(p_block->rank, p_dest);

	switch (p_block->cell_type) {
	case CELL_TYPE_BYTE: {
		if (p_fmt == nullptr)
			p_fmt = (pChar) &DEF_INT8_FMT;

		uint8_t *p_t = &p_block->tensor.cell_byte[0];

		for (int i = 0; i < p_block->size; i++) {
			p_dest += sprintf(p_dest, p_fmt, p_t[0]);

			separator(rank_1, shape, idx, p_dest);
			p_t++;
		}

		*p_dest = 0;

		return 0;
	}
	case CELL_TYPE_INTEGER:
	case CELL_TYPE_FACTOR:
	case CELL_TYPE_GRADE: {
		if (p_fmt == nullptr)
			p_fmt = (pChar) &DEF_INT32_FMT;

		int *p_t = &p_block->tensor.cell_int[0];

		for (int i = 0; i < p_block->size; i++) {
			if (p_t[0] == INTEGER_NA) {
				strcpy(p_dest, NA);
				p_dest += LENGTH_NA_AS_TEXT;

			} else
				p_dest += sprintf(p_dest, p_fmt, p_t[0]);

			separator(rank_1, shape, idx, p_dest);
			p_t++;
		}

		*p_dest = 0;

		return 0;
	}
	case CELL_TYPE_LONG_INTEGER: {
		if (p_fmt == nullptr)
			p_fmt = (pChar) &DEF_INT64_FMT;

		long long *p_t = &p_block->tensor.cell_longint[0];

		for (int i = 0; i < p_block->size; i++) {
			if (p_t[0] == LONG_INTEGER_NA) {
				strcpy(p_dest, NA);
				p_dest += LENGTH_NA_AS_TEXT;

			} else
				p_dest += sprintf(p_dest, p_fmt, p_t[0]);

			separator(rank_1, shape, idx, p_dest);
			p_t++;
		}

		*p_dest = 0;

		return 0;
	}
	default:
		return 0;
	}
}


/** Serializes a Tensor of CELL_TYPE_BYTE_BOOLEAN or CELL_TYPE_BOOLEAN as a string.

	\param p_block	The raw block to be serialized as text (must have one of the types above).
	\param p_dest	Optionally, a pointer with the address to which the output is serialized. (If nullptr, only size counting is done)

	\return	The length in bytes required to store the output if p_dest == nullptr

The serialization includes NA identification, commas spaces an square brackets to define the shape.
*/
int Container::tensor_bool_as_text(pBlock p_block, pChar p_dest) {
	int shape[MAX_TENSOR_RANK];
	int idx[MAX_TENSOR_RANK] = {0, 0, 0, 0, 0, 0};
	int rank_1 = p_block->rank - 1;

	p_block->get_dimensions((int *) &shape);

	if (p_dest == nullptr) {
		int total_len = p_block->rank;	// Length of opening_brackets()

		switch (p_block->cell_type) {
		case CELL_TYPE_BYTE_BOOLEAN: {
			uint8_t *p_t = &p_block->tensor.cell_byte[0];

			for (int i = 0; i < p_block->size; i++) {
				if (p_t[0] == BYTE_BOOLEAN_NA)
					total_len += LENGTH_NA_AS_TEXT + separator_len(rank_1, shape, idx);
				else
					total_len += 1 + separator_len(rank_1, shape, idx);

				p_t++;
			}

			return total_len + 1;
		}
		case CELL_TYPE_BOOLEAN: {
			uint32_t *p_t = &p_block->tensor.cell_uint[0];

			for (int i = 0; i < p_block->size; i++) {
				if (p_t[0] == BOOLEAN_NA)
					total_len += LENGTH_NA_AS_TEXT + separator_len(rank_1, shape, idx);
				else
					total_len += 1 + separator_len(rank_1, shape, idx);

				p_t++;
			}

			return total_len + 1;
		}
		default:
			return 0;
		}
	}

	opening_brackets(p_block->rank, p_dest);

	switch (p_block->cell_type) {
	case CELL_TYPE_BYTE_BOOLEAN: {
		uint8_t *p_t = &p_block->tensor.cell_byte[0];

		for (int i = 0; i < p_block->size; i++) {
			if (p_t[0]) {
				if (p_t[0] == BYTE_BOOLEAN_NA) {
					strcpy(p_dest, NA);
					p_dest += LENGTH_NA_AS_TEXT;
				} else
					*(p_dest++) = '1';
			} else
				*(p_dest++) = '0';

			separator(rank_1, shape, idx, p_dest);

			p_t++;
		}
		*p_dest = 0;

		return 0;
	}
	case CELL_TYPE_BOOLEAN: {
		uint32_t *p_t = &p_block->tensor.cell_uint[0];

		for (int i = 0; i < p_block->size; i++) {
			if (p_t[0]) {
				if (p_t[0] == BOOLEAN_NA) {
					strcpy(p_dest, NA);
					p_dest += LENGTH_NA_AS_TEXT;
				} else
					*(p_dest++) = '1';
			} else
				*(p_dest++) = '0';

			separator(rank_1, shape, idx, p_dest);

			p_t++;
		}
		*p_dest = 0;

		return 0;
	}
	default:
		return 0;
	}
}


/** Serializes a Tensor of CELL_TYPE_SINGLE or CELL_TYPE_DOUBLE as a string.

	\param p_block	The raw block to be serialized as text (must have one of the types above).
	\param p_dest	Optionally, a pointer with the address to which the output is serialized. (If nullptr, only size counting is done)
	\param p_fmt	Optionally, format specifier that is understood by sprintf (default is %f)

	\return	The length in bytes required to store the output if p_dest == nullptr

The serialization includes NA identification, commas spaces an square brackets to define the shape.
*/
int Container::tensor_float_as_text(pBlock p_block, pChar p_dest, pChar p_fmt) {
	int shape[MAX_TENSOR_RANK];
	int idx[MAX_TENSOR_RANK] = {0, 0, 0, 0, 0, 0};
	int rank_1 = p_block->rank - 1;

	p_block->get_dimensions((int *) &shape);

	if (p_dest == nullptr) {
		int total_len = p_block->rank;	// Length of opening_brackets()

		switch (p_block->cell_type) {
		case CELL_TYPE_SINGLE: {
			if (p_fmt == nullptr)
				p_fmt = (pChar) &DEF_FLOAT32_FMT;

			for (int i = 0; i < p_block->size; i++) {
				if (p_block->tensor.cell_uint[i] == SINGLE_NA_UINT32)
					total_len += LENGTH_NA_AS_TEXT + separator_len(rank_1, shape, idx);

				else {
					char cell [MAX_SIZE_OF_CELL_AS_TEXT];

					total_len += sprintf(cell, p_fmt, p_block->tensor.cell_single[i]) + separator_len(rank_1, shape, idx);
				}
			}

			return total_len + 1;
		}
		case CELL_TYPE_DOUBLE: {
			if (p_fmt == nullptr)
				p_fmt = (pChar) &DEF_FLOAT64_FMT;

			for (int i = 0; i < p_block->size; i++) {
				if (p_block->tensor.cell_ulongint[i] == DOUBLE_NA_UINT64)
					total_len += LENGTH_NA_AS_TEXT + separator_len(rank_1, shape, idx);

				else {
					char cell [MAX_SIZE_OF_CELL_AS_TEXT];

					total_len += sprintf(cell, p_fmt, p_block->tensor.cell_double[i]) + separator_len(rank_1, shape, idx);
				}
			}

			return total_len + 1;
		}
		default:
			return 0;
		}
	}

	opening_brackets(p_block->rank, p_dest);

	switch (p_block->cell_type) {
	case CELL_TYPE_SINGLE: {
		if (p_fmt == nullptr)
			p_fmt = (pChar) &DEF_FLOAT32_FMT;

		for (int i = 0; i < p_block->size; i++) {
			if (p_block->tensor.cell_uint[i] == SINGLE_NA_UINT32) {
				strcpy(p_dest, NA);
				p_dest += LENGTH_NA_AS_TEXT;

			} else
				p_dest += sprintf(p_dest, p_fmt, p_block->tensor.cell_single[i]);

			separator(rank_1, shape, idx, p_dest);
		}
		*p_dest = 0;

		return 0;
	}
	case CELL_TYPE_DOUBLE: {
		if (p_fmt == nullptr)
			p_fmt = (pChar) &DEF_FLOAT64_FMT;

		for (int i = 0; i < p_block->size; i++) {
			if (p_block->tensor.cell_ulongint[i] == DOUBLE_NA_UINT64) {
				strcpy(p_dest, NA);
				p_dest += LENGTH_NA_AS_TEXT;

			} else
				p_dest += sprintf(p_dest, p_fmt, p_block->tensor.cell_double[i]);

			separator(rank_1, shape, idx, p_dest);
		}
		*p_dest = 0;

		return 0;
	}
	default:
		return 0;
	}
}


/** Serializes a Tensor of CELL_TYPE_STRING as a string.

	\param p_block	The raw block to be serialized as text (must have one of the types above).
	\param p_dest	Optionally, a pointer with the address to which the output is serialized. (If nullptr, only size counting is done)

	\return	The length in bytes required to store the output if p_dest == nullptr

The serialization includes NA identification, double quotes, commas spaces an square brackets to define the shape.

**NOTE** that this escapes all non-vanilla ASCI characters, non printable (below 32), the blackslash and the double quote.

How UTF-8 works
---------------

The RFC http://www.ietf.org/rfc/rfc3629.txt says:

    Char. number range	| UTF-8 octet sequence
    (hexadecimal)       | (binary)
    --------------------+---------------------------------------------
    0000 0000-0000 007F | 0xxxxxxx
    0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
int Container::tensor_string_as_text(pBlock p_block, pChar p_dest) {
	int shape[MAX_TENSOR_RANK];
	int idx[MAX_TENSOR_RANK] = {0, 0, 0, 0, 0, 0};
	int rank_1 = p_block->rank - 1;

	p_block->get_dimensions((int *) &shape);

	if (p_dest == nullptr) {
		int total_len = p_block->rank;	// Length of opening_brackets()

		char *p_string;
		int  *p_t = &p_block->tensor.cell_int[0];

		for (int i = 0; i < p_block->size; i++) {
			if (p_t[0] == STRING_NA)
				total_len += LENGTH_NA_AS_TEXT + separator_len(rank_1, shape, idx);
			else if (p_t[0] == STRING_EMPTY)
				total_len += 2 + separator_len(rank_1, shape, idx);	// 2 for two double quotes
			else {
				int str_len = 2;
				p_string = p_block->get_string(i);

				while (true) {
					char lb = *(p_string++);

					if (lb == 0)
						break;

					if (lb < 7)
						str_len += 4;					// Will serialize as \xHH
					else if (lb < 14)
						str_len += 2;					// Will serialize as \a, \b, \t, \n, \v, \f or \r
					else if (lb < 32)
						str_len += 4;					// Will serialize as \xHH
					else if (lb < 34)
						str_len++;
					else if (lb < 35)
						str_len += 2;					// Will serialize as \"
					else if (lb < 92)
						str_len++;
					else if (lb < 93)
						str_len += 2;					// Will serialize as \\ .
					else if (lb < 127)
						str_len++;
					else if ((lb & 0xE0) == 0xC0) {		// utf-8 double char will serialize as \xHH\xHH
						p_string++;
						str_len += 8;
					} else if ((lb & 0xF0) == 0xE0) {	// utf-8 triple char will serialize as \xHH\xHH\xHH
						p_string += 2;
						str_len  += 12;
					} else if ((lb & 0xF8) == 0xF0) {	// utf-8 quad char will serialize as \xHH\xHH\xHH\xHH
						p_string += 3;
						str_len  += 16;
					} else
						str_len += 4;					// Will serialize as \xHH
				}

				total_len += str_len + separator_len(rank_1, shape, idx);
			}
			p_t++;
		}
		return total_len + 1;
	}

	opening_brackets(p_block->rank, p_dest);

	char *p_string;
	int  *p_t = &p_block->tensor.cell_int[0];

	for (int i = 0; i < p_block->size; i++) {
		if (p_t[0] == STRING_NA) {
			strcpy(p_dest, NA);
			p_dest += LENGTH_NA_AS_TEXT;
		} else if (p_t[0] == STRING_EMPTY) {
			*(p_dest++) = '"';
			*(p_dest++) = '"';
		} else {
			*(p_dest++) = '"';
			p_string = p_block->get_string(i);

			while (true) {
				char lb = *p_string++;

				if (lb == 0)
					break;

				if (lb < 7)
					as_hex(p_dest, lb);
				else if (lb < 14) {
					*(p_dest++) = '\\';
					*(p_dest++) = ESCAPE_LOW_ASCII[lb - 7];
				} else if (lb < 32)
					as_hex(p_dest, lb);
				else if (lb < 34)
					*(p_dest++) = lb;
				else if (lb < 35) {
					*(p_dest++) = '\\';
					*(p_dest++) = '"';
				} else if (lb < 92)
					*(p_dest++) = lb;
				else if (lb < 93) {
					*(p_dest++) = '\\';
					*(p_dest++) = '\\';
				} else if (lb < 127)
					*(p_dest++) = lb;
				else if ((lb & 0xE0) == 0xC0) {		// utf-8 double char will serialize as \xHH\xHH
					as_hex(p_dest, lb);

					lb = *p_string++;
					as_hex(p_dest, lb);
				} else if ((lb & 0xF0) == 0xE0) {	// utf-8 triple char will serialize as \xHH\xHH\xHH
					as_hex(p_dest, lb);

					lb = *p_string++;
					as_hex(p_dest, lb);

					lb = *p_string++;
					as_hex(p_dest, lb);
				} else if ((lb & 0xF8) == 0xF0) {	// utf-8 quad char will serialize as \xHH\xHH\xHH\xHH
					as_hex(p_dest, lb);

					lb = *p_string++;
					as_hex(p_dest, lb);

					lb = *p_string++;
					as_hex(p_dest, lb);

					lb = *p_string++;
					as_hex(p_dest, lb);
				} else
					as_hex(p_dest, lb);
			}

			*(p_dest++) = '"';
		}
		separator(rank_1, shape, idx, p_dest);
		p_t++;
	}
	*p_dest = 0;

	return 0;
}


/** Serializes a Tensor of CELL_TYPE_TIME as a string.

	\param p_block	The raw block to be serialized as text (must have one of the types above).
	\param p_dest	Optionally, a pointer with the address to which the output is serialized. (If nullptr, only size counting is done)
	\param p_fmt	Optionally, format specifier that is understood by sprintf (default is %Y-%m-%d %H:%M:%S)

	\return	The length in bytes required to store the output if p_dest == nullptr

The serialization includes NA identification, commas spaces an square brackets to define the shape.
*/
int Container::tensor_time_as_text(pBlock p_block, pChar p_dest, pChar p_fmt) {

	if (p_fmt == nullptr)
		p_fmt = (pChar) &DEF_FLOAT_TIME;

	int shape[MAX_TENSOR_RANK];
	int idx[MAX_TENSOR_RANK] = {0, 0, 0, 0, 0, 0};
	int rank_1 = p_block->rank - 1;

	p_block->get_dimensions((int *) &shape);

	struct tm timeinfo;

	if (p_dest == nullptr) {
		int total_len = p_block->rank;	// Length of opening_brackets()

		time_t *p_t = &p_block->tensor.cell_time[0];

		for (int i = 0; i < p_block->size; i++) {
			if (p_t[0] == TIME_POINT_NA)
				total_len += LENGTH_NA_AS_TEXT + separator_len(rank_1, shape, idx);

			else {
				if (gmtime_r(p_t, &timeinfo) == nullptr)
					return 0;

				char cell [MAX_SIZE_OF_CELL_AS_TEXT];

				total_len += strftime(cell, MAX_SIZE_OF_CELL_AS_TEXT, p_fmt, &timeinfo) + separator_len(rank_1, shape, idx);
			}
			p_t++;
		}

		return total_len + 1;
	}

	opening_brackets(p_block->rank, p_dest);

	time_t *p_t = &p_block->tensor.cell_time[0];

	for (int i = 0; i < p_block->size; i++) {
		if (p_t[0] == TIME_POINT_NA) {
			strcpy(p_dest, NA);
			p_dest += LENGTH_NA_AS_TEXT;

		} else {
			if (gmtime_r(p_t, &timeinfo) == nullptr)
				return 0;

			p_dest += strftime(p_dest, MAX_SIZE_OF_CELL_AS_TEXT, p_fmt, &timeinfo);
		}

		separator(rank_1, shape, idx, p_dest);
		p_t++;
	}

	*p_dest = 0;

	return 0;
}


/** Serializes a Tuple as a string.

	\param p_tuple	The raw Tuple to be serialized as text.
	\param p_dest	Optionally, a pointer with the address to which the output is serialized. (If nullptr, only size counting is done)
	\param p_fmt	Optionally, format specifier that will be applied to all items. (default is each type uses its default)
	\param item_len	A buffer storing the length of each item: computed by tensor_tuple_as_text(p_dest == nullptr),
					used by tensor_tuple_as_text(p_dest != nullptr).

	\return	The length in bytes required to store the output if p_dest == nullptr

The serialization includes item names and the content of each tensor as written by the tensor methods.
*/
int Container::tensor_tuple_as_text(pTuple p_tuple, pChar p_dest, pChar p_fmt, int item_len[]) {
	if (p_dest == nullptr) {
		int total_len = 1;		// 3 for opening and closing () + /0 - 2 (for the last item not having final ', ')

		ItemHeader *p_t = &p_tuple->tensor.cell_item[0];

		for (int i = 0; i < p_tuple->size; i++) {
			total_len += 7 + strlen(p_tuple->item_name(i));		// 7 == length('"" : , ')

			switch (p_t[0].cell_type) {
			case CELL_TYPE_BYTE:
			case CELL_TYPE_INTEGER:
			case CELL_TYPE_FACTOR:
			case CELL_TYPE_GRADE:
			case CELL_TYPE_LONG_INTEGER:
				total_len += item_len[i] = tensor_int_as_text(p_tuple->get_block(i), nullptr, p_fmt) - 1;

				break;

			case CELL_TYPE_BYTE_BOOLEAN:
			case CELL_TYPE_BOOLEAN:
				total_len += item_len[i] = tensor_bool_as_text(p_tuple->get_block(i), nullptr) - 1;

				break;

			case CELL_TYPE_SINGLE:
			case CELL_TYPE_DOUBLE:
				total_len += item_len[i] = tensor_float_as_text(p_tuple->get_block(i), nullptr, p_fmt) - 1;

				break;

			case CELL_TYPE_STRING:
				total_len += item_len[i] = tensor_string_as_text(p_tuple->get_block(i), nullptr) - 1;

				break;

			case CELL_TYPE_TIME:
				total_len += item_len[i] = tensor_time_as_text(p_tuple->get_block(i), nullptr, p_fmt) - 1;

				break;

			default:
				return 0;
			}

			p_t++;
		}

		return total_len;
	}

	ItemHeader *p_t = &p_tuple->tensor.cell_item[0];

	*(p_dest++) = '(';

	for (int i = 0; i < p_tuple->size; i++) {
		p_dest += sprintf(p_dest, "\"%s\" : ", p_tuple->item_name(i));

		switch (p_t[0].cell_type) {
		case CELL_TYPE_BYTE:
		case CELL_TYPE_INTEGER:
		case CELL_TYPE_FACTOR:
		case CELL_TYPE_GRADE:
		case CELL_TYPE_LONG_INTEGER:
			tensor_int_as_text(p_tuple->get_block(i), p_dest, p_fmt);
			p_dest += item_len[i];

			break;

		case CELL_TYPE_BYTE_BOOLEAN:
		case CELL_TYPE_BOOLEAN:
			tensor_bool_as_text(p_tuple->get_block(i), p_dest);
			p_dest += item_len[i];

			break;

		case CELL_TYPE_SINGLE:
		case CELL_TYPE_DOUBLE:
			tensor_float_as_text(p_tuple->get_block(i), p_dest, p_fmt);
			p_dest += item_len[i];

			break;

		case CELL_TYPE_STRING:
			tensor_string_as_text(p_tuple->get_block(i), p_dest);
			p_dest += item_len[i];

			break;

		case CELL_TYPE_TIME:
			tensor_time_as_text(p_tuple->get_block(i), p_dest, p_fmt);
			p_dest += item_len[i];

			break;
		}

		if (i < p_tuple->size - 1) {
			*(p_dest++) = ',';
			*(p_dest++) = ' ';
		}

		p_t++;
	}
	*(p_dest++) = ')';
	*p_dest = 0;

	return 0;
}


/** Serializes a Kind as a string.

	\param p_kind	The raw Kind to be serialized as text.
	\param p_dest	Optionally, a pointer with the address to which the output is serialized. (If nullptr, only size counting is done)

	\return	The length in bytes required to store the output if p_dest == nullptr

The serialization includes item names, types and shapes.
*/
int Container::tensor_kind_as_text(pKind p_kind, pChar p_dest) {

	if (p_dest == nullptr) {
		int total_len = 1;		// 3 for opening and closing {} + /0 - 2 (for the last item not having final ', ')

		ItemHeader *p_t = &p_kind->tensor.cell_item[0];

		for (int i = 0; i < p_kind->size; i++) {
			char cell [MAX_SIZE_OF_CELL_AS_TEXT];

			as_shape(p_t[0].rank, p_t[0].dim, cell, p_kind);

			total_len += 7 + strlen(p_kind->item_name(i)) + strlen(cell);		// 7 == length('"" : , ')

			switch (p_t[0].cell_type) {
			case CELL_TYPE_BYTE:
			case CELL_TYPE_TIME:
				total_len += 4;
				break;

			case CELL_TYPE_GRADE:
				total_len += 5;
				break;

			case CELL_TYPE_FACTOR:
			case CELL_TYPE_SINGLE:
			case CELL_TYPE_DOUBLE:
			case CELL_TYPE_STRING:
				total_len += 6;
				break;

			case CELL_TYPE_INTEGER:
			case CELL_TYPE_BOOLEAN:
				total_len += 7;
				break;

			case CELL_TYPE_LONG_INTEGER:
			case CELL_TYPE_BYTE_BOOLEAN:
				total_len += 12;
				break;

			default:
				return 0;
			}

			p_t++;
		}

		return total_len;
	}

	ItemHeader *p_t = &p_kind->tensor.cell_item[0];

	*(p_dest++) = '{';

	for (int i = 0; i < p_kind->size; i++) {
		p_dest += sprintf(p_dest, "\"%s\" : ", p_kind->item_name(i));

		switch (p_t[0].cell_type) {
		case CELL_TYPE_BYTE:
			strcpy(p_dest, "BYTE");
			p_dest += 4;

			break;

		case CELL_TYPE_TIME:
			strcpy(p_dest, "TIME");
			p_dest += 4;

			break;

		case CELL_TYPE_GRADE:
			strcpy(p_dest, "GRADE");
			p_dest += 5;

			break;

		case CELL_TYPE_FACTOR:
			strcpy(p_dest, "FACTOR");
			p_dest += 6;

			break;

		case CELL_TYPE_SINGLE:
			strcpy(p_dest, "SINGLE");
			p_dest += 6;

			break;

		case CELL_TYPE_DOUBLE:
			strcpy(p_dest, "DOUBLE");
			p_dest += 6;

			break;

		case CELL_TYPE_STRING:
			strcpy(p_dest, "STRING");
			p_dest += 6;

			break;

		case CELL_TYPE_INTEGER:
			strcpy(p_dest, "INTEGER");
			p_dest += 7;

			break;

		case CELL_TYPE_BOOLEAN:
			strcpy(p_dest, "BOOLEAN");
			p_dest += 7;

			break;

		case CELL_TYPE_LONG_INTEGER:
			strcpy(p_dest, "LONG_INTEGER");
			p_dest += 12;

			break;

		case CELL_TYPE_BYTE_BOOLEAN:
			strcpy(p_dest, "BYTE_BOOLEAN");
			p_dest += 12;

			break;
		}
		p_dest = as_shape(p_t[0].rank, p_t[0].dim, p_dest, p_kind);

		if (i < p_kind->size - 1) {
			*(p_dest++) = ',';
			*(p_dest++) = ' ';
		}
		p_t++;
	}
	*(p_dest++) = '}';
	*p_dest = 0;

	return 0;
}


#ifdef CATCH_TEST

void compare_full_blocks(pBlock p_bl1, pBlock p_bl2, bool skip_value_check) {

	REQUIRE(p_bl1->cell_type == p_bl2->cell_type);
	REQUIRE(p_bl1->size == p_bl2->size);
	REQUIRE(p_bl1->rank == p_bl2->rank);

	for (int i = 0; i < p_bl1->rank; i++) {
		REQUIRE(p_bl1->range.dim[i] == p_bl2->range.dim[i]);
	}

	bool all_cells_equal = true;

	switch (p_bl1->cell_type) {
	case CELL_TYPE_BYTE:
	case CELL_TYPE_BYTE_BOOLEAN:
		for (int i = 0; i < p_bl1->size; i++) {
			if (p_bl1->tensor.cell_byte[i] != p_bl2->tensor.cell_byte[i]) {
				all_cells_equal = false;

				break;
			}
		}
		break;

	case CELL_TYPE_INTEGER:
	case CELL_TYPE_FACTOR:
	case CELL_TYPE_GRADE:
	case CELL_TYPE_BOOLEAN:
		for (int i = 0; i < p_bl1->size; i++) {
			if (p_bl1->tensor.cell_uint[i] != p_bl2->tensor.cell_uint[i]) {
				all_cells_equal = false;

				break;
			}
		}
		break;

	case CELL_TYPE_LONG_INTEGER:
	case CELL_TYPE_TIME:
		for (int i = 0; i < p_bl1->size; i++) {
			if (p_bl1->tensor.cell_ulongint[i] != p_bl2->tensor.cell_ulongint[i]) {
				all_cells_equal = false;

				break;
			}
		}
		break;

	case CELL_TYPE_SINGLE:
		for (int i = 0; i < p_bl1->size; i++) {
			if (p_bl1->tensor.cell_uint[i] == SINGLE_NA_UINT32) {
				if (p_bl2->tensor.cell_uint[i] != SINGLE_NA_UINT32)
					all_cells_equal = false;
			} else {
				if (p_bl2->tensor.cell_uint[i] == SINGLE_NA_UINT32)
					all_cells_equal = false;
				if (fabs(p_bl1->tensor.cell_single[i] - p_bl2->tensor.cell_single[i]) > 1e-5)
					all_cells_equal = false;
			}
		}
		break;

	case CELL_TYPE_DOUBLE:
		for (int i = 0; i < p_bl1->size; i++) {
			if (p_bl1->tensor.cell_ulongint[i] == DOUBLE_NA_UINT64) {
				if (p_bl2->tensor.cell_ulongint[i] != DOUBLE_NA_UINT64)
					all_cells_equal = false;
			} else {
				if (p_bl2->tensor.cell_ulongint[i] == DOUBLE_NA_UINT64)
					all_cells_equal = false;
				if (fabs(p_bl1->tensor.cell_double[i] - p_bl2->tensor.cell_double[i]) > 1e-9)
					all_cells_equal = false;
			}
		}
		break;

	case CELL_TYPE_STRING:
		for (int i = 0; i < p_bl1->size; i++) {
			if (strcmp(p_bl1->get_string(i), p_bl2->get_string(i)) != 0) {
				all_cells_equal = false;

				break;
			}
		}
		break;

	case CELL_TYPE_KIND_ITEM:
		for (int i = 0; i < p_bl1->size; i++) {
			if (p_bl1->tensor.cell_item[i].cell_type != p_bl2->tensor.cell_item[i].cell_type)
				all_cells_equal = false;

			if (p_bl1->tensor.cell_item[i].rank != p_bl2->tensor.cell_item[i].rank)
				all_cells_equal = false;

			if (strcmp(reinterpret_cast<pKind>(p_bl1)->item_name(i), reinterpret_cast<pKind>(p_bl2)->item_name(i)) != 0)
				all_cells_equal = false;

			for (int j = 0; j < p_bl1->tensor.cell_item[i].rank; j++) {
				int d1 = p_bl1->tensor.cell_item[i].dim[j];
				int d2 = p_bl2->tensor.cell_item[i].dim[j];

				if (d1 > 0) {
 					if (d2 != d1)
						all_cells_equal = false;
				} else if (d2 >= 0)
					all_cells_equal = false;
				else {
					pChar nd1 = (&p_bl1->p_string_buffer()->buffer[-d1]);
					pChar nd2 = (&p_bl2->p_string_buffer()->buffer[-d2]);

					if (strcmp(nd1, nd2) != 0)
						all_cells_equal = false;
				}
			}
		}
		break;

	case CELL_TYPE_TUPLE_ITEM:
		for (int i = 0; i < p_bl1->size; i++) {
			if (p_bl1->tensor.cell_item[i].cell_type != p_bl2->tensor.cell_item[i].cell_type)
				all_cells_equal = false;

			if (p_bl1->tensor.cell_item[i].rank != p_bl2->tensor.cell_item[i].rank)
				all_cells_equal = false;

			if (strcmp(reinterpret_cast<pKind>(p_bl1)->item_name(i), reinterpret_cast<pKind>(p_bl2)->item_name(i)) != 0)
				all_cells_equal = false;

			compare_full_blocks(reinterpret_cast<pTuple>(p_bl1)->get_block(i), reinterpret_cast<pTuple>(p_bl1)->get_block(i));
		}
		break;

	default:
		REQUIRE(strcmp("HALT:", "Wrong cell_type.") == 0);		// cppcheck-suppress staticStringCompare
	}

	if (!skip_value_check)
		REQUIRE(all_cells_equal);
}

ConfigFile CONFIG(JAZZ_DEFAULT_CONFIG_PATH);
Logger	   LOGGER(CONFIG, "LOGGER_PATH");
Container  CNT(&LOGGER, &CONFIG);

#endif

} // namespace jazz_elements

#ifdef CATCH_TEST
#include "src/jazz_elements/tests/test_container.ctest"
#endif
