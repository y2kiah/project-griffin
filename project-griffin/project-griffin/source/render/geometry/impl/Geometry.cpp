#include "../Geometry.h"

using namespace griffin;
using namespace griffin::geometry;
			

/**
* For an explanation of the following code, see "Fast Extraction of Viewing Frustum
* Planes from the World-View-Projection Matrix"
*/
void Frustum::extractFromMatrixGL(float matrix[16], bool normalize)
{
	const auto& m = *reinterpret_cast<MatrixColumnMajor*>(matrix);
				
	nx[Near]   = m._41 + m._31;
	ny[Near]   = m._42 + m._32;
	nz[Near]   = m._43 + m._33;
	d[Near]    = m._44 + m._34;

	nx[Far]    = m._41 + m._31;
	ny[Far]    = m._42 + m._32;
	nz[Far]    = m._43 + m._33;
	d[Far]     = m._44 + m._34;

	nx[Left]   = m._41 + m._11;
	ny[Left]   = m._42 + m._12;
	nz[Left]   = m._43 + m._13;
	d[Left]    = m._44 + m._14;

	nx[Right]  = m._41 - m._11;
	ny[Right]  = m._42 - m._12;
	nz[Right]  = m._43 - m._13;
	d[Right]   = m._44 - m._14;

	nx[Top]    = m._41 - m._21;
	ny[Top]    = m._42 - m._22;
	nz[Top]    = m._43 - m._23;
	d[Top]     = m._44 - m._24;

	nx[Bottom] = m._41 + m._21;
	ny[Bottom] = m._42 + m._22;
	nz[Bottom] = m._43 + m._23;
	d[Bottom]  = m._44 + m._24;

	if (normalize) {
		// TODO: find a faster way to normalize all planes, use SIMD code like the collision detection routine would
		Plane planes[6] = {};
		getPlanes(planes);
		for (int p = 0; p < 6; ++p) {
			planes[p].normalize();
			nx[p] = planes[p].nx;
			ny[p] = planes[p].ny;
			nz[p] = planes[p].nz;
			d[p] = planes[p].d;
		}
	}
}

void Frustum::extractFromMatrixD3D(float matrix[16], bool normalize)
{
	const auto& m = *reinterpret_cast<MatrixRowMajor*>(matrix);
				
	nx[Near]   = m._13;
	ny[Near]   = m._23;
	nz[Near]   = m._33;
	d[Near]    = m._43;

	nx[Far]    = m._14 - m._13;
	ny[Far]    = m._24 - m._23;
	nz[Far]    = m._34 - m._33;
	d[Far]     = m._44 - m._43;

	nx[Left]   = m._14 + m._11;
	ny[Left]   = m._24 + m._21;
	nz[Left]   = m._34 + m._31;
	d[Left]    = m._44 + m._41;

	nx[Right]  = m._14 - m._11;
	ny[Right]  = m._24 - m._21;
	nz[Right]  = m._34 - m._31;
	d[Right]   = m._44 - m._41;

	nx[Top]    = m._14 - m._12;
	ny[Top]    = m._24 - m._22;
	nz[Top]    = m._34 - m._32;
	d[Top]     = m._44 - m._42;

	nx[Bottom] = m._14 + m._12;
	ny[Bottom] = m._24 + m._22;
	nz[Bottom] = m._34 + m._32;
	d[Bottom]  = m._44 + m._42;

	if (normalize) {
		// TODO: find a faster way to normalize all planes, use SSE intrinsics like the collision detection code
		Plane planes[6] = {};
		getPlanes(planes);
		for (int p = 0; p < 6; ++p) {
			planes[p].normalize();
			nx[p] = planes[p].nx;
			ny[p] = planes[p].ny;
			nz[p] = planes[p].nz;
			d[p] = planes[p].d;
		}
	}
}
