#pragma once
#ifndef GRIFFIN_GEOMETRY_H_
#define GRIFFIN_GEOMETRY_H_

#include <cmath>
#include <cstdint>
#include <cassert>
#include <glm/vec3.hpp>

namespace griffin {
	namespace geometry {

		struct MatrixColumnMajor {
			float _11, _21, _31, _41;
			float _12, _22, _32, _42;
			float _13, _23, _33, _43;
			float _14, _24, _34, _44;
		};

		struct MatrixRowMajor {
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};

		struct Plane {
			// plane equation nx*x+ny*y+nz*z+d=0
			float nx, ny, nz;
			float d;

			void normalize();

			/**
			* In order to obtain a "true" distance, the plane must be normalized
			* @returns distance in units of magnitude of the plane's normal vector {nx,ny,nz}.
			*/
			float distanceToPoint(float p[3]) const;
			float distanceToPoint(float x, float y, float z) const;
			void set(float p1[3], float p2[3], float p3[3]);
			void set(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
			void set(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3);
		};

		struct Sphere {
			float x, y, z;
			float r;
		};

		struct AABB {
			// TODO: should I create a Vec3 or use glm::vec3??
			// also, is it really best to store AABB as center/extent rather than min/max
			struct {
				float x; float y; float z;
			} m_center, m_extent;
		};

		struct Frustum {
			enum PlaneIndex : uint8_t { Near=0, Far, Left, Right, Top, Bottom };
			// stores planes as SoA instead of AoS
			float nx[6];
			float ny[6];
			float nz[6];
			float d[6];


			Plane getPlane(PlaneIndex whichPlane);
			void getPlanes(Plane *outPlanes = nullptr);

			/**
			* Matrix is column-major order for GL, row-major for D3D.
			* For projection matrix, planes are extracted in camera space.
			* For view * projection matrix, world space.
			* For model * view * projection matrix, object space.
			*/
			void extractFromMatrixGL(float matrix[16], bool normalize = true);
			void extractFromMatrixD3D(float matrix[16], bool normalize = true);
		};


		// Plane Functions

		inline void Plane::normalize()
		{
			assert(nx != 0.0f || ny != 0.0f || nz != 0.0f);

			float iMag = 1.0f / sqrtf(nx*nx + ny*ny + nz*nz);
			nx *= iMag;
			ny *= iMag;
			nz *= iMag;
			d  *= iMag;
		}

		inline float Plane::distanceToPoint(float p[3]) const
		{
			// n*p + d
			return nx*p[0] + ny*p[1] + nz*p[2] + d;
		}

		inline float Plane::distanceToPoint(float x, float y, float z) const
		{
			// n*p + d
			return nx*x + ny*y + nz*z + d;
		}


		// Frustum Functions

		inline Plane Frustum::getPlane(PlaneIndex whichPlane)
		{
			return Plane{ nx[whichPlane], ny[whichPlane], nz[whichPlane], d[whichPlane] };
		}

		inline void Frustum::getPlanes(Plane* planes)
		{
			assert(planes != nullptr);
			for (int p = 0; p < 6; ++p) {
				Plane &plane = planes[p];
				plane.nx = nx[p];
				plane.ny = ny[p];
				plane.nz = nz[p];
				plane.d = d[p];
			}
		}


	}


	// Vector Functions

	inline float length2(const glm::vec3& v)
	{
		return (v.x*v.x + v.y*v.y + v.z*v.z);
	}

	inline double length2(const glm::dvec3& v)
	{
		return (v.x*v.x + v.y*v.y + v.z*v.z);
	}
}

#endif