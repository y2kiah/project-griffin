/**
 * @file	AutoLister.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_AUTOLISTER_H
#define GRIFFIN_AUTOLISTER_H

#include <vector>
#include <cassert>

using std::vector;

namespace griffin {

	/**
	 * @class auto_lister
	 */
	template <typename T>
	class auto_lister {
	public:
		auto_lister() {
			m_next = head();
			head() = static_cast<const T*>(this);
		}

		~auto_lister() {
			// must implement
			// look for this in the linked list and remove it
		}

		const T* next() const { return m_next; }

		static const T*& head() {
			static const T* s_head = 0;
			return s_head;
		}

	private:
		const T* m_next;
	};


	/**
	 * @class auto_stacker
	 */
	template <typename T>
	class auto_stacker {
	public:
		auto_stacker() {
			getStack().emplace_back(static_cast<T*>(this));
		}

		~auto_stacker() {
			assert(getStack().back() == this && "AutoStacker last element mismatch. AutoStacker must be used as a stack.");
			getStack().pop_back();
		}

		const vector<T*>& stack() const {
			return getStack();
		}

	private:
		vector<T*>& getStack() const {
			static vector<T*> s_stack;
			return s_stack;
		}
	};

}
#endif