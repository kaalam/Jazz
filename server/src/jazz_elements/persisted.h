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


#include "src/jazz_elements/volatile.h"

#ifdef CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_ELEMENTS_PERSISTED
#define INCLUDED_JAZZ_ELEMENTS_PERSISTED

#include "src/lmdb/lmdb.h"


namespace jazz_elements
{

#define MAX_POSSIBLE_SOURCES				32				///< Number of databases: configurable via MDB_ENV_SET_MAXDBS, but not above.
#define MAX_LMDB_HOME_LEN				   128				///< Number of chars for the LMDB home path
#define LMDB_UNIX_FILE_PERMISSIONS	      0664				///< The file permissions (as in chmod) for the database files
#define INVALID_MDB_DBI				0xefefEFEF				///< A constant to flag invalid MDB_dbi handle values


// Bit masks to trigger LMDB failures in Persisted wrappers during tests.
#define TRIGGER_FAIL_MDB_ENV_CREATE			(1u << 0)		///< Trigger a failure in mdb_env_create() to test error handling.
#define TRIGGER_FAIL_MDB_ENV_SET_MAXREADERS	(1u << 1)		///< Trigger a failure in mdb_env_set_maxreaders() to test error handling.
#define TRIGGER_FAIL_MDB_ENV_SET_MAXDBS		(1u << 2)		///< Trigger a failure in mdb_env_set_maxdbs() to test error handling.
#define TRIGGER_FAIL_MDB_ENV_SET_MAPSIZE	(1u << 3)		///< Trigger a failure in mdb_env_set_mapsize() to test error handling.
#define TRIGGER_FAIL_MDB_ENV_OPEN			(1u << 4)		///< Trigger a failure in mdb_env_open() to test error handling.
#define TRIGGER_FAIL_MDB_ENV_SYNC			(1u << 5)		///< Trigger a failure in mdb_env_sync() to test error handling.
#define TRIGGER_FAIL_MDB_TXN_BEGIN			(1u << 6)		///< Trigger a failure in mdb_txn_begin() to test error handling.
#define TRIGGER_FAIL_MDB_DBI_OPEN			(1u << 7)		///< Trigger a failure in mdb_dbi_open() to test error handling.
#define TRIGGER_FAIL_MDB_PUT				(1u << 8)		///< Trigger a failure in mdb_put() to test error handling.
#define TRIGGER_FAIL_MDB_TXN_COMMIT			(1u << 9)		///< Trigger a failure in mdb_txn_commit() to test error handling.
#define TRIGGER_FAIL_MDB_DEL				(1u << 10)		///< Trigger a failure in mdb_del() to test error handling.
#define TRIGGER_FAIL_MDB_GET				(1u << 11)		///< Trigger a failure in mdb_get() to test error handling.
#define TRIGGER_FAIL_MDB_CURSOR_OPEN		(1u << 12)		///< Trigger a failure in mdb_cursor_open() to test error handling.
#define TRIGGER_FAIL_MDB_CURSOR_GET			(1u << 13)		///< Trigger a failure in mdb_cursor_get() to test error handling.
#define TRIGGER_FAIL_MDB_DROP				(1u << 14)		///< Trigger a failure in mdb_drop() to test error handling.


/** \brief All the necessary LMDB options (a binary representation of the values in the config file)
*/
struct JazzLmdbOptions {
	char path[MAX_LMDB_HOME_LEN];			///< The path to the LMDB home directory

	int env_set_mapsize;					///< The size of the memory map as defined in configuration key MDB_ENV_SET_MAPSIZE
	int	env_set_maxreaders;					///< The maximum number of reader slots as defined in configuration key MDB_ENV_SET_MAXREADERS
	int	env_set_maxdbs;						///< The maximum number of databases as defined in configuration key MDB_ENV_SET_MAXDBS
	int	flags;								///< The flags as defined in many configuration keys MDB_FIXEDMAP, .. MDB_NOMEMINIT
};


typedef std::map <String, MDB_dbi> DBImap;	///< The lmdb MDB_dbi handles for each source.
typedef MDB_txn *pMDB_txn;					///< A pointer to a MDB_txn structure which is what mdb_txn_begin() returns.


/** \brief Persisted: A Service to manage data objects in LMDB.

This Container implements the full crud (.get(), .header(), .put(), .new_entity(), .remove(), .copy()) interface storing blocks
in LMDB tables.

For reference:
--------------

1. See Container and each of the crud methods in this class.
2. Each entity (whose name is parsed by Container.as_locator()) is a named LMDB (dbi) database.
3. Outside Jazz, the database files can be managed (even while running) with LMDB utils: mdb_copy, mdb_drop, mdb_dump, mdb_load & mdb_stat
4. For how LMDB is used, see http://www.lmdb.tech/doc/ for coding reference.
5. For specific details, that may be experimented with, see the config file: server/config/jazz_config.ini

*/
class Persisted : public Container {

	public:

		Persisted(pLogger a_logger, pConfigFile a_config);
	   ~Persisted();

		virtual pChar const id();

		StatusCode start	();
		StatusCode shut_down();

		// The easy interface (Requires explicit pulling because of the native interface using the same names.)

		using Container::get;
		using Container::header;
		using Container::put;
		using Container::new_entity;
		using Container::remove;
		using Container::copy;

		// The "native" interface

		virtual StatusCode get		 (pTransaction		&p_txn,
									  Locator			&what);
		virtual StatusCode get		 (pTransaction		&p_txn,
									  Locator			&what,
									  pBlock			 p_row_filter);
		virtual StatusCode get		 (pTransaction		&p_txn,
							  		  Locator			&what,
							  		  pChar				 name);
		virtual StatusCode header	 (StaticBlockHeader	&hea,
									  Locator			&what);
		virtual StatusCode header	 (pTransaction		&p_txn,
									  Locator			&what);
		virtual StatusCode put		 (Locator			&where,
									  pBlock			 p_block,
									  int				 mode = WRITE_AS_FULL_BLOCK);
		virtual StatusCode new_entity(Locator			&where);
		virtual StatusCode remove	 (Locator			&where);
		virtual StatusCode copy		 (Locator			&where,
									  Locator			&what);

		// Support for container names in the BaseAPI .base_names()

		virtual void base_names(BaseNames &base_names);
		bool		 dbi_exists(Name	   dbi_name);

		/**	\brief Check if the service is running.

			\return True if the service is running.
		*/
		inline bool is_running() {
			return lmdb_env != nullptr;
		}

#ifndef CATCH_TEST
	private:
#endif

		// Hot LMDB get

		pBlock lock_pointer_to_block(Locator &what, pMDB_txn &p_txn);
		void   done_pointer_to_block(pMDB_txn &p_txn);

		// Internal dbi management

		bool open_all_databases	();
		void close_all_databases();
		StatusCode new_database	  (pChar name);
		StatusCode remove_database(pChar name);

		// Logger with full messages for lmdb errors.

		void log_lmdb_err(int loglevel, int lmdb_err, const char *msg);

#ifdef CATCH_TEST
		// LMDB wrappers for tests, allowing deterministic failure injection.
		int mdb_env_create		   (MDB_env **env);
		int mdb_env_set_maxreaders (MDB_env *env,
									unsigned int readers);
		int mdb_env_set_maxdbs	   (MDB_env *env,
									MDB_dbi dbs);
		int mdb_env_set_mapsize	   (MDB_env *env,
									mdb_size_t size);
		int mdb_env_open		   (MDB_env *env,
									const char *path,
									unsigned int flags,
									mdb_mode_t mode);
		int mdb_env_sync		   (MDB_env *env,
									int force);
		int mdb_txn_begin		   (MDB_env *env,
									MDB_txn *parent,
									unsigned int flags,
									MDB_txn **txn);
		int mdb_dbi_open		   (MDB_txn *txn,
									const char *name,
									unsigned int flags,
									MDB_dbi *dbi);
		int mdb_put				   (MDB_txn *txn,
									MDB_dbi dbi,
									MDB_val *key,
									MDB_val *data,
									unsigned int flags);
		int mdb_txn_commit		   (MDB_txn *txn);
		int mdb_del				   (MDB_txn *txn,
									MDB_dbi dbi,
									MDB_val *key,
									MDB_val *data);
		int mdb_get				   (MDB_txn *txn,
									MDB_dbi dbi,
									MDB_val *key,
									MDB_val *data);
		int mdb_cursor_open		   (MDB_txn *txn,
									MDB_dbi dbi,
									MDB_cursor **cursor);
		int mdb_cursor_get		   (MDB_cursor *cursor,
									MDB_val *key,
									MDB_val *data,
									MDB_cursor_op op);
		int mdb_drop			   (MDB_txn *txn,
									MDB_dbi dbi,
									int del);

		uint32_t debug_trigger_failure = 0;
#endif

		DBImap			 source_dbi = {};		///< The lmdb MDB_dbi handles for each source.
		JazzLmdbOptions  lmdb_opt;				///< The LMDB options
		MDB_env		    *lmdb_env = nullptr;	///< The LMDB environment
};
typedef Persisted *pPersisted;					///< A pointer to a Persisted object

// Instancing Persisted
// --------------------

#ifdef CATCH_TEST

extern Persisted PER;

#endif

} // namespace jazz_elements

#endif // ifndef INCLUDED_JAZZ_ELEMENTS_PERSISTED
