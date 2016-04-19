#include <GL/glew.h>
#include <render/RenderHelpers.h>
#include <render/VertexBuffer_GL.h>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

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

		void drawSphere()
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


		/**
		* Tesselates a section of a cube, spacing the vertices according to the tangent of the
		*	vertex normal. This will minimize spacing error after the vertices of the cube are
		*	projected into a sphere by normalizing.
		*
		* @xStartRads	starting x radian of sphere section
		* @yStartRads	starting y radian of sphere section
		* @radiansSpan	radians of sphere surface, should be no more than pi/4 (45 degrees)
		* @numVertices	number of vertices per side to tesselate
		* @xStride		byte distance between start of vertices in the data buffer
		* @yStride		byte distance between start of rows in the data buffer
		* @data			location of first vertex in data buffer to be written
		*/
		void tesselateCubeSectionByTangent(float xStartRads, float yStartRads, float radiansSpan,
										   int numVertices, int xStride, int yStride, float* data)
		{
			assert(radiansSpan >= 0.0f && radiansSpan <= glm::pi<float>() / 4.0f); // radian span for one section should be no more than 45 degrees
			assert(numVertices >= 2 && xStartRads >= 0.0f && yStartRads >= 0.0f);
			assert(xStride >= 3 && yStride >= numVertices * 3);

			float radianInc = radiansSpan / (numVertices - 1);

			float currentYRad = yStartRads;

			for (int y = 0; y < numVertices; ++y) {
				float valueY = tanf(currentYRad);
				
				float currentXRad = xStartRads;

				for (int x = 0; x < numVertices; ++x) {
					float valueX = tanf(currentXRad);

					glm::vec3 v = glm::normalize(glm::vec3(valueX, valueY, -1.0f));
					//v *= radius; // 6378000.0f;

					float *vert = data + (y * yStride) + (x * xStride);
					vert[0] = v.x;
					vert[1] = v.y;
					vert[2] = v.z;

					currentXRad += radianInc;
				}
				currentYRad += radianInc;
			}
		}


		// Lighting

		/**
		* Solve light attenuation for 5/256 to get light radius
		* 5/256 = Imax / (Kc + Kl*d + Kq*d*d)
		*	where Imax is light's largest RGB component value
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