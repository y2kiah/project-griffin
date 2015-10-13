#include <GL/glew.h>
#include <render/RenderHelpers.h>
#include <render/VertexBuffer_GL.h>
#include <cmath>

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


		// Draw shapes and volumes

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


		// Lighting

		/**
		* Solve light attenuation for 5/256 to get light radius
		* 5/256 = Imax / (Kc + Kl*d + Kq*d*d)
		*	where Imax is 
		*
		* @Imax	light's brightest color component
		* @Kc	attenuation base constant
		* @Kl	attenuation linear constant
		* @Kq	attenuation quadratic constant
		* @see	http://learnopengl.com/#!Advanced-Lighting/Deferred-Shading
		*
		* Table to help choose values:
		*	Distance	Constant	Linear	Quadratic
		*	7			1.0			0.7		1.8
		*	13			1.0			0.35	0.44
		*	20			1.0			0.22	0.20
		*	32			1.0			0.14	0.07
		*	50			1.0			0.09	0.032
		*	65			1.0			0.07	0.017
		*	100			1.0			0.045	0.0075
		*	160			1.0			0.027	0.0028
		*	200			1.0			0.022	0.0019
		*	325			1.0			0.014	0.0007
		*	600			1.0			0.007	0.0002
		*	3250		1.0			0.0014	0.000007
		*/
		float getLightVolumeRadius(float Imax, float Kc, float Kl, float Kq)
		{
			return (-Kl + std::sqrtf(Kl * Kl - 4.0f * Kq * (Kc - (256.0f / 5.0f) * Imax))) / (2.0f * Kq);
		}
	}
}