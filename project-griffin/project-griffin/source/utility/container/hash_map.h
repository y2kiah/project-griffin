/**
* @file	hash_map.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_HASH_MAP_H_
#define GRIFFIN_HASH_MAP_H_

#include "handle_map.h"

namespace griffin {

	/**
	* @struct hash_map_meta
	*/
	struct hash_map_meta {
		uint32_t	denseToSparse;	//!< index into m_sparseIds array stored in handle_map::m_meta
	};


	/**
	* @class hash_map
	*	Stores objects using a dense inner array and sparse outer array utilizing the handle_map.
	*
	* @tparam	T		type of item to be stored
	*/
	template <typename T>
	class hash_map {
	public:

	private:
		handle_map<T, hash_map_meta>

	};

}

#include "impl/hash_map-inl.h"

#endif