/*
 Copyright (c) 2011, The Cinder Project, All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org
 
 Portions of this code (C) Paul Houx
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "Plane.h"

namespace griffin {

	Planef::Planef(const vec3 &v1, const vec3 &v2, const vec3 &v3)
	{
		set(v1, v2, v3);
	}

	Planef::Planef(const vec3 &point, const vec3 &normal)
	{
		set(point, normal);
	}

	Planef::Planef(float a, float b, float c, float d)
	{
		set(a, b, c, d);
	}

	void Planef::set(const vec3 &v1, const vec3 &v2, const vec3 &v3)
	{
		vec3 normal = (v2 - v1).cross(v3 - v1);

		if (normal.lengthSquared() == 0)
			// error! invalid parameters
			throw PlaneExc();

		mNormal = normal.normalized();
		mDistance = mNormal.dot(v1);
	}

	void Planef::set(const vec3 &point, const vec3 &normal)
	{
		if (normal.lengthSquared() == 0)
			// error! invalid parameters
			throw PlaneExc();

		mNormal = normal.normalized();
		mDistance = mNormal.dot(point);
	}

	void Planef::set(float a, float b, float c, float d)
	{
		vec3 normal(a, b, c);

		float length = normal.length();
		if (length == 0)
			// error! invalid parameters
			throw PlaneExc();

		mNormal = normal.normalized();
		mDistance = d / length;
	}

	// class Planed

	Planed::Planed(const dvec3 &v1, const dvec3 &v2, const dvec3 &v3)
	{
		set(v1, v2, v3);
	}

	Planed::Planed(const dvec3 &point, const dvec3 &normal)
	{
		set(point, normal);
	}

	Planed::Planed(double a, double b, double c, double d)
	{
		set(a, b, c, d);
	}

	void Planed::set(const dvec3 &v1, const dvec3 &v2, const dvec3 &v3)
	{
		vec3 normal = (v2 - v1).cross(v3 - v1);

		if (normal.lengthSquared() == 0)
			// error! invalid parameters
			throw PlaneExc();

		mNormal = normal.normalized();
		mDistance = mNormal.dot(v1);
	}

	void Planed::set(const dvec3 &point, const dvec3 &normal)
	{
		if (normal.lengthSquared() == 0)
			// error! invalid parameters
			throw PlaneExc();

		mNormal = normal.normalized();
		mDistance = mNormal.dot(point);
	}

	void Planed::set(double a, double b, double c, double d)
	{
		dvec3 normal(a, b, c);

		double length = normal.length();
		if (length == 0)
			// error! invalid parameters
			throw PlaneExc();

		mNormal = normal.normalized();
		mDistance = d / length;
	}

}