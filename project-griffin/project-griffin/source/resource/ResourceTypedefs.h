/**
* @file	ResourceTypedefs.h
* @author	Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_RESOURCETYPEDEFS_H_
#define GRIFFIN_RESOURCETYPEDEFS_H_

#include <memory>

namespace griffin {
	namespace resource {
		class Resource_T;
		typedef std::shared_ptr<Resource_T> ResourcePtr;

		class IResourceSource;
		typedef std::shared_ptr<IResourceSource> IResourceSourcePtr;

		class ResourceCache;
		typedef std::shared_ptr<ResourceCache> ResourceCachePtr;

		typedef std::unique_ptr<unsigned char[]> DataPtr;
	}
}

#endif