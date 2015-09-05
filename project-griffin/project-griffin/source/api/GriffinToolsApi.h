#pragma once
#ifndef GRIFFIN_TOOLS_API_H_
#define GRIFFIN_TOOLS_API_H_

#include <utility/export.h>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// begin inclusion in lua FFI declaration
#define ffi
#ifdef ffi

	// Functions

	GRIFFIN_EXPORT
	uint64_t griffin_tools_importMesh(const char* filename, bool optimizeGraph, bool preTransformVertices, bool flipUVs);

	GRIFFIN_EXPORT
	bool griffin_tools_saveMesh(uint64_t mesh, const char* filename);

	GRIFFIN_EXPORT
	uint64_t griffin_tools_convertMesh(const char* sourceFilename, const char* destFilename,
									   bool optimizeGraph, bool preTransformVertices, bool flipUVs);

#endif ffi
#undef ffi
// end inclusion in lua FFI declaration

#ifdef __cplusplus
}
#endif

#endif