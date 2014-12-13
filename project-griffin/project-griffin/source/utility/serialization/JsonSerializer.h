/*
* Copyright (c) 2011-2012 Promit Roy
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

// http://ventspace.wordpress.com/2012/10/08/c-json-serialization/

#pragma once
#ifndef JSON_SERIALIZER_H
#define JSON_SERIALIZER_H

#include <utility/json/json.h>
#include <vector>
#include <string>
#include <type_traits>

class JsonSerializer {
private:
	//SFINAE garbage to detect whether a type has a Serialize member
	typedef char SerializeNotFound;
	struct SerializeFound { char x[2]; };
	struct SerializeFoundStatic { char x[3]; };

	template<typename T, void (T::*)(JsonSerializer&)>
	struct SerializeTester {};
	
	template<typename T, void(*)(JsonSerializer&)>
	struct SerializeTesterStatic {};
	
	template<typename T>
	static SerializeFound SerializeTest(SerializeTester<T, &T::serialize>*);
	
	template<typename T>
	static SerializeFoundStatic SerializeTest(SerializeTesterStatic<T, &T::serialize>*);
	
	template<typename T>
	static SerializeNotFound SerializeTest(...);

	template<typename T>
	struct HasSerialize
	{
		static const bool value = sizeof(SerializeTest<T>(nullptr)) > sizeof(SerializeNotFound);
	};

	//Serialize using a free function defined for the type (default fallback)
	template<typename TValue>
	void serializeImpl(TValue& value,
					   typename std::enable_if<!HasSerialize<TValue>::value>::type* dummy = 0)
					   //typename std::enable_if<!std::is_member_function_pointer<decltype(&TValue::serialize)>::value>::type* dummy = 0)
	{
		//prototype for the serialize free function, so we will get a link error if it's missing
		//this way we don't need a header with all the serialize functions for misc types (eg math)
		void serialize(TValue&, JsonSerializer&);

		serialize(value, *this);
	}

	//Serialize using a member function Serialize(JsonSerializer&)
	template<typename TValue>
	void serializeImpl(TValue& value,
					   typename std::enable_if<HasSerialize<TValue>::value>::type* dummy = 0)
	{
		value.serialize(*this);
	}

public:
	JsonSerializer(bool isWriter)
		: m_isWriter(isWriter)
	{}

	template<typename TKey, typename TValue>
	void serialize(TKey key, TValue& value,
				   typename std::enable_if<std::is_class<TValue>::value>::type* dummy = 0)
	{
		JsonSerializer subVal(m_isWriter);
		if (!m_isWriter) {
			subVal.m_jsonValue = m_jsonValue[key];
		}

		subVal.serializeImpl(value);

		if (m_isWriter) {
			m_jsonValue[key] = subVal.m_jsonValue;
		}
	}

	//Serialize a string value
	template<typename TKey>
	void serialize(TKey key, std::string& value)
	{
		if (m_isWriter) {
			write(key, value);
		} else {
			read(key, value);
		}
	}

	//Serialize a non class type directly using JsonCpp
	template<typename TKey, typename TValue>
	void serialize(TKey key, TValue& value,
				   typename std::enable_if<std::is_fundamental<TValue>::value>::type* dummy = 0)
	{
		if (m_isWriter) {
			write(key, value);
		} else {
			read(key, value);
		}
	}

	//Serialize an enum type to JsonCpp 
	template<typename TKey, typename TEnum>
	void serialize(TKey key, TEnum& value,
				   typename std::enable_if<std::is_enum<TEnum>::value>::type* dummy = 0)
	{
		int ival = (int)value;
		if (m_isWriter) {
			write(key, ival);

		} else {
			read(key, ival);
			value = (TEnum)ival;
		}
	}

	//Serialize only when writing (saving), useful for r-values
	template<typename TKey, typename TValue>
	void writeOnly(TKey key, TValue value,
				   typename std::enable_if<std::is_fundamental<TValue>::value>::type* dummy = 0)
	{
		if (m_isWriter) {
			write(key, value);
		}
	}

	//Serialize a series of items by start and end iterators
	template<typename TKey, typename TItor>
	void writeOnly(TKey key, TItor first, TItor last)
	{
		if (!m_isWriter) { return; }

		JsonSerializer subVal(m_isWriter);
		int index = 0;
		for (TItor it = first; it != last; ++it) {
			subVal.serialize(index, *it);
			++index;
		}
		m_jsonValue[key] = subVal.m_jsonValue;
	}

	template<typename TKey, typename TValue>
	void readOnly(TKey key, TValue& value,
				  typename std::enable_if<std::is_fundamental<TValue>::value>::type* dummy = 0)
	{
		if (!m_isWriter) {
			read(key, value);
		}
	}

	template<typename TValue>
	void readOnly(std::vector<TValue>& vec)
	{
		if (m_isWriter) { return; }
		if (!m_jsonValue.isArray()) { return; }

		vec.clear();
		vec.reserve(vec.size() + m_jsonValue.size());
		for (unsigned int i = 0; i < m_jsonValue.size(); ++i) {
			TValue val;
			serialize(i, val);
			vec.push_back(val);
		}
	}

	template<typename TKey, typename TValue>
	void serialize(TKey key, std::vector<TValue>& vec)
	{
		if (m_isWriter) {
			writeOnly(key, vec.begin(), vec.end());

		} else {
			JsonSerializer subVal(m_isWriter);
			subVal.m_jsonValue = m_jsonValue[key];
			subVal.readOnly(vec);
		}
	}

	//Append a Json::Value directly
	template<typename TKey>
	void writeOnly(TKey key, const Json::Value& value)
	{
		write(key, value);
	}

	//Forward a pointer
	template<typename TKey, typename TValue>
	void serialize(TKey key, TValue* value,
				   typename std::enable_if<!std::is_fundamental<TValue>::value>::type* dummy = 0)
	{
		serialize(key, *value);
	}

	template<typename TKey, typename TValue>
	void writeOnly(TKey key, TValue* value,
				   typename std::enable_if<!std::is_fundamental<TValue>::value>::type* dummy = 0)
	{
		serialize(key, *value);
	}

	template<typename TKey, typename TValue>
	void readOnly(TKey key, TValue* value,
				  typename std::enable_if<!std::is_fundamental<TValue>::value>::type* dummy = 0)
	{
		readOnly(key, *value);
	}

	//Shorthand operator to serialize
	template<typename TKey, typename TValue>
	void operator()(TKey key, TValue& value)
	{
		serialize(key, value);
	}

	Json::Value m_jsonValue;
	bool m_isWriter;

private:
	template<typename TKey, typename TValue>
	void write(TKey key, TValue value)
	{
		m_jsonValue[key] = value;
	}

	template<typename TKey, typename TValue>
	void read(TKey key, TValue& value,
			  typename std::enable_if<std::is_arithmetic<TValue>::value>::type* dummy = 0)
	{
		int ival = m_jsonValue[key].asInt();
		value = static_cast<TValue>(ival);
	}

	template<typename TKey>
	void read(TKey key, bool& value)
	{
		value = m_jsonValue[key].asBool();
	}

	template<typename TKey>
	void read(TKey key, int& value)
	{
		value = m_jsonValue[key].asInt();
	}

	template<typename TKey>
	void read(TKey key, int64_t& value)
	{
		value = m_jsonValue[key].asInt64();
	}

	template<typename TKey>
	void read(TKey key, unsigned int& value)
	{
		value = m_jsonValue[key].asUInt();
	}

	template<typename TKey>
	void read(TKey key, uint64_t& value)
	{
		value = m_jsonValue[key].asUInt64();
	}

	template<typename TKey>
	void read(TKey key, float& value)
	{
		value = m_jsonValue[key].asFloat();
	}

	template<typename TKey>
	void read(TKey key, double& value)
	{
		value = m_jsonValue[key].asDouble();
	}

	template<typename TKey>
	void read(TKey key, std::string& value)
	{
		value = m_jsonValue[key].asString();
	}
};

// name value pair
#define NVP(name) #name, name
#define serializeNVP(name) serialize(NVP(name))

#endif