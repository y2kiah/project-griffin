#pragma once
#ifndef GRIFFIN_RENDERTARGET_H_
#define GRIFFIN_RENDERTARGET_H_

namespace griffin {
	namespace render {

		class RenderTarget_GL {
		public:
			explicit RenderTarget_GL();
			~RenderTarget_GL() {};

			/**
			* Start rendering to the texture
			* Both color and depth buffers are cleared.
			*/
			void start();

			/**
			* Stop rendering to this texture.
			*/
			void stop();

		private:
			// Variables
			unsigned int	m_fboId;			// FBO ID
			unsigned int	m_diffuseRT;		// diffuse color (rgb) + displacement (a) render target
			unsigned int	m_diffuseTexture;	// texture id for the diffuse render target
			//unsigned int	m_positionRT;		// position render target
			//unsigned int	m_positionTexture;	// texture id for the position render target
			unsigned int	m_normalsRT;		// normal (rgb) + reflectance (a) render target
			unsigned int	m_normalsTexture;	// texture id for the normals render target
			unsigned int	m_depthBuffer;		// depth buffer (rgb) + stencil (a) handle

		};
		

	}
}

#endif