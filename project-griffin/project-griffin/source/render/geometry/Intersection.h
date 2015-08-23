#pragma once
#ifndef GRIFFIN_GEOMETRY_INTERSECTION_H_
#define GRIFFIN_GEOMETRY_INTERSECTION_H_

#include "Geometry.h"


namespace griffin {
	namespace geometry {
		enum IntersectionResult : uint8_t {
			Outside   = 0,
			Inside    = 1,
			Intersect = 2 // or 3 depending on the algorithm used (see the discussion on the 2nd SSE version)
		};


		// Plane-Vector
		

		// Plane-Sphere


		// Plane-Ray


		// Sphere-Sphere


		// AABB-AABB


		// Frustum-Sphere


		// Frustum-AABB
		void CullAABBList_SSE_1(AABB* aabbList, unsigned int numAABBs, Plane* frustumPlanes, unsigned int* result);
		void CullAABBList_SSE_4(AABB* aabbList, unsigned int numAABBs, Plane* frustumPlanes, unsigned int* result);
	}
}

#endif