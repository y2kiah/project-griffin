#include "../Intersection.h"
//#include <glm/gtx/intersect.hpp>
#include <xmmintrin.h>
#include <glm/geometric.hpp>


using namespace griffin;
using namespace griffin::geometry;


__declspec(align(16)) static const unsigned int absPlaneMask[4] = { 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF };


// Frustum-Sphere

IntersectionResult griffin::geometry::intersect(const Plane* frustumPlanes, Sphere& s)
{
	enum : uint8_t { IntersectingBits=1, FullInsideBits=3 };
	float px = s.x;
	float py = s.y;
	float pz = s.z;
	float radius = s.r;
	uint8_t result = FullInsideBits;

	for (int p = 0; p < 6; ++p) {
		float dist = frustumPlanes[p].distanceToPoint(px, py, pz);
		if (dist < -radius) {
			result &= FullInsideBits;
		}
		else if (dist <= radius) {
			result &= IntersectingBits;
		}
		else {
			return Outside;
		}
	}
	// TODO: this is aweful, can we just settle on a single return enum?
	// need to look at the SSE collision detection function to see why 0,1,2 is used there
	return (result == IntersectingBits ? Intersect : Inside);
}


/*
uint8_t intersect(Frustum& f, float p[3])
{
	Vec posA_xxxx = vecShuffle<VecMask::_xxxx>(posA);
	Vec posA_yyyy = vecShuffle<VecMask::_yyyy>(posA);
	Vec posA_zzzz = vecShuffle<VecMask::_zzzz>(posA);
	Vec posA_rrrr = vecShuffle<VecMask::_wwww>(posA);
	// 4 dot products
	dotA_0123 = vecMulAdd(posA_zzzz, pl_z0z1z2z3, pl_w0w1w2w3);
	dotA_0123 = vecMulAdd(posA_yyyy, pl_y0y1y2y3, dotA_0123);
	dotA_0123 = vecMulAdd(posA_xxxx, pl_x0x1x2x3, dotA_0123);

	Vec posB_xxxx = vecShuffle<VecMask::_xxxx>(posB);
	Vec posB_yyyy = vecShuffle<VecMask::_yyyy>(posB);
	Vec posB_zzzz = vecShuffle<VecMask::_zzzz>(posB);
	Vec posB_rrrr = vecShuffle<VecMask::_wwww>(posB);
	// 4 dot products
	dotB_0123 = vecMulAdd(posB_zzzz, pl_z0z1z2z3, pl_w0w1w2w3);
	dotB_0123 = vecMulAdd(posB_yyyy, pl_y0y1y2y3, dotB_0123);
	dotB_0123 = vecMulAdd(posB_xxxx, pl_x0x1x2x3, dotB_0123);

	Vec posAB_xxxx = vecInsert<VecMask::_0011>(posA_xxxx, posB_xxxx);
	Vec posAB_yyyy = vecInsert<VecMask::_0011>(posA_yyyy, posB_yyyy);
	Vec posAB_zzzz = vecInsert<VecMask::_0011>(posA_zzzz, posB_zzzz);
	Vec posAB_rrrr = vecInsert<VecMask::_0011>(posA_rrrr, posB_rrrr);
	// 4 dot products
	dotA45B45 = vecMulAdd(posAB_zzzz, pl_z4z5z4z5, pl_w4w5w4w5);
	dotA45B45 = vecMulAdd(posAB_yyyy, pl_y4y5y4y5, dotA45B45);
	dotA45B45 = vecMulAdd(posAB_xxxx, pl_x4x5x4x5, dotA45B45);

	// Compare against radius
	dotA_0123 = vecCmpGTMask(dotA_0123, posA_rrrr);
	dotB_0123 = vecCmpGTMask(dotB_0123, posB_rrrr);
	dotA45B45 = vecCmpGTMask(dotA45B45, posAB_rrrr);
	Vec dotA45 = vecInsert<VecMask::_0011>(dotA45B45, zero);
	Vec dotB45 = vecInsert<VecMask::_0011>(zero, dotA45B45);
	// collect the results
	Vec resA = vecOrx(dotA_0123);
	Vec resB = vecOrx(dotB_0123);
	resA = vecOr(resA, vecOrx(dotA45));
	resB = vecOr(resB, vecOrx(dotB45));
	// resA = inside or outside of frustum for point A, resB for point B
	Vec rA = vecNotMask(resA);
	Vec rB = vecNotMask(resB);
	masksCurrent[0] |= frustumMask & rA;
	masksCurrent[1] |= frustumMask & rB;
}
*/


// Frustum-AABB

/**
* @see http://www.gamedev.net/page/resources/_/technical/general-programming/useless-snippet-2-aabbfrustum-test-r3342
*/
void CullAABBList_SSE_1(AABB* aabbList, unsigned int numAABBs, Plane* frustumPlanes, unsigned int* result)
{
	__declspec(align(16)) Plane absFrustumPlanes[6];

	__m128 xmm_absPlaneMask = _mm_load_ps((float*)&absPlaneMask[0]);
	for (unsigned int iPlane = 0; iPlane < 6; ++iPlane) {
		__m128 xmm_frustumPlane = _mm_load_ps(&frustumPlanes[iPlane].nx);
		__m128 xmm_absFrustumPlane = _mm_and_ps(xmm_frustumPlane, xmm_absPlaneMask);
		_mm_store_ps(&absFrustumPlanes[iPlane].nx, xmm_absFrustumPlane);
	}

	for (unsigned int iAABB = 0; iAABB < numAABBs; ++iAABB) {
		__m128 xmm_aabbCenter_x = _mm_load_ss(&aabbList[iAABB].m_center.x);
		__m128 xmm_aabbCenter_y = _mm_load_ss(&aabbList[iAABB].m_center.y);
		__m128 xmm_aabbCenter_z = _mm_load_ss(&aabbList[iAABB].m_center.z);
		__m128 xmm_aabbExtent_x = _mm_load_ss(&aabbList[iAABB].m_extent.x);
		__m128 xmm_aabbExtent_y = _mm_load_ss(&aabbList[iAABB].m_extent.y);
		__m128 xmm_aabbExtent_z = _mm_load_ss(&aabbList[iAABB].m_extent.z);

		unsigned int thisResult = Inside; // Assume that the aabb will be inside the frustum
		for (unsigned int iPlane = 0; iPlane < 6; ++iPlane) {
			__m128 xmm_frustumPlane_Component = _mm_load_ss(&frustumPlanes[iPlane].nx);
			__m128 xmm_d = _mm_mul_ss(xmm_aabbCenter_x, xmm_frustumPlane_Component);

			xmm_frustumPlane_Component = _mm_load_ss(&frustumPlanes[iPlane].ny);
			xmm_d = _mm_add_ss(xmm_d, _mm_mul_ss(xmm_aabbCenter_y, xmm_frustumPlane_Component));

			xmm_frustumPlane_Component = _mm_load_ss(&frustumPlanes[iPlane].nz);
			xmm_d = _mm_add_ss(xmm_d, _mm_mul_ss(xmm_aabbCenter_z, xmm_frustumPlane_Component));

			__m128 xmm_absFrustumPlane_Component = _mm_load_ss(&absFrustumPlanes[iPlane].nx);
			__m128 xmm_r = _mm_mul_ss(xmm_aabbExtent_x, xmm_absFrustumPlane_Component);

			xmm_absFrustumPlane_Component = _mm_load_ss(&absFrustumPlanes[iPlane].ny);
			xmm_r = _mm_add_ss(xmm_r, _mm_mul_ss(xmm_aabbExtent_y, xmm_absFrustumPlane_Component));

			xmm_absFrustumPlane_Component = _mm_load_ss(&absFrustumPlanes[iPlane].nz);
			xmm_r = _mm_add_ss(xmm_r, _mm_mul_ss(xmm_aabbExtent_z, xmm_absFrustumPlane_Component));

			__m128 xmm_frustumPlane_d = _mm_load_ss(&frustumPlanes[iPlane].d);
			__m128 xmm_d_p_r = _mm_add_ss(_mm_add_ss(xmm_d, xmm_r), xmm_frustumPlane_d);
			__m128 xmm_d_m_r = _mm_add_ss(_mm_sub_ss(xmm_d, xmm_r), xmm_frustumPlane_d);

			// Shuffle d_p_r and d_m_r in order to perform only one _mm_movmask_ps
			__m128 xmm_d_p_r__d_m_r = _mm_shuffle_ps(xmm_d_p_r, xmm_d_m_r, _MM_SHUFFLE(0, 0, 0, 0));
			int negativeMask = _mm_movemask_ps(xmm_d_p_r__d_m_r);

			// Bit 0 holds the sign of d + r and bit 2 holds the sign of d - r
			if (negativeMask & 0x01) {
				thisResult = Outside;
				break;
			}
			else if (negativeMask & 0x04) {
				thisResult = Intersect;
			}
		}

		result[iAABB] = thisResult;
	}
}


void CullAABBList_SSE_4(AABB* aabbList, unsigned int numAABBs, Plane* frustumPlanes, unsigned int* result)
{
	__declspec(align(16)) Plane absFrustumPlanes[6];
	__m128 xmm_absPlaneMask = _mm_load_ps((float*)&absPlaneMask[0]);
	for (unsigned int iPlane = 0; iPlane < 6; ++iPlane) {
		__m128 xmm_frustumPlane = _mm_load_ps(&frustumPlanes[iPlane].nx);
		__m128 xmm_absFrustumPlane = _mm_and_ps(xmm_frustumPlane, xmm_absPlaneMask);
		_mm_store_ps(&absFrustumPlanes[iPlane].nx, xmm_absFrustumPlane);
	}

	// Process 4 AABBs in each iteration...
	unsigned int numIterations = numAABBs >> 2;
	for (unsigned int iIter = 0; iIter < numIterations; ++iIter) {
		// NOTE: Since the aabbList is 16-byte aligned, we can use aligned moves.
		// Load the 4 Center/Extents pairs for the 4 AABBs.
		__m128 xmm_cx0_cy0_cz0_ex0 = _mm_load_ps(&aabbList[(iIter << 2) + 0].m_center.x);
		__m128 xmm_ey0_ez0_cx1_cy1 = _mm_load_ps(&aabbList[(iIter << 2) + 0].m_extent.y);
		__m128 xmm_cz1_ex1_ey1_ez1 = _mm_load_ps(&aabbList[(iIter << 2) + 1].m_center.z);
		__m128 xmm_cx2_cy2_cz2_ex2 = _mm_load_ps(&aabbList[(iIter << 2) + 2].m_center.x);
		__m128 xmm_ey2_ez2_cx3_cy3 = _mm_load_ps(&aabbList[(iIter << 2) + 2].m_extent.y);
		__m128 xmm_cz3_ex3_ey3_ez3 = _mm_load_ps(&aabbList[(iIter << 2) + 3].m_center.z);

		// Shuffle the data in order to get all Xs, Ys, etc. in the same register.
		__m128 xmm_cx0_cy0_cx1_cy1 = _mm_shuffle_ps(xmm_cx0_cy0_cz0_ex0, xmm_ey0_ez0_cx1_cy1, _MM_SHUFFLE(3, 2, 1, 0));
		__m128 xmm_cx2_cy2_cx3_cy3 = _mm_shuffle_ps(xmm_cx2_cy2_cz2_ex2, xmm_ey2_ez2_cx3_cy3, _MM_SHUFFLE(3, 2, 1, 0));
		__m128 xmm_aabbCenter0123_x = _mm_shuffle_ps(xmm_cx0_cy0_cx1_cy1, xmm_cx2_cy2_cx3_cy3, _MM_SHUFFLE(2, 0, 2, 0));
		__m128 xmm_aabbCenter0123_y = _mm_shuffle_ps(xmm_cx0_cy0_cx1_cy1, xmm_cx2_cy2_cx3_cy3, _MM_SHUFFLE(3, 1, 3, 1));

		__m128 xmm_cz0_ex0_cz1_ex1 = _mm_shuffle_ps(xmm_cx0_cy0_cz0_ex0, xmm_cz1_ex1_ey1_ez1, _MM_SHUFFLE(1, 0, 3, 2));
		__m128 xmm_cz2_ex2_cz3_ex3 = _mm_shuffle_ps(xmm_cx2_cy2_cz2_ex2, xmm_cz3_ex3_ey3_ez3, _MM_SHUFFLE(1, 0, 3, 2));
		__m128 xmm_aabbCenter0123_z = _mm_shuffle_ps(xmm_cz0_ex0_cz1_ex1, xmm_cz2_ex2_cz3_ex3, _MM_SHUFFLE(2, 0, 2, 0));
		__m128 xmm_aabbExtent0123_x = _mm_shuffle_ps(xmm_cz0_ex0_cz1_ex1, xmm_cz2_ex2_cz3_ex3, _MM_SHUFFLE(3, 1, 3, 1));

		__m128 xmm_ey0_ez0_ey1_ez1 = _mm_shuffle_ps(xmm_ey0_ez0_cx1_cy1, xmm_cz1_ex1_ey1_ez1, _MM_SHUFFLE(3, 2, 1, 0));
		__m128 xmm_ey2_ez2_ey3_ez3 = _mm_shuffle_ps(xmm_ey2_ez2_cx3_cy3, xmm_cz3_ex3_ey3_ez3, _MM_SHUFFLE(3, 2, 1, 0));
		__m128 xmm_aabbExtent0123_y = _mm_shuffle_ps(xmm_ey0_ez0_ey1_ez1, xmm_ey2_ez2_ey3_ez3, _MM_SHUFFLE(2, 0, 2, 0));
		__m128 xmm_aabbExtent0123_z = _mm_shuffle_ps(xmm_ey0_ez0_ey1_ez1, xmm_ey2_ez2_ey3_ez3, _MM_SHUFFLE(3, 1, 3, 1));

		unsigned int in_out_flag = 0x0F; // = 01111b Assume that all 4 boxes are inside the frustum.
		unsigned int intersect_flag = 0x00; // = 00000b if intersect_flag[i] == 1 then this box intersects the frustum.
		for (unsigned int iPlane = 0; iPlane < 6; ++iPlane) {
			// Calculate d...
			__m128 xmm_frustumPlane_Component = _mm_load_ps1(&frustumPlanes[iPlane].nx);
			__m128 xmm_d = _mm_mul_ps(xmm_frustumPlane_Component, xmm_aabbCenter0123_x);

			xmm_frustumPlane_Component = _mm_load_ps1(&frustumPlanes[iPlane].ny);
			xmm_frustumPlane_Component = _mm_mul_ps(xmm_frustumPlane_Component, xmm_aabbCenter0123_y);
			xmm_d = _mm_add_ps(xmm_d, xmm_frustumPlane_Component);

			xmm_frustumPlane_Component = _mm_load_ps1(&frustumPlanes[iPlane].nz);
			xmm_frustumPlane_Component = _mm_mul_ps(xmm_frustumPlane_Component, xmm_aabbCenter0123_z);
			xmm_d = _mm_add_ps(xmm_d, xmm_frustumPlane_Component);

			// Calculate r...
			xmm_frustumPlane_Component = _mm_load_ps1(&absFrustumPlanes[iPlane].nx);
			__m128 xmm_r = _mm_mul_ps(xmm_aabbExtent0123_x, xmm_frustumPlane_Component);

			xmm_frustumPlane_Component = _mm_load_ps1(&absFrustumPlanes[iPlane].ny);
			xmm_frustumPlane_Component = _mm_mul_ps(xmm_frustumPlane_Component, xmm_aabbExtent0123_y);
			xmm_r = _mm_add_ps(xmm_r, xmm_frustumPlane_Component);

			xmm_frustumPlane_Component = _mm_load_ps1(&absFrustumPlanes[iPlane].nz);
			xmm_frustumPlane_Component = _mm_mul_ps(xmm_frustumPlane_Component, xmm_aabbExtent0123_z);
			xmm_r = _mm_add_ps(xmm_r, xmm_frustumPlane_Component);

			// Calculate d + r + frustumPlane.d
			__m128 xmm_d_p_r = _mm_add_ps(xmm_d, xmm_r);
			xmm_frustumPlane_Component = _mm_load_ps1(&frustumPlanes[iPlane].d);
			xmm_d_p_r = _mm_add_ps(xmm_d_p_r, xmm_frustumPlane_Component);

			// Check which boxes are outside this plane (if any)...
			// NOTE: At this point whichever component of the xmm_d_p_r reg is negative, the corresponding 
			// box is outside the frustum. 
			unsigned int in_out_flag_curPlane = _mm_movemask_ps(xmm_d_p_r);
			in_out_flag &= ~in_out_flag_curPlane; // NOTed the mask because it's 1 for each box which is outside the frustum, and in_out_flag holds the opposite.

			// If all boxes have been marked as outside the frustum, stop checking the rest of the planes.
			if (!in_out_flag) {
				break;
			}

			// Calculate d - r + frustumPlane.d
			__m128 xmm_d_m_r = _mm_sub_ps(xmm_d, xmm_r);
			xmm_d_m_r = _mm_add_ps(xmm_d_m_r, xmm_frustumPlane_Component);

			// Check which boxes intersect the frustum...
			unsigned int intersect_flag_curPlane = _mm_movemask_ps(xmm_d_m_r);
			intersect_flag |= intersect_flag_curPlane;
		}

		// Calculate the state of the AABB from the 2 flags.
		// If the i-th bit from in_out_flag is 0, then the result will be 0 independent of the value of intersect_flag
		// If the i-th bit from in_out_flag is 1, then the result will be either 1 or 2 depending on the intersect_flag.
		result[(iIter << 2) + 0] = ((in_out_flag & 0x00000001) >> 0) << ((intersect_flag & 0x00000001) >> 0);
		result[(iIter << 2) + 1] = ((in_out_flag & 0x00000002) >> 1) << ((intersect_flag & 0x00000002) >> 1);
		result[(iIter << 2) + 2] = ((in_out_flag & 0x00000004) >> 2) << ((intersect_flag & 0x00000004) >> 2);
		result[(iIter << 2) + 3] = ((in_out_flag & 0x00000008) >> 3) << ((intersect_flag & 0x00000008) >> 3);
	}

	// Process the rest of the AABBs one by one...
	for (unsigned int iAABB = numIterations << 2; iAABB < numAABBs; ++iAABB) {
		// NOTE: This loop is identical to the CullAABBList_SSE_1() loop
		__m128 xmm_aabbCenter_x = _mm_load_ss(&aabbList[iAABB].m_center.x);
		__m128 xmm_aabbCenter_y = _mm_load_ss(&aabbList[iAABB].m_center.y);
		__m128 xmm_aabbCenter_z = _mm_load_ss(&aabbList[iAABB].m_center.z);
		__m128 xmm_aabbExtent_x = _mm_load_ss(&aabbList[iAABB].m_extent.x);
		__m128 xmm_aabbExtent_y = _mm_load_ss(&aabbList[iAABB].m_extent.y);
		__m128 xmm_aabbExtent_z = _mm_load_ss(&aabbList[iAABB].m_extent.z);

		unsigned int thisResult = Inside; // Assume that the aabb will be inside the frustum
		for (unsigned int iPlane = 0; iPlane < 6; ++iPlane) {
			__m128 xmm_frustumPlane_Component = _mm_load_ss(&frustumPlanes[iPlane].nx);
			__m128 xmm_d = _mm_mul_ss(xmm_aabbCenter_x, xmm_frustumPlane_Component);

			xmm_frustumPlane_Component = _mm_load_ss(&frustumPlanes[iPlane].ny);
			xmm_d = _mm_add_ss(xmm_d, _mm_mul_ss(xmm_aabbCenter_y, xmm_frustumPlane_Component));

			xmm_frustumPlane_Component = _mm_load_ss(&frustumPlanes[iPlane].nz);
			xmm_d = _mm_add_ss(xmm_d, _mm_mul_ss(xmm_aabbCenter_z, xmm_frustumPlane_Component));

			__m128 xmm_absFrustumPlane_Component = _mm_load_ss(&absFrustumPlanes[iPlane].nx);
			__m128 xmm_r = _mm_mul_ss(xmm_aabbExtent_x, xmm_absFrustumPlane_Component);

			xmm_absFrustumPlane_Component = _mm_load_ss(&absFrustumPlanes[iPlane].ny);
			xmm_r = _mm_add_ss(xmm_r, _mm_mul_ss(xmm_aabbExtent_y, xmm_absFrustumPlane_Component));

			xmm_absFrustumPlane_Component = _mm_load_ss(&absFrustumPlanes[iPlane].nz);
			xmm_r = _mm_add_ss(xmm_r, _mm_mul_ss(xmm_aabbExtent_z, xmm_absFrustumPlane_Component));

			__m128 xmm_frustumPlane_d = _mm_load_ss(&frustumPlanes[iPlane].d);
			__m128 xmm_d_p_r = _mm_add_ss(_mm_add_ss(xmm_d, xmm_r), xmm_frustumPlane_d);
			__m128 xmm_d_m_r = _mm_add_ss(_mm_sub_ss(xmm_d, xmm_r), xmm_frustumPlane_d);

			// Shuffle d_p_r and d_m_r in order to perform only one _mm_movmask_ps
			__m128 xmm_d_p_r__d_m_r = _mm_shuffle_ps(xmm_d_p_r, xmm_d_m_r, _MM_SHUFFLE(0, 0, 0, 0));
			int negativeMask = _mm_movemask_ps(xmm_d_p_r__d_m_r);

			// Bit 0 holds the sign of d + r and bit 2 holds the sign of d - r
			if (negativeMask & 0x01) {
				thisResult = Outside;
				break;
			}
			else if (negativeMask & 0x04) {
				thisResult = Intersect;
			}
		}

		result[iAABB] = thisResult;
	}
}


// Point-Sphere

/**
* Point-Sphere horizon culling.
*/
bool beyondHorizon(const glm::dvec3& p, const glm::dvec3& camera, const glm::dvec3 &center, double offset)
{
	auto vt = p - camera;
	auto vc = center - camera;

	float d = glm::dot(vt, vc);

	return d > length2(vc) - offset;
}