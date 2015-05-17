#include "../RenderTarget_GL.h"
#include <gl/glew.h>
#include <SDL_log.h>

namespace griffin {
	namespace render {

		RenderTarget_GL::~RenderTarget_GL()
		{
			/*if (m_depthBufferId != 0) {
				glDeleteRenderbuffers(1, &m_depthBufferId);
			}*/
			if (m_diffuseTexture != 0) {
				glDeleteTextures(1, &m_diffuseTexture);
			}
			if (m_positionTexture != 0) {
				glDeleteTextures(1, &m_positionTexture);
			}
			if (m_normalsTexture != 0) {
				glDeleteTextures(1, &m_normalsTexture);
			}
			if (m_depthTexture != 0) {
				glDeleteTextures(1, &m_depthTexture);
			}
			if (m_fboId != 0) {
				glDeleteFramebuffers(1, &m_fboId);
			}
		}

		bool RenderTarget_GL::init()
		{
			int maxDrawBuffers = 0;
			glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
			if (maxDrawBuffers < 4) {
				SDL_Log("System does not support enough draw buffers.");
				return false;
			}

			// Create and bind the frame buffer object
			glGenFramebuffers(1, &m_fboId);
			glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);

			// Bind the diffuse render target
			/*glGenRenderbuffers(1, &m_diffuseId);
			glBindRenderbuffer(GL_RENDERBUFFER, m_diffuseId);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, m_width, m_height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_diffuseId);

			// Bind the position render target
			glGenRenderbuffers(1, &m_positionId);
			glBindRenderbuffer(GL_RENDERBUFFER, m_positionId);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, m_width, m_height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, m_positionId);

			// Bind the normal render target
			glGenRenderbuffers(1, &m_normalsId);
			glBindRenderbuffer(GL_RENDERBUFFER, m_normalsId);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA16F, m_width, m_height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_RENDERBUFFER, m_normalsId);

			// Bind the depth buffer
			glGenRenderbuffers(1, &m_depthBufferId);
			glBindRenderbuffer(GL_RENDERBUFFER, m_depthBufferId);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, m_width, m_height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBufferId);
			*/

			// Generate and bind the texture for diffuse + displacement
			glGenTextures(1, &m_diffuseTexture);
			glBindTexture(GL_TEXTURE_2D, m_diffuseTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// Attach the texture to the FBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_diffuseTexture, 0);

			// Generate and bind the texture for eye-space position
			glGenTextures(1, &m_positionTexture);
			glBindTexture(GL_TEXTURE_2D, m_positionTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// Attach the texture to the FBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_positionTexture, 0);

			// Generate and bind the texture for normal + reflectance
			glGenTextures(1, &m_normalsTexture);
			glBindTexture(GL_TEXTURE_2D, m_normalsTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// Attach the texture to the FBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_normalsTexture, 0);

			// Generate and bind the texture for depth + stencil
			glGenTextures(1, &m_depthTexture);
			glBindTexture(GL_TEXTURE_2D, m_depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// Attach the texture to the FBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_depthTexture, 0);

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
			// Bind our FBO and set the viewport to the proper size
			glBindFramebuffer(GL_FRAMEBUFFER, m_fboId);
			glPushAttrib(GL_VIEWPORT_BIT);
			glViewport(0, 0, 1, 1); // why 1 for deferred rendering? Due to shader? Some tutorials use pixel width and height.

			// Clear the render targets
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

			//glActiveTexture(GL_TEXTURE0);
			//glEnable(GL_TEXTURE_2D);

			// Specify what to render an start acquiring
			GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };//, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
			//glDrawBuffers(3, buffers);
			glDrawBuffers(1, buffers);
		}

		/**
		* Stop rendering to this texture.
		*/
		void RenderTarget_GL::stop()
		{
			// Stop acquiring and unbind the FBO
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glPopAttrib();
		}


	}
}