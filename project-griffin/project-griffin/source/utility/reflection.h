#pragma once
#ifndef GRIFFIN_REFLECTION_H_
#define GRIFFIN_REFLECTION_H_

#define BOOST_PP_VARIADICS 1
#define BOOST_PP_VARIADICS_MSVC 1

#include <string>
#include <iostream>
#include <array>
#include <tuple>
#include <cstdint>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/to_tuple.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/rem.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/logical/or.hpp>
#include <boost/preprocessor/logical/not.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include "enum.h"

struct PropertyMetaData {
	std::string name;			//<! variable's name
	std::string description;	//<! description, useful for tools
	size_t size;				//<! total size of the variable
	size_t elementSize;			//<! size of one element of an array (same as size if not array)
	size_t numElements;			//<! number of elements in an array (1 if not array)
	int offset;					//<! pointer to data member reinterpreted as an int
	bool isArray;				//<! http://en.cppreference.com/w/cpp/types/is_array
	bool isTriviallyCopyable;	//<! http://en.cppreference.com/w/cpp/types/is_trivially_copyable
};

#define NUM_PROPERTY_FIELDS 4
#define PROP(tuple, i) \
	BOOST_PP_TUPLE_ELEM(NUM_PROPERTY_FIELDS, i, tuple)

#define EACH_ENUM(r, data, i, elem) \
	(PROP(elem,1))

#define EACH_NAME(r, prefix, i, elem) \
	(prefix##PROP(elem,1))

#define EACH_INIT(r, ClassType, i, elem) \
	PropertyMetaData{ \
		std::string{ BOOST_PP_STRINGIZE(PROP(elem, 1)) },	/* name */ \
		std::string{ PROP(elem,3) },						/* description */ \
		sizeof(PROP(elem,0)PROP(elem,2)),					/* size */ \
		sizeof(PROP(elem,0)),								/* elementSize */ \
		sizeof(PROP(elem,0)PROP(elem,2)) / sizeof(PROP(elem,0)),	/* numElements */ \
		offsetof(ClassType,PROP(elem,1)),					/* offset */ \
		std::is_array<PROP(elem,0)PROP(elem,2)>::value,		/* isArray */ \
		std::is_trivially_copyable<PROP(elem,0)>::value		/* isTriviallyCopyable */ \
	},

#define FOR_EACH_I(macro, data, ...) \
	BOOST_PP_SEQ_FOR_EACH_I(macro, data, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define SEQ_SIZE(...) \
	BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define CSV(macro, data, ...) \
	BOOST_PP_TUPLE_REM_CTOR(SEQ_SIZE(__VA_ARGS__), BOOST_PP_SEQ_TO_TUPLE(FOR_EACH_I(macro, data, __VA_ARGS__)))


#define REFLECT(ReflectionClassName, ClassType, ...) \
	struct ReflectionClassName { \
		MakeEnum(Field, uint8_t, \
			FOR_EACH_I(EACH_ENUM, , __VA_ARGS__), \
		) \
		typedef std::array<PropertyMetaData, SEQ_SIZE(__VA_ARGS__)> PropertiesArray; \
		\
		inline static const std::string& getClassType() { \
			static std::string sClassType = #ClassType; \
			return sClassType; \
		} \
		\
		static const PropertiesArray& getProperties() { \
			static PropertiesArray sPropertyMetaData = { \
				FOR_EACH_I(EACH_INIT, ClassType, __VA_ARGS__) \
			}; \
			return sPropertyMetaData; \
		} \
		\
		inline static auto copyAllValues(ClassType &inst) -> decltype(std::make_tuple(CSV(EACH_NAME,inst., __VA_ARGS__))) { \
			return std::make_tuple(CSV(EACH_NAME, inst., __VA_ARGS__)); \
		} \
		\
		inline static auto getAllValues(ClassType &inst) -> decltype(std::tie(CSV(EACH_NAME,inst., __VA_ARGS__))) { \
			return std::tie(CSV(EACH_NAME, inst., __VA_ARGS__)); \
		} \
		\
		inline static size_t getSize() { \
			static const size_t size = sizeof(ClassType); \
			return size; \
		} \
	};

#endif