#pragma once
#ifndef GRIFFIN_SCENE_API_H_
#define GRIFFIN_SCENE_API_H_

#include <utility/export.h>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// begin inclusion in lua FFI declaration
#define ffi
#ifdef ffi
	//typedef void(*Callback_T)(griffin_FrameMappedInput*);

	// Functions

	GRIFFIN_EXPORT
	uint64_t griffin_scene_createScene(const char name[32], bool makeActive);

	

#endif ffi
#undef ffi
// end inclusion in lua FFI declaration

#ifdef __cplusplus
}
#endif

#endif