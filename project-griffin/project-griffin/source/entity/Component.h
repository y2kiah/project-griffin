#pragma once
#ifndef GRIFFIN_COMPONENT_H_
#define GRIFFIN_COMPONENT_H_

#include <cstdint>
#include "utility/reflection.h"

// disable BOOST_PP_EXPAND_I warning that showed up in 1.57
// see this thread http://comments.gmane.org/gmane.comp.lib.boost.user/82952
#pragma warning(disable:4003)


/**
* Outputs type and name of the field
*/
#define EACH_FIELD(r, data, elem) \
	BOOST_PP_LIST_AT(elem, 0) BOOST_PP_LIST_AT(elem, 1);

//#define SET_FIELD(r, data, i, elem) \
//	this->BOOST_PP_LIST_AT(elem, 1) = _c.BOOST_PP_LIST_AT(elem, 1);

/**
* This is the component struct definition. It starts with a static id mapping its type to the
* ComponentType enum that represents it. Next is the listing of fields themselves. Finally, there
* is the reflection macro that adds a static Reflection class containing lots of goodies.
* Component declaration format:
*	COMPONENT(component name,
		(type, (name, (FieldType enum, (string description, NIL)))),
		...
	)
*	where the enum must match the type, used for serialization, and the description is just
*	metadata used in the reflection system, useful for tools development
*	
* Example component declaration:
*	COMPONENT(Transform,
*		(glm::dvec3,		(position,		(FieldType::dvec3_T,		("3D position coordinate", NIL)))),
*		(glm::quat,			(orientation,	(FieldType::quat_T,			("Orientation quaternion", NIL))))
*	)
*/
#define COMPONENT(name, ...) \
	struct name { \
		static const ComponentType componentType = ComponentType::name##_T; \
		FOR_EACH(EACH_FIELD, __VA_ARGS__) \
		REFLECT(name, __VA_ARGS__) \
	};

/**
* Simply declares the component struct
*/
#define COMPONENT_DECLARATION(r, data, name) \
	struct STRIP(name);

/**
* Defines a templated type from its integer id for the component_cast
*/
#define COMPONENT_CAST(r, data, name) \
	template <> \
	struct component_type<ComponentType::STRIP(name)_T> { \
		typedef STRIP(name) type; \
	};

/**
* Forward declares all component structs, then an enum to give each a unique integer id, then a
* templated type cast by id
*/
#define COMPONENT_LIST(SEQ) \
	template <uint16_t Ct> \
	struct component_type { \
		typedef void type; \
	}; \
	\
	BOOST_PP_SEQ_FOR_EACH(COMPONENT_DECLARATION, , SEQ) \
	\
	MakeEnum(ComponentType, uint16_t, SEQ, _T) \
	\
	BOOST_PP_SEQ_FOR_EACH(COMPONENT_CAST, , SEQ) \
	\
	template <typename T, uint16_t Ct> \
	auto component_cast(T x) -> decltype(component_type<Ct>::type) \
	{ \
		return reinterpret_cast<component_type<Ct>::type>(x); \
	}

extern void test_reflection(); // TEMP

#endif