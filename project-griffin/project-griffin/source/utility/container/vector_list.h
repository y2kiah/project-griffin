/**
* improves the cache locality of a list by storing all items in a contiguous vector, rather than
* allocating each list entry at random locations on the heap
*/

#pragma once

#include <vector>

namespace griffin {
	namespace container {

		/**
		 *
		 */
		template <typename T>
		class vector_list {
		public:
			explicit vector_list() {}

			void push_back(T&& i) {

			}

		private:
			struct internal_T {
				internal_T *previous;
				internal_T *next;
				T item;
			};

			std::vector<internal_T>	m_list;
		};

	}
}