#pragma once
#ifndef GRIFFIN_INPUT_SYSTEM_API_H_
#define GRIFFIN_INPUT_SYSTEM_API_H_

#include <utility/export.h>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

	GRIFFIN_EXPORT
	uint64_t griffin_input_createContext();

#ifdef __cplusplus
}
#endif

#endif