/**
 * @file	AutoLister.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef AUTOLISTER_H
#define AUTOLISTER_H

#include <vector>
#include <cassert>

using std::vector;

/**
 * @class AutoLister
 */
template <typename T>
class AutoLister {
public:
	AutoLister() {
		m_next = head();
		head() = static_cast<const T*>(this);
	}

	~AutoLister() {
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
* @class AutoStacker
*/
template <typename T>
class AutoStacker {
public:
	AutoStacker() {
		getStack().emplace_back(static_cast<T*>(this));
	}

	~AutoStacker() {
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


#endif