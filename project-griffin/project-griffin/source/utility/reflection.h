#pragma once
#ifndef _REFLECTION_H
#define _REFLECTION_H

#define BOOST_PP_VARIADICS 1
#define BOOST_PP_VARIADICS_MSVC 1

#include <string>
#include <iostream>
#include <vector>
#include <tuple>
#include <cstdint>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/list/at.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/logical/or.hpp>
#include <boost/preprocessor/logical/not.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include "enum.h"

#define NIL BOOST_PP_NIL

#define INSERT_COMMA_FROM_LIST(i, elem) \
	BOOST_PP_COMMA_IF(BOOST_PP_NOT(BOOST_PP_OR(BOOST_PP_EQUAL(i, 0), BOOST_PP_LIST_IS_NIL(elem))))

#define EACH_FIELD(r, data, elem) \
	BOOST_PP_LIST_AT(elem, 0) BOOST_PP_LIST_AT(elem, 1);

#define EACH_TYPE(r, data, i, elem) \
	INSERT_COMMA_FROM_LIST(i, elem) \
	BOOST_PP_LIST_AT(elem, 0)

#define EACH_ENUM(r, data, elem) \
	(BOOST_PP_LIST_AT(elem, 1))

#define EACH_NAME(r, prefix, i, elem) \
	INSERT_COMMA_FROM_LIST(i, elem) \
	prefix##BOOST_PP_LIST_AT(elem, 1)

#define EACH_EMPLACE(r, data, elem) \
	out.emplace_back(BOOST_PP_LIST_AT(elem, 2), BOOST_PP_STRINGIZE(BOOST_PP_LIST_AT(elem, 1)), BOOST_PP_LIST_AT(elem, 3));

#define EACH_INIT(r, data, i, elem) \
	INSERT_COMMA_FROM_LIST(i, elem) \
	{ BOOST_PP_LIST_AT(elem, 2), BOOST_PP_STRINGIZE(BOOST_PP_LIST_AT(elem, 1)), BOOST_PP_LIST_AT(elem, 3) }

#define REFLECT(ClassType, ...) \
	BOOST_PP_SEQ_FOR_EACH(EACH_FIELD, , BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
	\
	struct Reflection { \
		MakeEnum(Fields, uint8_t, \
			BOOST_PP_SEQ_FOR_EACH(EACH_ENUM, , BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
			, \
		) \
		typedef std::tuple<BOOST_PP_SEQ_FOR_EACH_I(EACH_TYPE, , BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) > PropertiesTuple; \
		\
		inline static std::string getClassType() { \
			return #ClassType; \
		} \
		\
		inline static void addProperties(std::vector<ReflectionData> &out) { \
			BOOST_PP_SEQ_FOR_EACH(EACH_EMPLACE, , BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
		} \
		\
		inline static std::vector<ReflectionData> & getProperties() { \
			static std::vector<ReflectionData> sReflectionData = { \
				BOOST_PP_SEQ_FOR_EACH_I(EACH_INIT, , BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
			}; \
			return sReflectionData; \
		} \
		\
		inline static ClassType::Reflection::PropertiesTuple getAllValues(ClassType &inst) { \
			return std::make_tuple(BOOST_PP_SEQ_FOR_EACH_I(EACH_NAME, inst., BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))); \
		} \
	};

MakeEnum(FieldType, uint8_t,
	(int)
	(long)
	(float)
	(double)
	(bool)
	(string)
	(vector)
	(list)
	(queue)
	(stack)
	(map)
	(unordered_map)
	(set)
	(unordered_set)
	(vec2)
	(vec3)
	(vec4)
	(mat4)
	(quat),
	_T	/* postfix _T */
);

struct ReflectionData {
	FieldType type;
	std::string name;
	std::string description;

	ReflectionData(FieldType type,
		std::string&& name,
		std::string&& description) :
		type(type),
		name(std::move(name)),
		description(std::move(description))
	{}
};

//struct field_visitor
//{
//	template<class C, class Visitor, class I>
//	void operator()(C& c, Visitor v, I)
//	{
//		v(reflector::get_field_data<I::value>(c));
//	}
//};

#endif