/*
 Copyright (c) 2010, The Barbarian Group
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

#include <glm/detail/type_vec.hpp>
#include <glm/detail/type_mat.hpp>
#include "Area.h"
#include <vector>

using glm::detail::tvec2;
using glm::detail::tmat4x4;

namespace griffin {

	template<typename T>
	class RectT {
	public:
		typedef tvec2  <T, glm::precision::defaultp>	vec2_t;
		typedef tmat4x4<T, glm::precision::defaultp>	mat4x4_t;

		RectT() {}
		/** Initializes the rectangle to be the bounding box of \a points **/
		RectT(const std::vector<vec2_t> &points);
		RectT(T aX1, T aY1, T aX2, T aY2) {
			set(aX1, aY1, aX2, aY2);
		}
		RectT(const vec2_t &v1, const vec2_t &v2) {
			set(v1.x, v1.y, v2.x, v2.y);
		}
		RectT(const Area &area);

		void		set(T aX1, T aY1, T aX2, T aY2);

		T			getWidth() const { return x2 - x1; }
		T			getHeight() const { return y2 - y1; }
		T			getAspectRatio() const { return getWidth() / getHeight(); }
		T			calcArea() const { return getWidth() * getHeight(); }

		void		canonicalize(); // return rect w/ properly ordered coordinates
		RectT		canonicalized() const; // return rect w/ properly ordered coordinates

		void		clipBy(const RectT &clip);
		RectT		getClipBy(const RectT &clip) const;
		Area		getInteriorArea() const;
		void		offset(const vec2_t &offset);
		RectT		getOffset(const vec2_t &off) const { RectT result(*this); result.offset(off); return result; }
		void		inflate(const vec2_t &amount);
		RectT		inflated(const vec2_t &amount) const;
		//! Translates the rectangle so that its center is at \a center
		void		offsetCenterTo(const vec2_t &center) { offset(center - getCenter()); }
		void		scaleCentered(const vec2_t &scale);
		void		scaleCentered(T scale);
		RectT		scaledCentered(T scale) const;
		void		scale(T scale);
		void		scale(const vec2_t &scale);
		RectT		scaled(T scale) const;
		RectT		scaled(const vec2_t &scale) const;

		//! Returns a copy of the Rect transformed by \a matrix. Represents the bounding box of the transformed Rect when \a matrix expresses non-scale/translate operations.
		RectT		transformCopy(const mat4x4_t& matrix) const;

		/** \brief Is a point \a pt inside the rectangle **/
		template<typename Y>
		bool		contains(const vec2_t &pt) const { return (pt.x >= x1) && (pt.x <= x2) && (pt.y >= y1) && (pt.y <= y2); }
		//! Returns whether \a rect intersects with this
		bool		intersects(const RectT &rect) const;

		//! Returns the distance between the point \a pt and the rectangle. Points inside the rectangle return \c 0.
		T			distance(const vec2_t &pt) const;
		//! Returns the squared distance between the point \a pt and the rectangle. Points inside the rectangle return \c 0.
		T			distanceSquared(const vec2_t &pt) const;

		//! Returns the nearest point on the Rect \a rect. Points inside the rectangle return \a pt.
		vec2_t		closestPoint(const vec2_t &pt) const;

		T			getX1() const { return x1; }
		T			getY1() const { return y1; }
		T			getX2() const { return x2; }
		T			getY2() const { return y2; }

		vec2_t		getUpperLeft() const { return vec2_t(x1, y1); };
		vec2_t		getUpperRight() const { return vec2_t(x2, y1); };
		vec2_t		getLowerRight() const { return vec2_t(x2, y2); };
		vec2_t		getLowerLeft() const { return vec2_t(x1, y2); };
		vec2_t		getCenter() const { return vec2_t((x1 + x2) / 2, (y1 + y2) / 2); }
		vec2_t		getSize() const { return vec2_t(x2 - x1, y2 - y1); }

		/** \return Scaled copy with the same aspect ratio centered relative to and scaled to fit inside \a other. If \a expand then the rectangle is expanded if it is smaller than \a other */
		RectT		getCenteredFit(const RectT &other, bool expand) const;

		/** Expands the Rect to include \a point in its interior **/
		void		include(const vec2_t &point);
		/** Expands the Rect to include all points in \a points in its interior **/
		void		include(const std::vector<vec2_t > &points);
		/** Expands the Rect to include \a rect in its interior **/
		void		include(const RectT &rect);

		const RectT<T>	operator+(const vec2_t &o) const { return this->getOffset(o); }
		const RectT<T>	operator-(const vec2_t &o) const { return this->getOffset(-o); }
		const RectT<T>	operator*(T s) const { return this->scaled(s); }
		const RectT<T>	operator/(T s) const { return this->scaled(((T)1) / s); }

		const RectT<T>	operator+(const RectT<T>& rhs) const { return RectT<T>(x1 + rhs.x1, y1 + rhs.y1, x2 + rhs.x2, y2 + rhs.y2); }
		const RectT<T>	operator-(const RectT<T>& rhs) const { return RectT<T>(x1 - rhs.x1, y1 - rhs.y1, x2 - rhs.x2, y2 - rhs.y2); }

		RectT<T>&		operator+=(const vec2_t &o) { offset(o); return *this; }
		RectT<T>&		operator-=(const vec2_t &o) { offset(-o); return *this; }
		RectT<T>&		operator*=(T s) { scale(s); return *this; }
		RectT<T>&		operator/=(T s) { scale(((T)1) / s); return *this; }

		//! Constructs a rectangle with all values initialized to zero.
		static	RectT zero()			{ return RectT(0, 0, 0, 0); }

		T			x1, y1, x2, y2;

		friend std::ostream& operator<<(std::ostream &o, const RectT &rect)
		{
			return o << "(" << rect.x1 << ", " << rect.y1 << ")-(" << rect.x2 << ", " << rect.y2 << ")";
		}

	};

	typedef RectT<float>	Rectf;
	typedef RectT<double>	Rectd;


	// This class maps a rectangle into another rectangle
	class RectMapping {
	public:
		RectMapping()
			: mSrcRect(0, 0, 0, 0), mDstRect(0, 0, 0, 0) {}
		RectMapping(const Rectf &aSrcRect, const Rectf &aDstRect)
			: mSrcRect(aSrcRect), mDstRect(aDstRect) {}
		RectMapping(const Rectf &aSrcRect, const Rectf &aDstRect, bool preserveSrcAspect);

		vec2	map(const vec2 &srcPoint) const;
		Rectf	map(const Rectf &srcRect) const;

	private:
		Rectf	mSrcRect, mDstRect;
	};

	extern void getClippedScaledRects(const Area &srcSurfaceBounds, const Rectf &srcRect, const Area &dstSurfaceBounds, const Area &dstArea, Rectf *resultSrcRect, Area *resultDstRect);

}