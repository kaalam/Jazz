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


#include "src/jazz_core/base_api.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_LOCATORS
#define INCLUDED_JAZZ_BEBOP_LOCATORS


/** \brief This file has wrappers to convert static C structures that make part of the Space interface into C++ stdlib containers.

	The classes are: stdNames (a vector of stdName which is just a Name with some operators), stdLocators (a vector of stdLocator
	which is just a Locator with some operators), stdAPIQueryStates (a vector of stdAPIQueryState which is just an ApiQueryState
	with some operators) and stdUrls (a vector of String (an std::string)).
*/
namespace jazz_core
{

/** \brief A wrapped Name that supports being stacked in an std::vector and used as a key in an std::map.
*/
class stdName {

	public:
		stdName() {}

		/** \brief Constructor from cChar.

			\param name	A C string with a name.
		*/
		stdName(const pChar &name) {
			strncpy(this->name, name, NAME_SIZE);
			this->name[NAME_LENGTH] = 0;
		}

		/** \brief Copy constructor for stdName.

			\param name	The stdName to copy.
		*/
		stdName(const stdName &name) {
			memcpy(&this->name, &name, sizeof(stdName));
		}

		/** \brief Operator name == o.name.

			\param o	The object to compare with.

			\return True if the names are equal.
		*/
		bool operator==(const stdName &o) const {
			return strcmp(name, o.name) == 0;
		}

		/** \brief Operator name < o.name.

			\param o	The object to compare with.

			\return True if the name is less than o.name.
		*/
		bool operator<(const stdName &o) const {
			return strcmp(name, o.name) < 0;
		}

		Name name;				///< The name
};

typedef std::vector<stdName> stdNames;	///< A vector of stdName

/** \brief A wrapped Locator that supports being stacked in an std::vector and used as a key in an std::map.
*/

class stdLocator {

	public:
		stdLocator() {}

		/** \brief Constructor from cLocator.

			\param locator	A C Locator.
		*/
		stdLocator(const Locator &locator) {
			memcpy(&this->locator, &locator, sizeof(Locator));
		}

		/** \brief Copy constructor for stdLocator.

			\param locator	The stdLocator to copy.
		*/
		stdLocator(const stdLocator &locator) {
			memcpy(&this->locator, &locator, sizeof(stdLocator));
		}

		/** \brief Operator locator == o.locator.

			\param o	The object to compare with.

			\return True if the locators are equal.
		*/
		bool operator==(const stdLocator &o) const {
			return memcmp(&locator, &o.locator, sizeof(Locator)) == 0;
		}

		/** \brief Operator locator < o.locator.

			\param o	The object to compare with.

			\return True if the locator is less than o.locator.
		*/
		bool operator<(const stdLocator &o) const {
			return memcmp(&locator, &o.locator, sizeof(Locator)) < 0;
		}

		Locator locator;				///< The locator
};

typedef std::vector<stdLocator> stdLocators;	///< A vector of stdLocator

/** \brief A wrapped ApiQueryState that supports being stacked in an std::vector.
*/
class stdAPIQueryState {

	public:
		stdAPIQueryState() {}

		/** \brief Constructor from cApiQueryState.

			\param api_query_state	A C ApiQueryState.
		*/
		stdAPIQueryState(const ApiQueryState &api_query_state) {
			memcpy(&this->api_query_state, &api_query_state, sizeof(ApiQueryState));
		}

		/** \brief Copy constructor for stdAPIQueryState.

			\param api_query_state	The stdAPIQueryState to copy.
		*/
		stdAPIQueryState(const stdAPIQueryState &api_query_state) {
			memcpy(&this->api_query_state, &api_query_state, sizeof(stdAPIQueryState));
		}

		/** \brief Operator api_query_state == o.api_query_state.

			\param o	The object to compare with.

			\return True if the ApiQueryStates are equal.
		*/
		bool operator==(const stdAPIQueryState &o) const {
			return memcmp(&api_query_state, &o.api_query_state, sizeof(ApiQueryState)) == 0;
		}

		/** \brief Operator api_query_state < o.api_query_state.

			\param o	The object to compare with.

			\return True if the ApiQueryState is less than o.api_query_state.
		*/
		bool operator<(const stdAPIQueryState &o) const {
			return memcmp(&api_query_state, &o.api_query_state, sizeof(ApiQueryState)) < 0;
		}

		ApiQueryState api_query_state;	///< The API query state
};

typedef std::vector<stdAPIQueryState> stdAPIQueryStates;	///< A vector of stdAPIQueryState

typedef std::vector<String> stdUrls;						///< A vector of String (an std::string)

} // namespace jazz_core

#endif // ifndef INCLUDED_JAZZ_BEBOP_LOCATORS
