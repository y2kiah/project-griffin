#include "../RenderTarget_GL.h"
#include <GL/glew.h>
#include <SDL_log.h>

namespace griffin {
	namespace render {

		RenderTarget_GL::~RenderTarget_GL()
		{
			/*if (m_depthBufferId != 0) {
				glDeleteRenderbuffers(1, &m_depthBufferId);
			}*/
			if (m_textureIds[Albedo_Displacement] != 0) {
				glDeleteTextures(1, &m_textureIds[Albedo_Displacement]);
			}
			if (m_textureIds[Position] != 0) {
				glDeleteTextures(1, &m_textureIds[Position]);
			}
			if (m_textureIds[Normal_Reflectance] != 0) {
				glDeleteTextures(1, &m_textureIds[Normal_Reflectance]);
			}
			if (m_textureIds[Depth_Stencil] != 0) {
				glDeleteTextures(1, &m_textureIds[Depth_Stencil]);
			}
			if (m_fboId != 0) {
				glDeleteFramebuffers(1, &m_fboId);
			}
		}

		bool RenderTarget_GL::init(int width, int height)
		{
			m_width = width;
			m_height = height;

			int maxDrawBuffers = 0;
			glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
			if (maxDrawBuffers < 4) {
				SDL_Log("System does not support enough draw buffers.");
				return false;
			}

			// Create and bind the frame buffer object
			glGenFramebuffers(1, &m_fboId);
			glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);

			/*
			// Bind the depth buffer
			glGenRenderbuffers(1, &m_depthBufferId);
			glBindRenderbuffer(GL_RENDERBUFFER, m_depthBufferId);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, m_width, m_height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBufferId);
			*/

			if (m_type != TypeDepthStencil) {
				unsigned int albedo = m_textureIds[Albedo_Displacement];

				if (albedo != 0) {
					glDeleteTextures(1, &albedo);
				}
				// Generate and bind the texture for diffuse + displacement
				glGenTextures(1, &albedo);
				m_textureIds[Albedo_Displacement] = albedo;
				glBindTexture(GL_TEXTURE_2D, albedo);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				// Attach the texture to the FBO
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedo, 0);
			}

			if (m_type == TypeGBuffer) {
				unsigned int position = m_textureIds[Position];
				unsigned int normal = m_textureIds[Normal_Reflectance];

				if (position != 0) {
					glDeleteTextures(1, &position);
				}
				// Generate and bind the texture for eye-space position
				glGenTextures(1, &position);
				m_textureIds[Position] = position;
				glBindTexture(GL_TEXTURE_2D, position);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				// Attach the texture to the FBO
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, position, 0);

				if (normal != 0) {
					glDeleteTextures(1, &normal);
				}
				// Generate and bind the texture for normal + reflectance
				glGenTextures(1, &normal);
				m_textureIds[Normal_Reflectance] = normal;
				glBindTexture(GL_TEXTURE_2D, normal);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				// Attach the texture to the FBO
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, normal, 0);
			}

			if (m_type != TypeColor) {
				unsigned int depth = m_textureIds[Depth_Stencil];

				if (depth != 0) {
					glDeleteTextures(1, &depth);
				}
				// Generate and bind the texture for depth + stencil
				glGenTextures(1, &depth);
				m_textureIds[Depth_Stencil] = depth;
				glBindTexture(GL_TEXTURE_2D, depth);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				
				//glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
				
				// Attach the texture to the FBO
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
			}

			if (m_type == TypeColor) {
				GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
				glDrawBuffers(1, buffers);
			}
			else if (m_type == TypeDepthStencil) {
				glDrawBuffer(GL_NONE); // no color buffer
				glReadBuffer(GL_NONE);
			}
			else if (m_type == TypeColorDepthStencil) {
				GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
				glDrawBuffers(1, buffers);
			}
			else if (m_type == TypeGBuffer) {
				GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
				glDrawBuffers(3, buffers);
			}

			// Check if all worked fine and unbind the FBO
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				SDL_Log("Can't initialize an FBO render texture. FBO initialization failed.");
				return false;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			return true;
		}

		/**
		* Start rendering to the texture
		* Both color and depth buffers are cleared.
		*/
		void RenderTarget_GL::start()
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
		void RenderTarget_GL::stop()
		{
			// Stop acquiring and unbind the FBO
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			//glPopAttrib();
		}

		void RenderTarget_GL::bind(RenderTargetTexture renderTarget, unsigned int textureSlot) const
		{
			glActiveTexture(textureSlot);
			glBindTexture(GL_TEXTURE_2D, m_textureIds[renderTarget]);
		}
	}
}