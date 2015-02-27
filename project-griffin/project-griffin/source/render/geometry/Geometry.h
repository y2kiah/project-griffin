#pragma once
#ifndef GRIFFIN_GEOMETRY_H_
#define GRIFFIN_GEOMETRY_H_

#include <cmath>
#include <cassert>

namespace griffin {
	namespace render {
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
				// plane equation ax+by+cz+d=0
				float a, b, c, d;

				void normalize();

				/**
				* In order to obtain a "true" distance, the plane must be normalized
				* @returns distance in units of magnitude of the plane's normal vector {a,b,c}.
				*/
				float distanceToPoint(float p[3]);
			};

			struct Frustum {
				union {
					struct {
						Plane left, right, top, bottom, near, far;
					};
					Plane* planes[6];
				};

				/**
				* Matrix is column-major order for GL, row-major for D3D.
				* For projection matrix, planes are extracted in camera space.
				* For view * projection matrix, world space.
				* For model * view * projection matrix, object space.
				*/
				void extractFromMatrixGL(float matrix[16], bool normalize = true);
				void extractFromMatrixD3D(float matrix[16], bool normalize = true);
			};


			// Inline Functions

			inline void Plane::normalize()
			{
				assert(a != 0.0f && b != 0.0f && c != 0.0f);

				float iMag = 1.0f / sqrtf(a*a + b*b + c*c);
				a *= iMag;
				b *= iMag;
				c *= iMag;
				d *= iMag;
			}

			inline float Plane::distanceToPoint(float p[3])
			{
				return a*p[0] + b*p[1] + c*p[2] + d;
			}
		}
	}
}

#endif