#pragma once
#ifndef REFLECTION_H
#define REFLECTION_H

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

#define EACH_TYPE(r, data, i, elem) \
	INSERT_COMMA_FROM_LIST(i, elem) \
	BOOST_PP_LIST_AT(elem, 0)

#define EACH_TYPE_REF(r, data, i, elem) \
	INSERT_COMMA_FROM_LIST(i, elem) \
	BOOST_PP_LIST_AT(elem, 0)&

#define EACH_ENUM(r, data, elem) \
	(BOOST_PP_LIST_AT(elem, 1))

#define EACH_NAME(r, prefix, i, elem) \
	INSERT_COMMA_FROM_LIST(i, elem) \
	prefix##BOOST_PP_LIST_AT(elem, 1)

//#define EACH_EMPLACE(r, data, elem) \
//	out.emplace_back(BOOST_PP_LIST_AT(elem, 2), /* FieldType */ \
//					 BOOST_PP_STRINGIZE(BOOST_PP_LIST_AT(elem, 1)), /* name */ \
//					 BOOST_PP_LIST_AT(elem, 3), /* description */ \
//					 sizeof(BOOST_PP_LIST_AT(elem, 1)));

#define EACH_INIT(r, data, i, elem) \
	INSERT_COMMA_FROM_LIST(i, elem) { \
		BOOST_PP_LIST_AT(elem, 2), /* FieldType */ \
		std::string(BOOST_PP_STRINGIZE(BOOST_PP_LIST_AT(elem, 1))), /* name */ \
		std::string(BOOST_PP_LIST_AT(elem, 3)), /* description */ \
		sizeof(BOOST_PP_LIST_AT(elem, 0)) \
	}

#define FOR_EACH(macro, ...) \
	BOOST_PP_SEQ_FOR_EACH(macro, , BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define FOR_EACH_I(macro, ...) \
	BOOST_PP_SEQ_FOR_EACH_I(macro, , BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define REFLECT(ClassType, ...) \
	struct Reflection { \
		MakeEnum(Field, uint8_t, \
			FOR_EACH(EACH_ENUM, __VA_ARGS__), \
		) \
		typedef std::tuple<FOR_EACH_I(EACH_TYPE, __VA_ARGS__)> PropertiesTuple; \
		typedef std::tuple<FOR_EACH_I(EACH_TYPE_REF, __VA_ARGS__)> RefPropertiesTuple; \
		\
		inline static const std::string& getClassType() { \
			static std::string sClassType = #ClassType; \
			return sClassType; \
		} \
		\
		static std::vector<ReflectionData>& getProperties() { \
			static std::vector<ReflectionData> sReflectionData = { \
				FOR_EACH_I(EACH_INIT, __VA_ARGS__) \
			}; \
			return sReflectionData; \
		} \
		\
		inline static ClassType::Reflection::PropertiesTuple copyAllValues(const ClassType &inst) { \
			return std::make_tuple(BOOST_PP_SEQ_FOR_EACH_I(EACH_NAME, inst., BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))); \
		} \
		\
		inline static ClassType::Reflection::RefPropertiesTuple getAllValues(ClassType &inst) { \
			return std::tie(BOOST_PP_SEQ_FOR_EACH_I(EACH_NAME, inst., BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))); \
		} \
		\
		inline static size_t getSize() { \
			static const size_t size = sizeof(ClassType); \
			return size; \
		} \
	};

MakeEnum(FieldType, uint8_t,
	(int)
	(float)
	(double)
	(char)
	(short)
	(long)
	(longlong)
	(bool)
	(uint)
	(ushort)
	(uchar)
	(ulong)
	(ulonglong)
	(wchar_t)
	(int8)
	(int16)
	(int32)
	(int64)
	(uint8)
	(uint16)
	(uint32)
	(uint64)
	(m64)
	(m128)
	(m256)
	(string)
	(wstring)
	(vec2)
	(vec3)
	(vec4)
	(dvec2)
	(dvec3)
	(dvec4)
	(mat3)
	(mat4)
	(dmat3)
	(dmat4)
	(quat)
	(vectorChar)
	(vectorShort)
	(vectorInt)
	(vectorLong)
	(vectorLonglong)
	(vectorBool)
	(vectorUchar)
	(vectorUshort)
	(vectorUint)
	(vectorUlong)
	(vectorUlonglong)
	(vectorFloat)
	(vectorDouble)
	(vectorWchar)
	(vectorInt8)
	(vectorInt16)
	(vectorInt32)
	(vectorInt64)
	(vectorUint8)
	(vectorUint16)
	(vectorUint32)
	(vectorUint64)
	(vectorM64)
	(vectorM128)
	(vectorM256)
	(vectorString)
	(vectorWstring)
	(vectorVec2)
	(vectorVec3)
	(vectorVec4)
	(vectorDvec2)
	(vectorDvec3)
	(vectorDvec4)
	(vectorMat3)
	(vectorMat4)
	(vectorDmat3)
	(vectorDmat4)
	(vectorQuat)
	, _T	/* postfix _T */
);

struct ReflectionData {
	FieldType type;
	std::string name;
	std::string description;
	size_t size;

	ReflectionData(FieldType type_,
				   std::string&& name_,
				   std::string&& description_,
				   size_t size_) :
		type(type_),
		name(std::forward<std::string>(name_)),
		description(std::forward<std::string>(description_)),
		size(size_)
	{}
};

#endif