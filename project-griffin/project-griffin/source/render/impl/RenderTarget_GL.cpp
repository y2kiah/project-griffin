#include "../RenderTarget_GL.h"
#include <gl/glew.h>

namespace griffin {
	namespace render {

		RenderTarget_GL::RenderTarget_GL()
		{
			// Save extensions
			/*m_width = _dWidth;
			m_height = _dHeight;

			// Generate the OGL resources for what we need
			glGenFramebuffersEXT(1, &m_fbo);
			glGenRenderbuffersEXT(1, &m_diffuseRT);
			glGenRenderbuffersEXT(1, &m_positionRT);
			glGenRenderbuffersEXT(1, &m_normalsRT);
			glGenRenderbuffersEXT(1, &m_depthBuffer);

			// Bind the FBO so that the next operations will be bound to it
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);

			// Bind the diffuse render target
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_diffuseRT);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA, m_width, m_height);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, m_diffuseRT);

			// Bind the position render target
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_positionRT);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA32F_ARB, m_width, m_height);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_RENDERBUFFER_EXT, m_positionRT);

			// Bind the normal render target
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_normalsRT);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA16F_ARB, m_width, m_height);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT2_EXT, GL_RENDERBUFFER_EXT, m_normalsRT);

			// Bind the depth buffer
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_depthBuffer);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, m_width, m_height);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depthBuffer);

			// Generate and bind the OGL texture for diffuse
			glGenTextures(1, &m_diffuseTexture);
			glBindTexture(GL_TEXTURE_2D, m_diffuseTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// Attach the texture to the FBO
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_diffuseTexture, 0);

			// Generate and bind the OGL texture for positions
			glGenTextures(1, &m_positionTexture);
			glBindTexture(GL_TEXTURE_2D, m_positionTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, m_width, m_height, 0, GL_RGBA, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// Attach the texture to the FBO
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, m_positionTexture, 0);

			// Generate and bind the OGL texture for normals
			glGenTextures(1, &m_normalsTexture);
			glBindTexture(GL_TEXTURE_2D, m_normalsTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, m_width, m_height, 0, GL_RGBA, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// Attach the texture to the FBO
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT2_EXT, GL_TEXTURE_2D, m_normalsTexture, 0);

			// Check if all worked fine and unbind the FBO
			GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
			if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
				throw std::runtime_error("Can't initialize an FBO render texture. FBO initialization failed.");

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);*/
		}

		/**
		* Start rendering to the texture
		* Both color and depth buffers are cleared.
		*/
		void RenderTarget_GL::start()
		{
			// Bind our FBO and set the viewport to the proper size
			/*glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
			glPushAttrib(GL_VIEWPORT_BIT);
			glViewport(0, 0, 1.0f, 1.0f);

			// Clear the render targets
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

			glActiveTextureARB(GL_TEXTURE0_ARB);
			glEnable(GL_TEXTURE_2D);

			// Specify what to render an start acquiring
			GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT,
				GL_COLOR_ATTACHMENT2_EXT };
			glDrawBuffers(3, buffers);*/
		}

		/**
		* Stop rendering to this texture.
		*/
		void RenderTarget_GL::stop()
		{
			// Stop acquiring and unbind the FBO
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			glPopAttrib();
		}


	}
}