#pragma once
#ifndef _ENUM_H
#define _ENUM_H

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
	const int name##Count = BOOST_PP_SEQ_SIZE(SEQ); \
	static const char* name##ToString(const enum name eel) { \
		switch (eel) { \
			BOOST_PP_SEQ_FOR_EACH(ENUM_TO_STRING, postfix, SEQ) \
			default: return ""; \
		} \
	} \
	static enum name name##ToEnum(const std::string &eel) { \
		BOOST_PP_SEQ_FOR_EACH(STRING_TO_ENUM, postfix, SEQ) \
		return (enum name)0; \
	}

#endif //_ENUM_H