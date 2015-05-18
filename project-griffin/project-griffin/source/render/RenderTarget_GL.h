#pragma once
#ifndef GRIFFIN_RENDERTARGET_H_
#define GRIFFIN_RENDERTARGET_H_

#include <cstdint>

namespace griffin {
	namespace render {

		class RenderTarget_GL {
		public:
			enum RenderTargetType : uint8_t {
				Color = 0,		//<! builds only the diffuse render target texture
				Depth = 1,		//<! builds only the depth render target texture
				GBuffer = 2		//<! builds diffuse, position, normals, and depth render target textures
			};

			explicit RenderTarget_GL(RenderTargetType type, int width, int height) :
				m_width{ width }, m_height{ height }
			{}
			~RenderTarget_GL();

			/**
			* Initialize the render target with OpenGL
			*/
			bool init();

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
			RenderTargetType	m_type = Color;
			int					m_width = 0;
			int					m_height = 0;
			unsigned int		m_fboId = 0;			// frame buffer object handle
			//unsigned int		m_diffuseId = 0;		// diffuse color (rgb) + displacement (a) render target buffer
			unsigned int		m_diffuseTexture = 0;	// texture id for the diffuse (rgb) + displacement (a) render target
			//unsigned int		m_positionId = 0;		// position render target buffer
			unsigned int		m_positionTexture = 0;	// texture id for the eye-space position render target
			//unsigned int		m_normalsId = 0;		// normal (rgb) + reflectance (a) render target buffer
			unsigned int		m_normalsTexture = 0;	// texture id for the normal (rgb) + reflectance (a) render target
			//unsigned int		m_depthBufferId = 0;	// depth buffer (rgb) + stencil (a) handle
			unsigned int		m_depthTexture = 0;		// texture id for the depth (rgb) + stencil (a) render target

		};
		

	}
}

#endif