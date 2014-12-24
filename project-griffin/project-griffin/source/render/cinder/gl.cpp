/*
Copyright (c) 2010, The Cinder Project, All rights reserved.
This code is intended for use with the Cinder C++ library: http://libcinder.org

Portions Copyright (c) 2010, The Barbarian Group
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and
the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include "gl.h"
#include "Vbo.h"
//#include "cinder/CinderMath.h"
#include "Camera.h"
#include "TriMesh.h"
#include "Sphere.h"
#include "texture/Texture.h"
#include "font/Text.h"
//#include "cinder/PolyLine.h"
//#include "cinder/Path2d.h"
//#include "cinder/Shape2d.h"
//#include "cinder/Triangulate.h"
#include <cmath>
#include <map>

namespace griffin {
	namespace gl {

		bool isExtensionAvailable(const std::string &extName)
		{
			static const char *sExtStr = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
			static std::map<std::string, bool> sExtMap;

			std::map<std::string, bool>::const_iterator extIt = sExtMap.find(extName);
			if (extIt == sExtMap.end()) {
				bool found = false;
				size_t extNameLen = extName.size();
				const char *p = sExtStr;
				const char *end = sExtStr + strlen(sExtStr);
				while (p < end) {
					size_t n = strcspn(p, " ");
					if ((extNameLen == n) && (strncmp(extName.c_str(), p, n) == 0)) {
						found = true;
						break;
					}
					p += (n + 1);
				}
				sExtMap[extName] = found;
				return found;
			} else
				return extIt->second;
		}

		void clear(const ColorA &color, bool clearDepthBuffer)
		{
			glClearColor(color.r, color.g, color.b, color.a);
			if (clearDepthBuffer) {
				glDepthMask(GL_TRUE);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			} else
				glClear(GL_COLOR_BUFFER_BIT);
		}

		void enableVerticalSync(bool enable)
		{
			GLint sync = (enable) ? 1 : 0;
			if (WGL_EXT_swap_control) {
				::wglSwapIntervalEXT(sync);
			}
		}

		bool isVerticalSyncEnabled()
		{
			if (WGL_EXT_swap_control) {
				return ::wglGetSwapIntervalEXT() > 0;
			} else {
				return true;
			}
		}

		void setModelView(const Camera &cam)
		{
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(cam.getModelViewMatrix().m);
		}

		void setProjection(const Camera &cam)
		{
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(cam.getProjectionMatrix().m);
		}

		void setMatrices(const Camera &cam)
		{
			setProjection(cam);
			setModelView(cam);
		}

		void pushModelView()
		{
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
		}

		void popModelView()
		{
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}

		void pushModelView(const Camera &cam)
		{
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadMatrixf(cam.getModelViewMatrix().m);
		}

		void pushProjection(const Camera &cam)
		{
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadMatrixf(cam.getProjectionMatrix().m);
		}

		void pushMatrices()
		{
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
		}

		void popMatrices()
		{
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}

		void multModelView(const Matrix44f &mtx)
		{
			glMatrixMode(GL_MODELVIEW);
			glMultMatrixf(mtx);
		}

		void multProjection(const Matrix44f &mtx)
		{
			glMatrixMode(GL_PROJECTION);
			glMultMatrixf(mtx);
		}

		Matrix44f getModelView()
		{
			Matrix44f result;
			glGetFloatv(GL_MODELVIEW_MATRIX, reinterpret_cast<GLfloat*>(&(result.m)));
			return result;
		}

		Matrix44f getProjection()
		{
			Matrix44f result;
			glGetFloatv(GL_PROJECTION_MATRIX, reinterpret_cast<GLfloat*>(&(result.m)));
			return result;
		}

		void setMatricesWindowPersp(int screenWidth, int screenHeight, float fovDegrees, float nearPlane, float farPlane, bool originUpperLeft)
		{
			CameraPersp cam(screenWidth, screenHeight, fovDegrees, nearPlane, farPlane);

			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(cam.getProjectionMatrix().m);

			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(cam.getModelViewMatrix().m);
			if (originUpperLeft) {
				glScalef(1.0f, -1.0f, 1.0f);           // invert Y axis so increasing Y goes down.
				glTranslatef(0.0f, (float)-screenHeight, 0.0f);       // shift origin up to upper-left corner.
			}
		}

		void setMatricesWindow(int screenWidth, int screenHeight, bool originUpperLeft)
		{
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			if (originUpperLeft) {
				glOrtho(0, screenWidth, screenHeight, 0, -1.0f, 1.0f);
			} else {
				glOrtho(0, screenWidth, 0, screenHeight, -1.0f, 1.0f);
			}

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		}

		Area getViewport()
		{
			GLint params[4];
			glGetIntegerv(GL_VIEWPORT, params);
			Area result;
			return Area(params[0], params[1], params[0] + params[2], params[1] + params[3]);
		}

		void setViewport(const Area &area)
		{
			glViewport(area.x1, area.y1, (area.x2 - area.x1), (area.y2 - area.y1));
		}

		void translate(const vec2 &pos)
		{
			glTranslatef(pos.x, pos.y, 0);
		}

		void translate(const vec3 &pos)
		{
			glTranslatef(pos.x, pos.y, pos.z);
		}

		void scale(const vec3 &scale)
		{
			glScalef(scale.x, scale.y, scale.z);
		}

		void rotate(const vec3 &xyz)
		{
			glRotatef(xyz.x, 1.0f, 0.0f, 0.0f);
			glRotatef(xyz.y, 0.0f, 1.0f, 0.0f);
			glRotatef(xyz.z, 0.0f, 0.0f, 1.0f);
		}

		void rotate(const quat &quat)
		{
			vec3 axis;
			float angle;
			quat.getAxisAngle(&axis, &angle);
			if (math<float>::abs(angle) > EPSILON_VALUE)
				glRotatef(toDegrees(angle), axis.x, axis.y, axis.z);
		}

		void enableAlphaBlending(bool premultiplied)
		{
			glEnable(GL_BLEND);
			if (!premultiplied)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}

		void disableAlphaBlending()
		{
			glDisable(GL_BLEND);
		}

		void enableAdditiveBlending()
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}

		void enableAlphaTest(float value, int func)
		{
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(func, value);
		}

		void disableAlphaTest()
		{
			glDisable(GL_ALPHA_TEST);
		}

#if ! defined( CINDER_GLES )
		void enableWireframe()
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		void disableWireframe()
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
#endif

		void disableDepthRead()
		{
			glDisable(GL_DEPTH_TEST);
		}

		void enableDepthRead(bool enable)
		{
			if (enable)
				glEnable(GL_DEPTH_TEST);
			else
				glDisable(GL_DEPTH_TEST);
		}

		void enableDepthWrite(bool enable)
		{
			glDepthMask((enable) ? GL_TRUE : GL_FALSE);
		}

		void disableDepthWrite()
		{
			glDepthMask(GL_FALSE);
		}

		void drawLine(const vec2 &start, const vec2 &end)
		{
			float lineVerts[2 * 2];
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, lineVerts);
			lineVerts[0] = start.x; lineVerts[1] = start.y;
			lineVerts[2] = end.x; lineVerts[3] = end.y;
			glDrawArrays(GL_LINES, 0, 2);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		void drawLine(const vec3 &start, const vec3 &end)
		{
			float lineVerts[3 * 2];
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, lineVerts);
			lineVerts[0] = start.x; lineVerts[1] = start.y; lineVerts[2] = start.z;
			lineVerts[3] = end.x; lineVerts[4] = end.y; lineVerts[5] = end.z;
			glDrawArrays(GL_LINES, 0, 2);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		namespace {
			void drawCubeImpl(const vec3 &c, const vec3 &size, bool drawColors)
			{
				GLfloat sx = size.x * 0.5f;
				GLfloat sy = size.y * 0.5f;
				GLfloat sz = size.z * 0.5f;
				GLfloat vertices[24 * 3] = { c.x + 1.0f*sx, c.y + 1.0f*sy, c.z + 1.0f*sz, c.x + 1.0f*sx, c.y + -1.0f*sy, c.z + 1.0f*sz, c.x + 1.0f*sx, c.y + -1.0f*sy, c.z + -1.0f*sz, c.x + 1.0f*sx, c.y + 1.0f*sy, c.z + -1.0f*sz,		// +X
					c.x + 1.0f*sx, c.y + 1.0f*sy, c.z + 1.0f*sz, c.x + 1.0f*sx, c.y + 1.0f*sy, c.z + -1.0f*sz, c.x + -1.0f*sx, c.y + 1.0f*sy, c.z + -1.0f*sz, c.x + -1.0f*sx, c.y + 1.0f*sy, c.z + 1.0f*sz,		// +Y
					c.x + 1.0f*sx, c.y + 1.0f*sy, c.z + 1.0f*sz, c.x + -1.0f*sx, c.y + 1.0f*sy, c.z + 1.0f*sz, c.x + -1.0f*sx, c.y + -1.0f*sy, c.z + 1.0f*sz, c.x + 1.0f*sx, c.y + -1.0f*sy, c.z + 1.0f*sz,		// +Z
					c.x + -1.0f*sx, c.y + 1.0f*sy, c.z + 1.0f*sz, c.x + -1.0f*sx, c.y + 1.0f*sy, c.z + -1.0f*sz, c.x + -1.0f*sx, c.y + -1.0f*sy, c.z + -1.0f*sz, c.x + -1.0f*sx, c.y + -1.0f*sy, c.z + 1.0f*sz,	// -X
					c.x + -1.0f*sx, c.y + -1.0f*sy, c.z + -1.0f*sz, c.x + 1.0f*sx, c.y + -1.0f*sy, c.z + -1.0f*sz, c.x + 1.0f*sx, c.y + -1.0f*sy, c.z + 1.0f*sz, c.x + -1.0f*sx, c.y + -1.0f*sy, c.z + 1.0f*sz,	// -Y
					c.x + 1.0f*sx, c.y + -1.0f*sy, c.z + -1.0f*sz, c.x + -1.0f*sx, c.y + -1.0f*sy, c.z + -1.0f*sz, c.x + -1.0f*sx, c.y + 1.0f*sy, c.z + -1.0f*sz, c.x + 1.0f*sx, c.y + 1.0f*sy, c.z + -1.0f*sz };	// -Z


				static GLfloat normals[24 * 3] = { 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
					0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
					0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1,
					-1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0,
					0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0,
					0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1 };

				static GLubyte colors[24 * 4] = { 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255,	// +X = red
					0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,	// +Y = green
					0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255,	// +Z = blue
					0, 255, 255, 255, 0, 255, 255, 255, 0, 255, 255, 255, 0, 255, 255, 255,	// -X = cyan
					255, 0, 255, 255, 255, 0, 255, 255, 255, 0, 255, 255, 255, 0, 255, 255,	// -Y = purple
					255, 255, 0, 255, 255, 255, 0, 255, 255, 255, 0, 255, 255, 255, 0, 255 };// -Z = yellow

				static GLfloat texs[24 * 2] = { 0, 1, 1, 1, 1, 0, 0, 0,
					1, 1, 1, 0, 0, 0, 0, 1,
					0, 1, 1, 1, 1, 0, 0, 0,
					1, 1, 1, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 1, 1, 1,
					1, 0, 0, 0, 0, 1, 1, 1 };

				static GLubyte elements[6 * 6] = { 0, 1, 2, 0, 2, 3,
					4, 5, 6, 4, 6, 7,
					8, 9, 10, 8, 10, 11,
					12, 13, 14, 12, 14, 15,
					16, 17, 18, 16, 18, 19,
					20, 21, 22, 20, 22, 23 };

				glEnableClientState(GL_NORMAL_ARRAY);
				glNormalPointer(GL_FLOAT, 0, normals);

				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 0, texs);

				if (drawColors) {
					glEnableClientState(GL_COLOR_ARRAY);
					glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
				}

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, vertices);

				glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, elements);

				glDisableClientState(GL_VERTEX_ARRAY);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glDisableClientState(GL_NORMAL_ARRAY);
				if (drawColors)
					glDisableClientState(GL_COLOR_ARRAY);

			}
		} // anonymous namespace

		void drawCube(const vec3 &center, const vec3 &size)
		{
			drawCubeImpl(center, size, false);
		}

		void drawColorCube(const vec3 &center, const vec3 &size)
		{
			drawCubeImpl(center, size, true);
		}

		void drawStrokedCube(const vec3 &center, const vec3 &size)
		{
			vec3 min = center - size * 0.5f;
			vec3 max = center + size * 0.5f;

			gl::drawLine(vec3(min.x, min.y, min.z), vec3(max.x, min.y, min.z));
			gl::drawLine(vec3(max.x, min.y, min.z), vec3(max.x, max.y, min.z));
			gl::drawLine(vec3(max.x, max.y, min.z), vec3(min.x, max.y, min.z));
			gl::drawLine(vec3(min.x, max.y, min.z), vec3(min.x, min.y, min.z));

			gl::drawLine(vec3(min.x, min.y, max.z), vec3(max.x, min.y, max.z));
			gl::drawLine(vec3(max.x, min.y, max.z), vec3(max.x, max.y, max.z));
			gl::drawLine(vec3(max.x, max.y, max.z), vec3(min.x, max.y, max.z));
			gl::drawLine(vec3(min.x, max.y, max.z), vec3(min.x, min.y, max.z));

			gl::drawLine(vec3(min.x, min.y, min.z), vec3(min.x, min.y, max.z));
			gl::drawLine(vec3(min.x, max.y, min.z), vec3(min.x, max.y, max.z));
			gl::drawLine(vec3(max.x, max.y, min.z), vec3(max.x, max.y, max.z));
			gl::drawLine(vec3(max.x, min.y, min.z), vec3(max.x, min.y, max.z));
		}

		// http://local.wasp.uwa.edu.au/~pbourke/texture_colour/spheremap/  Paul Bourke's sphere code
		// We should weigh an alternative that reduces the batch count by using GL_TRIANGLES instead
		void drawSphere(const vec3 &center, float radius, int segments)
		{
			if (segments < 0)
				return;

			float *verts = new float[(segments + 1) * 2 * 3];
			float *normals = new float[(segments + 1) * 2 * 3];
			float *texCoords = new float[(segments + 1) * 2 * 2];

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, verts);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, normals);

			for (int j = 0; j < segments / 2; j++) {
				float theta1 = j * 2 * 3.14159f / segments - (3.14159f / 2.0f);
				float theta2 = (j + 1) * 2 * 3.14159f / segments - (3.14159f / 2.0f);

				for (int i = 0; i <= segments; i++) {
					vec3 e, p;
					float theta3 = i * 2 * 3.14159f / segments;

					e.x = math<float>::cos(theta1) * math<float>::cos(theta3);
					e.y = math<float>::sin(theta1);
					e.z = math<float>::cos(theta1) * math<float>::sin(theta3);
					p = e * radius + center;
					normals[i * 3 * 2 + 0] = e.x; normals[i * 3 * 2 + 1] = e.y; normals[i * 3 * 2 + 2] = e.z;
					texCoords[i * 2 * 2 + 0] = 0.999f - i / (float)segments; texCoords[i * 2 * 2 + 1] = 0.999f - 2 * j / (float)segments;
					verts[i * 3 * 2 + 0] = p.x; verts[i * 3 * 2 + 1] = p.y; verts[i * 3 * 2 + 2] = p.z;

					e.x = math<float>::cos(theta2) * math<float>::cos(theta3);
					e.y = math<float>::sin(theta2);
					e.z = math<float>::cos(theta2) * math<float>::sin(theta3);
					p = e * radius + center;
					normals[i * 3 * 2 + 3] = e.x; normals[i * 3 * 2 + 4] = e.y; normals[i * 3 * 2 + 5] = e.z;
					texCoords[i * 2 * 2 + 2] = 0.999f - i / (float)segments; texCoords[i * 2 * 2 + 3] = 0.999f - 2 * (j + 1) / (float)segments;
					verts[i * 3 * 2 + 3] = p.x; verts[i * 3 * 2 + 4] = p.y; verts[i * 3 * 2 + 5] = p.z;
				}
				glDrawArrays(GL_TRIANGLE_STRIP, 0, (segments + 1) * 2);
			}

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);

			delete[] verts;
			delete[] normals;
			delete[] texCoords;
		}

		void draw(const class Sphere &sphere, int segments)
		{
			drawSphere(sphere.getCenter(), sphere.getRadius(), segments);
		}

		void drawSolidCircle(const vec2 &center, float radius, int numSegments)
		{
			// automatically determine the number of segments from the circumference
			if (numSegments <= 0) {
				numSegments = (int)math<double>::floor(radius * M_PI * 2);
			}
			if (numSegments < 2) numSegments = 2;

			GLfloat *verts = new float[(numSegments + 2) * 2];
			verts[0] = center.x;
			verts[1] = center.y;
			for (int s = 0; s <= numSegments; s++) {
				float t = s / (float)numSegments * 2.0f * 3.14159f;
				verts[(s + 1) * 2 + 0] = center.x + math<float>::cos(t) * radius;
				verts[(s + 1) * 2 + 1] = center.y + math<float>::sin(t) * radius;
			}
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, verts);
			glDrawArrays(GL_TRIANGLE_FAN, 0, numSegments + 2);
			glDisableClientState(GL_VERTEX_ARRAY);
			delete[] verts;
		}

		void drawStrokedCircle(const vec2 &center, float radius, int numSegments)
		{
			// automatically determine the number of segments from the circumference
			if (numSegments <= 0) {
				numSegments = (int)math<double>::floor(radius * M_PI * 2);
			}
			if (numSegments < 2) numSegments = 2;

			GLfloat *verts = new float[numSegments * 2];
			for (int s = 0; s < numSegments; s++) {
				float t = s / (float)numSegments * 2.0f * 3.14159f;
				verts[s * 2 + 0] = center.x + math<float>::cos(t) * radius;
				verts[s * 2 + 1] = center.y + math<float>::sin(t) * radius;
			}
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, verts);
			glDrawArrays(GL_LINE_LOOP, 0, numSegments);
			glDisableClientState(GL_VERTEX_ARRAY);
			delete[] verts;
		}

		void drawSolidEllipse(const vec2 &center, float radiusX, float radiusY, int numSegments)
		{
			// automatically determine the number of segments from the circumference
			if (numSegments <= 0) {
				numSegments = (int)math<double>::floor(std::max(radiusX, radiusY) * M_PI * 2);
			}
			if (numSegments < 2) numSegments = 2;

			GLfloat *verts = new float[(numSegments + 2) * 2];
			verts[0] = center.x;
			verts[1] = center.y;
			for (int s = 0; s <= numSegments; s++) {
				float t = s / (float)numSegments * 2.0f * 3.14159f;
				verts[(s + 1) * 2 + 0] = center.x + math<float>::cos(t) * radiusX;
				verts[(s + 1) * 2 + 1] = center.y + math<float>::sin(t) * radiusY;
			}
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, verts);
			glDrawArrays(GL_TRIANGLE_FAN, 0, numSegments + 2);
			glDisableClientState(GL_VERTEX_ARRAY);
			delete[] verts;
		}

		void drawStrokedEllipse(const vec2 &center, float radiusX, float radiusY, int numSegments)
		{
			// automatically determine the number of segments from the circumference
			if (numSegments <= 0) {
				numSegments = (int)math<double>::floor(std::max(radiusX, radiusY) * M_PI * 2);
			}
			if (numSegments < 2) numSegments = 2;

			GLfloat *verts = new float[numSegments * 2];
			for (int s = 0; s < numSegments; s++) {
				float t = s / (float)numSegments * 2.0f * 3.14159f;
				verts[s * 2 + 0] = center.x + math<float>::cos(t) * radiusX;
				verts[s * 2 + 1] = center.y + math<float>::sin(t) * radiusY;
			}
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, verts);
			glDrawArrays(GL_LINE_LOOP, 0, numSegments);
			glDisableClientState(GL_VERTEX_ARRAY);
			delete[] verts;
		}

		void drawSolidRect(const Rectf &rect, bool textureRectangle)
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			GLfloat verts[8];
			glVertexPointer(2, GL_FLOAT, 0, verts);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			GLfloat texCoords[8];
			glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
			verts[0 * 2 + 0] = rect.getX2(); texCoords[0 * 2 + 0] = (textureRectangle) ? rect.getX2() : 1;
			verts[0 * 2 + 1] = rect.getY1(); texCoords[0 * 2 + 1] = (textureRectangle) ? rect.getY1() : 0;
			verts[1 * 2 + 0] = rect.getX1(); texCoords[1 * 2 + 0] = (textureRectangle) ? rect.getX1() : 0;
			verts[1 * 2 + 1] = rect.getY1(); texCoords[1 * 2 + 1] = (textureRectangle) ? rect.getY1() : 0;
			verts[2 * 2 + 0] = rect.getX2(); texCoords[2 * 2 + 0] = (textureRectangle) ? rect.getX2() : 1;
			verts[2 * 2 + 1] = rect.getY2(); texCoords[2 * 2 + 1] = (textureRectangle) ? rect.getY2() : 1;
			verts[3 * 2 + 0] = rect.getX1(); texCoords[3 * 2 + 0] = (textureRectangle) ? rect.getX1() : 0;
			verts[3 * 2 + 1] = rect.getY2(); texCoords[3 * 2 + 1] = (textureRectangle) ? rect.getY2() : 1;

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		void drawStrokedRect(const Rectf &rect)
		{
			GLfloat verts[8];
			verts[0] = rect.getX1();	verts[1] = rect.getY1();
			verts[2] = rect.getX2();	verts[3] = rect.getY1();
			verts[4] = rect.getX2();	verts[5] = rect.getY2();
			verts[6] = rect.getX1();	verts[7] = rect.getY2();
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, verts);
			glDrawArrays(GL_LINE_LOOP, 0, 4);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		void drawSolidRoundedRect(const Rectf &r, float cornerRadius, int numSegmentsPerCorner)
		{
			// automatically determine the number of segments from the circumference
			if (numSegmentsPerCorner <= 0) {
				numSegmentsPerCorner = (int)math<double>::floor(cornerRadius * M_PI * 2 / 4);
			}
			if (numSegmentsPerCorner < 2) numSegmentsPerCorner = 2;

			vec2 center = r.getCenter();

			GLfloat *verts = new float[(numSegmentsPerCorner + 2) * 2 * 4 + 4];
			verts[0] = center.x;
			verts[1] = center.y;
			size_t tri = 1;
			const float angleDelta = 1 / (float)numSegmentsPerCorner * M_PI / 2;
			const float cornerCenterVerts[8] = { r.x2 - cornerRadius, r.y2 - cornerRadius, r.x1 + cornerRadius, r.y2 - cornerRadius,
				r.x1 + cornerRadius, r.y1 + cornerRadius, r.x2 - cornerRadius, r.y1 + cornerRadius };
			for (size_t corner = 0; corner < 4; ++corner) {
				float angle = corner * M_PI / 2.0f;
				vec2 cornerCenter(cornerCenterVerts[corner * 2 + 0], cornerCenterVerts[corner * 2 + 1]);
				for (int s = 0; s <= numSegmentsPerCorner; s++) {
					vec2 pt(cornerCenter.x + math<float>::cos(angle) * cornerRadius, cornerCenter.y + math<float>::sin(angle) * cornerRadius);
					verts[tri * 2 + 0] = pt.x;
					verts[tri * 2 + 1] = pt.y;
					++tri;
					angle += angleDelta;
				}
			}
			// close it off
			verts[tri * 2 + 0] = r.x2;
			verts[tri * 2 + 1] = r.y2 - cornerRadius;
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, verts);
			glDrawArrays(GL_TRIANGLE_FAN, 0, (numSegmentsPerCorner + 1) * 4 + 2);
			glDisableClientState(GL_VERTEX_ARRAY);
			delete[] verts;
		}

		void drawStrokedRoundedRect(const Rectf &r, float cornerRadius, int numSegmentsPerCorner)
		{
			// automatically determine the number of segments from the circumference
			if (numSegmentsPerCorner <= 0) {
				numSegmentsPerCorner = (int)math<double>::floor(cornerRadius * M_PI * 2 / 4);
			}
			if (numSegmentsPerCorner < 2) numSegmentsPerCorner = 2;

			GLfloat *verts = new float[(numSegmentsPerCorner + 2) * 2 * 4];
			GLsizei tri = 0;
			const float angleDelta = 1 / (float)numSegmentsPerCorner * M_PI / 2;
			const float cornerCenterVerts[8] = { r.x2 - cornerRadius, r.y2 - cornerRadius, r.x1 + cornerRadius, r.y2 - cornerRadius,
				r.x1 + cornerRadius, r.y1 + cornerRadius, r.x2 - cornerRadius, r.y1 + cornerRadius };
			for (size_t corner = 0; corner < 4; ++corner) {
				float angle = corner * M_PI / 2.0f;
				vec2 cornerCenter(cornerCenterVerts[corner * 2 + 0], cornerCenterVerts[corner * 2 + 1]);
				for (int s = 0; s <= numSegmentsPerCorner; s++) {
					vec2 pt(cornerCenter.x + math<float>::cos(angle) * cornerRadius, cornerCenter.y + math<float>::sin(angle) * cornerRadius);
					verts[tri * 2 + 0] = pt.x;
					verts[tri * 2 + 1] = pt.y;
					++tri;
					angle += angleDelta;
				}
			}
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, verts);
			glDrawArrays(GL_LINE_LOOP, 0, tri);
			glDisableClientState(GL_VERTEX_ARRAY);
			delete[] verts;
		}

		void drawSolidTriangle(const vec2 &pt1, const vec2 &pt2, const vec2 &pt3)
		{
			vec2 pts[3] = { pt1, pt2, pt3 };
			drawSolidTriangle(pts);
		}

		void drawSolidTriangle(const vec2 pts[3])
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, &pts[0].x);
			glDrawArrays(GL_TRIANGLES, 0, 3);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		void drawSolidTriangle(const vec2 &pt1, const vec2 &pt2, const vec2 &pt3, const vec2 &texPt1, const vec2 &texPt2, const vec2 &texPt3)
		{
			vec2 pts[3] = { pt1, pt2, pt3 };
			vec2 texCoords[3] = { texPt1, texPt2, texPt3 };
			drawSolidTriangle(pts, texCoords);
		}

		void drawSolidTriangle(const vec2 pts[3], const vec2 texCoord[3])
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, &pts[0].x);
			glTexCoordPointer(2, GL_FLOAT, 0, &texCoord[0].x);
			glDrawArrays(GL_TRIANGLES, 0, 3);
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		void drawStrokedTriangle(const vec2 &pt1, const vec2 &pt2, const vec2 &pt3)
		{
			vec2 pts[3] = { pt1, pt2, pt3 };
			drawStrokedTriangle(pts);
		}

		void drawStrokedTriangle(const vec2 pts[3])
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, &pts[0].x);
			glDrawArrays(GL_LINE_LOOP, 0, 3);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		void drawCoordinateFrame(float axisLength, float headLength, float headRadius)
		{
			glColor4ub(255, 0, 0, 255);
			drawVector(vec3::zero(), vec3::xAxis() * axisLength, headLength, headRadius);
			glColor4ub(0, 255, 0, 255);
			drawVector(vec3::zero(), vec3::yAxis() * axisLength, headLength, headRadius);
			glColor4ub(0, 0, 255, 255);
			drawVector(vec3::zero(), vec3::zAxis() * axisLength, headLength, headRadius);
		}

		void drawVector(const vec3 &start, const vec3 &end, float headLength, float headRadius)
		{
			const int NUM_SEGMENTS = 32;
			float lineVerts[3 * 2];
			vec3 coneVerts[NUM_SEGMENTS + 2];
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, lineVerts);
			lineVerts[0] = start.x; lineVerts[1] = start.y; lineVerts[2] = start.z;
			lineVerts[3] = end.x; lineVerts[4] = end.y; lineVerts[5] = end.z;
			glDrawArrays(GL_LINES, 0, 2);

			// Draw the cone
			vec3 axis = (end - start).normalized();
			vec3 temp = (axis.dot(vec3::yAxis()) > 0.999f) ? axis.cross(vec3::xAxis()) : axis.cross(vec3::yAxis());
			vec3 left = axis.cross(temp).normalized();
			vec3 up = axis.cross(left).normalized();

			glVertexPointer(3, GL_FLOAT, 0, &coneVerts[0].x);
			coneVerts[0] = vec3(end + axis * headLength);
			for (int s = 0; s <= NUM_SEGMENTS; ++s) {
				float t = s / (float)NUM_SEGMENTS;
				coneVerts[s + 1] = vec3(end + left * headRadius * math<float>::cos(t * 2 * 3.14159f)
										 + up * headRadius * math<float>::sin(t * 2 * 3.14159f));
			}
			glDrawArrays(GL_TRIANGLE_FAN, 0, NUM_SEGMENTS + 2);

			// draw the cap
			glVertexPointer(3, GL_FLOAT, 0, &coneVerts[0].x);
			coneVerts[0] = end;
			for (int s = 0; s <= NUM_SEGMENTS; ++s) {
				float t = s / (float)NUM_SEGMENTS;
				coneVerts[s + 1] = vec3(end - left * headRadius * math<float>::cos(t * 2 * 3.14159f)
										 + up * headRadius * math<float>::sin(t * 2 * 3.14159f));
			}
			glDrawArrays(GL_TRIANGLE_FAN, 0, NUM_SEGMENTS + 2);

			glDisableClientState(GL_VERTEX_ARRAY);
		}

		void drawFrustum(const Camera &cam)
		{
			vec3 vertex[8];
			vec3 nearTopLeft, nearTopRight, nearBottomLeft, nearBottomRight;
			cam.getNearClipCoordinates(&nearTopLeft, &nearTopRight, &nearBottomLeft, &nearBottomRight);

			vec3 farTopLeft, farTopRight, farBottomLeft, farBottomRight;
			cam.getFarClipCoordinates(&farTopLeft, &farTopRight, &farBottomLeft, &farBottomRight);

			// extract camera position from modelview matrix, so that it will work with CameraStereo as well	
			//  see: http://www.gamedev.net/topic/397751-how-to-get-camera-position/page__p__3638207#entry3638207
			mat4x4 modelview = cam.getModelViewMatrix();
			vec3 eye;
			eye.x = -(modelview.at(0, 0) * modelview.at(0, 3) + modelview.at(1, 0) * modelview.at(1, 3) + modelview.at(2, 0) * modelview.at(2, 3));
			eye.y = -(modelview.at(0, 1) * modelview.at(0, 3) + modelview.at(1, 1) * modelview.at(1, 3) + modelview.at(2, 1) * modelview.at(2, 3));
			eye.z = -(modelview.at(0, 2) * modelview.at(0, 3) + modelview.at(1, 2) * modelview.at(1, 3) + modelview.at(2, 2) * modelview.at(2, 3));

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, &vertex[0].x);

#if ! defined( CINDER_GLES )
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(3, 0xAAAA);
#endif

			vertex[0] = eye;
			vertex[1] = nearTopLeft;
			vertex[2] = eye;
			vertex[3] = nearTopRight;
			vertex[4] = eye;
			vertex[5] = nearBottomRight;
			vertex[6] = eye;
			vertex[7] = nearBottomLeft;
			glDrawArrays(GL_LINES, 0, 8);

#if ! defined( CINDER_GLES )
			glDisable(GL_LINE_STIPPLE);
#endif
			vertex[0] = farTopLeft;
			vertex[1] = nearTopLeft;
			vertex[2] = farTopRight;
			vertex[3] = nearTopRight;
			vertex[4] = farBottomRight;
			vertex[5] = nearBottomRight;
			vertex[6] = farBottomLeft;
			vertex[7] = nearBottomLeft;
			glDrawArrays(GL_LINES, 0, 8);

			glLineWidth(2.0f);
			vertex[0] = nearTopLeft;
			vertex[1] = nearTopRight;
			vertex[2] = nearBottomRight;
			vertex[3] = nearBottomLeft;
			glDrawArrays(GL_LINE_LOOP, 0, 4);

			vertex[0] = farTopLeft;
			vertex[1] = farTopRight;
			vertex[2] = farBottomRight;
			vertex[3] = farBottomLeft;
			glDrawArrays(GL_LINE_LOOP, 0, 4);

			glLineWidth(1.0f);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		void drawTorus(float outterRadius, float innerRadius, int longitudeSegments, int latitudeSegments)
		{
			longitudeSegments = std::min(std::max(7, longitudeSegments) + 1, 255);
			latitudeSegments = std::min(std::max(7, latitudeSegments) + 1, 255);

			int i, j;
			float *normal = new float[longitudeSegments * latitudeSegments * 3];
			float *vertex = new float[longitudeSegments * latitudeSegments * 3];
			float *tex = new float[longitudeSegments * latitudeSegments * 2];
			GLushort *indices = new GLushort[latitudeSegments * 2];
			float ct, st, cp, sp;

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, vertex);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, tex);
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, normal);

			for (i = 0; i < longitudeSegments; i++) {
				ct = cos(2.0f * (float)M_PI * (float)i / (float)(longitudeSegments - 1));
				st = sin(2.0f * (float)M_PI * (float)i / (float)(longitudeSegments - 1));

				for (j = 0; j < latitudeSegments; j++) {
					cp = cos(2.0f * (float)M_PI * (float)j / (float)(latitudeSegments - 1));
					sp = sin(2.0f * (float)M_PI * (float)j / (float)(latitudeSegments - 1));

					normal[3 * (i + longitudeSegments * j)] = cp * ct;
					normal[3 * (i + longitudeSegments * j) + 1] = sp * ct;
					normal[3 * (i + longitudeSegments * j) + 2] = st;

					tex[2 * (i + longitudeSegments * j)] = 1.0f * (float)i / (float)(longitudeSegments - 1);
					tex[2 * (i + longitudeSegments * j) + 1] = 5.0f * (float)j / (float)(latitudeSegments - 1);

					vertex[3 * (i + longitudeSegments * j)] = cp * (outterRadius + innerRadius * ct);
					vertex[3 * (i + longitudeSegments * j) + 1] = sp * (outterRadius + innerRadius * ct);
					vertex[3 * (i + longitudeSegments * j) + 2] = innerRadius * st;
				}
			}

			for (i = 0; i < longitudeSegments - 1; i++) {
				for (j = 0; j < latitudeSegments; j++) {
					indices[j * 2 + 0] = i + 1 + longitudeSegments * j;
					indices[j * 2 + 1] = i + longitudeSegments * j;
				}
				glDrawElements(GL_TRIANGLE_STRIP, (latitudeSegments)* 2, GL_UNSIGNED_SHORT, indices);
			}

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);

			delete[] normal;
			delete[] tex;
			delete[] vertex;
			delete[] indices;
		}

		void drawCylinder(float base, float top, float height, int slices, int stacks)
		{
			stacks = math<int>::max(2, stacks + 1);	// minimum of 1 stack
			slices = math<int>::max(4, slices + 1);	// minimum of 3 slices

			int i, j;
			float *normal = new float[stacks * slices * 3];
			float *vertex = new float[stacks * slices * 3];
			float *tex = new float[stacks * slices * 2];
			GLushort *indices = new GLushort[slices * 2];

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, vertex);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, tex);
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, normal);

			for (i = 0; i<slices; i++) {
				float u = (float)i / (float)(slices - 1);
				float ct = cos(2.0f * (float)M_PI * u);
				float st = sin(2.0f * (float)M_PI * u);

				for (j = 0; j<stacks; j++) {
					float v = (float)j / (float)(stacks - 1);
					float radius = lerp<float>(base, top, v);

					int index = 3 * (i * stacks + j);

					normal[index] = ct;
					normal[index + 1] = 0;
					normal[index + 2] = st;

					tex[2 * (i * stacks + j)] = u;
					tex[2 * (i * stacks + j) + 1] = 1.0f - v; // top of texture is top of cylinder

					vertex[index] = ct * radius;
					vertex[index + 1] = v * height;
					vertex[index + 2] = st * radius;
				}
			}

			for (i = 0; i<(stacks - 1); i++) {
				for (j = 0; j<slices; j++) {
					indices[j * 2 + 0] = i + 0 + j * stacks;
					indices[j * 2 + 1] = i + 1 + j * stacks;
				}
				glDrawElements(GL_TRIANGLE_STRIP, (slices)* 2, GL_UNSIGNED_SHORT, indices);
			}

			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			delete[] normal;
			delete[] tex;
			delete[] vertex;
			delete[] indices;
		}
		/*
		void draw(const PolyLine<vec2> &polyLine)
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, &(polyLine.getPoints()[0]));
			glDrawArrays((polyLine.isClosed()) ? GL_LINE_LOOP : GL_LINE_STRIP, 0, (GLsizei)polyLine.size());
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		void draw(const PolyLine<vec3> &polyLine)
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, &(polyLine.getPoints()[0]));
			glDrawArrays((polyLine.isClosed()) ? GL_LINE_LOOP : GL_LINE_STRIP, 0, (GLsizei)polyLine.size());
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		void draw(const Path2d &path2d, float approximationScale)
		{
			if (path2d.getNumSegments() == 0)
				return;
			std::vector<vec2> points = path2d.subdivide(approximationScale);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, &(points[0]));
			glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)points.size());
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		void draw(const Shape2d &shape2d, float approximationScale)
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			for (std::vector<Path2d>::const_iterator contourIt = shape2d.getContours().begin(); contourIt != shape2d.getContours().end(); ++contourIt) {
				if (contourIt->getNumSegments() == 0)
					continue;
				std::vector<vec2> points = contourIt->subdivide(approximationScale);
				glVertexPointer(2, GL_FLOAT, 0, &(points[0]));
				glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)points.size());
			}
			glDisableClientState(GL_VERTEX_ARRAY);
		}


		void drawSolid(const Path2d &path2d, float approximationScale)
		{
			draw(Triangulator(path2d).calcMesh());
		}

		void drawSolid(const Shape2d &shape2d, float approximationScale)
		{
			draw(Triangulator(shape2d).calcMesh());
		}

		void drawSolid(const PolyLine2f &polyLine)
		{
			draw(Triangulator(polyLine).calcMesh());
		}
		*/
		// TriMesh2d
		void draw(const TriMesh2d &mesh)
		{
			if (mesh.getNumVertices() <= 0) {
				return;
			}

			glVertexPointer(2, GL_FLOAT, 0, &(mesh.getVertices()[0]));
			glEnableClientState(GL_VERTEX_ARRAY);

			glDisableClientState(GL_NORMAL_ARRAY);

			if (mesh.hasColorsRgb()) {
				glColorPointer(3, GL_FLOAT, 0, &(mesh.getColorsRGB()[0]));
				glEnableClientState(GL_COLOR_ARRAY);
			} else if (mesh.hasColorsRgba()) {
				glColorPointer(4, GL_FLOAT, 0, &(mesh.getColorsRGBA()[0]));
				glEnableClientState(GL_COLOR_ARRAY);
			} else {
				glDisableClientState(GL_COLOR_ARRAY);
			}

			if (mesh.hasTexCoords()) {
				glTexCoordPointer(2, GL_FLOAT, 0, &(mesh.getTexCoords()[0]));
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			} else {
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}

			glDrawElements(GL_TRIANGLES, (GLsizei)mesh.getNumIndices(), GL_UNSIGNED_INT, &(mesh.getIndices()[0]));

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		// TriMesh2d
		void drawRange(const TriMesh2d &mesh, size_t startTriangle, size_t triangleCount)
		{
			glVertexPointer(2, GL_FLOAT, 0, &(mesh.getVertices()[0]));
			glEnableClientState(GL_VERTEX_ARRAY);

			glDisableClientState(GL_NORMAL_ARRAY);

			if (mesh.hasColorsRgb()) {
				glColorPointer(3, GL_FLOAT, 0, &(mesh.getColorsRGB()[0]));
				glEnableClientState(GL_COLOR_ARRAY);
			} else if (mesh.hasColorsRgba()) {
				glColorPointer(4, GL_FLOAT, 0, &(mesh.getColorsRGBA()[0]));
				glEnableClientState(GL_COLOR_ARRAY);
			} else {
				glDisableClientState(GL_COLOR_ARRAY);
			}

			if (mesh.hasTexCoords()) {
				glTexCoordPointer(2, GL_FLOAT, 0, &(mesh.getTexCoords()[0]));
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			} else {
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}

			glDrawRangeElements(GL_TRIANGLES, 0, (GLuint)mesh.getNumVertices(), (GLsizei)triangleCount * 3, GL_UNSIGNED_INT, &(mesh.getIndices()[startTriangle * 3]));
			
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		// TriMesh
		void draw(const TriMesh &mesh)
		{
			glVertexPointer(3, GL_FLOAT, 0, &(mesh.getVertices()[0]));
			glEnableClientState(GL_VERTEX_ARRAY);

			if (mesh.hasNormals()) {
				glNormalPointer(GL_FLOAT, 0, &(mesh.getNormals()[0]));
				glEnableClientState(GL_NORMAL_ARRAY);
			} else {
				glDisableClientState(GL_NORMAL_ARRAY);
			}

			if (mesh.hasColorsRGB()) {
				glColorPointer(3, GL_FLOAT, 0, &(mesh.getColorsRGB()[0]));
				glEnableClientState(GL_COLOR_ARRAY);
			} else if (mesh.hasColorsRGBA()) {
				glColorPointer(4, GL_FLOAT, 0, &(mesh.getColorsRGBA()[0]));
				glEnableClientState(GL_COLOR_ARRAY);
			} else {
				glDisableClientState(GL_COLOR_ARRAY);
			}

			if (mesh.hasTexCoords()) {
				glTexCoordPointer(2, GL_FLOAT, 0, &(mesh.getTexCoords()[0]));
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			} else {
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}

			glDrawElements(GL_TRIANGLES, (GLsizei)mesh.getNumIndices(), GL_UNSIGNED_INT, &(mesh.getIndices()[0]));

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		// TriMesh2d
		void drawRange(const TriMesh &mesh, size_t startTriangle, size_t triangleCount)
		{
			glVertexPointer(3, GL_FLOAT, 0, &(mesh.getVertices()[0]));
			glEnableClientState(GL_VERTEX_ARRAY);

			if (mesh.hasNormals()) {
				glNormalPointer(GL_FLOAT, 0, &(mesh.getNormals()[0]));
				glEnableClientState(GL_NORMAL_ARRAY);
			} else
				glDisableClientState(GL_NORMAL_ARRAY);

			if (mesh.hasColorsRGB()) {
				glColorPointer(3, GL_FLOAT, 0, &(mesh.getColorsRGB()[0]));
				glEnableClientState(GL_COLOR_ARRAY);
			} else if (mesh.hasColorsRGBA()) {
				glColorPointer(4, GL_FLOAT, 0, &(mesh.getColorsRGBA()[0]));
				glEnableClientState(GL_COLOR_ARRAY);
			} else {
				glDisableClientState(GL_COLOR_ARRAY);
			}

			if (mesh.hasTexCoords()) {
				glTexCoordPointer(2, GL_FLOAT, 0, &(mesh.getTexCoords()[0]));
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			} else {
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			}

			glDrawRangeElements(GL_TRIANGLES, 0, (GLuint)mesh.getNumVertices(), (GLsizei)triangleCount * 3, GL_UNSIGNED_INT, &(mesh.getIndices()[startTriangle * 3]));

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		void draw(const VboMesh &vbo)
		{
			if (vbo.getNumIndices() > 0)
				drawRange(vbo, (size_t)0, vbo.getNumIndices());
			else
				drawArrays(vbo, 0, (GLsizei)vbo.getNumVertices());
		}

		void drawRange(const VboMesh &vbo, size_t startIndex, size_t indexCount, int vertexStart, int vertexEnd)
		{
			if (vbo.getNumIndices() <= 0)
				return;

			if (vertexStart < 0) vertexStart = 0;
			if (vertexEnd < 0) vertexEnd = (int)vbo.getNumVertices();

			vbo.enableClientStates();
			vbo.bindAllData();

			glDrawRangeElements(vbo.getPrimitiveType(), vertexStart, vertexEnd, (GLsizei)indexCount, GL_UNSIGNED_INT, (GLvoid*)(sizeof(uint32_t) * startIndex));

			gl::VboMesh::unbindBuffers();
			vbo.disableClientStates();
		}

		void drawArrays(const VboMesh &vbo, GLint first, GLsizei count)
		{
			vbo.enableClientStates();
			vbo.bindAllData();
			glDrawArrays(vbo.getPrimitiveType(), first, count);

			gl::VboMesh::unbindBuffers();
			vbo.disableClientStates();
		}

		void drawBillboard(const vec3 &pos, const vec2 &scale, float rotationDegrees, const vec3 &bbRight, const vec3 &bbUp)
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			vec3 verts[4];
			glVertexPointer(3, GL_FLOAT, 0, &verts[0].x);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			GLfloat texCoords[8] = { 0, 0, 0, 1, 1, 0, 1, 1 };
			glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

			float sinA = math<float>::sin(toRadians(rotationDegrees));
			float cosA = math<float>::cos(toRadians(rotationDegrees));

			verts[0] = pos + bbRight * (-0.5f * scale.x * cosA - 0.5f * sinA * scale.y) + bbUp * (-0.5f * scale.x * sinA + 0.5f * cosA * scale.y);
			verts[1] = pos + bbRight * (-0.5f * scale.x * cosA - -0.5f * sinA * scale.y) + bbUp * (-0.5f * scale.x * sinA + -0.5f * cosA * scale.y);
			verts[2] = pos + bbRight * (0.5f * scale.x * cosA - 0.5f * sinA * scale.y) + bbUp * (0.5f * scale.x * sinA + 0.5f * cosA * scale.y);
			verts[3] = pos + bbRight * (0.5f * scale.x * cosA - -0.5f * sinA * scale.y) + bbUp * (0.5f * scale.x * sinA + -0.5f * cosA * scale.y);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		void draw(const Texture &texture)
		{
			draw(texture, Area(texture.getCleanBounds()), texture.getCleanBounds());
		}

		void draw(const Texture &texture, const vec2 &pos)
		{
			draw(texture, texture.getCleanBounds(), Rectf(pos.x, pos.y, pos.x + texture.getCleanWidth(), pos.y + texture.getCleanHeight()));
		}

		void draw(const Texture &texture, const Rectf &rect)
		{
			draw(texture, texture.getCleanBounds(), rect);
		}

		void draw(const Texture &texture, const Area &srcArea, const Rectf &destRect)
		{
			SaveTextureBindState saveBindState(texture.getTarget());
			BoolState saveEnabledState(texture.getTarget());
			ClientBoolState vertexArrayState(GL_VERTEX_ARRAY);
			ClientBoolState texCoordArrayState(GL_TEXTURE_COORD_ARRAY);
			texture.enableAndBind();

			glEnableClientState(GL_VERTEX_ARRAY);
			GLfloat verts[8];
			glVertexPointer(2, GL_FLOAT, 0, verts);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			GLfloat texCoords[8];
			glTexCoordPointer(2, GL_FLOAT, 0, texCoords);

			verts[0 * 2 + 0] = destRect.getX2(); verts[0 * 2 + 1] = destRect.getY1();
			verts[1 * 2 + 0] = destRect.getX1(); verts[1 * 2 + 1] = destRect.getY1();
			verts[2 * 2 + 0] = destRect.getX2(); verts[2 * 2 + 1] = destRect.getY2();
			verts[3 * 2 + 0] = destRect.getX1(); verts[3 * 2 + 1] = destRect.getY2();

			const Rectf srcCoords = texture.getAreaTexCoords(srcArea);
			texCoords[0 * 2 + 0] = srcCoords.getX2(); texCoords[0 * 2 + 1] = srcCoords.getY1();
			texCoords[1 * 2 + 0] = srcCoords.getX1(); texCoords[1 * 2 + 1] = srcCoords.getY1();
			texCoords[2 * 2 + 0] = srcCoords.getX2(); texCoords[2 * 2 + 1] = srcCoords.getY2();
			texCoords[3 * 2 + 0] = srcCoords.getX1(); texCoords[3 * 2 + 1] = srcCoords.getY2();

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		namespace {
			void drawStringHelper(const std::string &str, const vec2 &pos, const ColorA &color, Font font, int justification)
			{
				if (str.empty())
					return;

				// justification: { left = -1, center = 0, right = 1 }
				SaveColorState colorState;

				static Font defaultFont = Font::getDefault();
				if (!font)
					font = defaultFont;

				float baselineOffset;
				Texture tex(renderString(str, font, color, &baselineOffset));
				glColor4ub(255, 255, 255, 255);

				if (justification == -1) // left
					draw(tex, pos - vec2(0, baselineOffset));
				else if (justification == 0) // center
					draw(tex, pos - vec2(tex.getWidth() * 0.5f, baselineOffset));
				else // right
					draw(tex, pos - vec2((float)tex.getWidth(), baselineOffset));
			}
		} // anonymous namespace

		void drawString(const std::string &str, const vec2 &pos, const ColorA &color, Font font)
		{
			drawStringHelper(str, pos, color, font, -1);
		}

		void drawStringCentered(const std::string &str, const vec2 &pos, const ColorA &color, Font font)
		{
			drawStringHelper(str, pos, color, font, 0);
		}

		void drawStringRight(const std::string &str, const vec2 &pos, const ColorA &color, Font font)
		{
			drawStringHelper(str, pos, color, font, 1);
		}

		///////////////////////////////////////////////////////////////////////////////
		// SaveTextureBindState
		SaveTextureBindState::SaveTextureBindState(GLint target)
			: mTarget(target), mOldID(-1)
		{
			switch (target) {
				case GL_TEXTURE_2D: glGetIntegerv(GL_TEXTURE_BINDING_2D, &mOldID); break;
				case GL_TEXTURE_RECTANGLE_ARB: glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_ARB, &mOldID); break;
				case GL_TEXTURE_1D: glGetIntegerv(GL_TEXTURE_BINDING_1D, &mOldID); break;
				case GL_TEXTURE_3D: glGetIntegerv(GL_TEXTURE_BINDING_3D, &mOldID); break;
				case GL_TEXTURE_CUBE_MAP: glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &mOldID); break;
				default:
					throw gl::ExceptionUnknownTarget();
			}
		}

		SaveTextureBindState::~SaveTextureBindState()
		{
			glBindTexture(mTarget, mOldID);
		}

		///////////////////////////////////////////////////////////////////////////////
		// BoolState
		BoolState::BoolState(GLint target)
			: mTarget(target)
		{
			glGetBooleanv(target, &mOldValue);
		}

		BoolState::~BoolState()
		{
			if (mOldValue)
				glEnable(mTarget);
			else
				glDisable(mTarget);
		}

		///////////////////////////////////////////////////////////////////////////////
		// ClientBoolState
		ClientBoolState::ClientBoolState(GLint target)
			: mTarget(target)
		{
			glGetBooleanv(target, &mOldValue);
		}

		ClientBoolState::~ClientBoolState()
		{
			if (mOldValue)
				glEnableClientState(mTarget);
			else
				glDisableClientState(mTarget);
		}

		///////////////////////////////////////////////////////////////////////////////
		// SaveColorState
		SaveColorState::SaveColorState()
		{
			glGetFloatv(GL_CURRENT_COLOR, mOldValues);
		}

		SaveColorState::~SaveColorState()
		{
			// GLES doesn't have glColor4fv
			glColor4f(mOldValues[0], mOldValues[1], mOldValues[2], mOldValues[3]);
		}

		///////////////////////////////////////////////////////////////////////////////
		// SaveFramebufferBinding
		SaveFramebufferBinding::SaveFramebufferBinding()
		{
			glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &mOldValue);
		}

		SaveFramebufferBinding::~SaveFramebufferBinding()
		{
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mOldValue);
		}

	}
}
