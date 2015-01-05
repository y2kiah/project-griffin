/**
* improves on the cache locality of a list by storing all items in a contiguous vector, rather
* than allocating each list entry at non-contiguous locations on the heap.
* 
* Will need to implement custom iterators for this container to be useful.
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

			void push_back(T&& val)
			{
				m_list.emplace_back(m_back, -1, std::forward<T>(val));
				int newBack = m_list.size() - 1;
				
				if (m_back > -1) {
					// previous back gets this as its "next"
					m_list[m_back].next = newBack;
				}

				// new back becomes this
				m_back = newBack;
				
				// if the list was empty, this is also the front
				if (m_front == -1) {
					m_front = newBack;
				}
			}

			void push_front(T&& val)
			{
				m_list.emplace_back(-1, m_front, std::forward<T>(val));
				int newFront = m_list.size() - 1;

				if (m_front > -1) {
					m_list[m_front].previous = newFront;
				}

				m_front = newFront;

				if (m_back == -1) {
					m_back = newFront;
				}
			}

			void pop_back()
			{
			}

		private:
			struct item_T {
				int				previous;
				int				next;
				T				item;
			};

			int					m_front = -1;
			int					m_back  = -1;
			std::vector<item_T>	m_list;
		};

	}
}