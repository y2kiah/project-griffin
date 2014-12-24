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

#pragma once

#include <glm/vec3.hpp>
#include <iostream>

using glm::vec3;
using glm::dvec3;

namespace griffin {

	class Planef {
	public:
		Planef() {}
		Planef(const vec3 &v1, const vec3 &v2, const vec3 &v3);
		Planef(const vec3 &point, const vec3 &normal);
		Planef(float a, float b, float c, float d);

		//! Defines a plane using 3 points. 
		void		set(const vec3 &v1, const vec3 &v2, const vec3 &v3);
		//! Defines a plane using a normal vector and a point.
		void		set(const vec3 &point, const vec3 &normal);
		//! Defines a plane using 4 coefficients.
		void		set(float a, float b, float c, float d);

		vec3		getPoint() const { return mNormal * mDistance; };
		const vec3&	getNormal() const { return mNormal; };
		float		getDistance() const { return mDistance; }
		float		distance(const vec3 &p) const { return (mNormal.dot(p) - mDistance); };

		vec3		reflectPoint(const vec3 &p) const { return mNormal * distance(p) * -2 + p; }
		vec3		reflectVector(const vec3 &v) const { return mNormal * mNormal.dot(v) * 2 - v; }

		vec3		mNormal;
		float		mDistance;
	};

	class Planed {
	public:
		Planed() {}
		Planed(const dvec3 &v1, const dvec3 &v2, const dvec3 &v3);
		Planed(const dvec3 &point, const dvec3 &normal);
		Planed(double a, double b, double c, double d);

		//! Defines a plane using 3 points. 
		void		set(const dvec3 &v1, const dvec3 &v2, const dvec3 &v3);
		//! Defines a plane using a normal vector and a point.
		void		set(const dvec3 &point, const dvec3 &normal);
		//! Defines a plane using 4 coefficients.
		void		set(double a, double b, double c, double d);

		dvec3			getPoint() const { return mNormal * mDistance; };
		const dvec3&	getNormal() const { return mNormal; };
		double			getDistance() const { return mDistance; }
		double			distance(const dvec3 &p) const { return (mNormal.dot(p) - mDistance); };

		dvec3			reflectPoint(const dvec3 &p) const { return mNormal * distance(p) * -2 + p; }
		dvec3			reflectVector(const dvec3 &v) const { return mNormal * mNormal.dot(v) * 2 - v; }

		dvec3			mNormal;
		double			mDistance;
	};

	std::ostream& operator<<(std::ostream &o, const Planef &p)
	{
		return o << "(" << p.mNormal << ", " << p.mDistance << ")";
	}

	std::ostream& operator<<(std::ostream &o, const Planed &p)
	{
		return o << "(" << p.mNormal << ", " << p.mDistance << ")";
	}

	class PlaneExc : public std::exception {
	public:
		virtual const char* what() const throw() { return "Invalid parameters specified"; }
	};

}