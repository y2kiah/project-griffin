#pragma once
#ifndef GRIFFIN_GL_DDS_H_
#define GRIFFIN_GL_DDS_H_

#include <cstdint>

namespace griffin {
	namespace gl {

		typedef struct { // DDCOLORKEY
			uint32_t		dw1;
			uint32_t		dw2;
		} ddColorKey;

		typedef struct  { // DDSCAPS2
			uint32_t		dwCaps1;
			uint32_t		dwCaps2;
			uint32_t		Reserved[2];
		} ddCaps2;

		typedef struct _DDPIXELFORMAT { // DDPIXELFORMAT
			uint32_t		dwSize;
			uint32_t		dwFlags;
			uint32_t		dwFourCC;
			union {
				uint32_t	dwRGBBitCount;
				uint32_t	dwYUVBitCount;
				uint32_t	dwZBufferBitDepth;
				uint32_t	dwAlphaBitDepth;
				uint32_t	dwLuminanceBitCount;
				uint32_t	dwBumpBitCount;
				uint32_t	dwPrivateFormatBitCount;
			};
			union {
				uint32_t	dwRBitMask;
				uint32_t	dwYBitMask;
				uint32_t	dwStencilBitDepth;
				uint32_t	dwLuminanceBitMask;
				uint32_t	dwBumpDuBitMask;
				uint32_t	dwOperations;
			};
			union {
				uint32_t	dwGBitMask;
				uint32_t	dwUBitMask;
				uint32_t	dwZBitMask;
				uint32_t	dwBumpDvBitMask;
				struct {
					int32_t	wFlipMSTypes;
					int32_t	wBltMSTypes;
				} MultiSampleCaps;
			};
			union {
				uint32_t	dwBBitMask;
				uint32_t	dwVBitMask;
				uint32_t	dwStencilBitMask;
				uint32_t	dwBumpLuminanceBitMask;
			};
			union {
				uint32_t	dwRGBAlphaBitMask;
				uint32_t	dwYUVAlphaBitMask;
				uint32_t	dwLuminanceAlphaBitMask;
				uint32_t	dwRGBZBitMask;
				uint32_t	dwYUVZBitMask;
			};
		} ddPixelFormat;

		typedef struct ddSurface // this is lifted and adapted from DDSURFACEDESC2
		{
			uint32_t		dwSize;                 // size of the DDSURFACEDESC structure
			uint32_t		dwFlags;                // determines what fields are valid
			uint32_t		dwHeight;               // height of surface to be created
			uint32_t		dwWidth;                // width of input surface
			union {
				int32_t		lPitch;                 // distance to start of next line (return value only)
				uint32_t	dwLinearSize;           // Formless late-allocated optimized surface size
			};
			union {
				uint32_t	dwBackBufferCount;      // number of back buffers requested
				uint32_t	dwDepth;                // the depth if this is a volume texture 
			};
			union {
				uint32_t	dwMipMapCount;          // number of mip-map levels requestde
				// dwZBufferBitDepth removed, use ddpfPixelFormat one instead
				uint32_t	dwRefreshRate;          // refresh rate (used when display mode is described)
				uint32_t	dwSrcVBHandle;          // The source used in VB::Optimize
			};
			uint32_t		dwAlphaBitDepth;        // depth of alpha buffer requested
			uint32_t		dwReserved;             // reserved
			uint32_t		lpSurface;              // pointer to the associated surface memory
			union {
				ddColorKey	ddckCKDestOverlay;      // color key for destination overlay use
				uint32_t	dwEmptyFaceColor;       // Physical color for empty cubemap faces
			};
			ddColorKey		ddckCKDestBlt;          // color key for destination blt use
			ddColorKey		ddckCKSrcOverlay;       // color key for source overlay use
			ddColorKey		ddckCKSrcBlt;           // color key for source blt use
			union {
				ddPixelFormat ddpfPixelFormat;      // pixel format description of the surface
				uint32_t	dwFVF;                  // vertex format description of vertex buffers
			};
			ddCaps2			ddsCaps;                // direct draw surface capabilities
			uint32_t		dwTextureStage;         // stage in multitexture cascade
		} ddSurface;

		enum { FOURCC_DXT1 = 0x31545844, FOURCC_DXT3 = 0x33545844, FOURCC_DXT5 = 0x35545844 };

	}
}

#endif