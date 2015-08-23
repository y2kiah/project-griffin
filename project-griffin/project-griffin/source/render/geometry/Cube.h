#pragma once
#ifndef GRIFFIN_RENDER_CUBE_H_
#define GRIFFIN_RENDER_CUBE_H_

#include <render/VertexBuffer_GL.h>
#include <render/IndexBuffer_GL.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>


namespace griffin {
	namespace render {

		class Cube {
		public:
			explicit Cube();

		private:
			VertexBuffer_GL m_vb;
			IndexBuffer_GL  m_ib;
		};


		struct CubeInstance {
			glm::mat4 transform;
		};

	}
}

#endif