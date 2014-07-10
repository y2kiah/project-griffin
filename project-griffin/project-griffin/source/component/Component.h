#pragma once
#ifndef COMPONENT_H
#define COMPONENT_H

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

//#define COMPONENT_STORE_LIST(r, data, name) \
//	ComponentStore<STRIP(name)> STRIP(name)##Store;
//
//#define COMPONENT_STORE_INIT(r, data, i, name) \
//	BOOST_PP_COMMA_IF(BOOST_PP_NOT_EQUAL(i, 0)) \
//	STRIP(name)Store(COMPONENT_STORE_RESERVE)
//
//#define COMPONENT_CREATE_CASE(r, data, name) \
//	case ComponentType::STRIP(name)_T: { \
//		newId = STRIP(name)Store.addComponent(); \
//		break; \
//	}
//
//#define COMPONENT_DATABASE(SEQ) \
//	class ComponentDatabase { \
//	public: \
//		template <typename T> \
//		/* rely on template specializations for implementation, or use Reflection ::id to get the Store from an array (not yet implemented) */ \
//		size_t createComponent(T&& component) { \
//			throw(std::runtime_error("Unresolved component creation request")); \
//			return -1; \
//		} \
//		\
//		size_t createComponent(ComponentType ct) { \
//			size_t newId = -1; \
//			switch (ct) { \
//				BOOST_PP_SEQ_FOR_EACH(COMPONENT_CREATE_CASE, , SEQ) \
//				default: \
//					throw(std::runtime_error("Unknown component creation request")); \
//			} \
//			return newId; \
//		} \
//		\
//		size_t createComponent(const std::string &ctStr) { \
//			ComponentType ct = ComponentTypeToEnum(ctStr); \
//			return createComponent(ct); \
//		} \
//		\
//		explicit ComponentDatabase() : \
//			BOOST_PP_SEQ_FOR_EACH_I(COMPONENT_STORE_INIT, , SEQ) \
//		{} \
//	private: \
//		BOOST_PP_SEQ_FOR_EACH(COMPONENT_STORE_LIST, , SEQ) \
//	};

#define COMPONENT_LIST(SEQ) \
	BOOST_PP_SEQ_FOR_EACH(COMPONENT_DECLARATION, , SEQ) \
	\
	MakeEnum(ComponentType, uint16_t, SEQ, _T) \
	\
//	COMPONENT_DATABASE(SEQ)

extern void test_reflection(); // TEMP

#endif