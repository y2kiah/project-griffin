#include <GL/glew.h>
#include <render/RenderHelpers.h>
#include <render/VertexBuffer_GL.h>

namespace griffin {
	namespace render {
		// Globals

		float g_fullScreenQuadBufferData[3 * 6] = {
			-1.0f, -1.0f,  0.0f,
			 1.0f, -1.0f,  0.0f,
			-1.0f,  1.0f,  0.0f,
			-1.0f,  1.0f,  0.0f,
			 1.0f, -1.0f,  0.0f,
			 1.0f,  1.0f,  0.0f
		};
		uint32_t		g_glQuadVAO = 0;	//<! Vertex Array Object for fullScreenQuad
		VertexBuffer_GL	g_fullScreenQuad;

		/*
		  4-----7
		 /|    /|
		0-+---3 |
		| 5---+-6
		|/    |/
		1-----2
		*/
		// cube vertices for vertex buffer object
		float g_cubeBufferData[3 * 36] = {
			// front face
			-1.0f,  1.0f,  1.0f, // 0
			 1.0f,  1.0f,  1.0f, // 3
			-1.0f, -1.0f,  1.0f, // 1
			-1.0f, -1.0f,  1.0f, // 1
			 1.0f,  1.0f,  1.0f, // 3
			 1.0f, -1.0f,  1.0f, // 2

			// right face
			 1.0f,  1.0f,  1.0f, // 3
			 1.0f,  1.0f, -1.0f, // 7
			 1.0f, -1.0f,  1.0f, // 2
			 1.0f, -1.0f,  1.0f, // 2
			 1.0f,  1.0f, -1.0f, // 7
			 1.0f, -1.0f, -1.0f, // 6

			// back face
			 1.0f,  1.0f, -1.0f, // 7
			-1.0f,  1.0f, -1.0f, // 4
			 1.0f, -1.0f, -1.0f, // 6
			 1.0f, -1.0f, -1.0f, // 6
			-1.0f,  1.0f, -1.0f, // 4
			-1.0f, -1.0f, -1.0f, // 5

			// left face
			-1.0f,  1.0f, -1.0f, // 4
			-1.0f,  1.0f,  1.0f, // 0
			-1.0f, -1.0f, -1.0f, // 5
			-1.0f, -1.0f, -1.0f, // 5
			-1.0f,  1.0f,  1.0f, // 0
			-1.0f, -1.0f,  1.0f, // 1

			// bottom face
			-1.0f, -1.0f,  1.0f, // 1
			 1.0f, -1.0f,  1.0f, // 2
			-1.0f, -1.0f, -1.0f, // 5
			-1.0f, -1.0f, -1.0f, // 5
			 1.0f, -1.0f,  1.0f, // 2
			 1.0f, -1.0f, -1.0f, // 6

			// top face
			-1.0f,  1.0f, -1.0f, // 4
			 1.0f,  1.0f, -1.0f, // 7
			-1.0f,  1.0f,  1.0f, // 0
			-1.0f,  1.0f,  1.0f, // 0
			 1.0f,  1.0f, -1.0f, // 7
			 1.0f,  1.0f,  1.0f, // 3
		};
		uint32_t		g_glCubeVAO = 0;
		VertexBuffer_GL	g_cubeBuffer;


		// Free Functions

		void drawFullscreenQuad()
		{
			glBindVertexArray(g_glQuadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		void drawCube()
		{
			glBindVertexArray(g_glCubeVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		void drawPixelPerfectQuad(float leftPx, float topPx, uint32_t widthPx, uint32_t heightPx)
		{

		}

		void drawScaledQuad(float left, float top, float width, float height)
		{
		}

	}
}