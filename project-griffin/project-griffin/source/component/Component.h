#pragma once
#ifndef _COMPONENT_H
#define _COMPONENT_H

#include "utility/reflection.h"

#define COMPONENT_STORE_RESERVE	1000

#define COMPONENT(name, ...) \
	struct name { \
		static const ComponentType id = ComponentType::name##_T; \
		REFLECT(name, __VA_ARGS__) \
	};

#define COMPONENT_DECLARATION(r, data, name) \
	struct STRIP(name);

#define COMPONENT_STORE_LIST(r, data, name) \
	ComponentStore<STRIP(name)> STRIP(name)##Store;

#define COMPONENT_STORE_INIT(r, data, i, name) \
	BOOST_PP_COMMA_IF(BOOST_PP_NOT_EQUAL(i, 0)) \
	STRIP(name)Store(COMPONENT_STORE_RESERVE)

#define COMPONENT_FACTORY_CASE(r, data, name) \
	case ComponentType::STRIP(name)_T: \
		newId = STRIP(name)Store.createComponent(); \
		break;

#define COMPONENT_FACTORY(SEQ) \
	class ComponentFactory { \
	public: \
		size_t createComponent(ComponentType ct) { \
			size_t newId = -1; \
			switch (ct) { \
				BOOST_PP_SEQ_FOR_EACH(COMPONENT_FACTORY_CASE, , SEQ) \
				default: \
					throw(std::runtime_error("Unknown component creation request")); \
			} \
			return newId; \
		} \
		\
		explicit ComponentFactory() : \
			BOOST_PP_SEQ_FOR_EACH_I(COMPONENT_STORE_INIT, , SEQ) \
		{} \
	private: \
		BOOST_PP_SEQ_FOR_EACH(COMPONENT_STORE_LIST, , SEQ) \
	};

#define COMPONENT_LIST(SEQ) \
	BOOST_PP_SEQ_FOR_EACH(COMPONENT_DECLARATION, , SEQ) \
	\
	MakeEnum(ComponentType, uint8_t, SEQ, _T) \
	\
	COMPONENT_FACTORY(SEQ)

#include "components.h"

extern void test_reflection(); // TEMP

#endif