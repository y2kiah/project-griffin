#pragma once
#ifndef GRIFFIN_RENDERTARGET3D_GL_H_
#define GRIFFIN_RENDERTARGET3D_GL_H_

#include <cstdint>
#include <utility/enum.h>
#include <cassert>

namespace griffin {
	namespace render {

		#define MAX_3D_RENDER_TARGETS	4


		class RenderTarget3D_GL {
		public:
			enum RenderTarget3DType : uint8_t {
				TypeFloat16 = 0,			//<! builds 16 bit floating point render targets
				TypeFloat32 = 1,			//<! builds 32 bit floating point render targets
				TypeUChar = 2				//<! builds unsigned char render targets
			};

			explicit RenderTarget3D_GL(RenderTarget3DType type, int numRenderTargets) :
				m_type{ type },
				m_numRenderTargets{ numRenderTargets }
			{
				assert(numRenderTargets > 0 && numRenderTargets <= MAX_3D_RENDER_TARGETS && "invalid number of 3D render targets");

				memset(&m_textureIds, 0, sizeof(m_textureIds));
			}

			~RenderTarget3D_GL();

			/**
			* Initialize the render target with OpenGL, can be called again to resize the target
			* buffers.
			*/
			bool init(int width, int height, int layers);

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
			* Bind a render target to an active texture slot (0 - 31) to be sampled from a shader
			* @var	renderTarget	which render target texture to bind
			* @var	textureSlot		which texture slot to bind into, added to GL_TEXTURE0
			*/
			void bind(unsigned int renderTarget, unsigned int textureSlot) const;

		private:
			// Variables
			RenderTarget3DType	m_type = TypeFloat16;
			int					m_numRenderTargets = 0;
			int					m_width = 0;
			int					m_height = 0;
			int					m_layers = 0;
			unsigned int		m_fboId = 0;	//<! frame buffer object handle
			unsigned int		m_textureIds[MAX_3D_RENDER_TARGETS];
		};


	}
}

#endif