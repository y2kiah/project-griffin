/**
* @file Component.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_COMPONENT_H_
#define GRIFFIN_COMPONENT_H_

#include <cstdint>
#include "utility/reflection.h"

// disable BOOST_PP_EXPAND_I warning that showed up in 1.57
// see this thread http://comments.gmane.org/gmane.comp.lib.boost.user/82952
#pragma warning(disable:4003)

namespace griffin {
	namespace entity {

		/**
		* Outputs type and name of the field
		*/
		#define EACH_FIELD(r, data, i, tuple) \
			PROP(tuple,0) PROP(tuple,1)PROP(tuple,2);

		
		/**
		* This is the component struct definition. It starts with a static id mapping its type to the
		* ComponentType enum that represents it. Next is the listing of fields themselves. Finally, there
		* is the reflection macro that adds a static Reflection class containing lots of goodies.
		* Component declaration format:
		*	COMPONENT(component_name,
				(type, name, postfix, description),
				...
			)
		*	where the enum must match the type, used for serialization, and the description is just
		*	metadata used in the reflection system, useful for tools development
		*	
		* Example component declaration:
		*	COMPONENT(Transform,
		*		(glm::dvec3,	position,,		"3D position coordinate"),
		*		(glm::quat,		orientation,,	"Orientation quaternion"),
		*		(char,			name,[32],		"name of the transform node")
		*	)
		*/
		#define COMPONENT(name, ...) \
			struct name { \
				static const griffin::entity::ComponentType componentType = griffin::entity::ComponentType::name##_T; \
				FOR_EACH_I(EACH_FIELD, , __VA_ARGS__) \
				\
				struct Reflection; \
			}; \
			\
			REFLECT(name::Reflection, name, __VA_ARGS__)

		/**
		* Simply declares the component struct
		*/
		#define COMPONENT_DECLARATION(r, data, i, name) \
			struct STRIP(name);

		/**
		* Defines a templated type from its integer id for the component_cast
		*/
		#define COMPONENT_CAST(r, data, i, name) \
			template <> \
			struct component_type<ComponentType::STRIP(name)_T> { \
				typedef STRIP(name) type; \
			};

		/**
		* Forward declares all component structs, then an enum to give each a unique integer id, then a
		* templated type cast by id
		*/
		#define COMPONENT_LIST(...) \
			FOR_EACH_I(COMPONENT_DECLARATION, , __VA_ARGS__) \
			\
			MakeEnum(ComponentType, uint16_t, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), _T) \
			\
			FOR_EACH_I(COMPONENT_CAST, , __VA_ARGS__)


		template <uint16_t Ct>
		struct component_type {
			typedef void type;
		};

		template <typename T, uint16_t Ct>
		auto component_cast(T x) -> decltype(component_type<Ct>::type)
		{
			return reinterpret_cast<component_type<Ct>::type>(x);
		}



		extern void test_reflection(); // TEMP

	}
}

#endif