#pragma once
#ifndef ENUM_H
#define ENUM_H

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <string>

#define STRIP(x) x
#define STRINGIFY(x) #x
#define ENUM_VAL(r, postfix, elem)	STRIP(elem)postfix,

#define ENUM_TO_STRING(r, postfix, elem) \
	case STRIP(elem)postfix: return STRINGIFY(elem);

#define STRING_TO_ENUM(r, postfix, elem) \
	if (STRINGIFY(elem) == eel) { return STRIP(elem)postfix; }

#define MakeEnum(name, type, SEQ, postfix) \
	enum name : type { \
		BOOST_PP_SEQ_FOR_EACH(ENUM_VAL, postfix, SEQ) \
		last_##name##_enum \
	}; \
	static const int name##Count = BOOST_PP_SEQ_SIZE(SEQ); \
	static const char* name##ToString(const enum name eel) { \
		switch (eel) { \
			BOOST_PP_SEQ_FOR_EACH(ENUM_TO_STRING, postfix, SEQ) \
			default: { throw(std::runtime_error("enum value not found")); } \
		} \
	} \
	static enum name name##ToEnum(const std::string &eel) { \
		BOOST_PP_SEQ_FOR_EACH(STRING_TO_ENUM, postfix, SEQ) \
		throw(std::runtime_error("enum value not found")); \
	}

#endif //_ENUM_H