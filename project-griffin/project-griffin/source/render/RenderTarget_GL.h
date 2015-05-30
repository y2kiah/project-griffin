#pragma once
#ifndef GRIFFIN_RENDERTARGET_GL_H_
#define GRIFFIN_RENDERTARGET_GL_H_

#include <cstdint>
#include <utility/enum.h>

namespace griffin {
	namespace render {

		class RenderTarget_GL {
		public:
			enum RenderTargetType : uint8_t {
				Color = 0,		//<! builds only the diffuse render target texture
				Depth = 1,		//<! builds only the depth render target texture
				GBuffer = 2		//<! builds diffuse, position, normals, and depth render target textures
			};

			enum RenderTargetTexture : uint8_t {
				Albedo_Displacement = 0,	//<! diffuse albedo (rgb) + displacement (a) render target
				Position = 1,				//<! the eye-space position render target
				Normal_Reflectance = 2,		//<! normal (rgb) + reflectance (a) render target
				Depth_Stencil = 3			//<! depth (rgb) + stencil (a) render target
			};

			explicit RenderTarget_GL(RenderTargetType type = Color) :
				m_type{ type }
			{
				memset(&m_textureIds, 0, sizeof(m_textureIds));
			}
			~RenderTarget_GL();

			/**
			* Initialize the render target with OpenGL, can be called again to resize the target
			* buffers.
			*/
			bool init(int width, int height);

			/**
			* Start rendering to the texture
			* Both color and depth buffers are cleared.
			*/
			void start();

			/**
			* Stop rendering to this texture.
			*/
			void stop();

			/**
			* Bind a render target to an active texture slot (GL_TEXTURE0 - GL_TEXTURE31) to be
			* sampled from a shader program.
			* @
			*/
			void bind(RenderTargetTexture renderTarget, unsigned int textureSlot) const;

		private:
			// Variables
			RenderTargetType	m_type = Color;
			int					m_width = 0;
			int					m_height = 0;
			unsigned int		m_fboId = 0;	//<! frame buffer object handle
			unsigned int		m_textureIds[4];
		};
		

	}
}

#endif