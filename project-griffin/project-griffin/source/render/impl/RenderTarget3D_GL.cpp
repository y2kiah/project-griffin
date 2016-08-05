#include "../RenderTarget3D_GL.h"
#include <GL/glew.h>
#include <utility/Logger.h>

namespace griffin {
	namespace render {

		RenderTarget3D_GL::~RenderTarget3D_GL()
		{
			for (int r = 0; r < MAX_3D_RENDER_TARGETS; ++r) {
				if (m_textureIds[r] != 0) {
					glDeleteTextures(1, &m_textureIds[r]);
				}
			}
			
			if (m_fboId != 0) {
				glDeleteFramebuffers(1, &m_fboId);
			}
		}

		bool RenderTarget3D_GL::init(int width, int height, int layers)
		{
			m_width = width;
			m_height = height;
			m_layers = layers;

			// TEMP, move this to capabilities check, outside of real-time code path
			int maxDrawBuffers = 0;
			glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
			if (maxDrawBuffers < MAX_3D_RENDER_TARGETS) {
				logger.critical(Logger::Category_Video, "System does not support enough draw buffers.");
				return false;
			}

			// Create and bind the frame buffer object
			glGenFramebuffers(1, &m_fboId);
			glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);

			for (int r = 0; r < m_numRenderTargets; ++r) {
				if (m_type != TypeFloat16) {
					unsigned int texId = m_textureIds[r];

					if (texId != 0) {
						glDeleteTextures(1, &texId);
					}
					// Generate and bind the texture for normal + reflectance
					glGenTextures(1, &texId);
					m_textureIds[r] = texId;
					glBindTexture(GL_TEXTURE_3D, texId);
					glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, width, height, layers, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
					// Attach the texture to the FBO
					glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + r, GL_TEXTURE_3D, texId, 0, 0);
				}

				else if (m_type != TypeFloat32) {
					unsigned int texId = m_textureIds[r];

					if (texId != 0) {
						glDeleteTextures(1, &texId);
					}
					// Generate and bind the texture for normal + reflectance
					glGenTextures(1, &texId);
					m_textureIds[r] = texId;
					glBindTexture(GL_TEXTURE_3D, texId);
					glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, width, height, layers, 0, GL_RGBA, GL_FLOAT, nullptr);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
					// Attach the texture to the FBO
					glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + r, GL_TEXTURE_3D, texId, 0, 0);
				}

				else if (m_type != TypeUChar) {
					unsigned int texId = m_textureIds[r];

					if (texId != 0) {
						glDeleteTextures(1, &texId);
					}
					// Generate and bind the texture for diffuse + displacement
					glGenTextures(1, &texId);
					m_textureIds[r] = texId;
					glBindTexture(GL_TEXTURE_3D, texId);
					glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, width, height, layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
					// Attach the texture to the FBO
					glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + r, GL_TEXTURE_3D, texId, 0, 0);
				}
			}

			assert(MAX_3D_RENDER_TARGETS <= 16 && "too many render targets");

			GLenum buffers[MAX_3D_RENDER_TARGETS] = {};
			for (int r = 0; r < m_numRenderTargets; ++r) {
				buffers[r] = GL_COLOR_ATTACHMENT0 + r;
			}
			glDrawBuffers(m_numRenderTargets, buffers);

			// Check if all worked fine and unbind the FBO
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				logger.critical(Logger::Category_Video, "Can't initialize an FBO render texture. FBO initialization failed.");
				return false;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			return true;
		}

		/**
		* Start rendering to the texture
		* Both color and depth buffers are cleared.
		*/
		void RenderTarget3D_GL::start()
		{
			// TODO: use gl shadow state to avoid these calls when possible

			// Bind our FBO and set the viewport to the proper size
			glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
			//glPushAttrib(GL_VIEWPORT_BIT);
			glViewport(0, 0, m_width, m_height);
		}

		/**
		* Stop rendering to this texture.
		*/
		void RenderTarget3D_GL::stop()
		{
			// Stop acquiring and unbind the FBO
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			//glPopAttrib();
		}

		void RenderTarget3D_GL::bind(unsigned int renderTarget, unsigned int textureSlot) const
		{
			assert(renderTarget >= 0 && renderTarget < m_numRenderTargets && "invalid renderTarget index");
			assert(textureSlot >= 0 && textureSlot < 32 && "textureSlot must be in 0-31 range");

			glActiveTexture(GL_TEXTURE0 + textureSlot);
			glBindTexture(GL_TEXTURE_3D, m_textureIds[renderTarget]);
		}
	}
}