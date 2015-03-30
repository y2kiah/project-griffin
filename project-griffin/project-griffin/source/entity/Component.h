#pragma once
#ifndef GRIFFIN_COMPONENT_H_
#define GRIFFIN_COMPONENT_H_

#include "utility/reflection.h"
#include <SDL_log.h>

#define COMPONENT_STORE_RESERVE	100

#define EACH_FIELD(r, data, elem) \
	BOOST_PP_LIST_AT(elem, 0) BOOST_PP_LIST_AT(elem, 1);

//#define SET_FIELD(r, data, i, elem) \
//	this->BOOST_PP_LIST_AT(elem, 1) = _c.BOOST_PP_LIST_AT(elem, 1);

/**
 * This is the component struct definition. It starts with a static id mapping its type to the
 * ComponentType enum that represents it. Next is the listing of fields themselves. Finally, there
 * is the reflection macro that adds a static Reflection class containing lots of goodies.
 */
#define COMPONENT(name, ...) \
	struct name { \
		static const ComponentType componentType = ComponentType::name##_T; \
		FOR_EACH(EACH_FIELD, __VA_ARGS__) \
		REFLECT(name, __VA_ARGS__) \
	};

#define COMPONENT_DECLARATION(r, data, name) \
	struct STRIP(name);

#define COMPONENT_LIST(SEQ) \
	BOOST_PP_SEQ_FOR_EACH(COMPONENT_DECLARATION, , SEQ) \
	\
	MakeEnum(ComponentType, uint16_t, SEQ, _T)

extern void test_reflection(); // TEMP

#endif