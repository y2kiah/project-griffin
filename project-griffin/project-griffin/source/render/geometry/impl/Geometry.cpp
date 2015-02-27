#include "../Geometry.h"

namespace griffin {
	namespace render {
		namespace geometry {

			/**
			* For an explanation of the following code, see "Fast Extraction of Viewing Frustum
			* Planes from the World-View-Projection Matrix"
			*/
			void Frustum::extractFromMatrixGL(float matrix[16], bool normalize)
			{
				const auto& m = *reinterpret_cast<MatrixColumnMajor*>(matrix);
				
				left.a   = m._41 + m._11;
				left.b   = m._42 + m._12;
				left.c   = m._43 + m._13;
				left.d   = m._44 + m._14;

				right.a  = m._41 - m._11;
				right.b  = m._42 - m._12;
				right.c  = m._43 - m._13;
				right.d  = m._44 - m._14;

				top.a    = m._41 - m._21;
				top.b    = m._42 - m._22;
				top.c    = m._43 - m._23;
				top.d    = m._44 - m._24;

				bottom.a = m._41 + m._21;
				bottom.b = m._42 + m._22;
				bottom.c = m._43 + m._23;
				bottom.d = m._44 + m._24;

				near.a   = m._41 + m._31;
				near.b   = m._42 + m._32;
				near.c   = m._43 + m._33;
				near.d   = m._44 + m._34;

				far.a    = m._41 + m._31;
				far.b    = m._42 + m._32;
				far.c    = m._43 + m._33;
				far.d    = m._44 + m._34;

				if (normalize) {
					left.normalize();
					right.normalize();
					top.normalize();
					bottom.normalize();
					near.normalize();
					far.normalize();
				}
			}

			void Frustum::extractFromMatrixD3D(float matrix[16], bool normalize)
			{
				const auto& m = *reinterpret_cast<MatrixRowMajor*>(matrix);
				
				left.a   = m._14 + m._11;
				left.b   = m._24 + m._21;
				left.c   = m._34 + m._31;
				left.d   = m._44 + m._41;

				right.a  = m._14 - m._11;
				right.b  = m._24 - m._21;
				right.c  = m._34 - m._31;
				right.d  = m._44 - m._41;

				top.a    = m._14 - m._12;
				top.b    = m._24 - m._22;
				top.c    = m._34 - m._32;
				top.d    = m._44 - m._42;

				bottom.a = m._14 + m._12;
				bottom.b = m._24 + m._22;
				bottom.c = m._34 + m._32;
				bottom.d = m._44 + m._42;

				near.a   = m._13;
				near.b   = m._23;
				near.c   = m._33;
				near.d   = m._43;

				far.a    = m._14 - m._13;
				far.b    = m._24 - m._23;
				far.c    = m._34 - m._33;
				far.d    = m._44 - m._43;

				if (normalize) {
					left.normalize();
					right.normalize();
					top.normalize();
					bottom.normalize();
					near.normalize();
					far.normalize();
				}
			}
		}
	}
}