#pragma once
#ifndef GRIFFIN_DDS_H_
#define GRIFFIN_DDS_H_

#include <vector>
#include <cassert>
#include <cstdint>

// TODO: get rid of use of vector, don't need private members or friend classes, do we need operator= and copy constructors?

namespace griffin {
	namespace render {
		
		// The typedefs below are not currently used but could be

		//typedef struct { // DDCOLORKEY
		//	uint32_t		dw1;
		//	uint32_t		dw2;
		//} ddColorKey;

		//typedef struct  { // DDSCAPS2
		//	uint32_t		dwCaps1;
		//	uint32_t		dwCaps2;
		//	uint32_t		Reserved[2];
		//} ddCaps2;

		//typedef struct _DDPIXELFORMAT { // DDPIXELFORMAT
		//	uint32_t		dwSize;
		//	uint32_t		dwFlags;
		//	uint32_t		dwFourCC;
		//	union {
		//		uint32_t	dwRGBBitCount;
		//		uint32_t	dwYUVBitCount;
		//		uint32_t	dwZBufferBitDepth;
		//		uint32_t	dwAlphaBitDepth;
		//		uint32_t	dwLuminanceBitCount;
		//		uint32_t	dwBumpBitCount;
		//		uint32_t	dwPrivateFormatBitCount;
		//	};
		//	union {
		//		uint32_t	dwRBitMask;
		//		uint32_t	dwYBitMask;
		//		uint32_t	dwStencilBitDepth;
		//		uint32_t	dwLuminanceBitMask;
		//		uint32_t	dwBumpDuBitMask;
		//		uint32_t	dwOperations;
		//	};
		//	union {
		//		uint32_t	dwGBitMask;
		//		uint32_t	dwUBitMask;
		//		uint32_t	dwZBitMask;
		//		uint32_t	dwBumpDvBitMask;
		//		struct {
		//			int32_t	wFlipMSTypes;
		//			int32_t	wBltMSTypes;
		//		} MultiSampleCaps;
		//	};
		//	union {
		//		uint32_t	dwBBitMask;
		//		uint32_t	dwVBitMask;
		//		uint32_t	dwStencilBitMask;
		//		uint32_t	dwBumpLuminanceBitMask;
		//	};
		//	union {
		//		uint32_t	dwRGBAlphaBitMask;
		//		uint32_t	dwYUVAlphaBitMask;
		//		uint32_t	dwLuminanceAlphaBitMask;
		//		uint32_t	dwRGBZBitMask;
		//		uint32_t	dwYUVZBitMask;
		//	};
		//} ddPixelFormat;

		//typedef struct ddSurface // this is lifted and adapted from DDSURFACEDESC2
		//{
		//	uint32_t		dwSize;                 // size of the DDSURFACEDESC structure
		//	uint32_t		dwFlags;                // determines what fields are valid
		//	uint32_t		dwHeight;               // height of surface to be created
		//	uint32_t		dwWidth;                // width of input surface
		//	union {
		//		int32_t		lPitch;                 // distance to start of next line (return value only)
		//		uint32_t	dwLinearSize;           // Formless late-allocated optimized surface size
		//	};
		//	union {
		//		uint32_t	dwBackBufferCount;      // number of back buffers requested
		//		uint32_t	dwDepth;                // the depth if this is a volume texture 
		//	};
		//	union {
		//		uint32_t	dwMipMapCount;          // number of mip-map levels requestde
		//		// dwZBufferBitDepth removed, use ddpfPixelFormat one instead
		//		uint32_t	dwRefreshRate;          // refresh rate (used when display mode is described)
		//		uint32_t	dwSrcVBHandle;          // The source used in VB::Optimize
		//	};
		//	uint32_t		dwAlphaBitDepth;        // depth of alpha buffer requested
		//	uint32_t		dwReserved;             // reserved
		//	uint32_t		lpSurface;              // pointer to the associated surface memory
		//	union {
		//		ddColorKey	ddckCKDestOverlay;      // color key for destination overlay use
		//		uint32_t	dwEmptyFaceColor;       // Physical color for empty cubemap faces
		//	};
		//	ddColorKey		ddckCKDestBlt;          // color key for destination blt use
		//	ddColorKey		ddckCKSrcOverlay;       // color key for source overlay use
		//	ddColorKey		ddckCKSrcBlt;           // color key for source blt use
		//	union {
		//		ddPixelFormat ddpfPixelFormat;      // pixel format description of the surface
		//		uint32_t	dwFVF;                  // vertex format description of vertex buffers
		//	};
		//	ddCaps2			ddsCaps;                // direct draw surface capabilities
		//	uint32_t		dwTextureStage;         // stage in multitexture cascade
		//} ddSurface;

		//enum { FOURCC_DXT1 = 0x31545844, FOURCC_DXT3 = 0x33545844, FOURCC_DXT5 = 0x35545844 };


		const uint32_t DDS_FOURCC  = 0x00000004;
		const uint32_t DDS_RGB     = 0x00000040;
		const uint32_t DDS_RGBA    = 0x00000041;
		const uint32_t DDS_DEPTH   = 0x00800000;

		const uint32_t DDS_COMPLEX = 0x00000008;
		const uint32_t DDS_CUBEMAP = 0x00000200;
		const uint32_t DDS_VOLUME  = 0x00200000;

		const uint32_t FOURCC_DXT1 = 0x31545844; //(MAKEFOURCC('D','X','T','1'))
		const uint32_t FOURCC_DXT3 = 0x33545844; //(MAKEFOURCC('D','X','T','3'))
		const uint32_t FOURCC_DXT5 = 0x35545844; //(MAKEFOURCC('D','X','T','5'))

		struct DDSPixelFormat
		{
			uint32_t dwSize;
			uint32_t dwFlags;
			uint32_t dwFourCC;
			uint32_t dwRGBBitCount;
			uint32_t dwRBitMask;
			uint32_t dwGBitMask;
			uint32_t dwBBitMask;
			uint32_t dwABitMask;
		};

		struct DXTColBlock
		{
			uint16_t col0;
			uint16_t col1;

			uint8_t  row[4];
		};

		struct DXT3AlphaBlock
		{
			uint16_t row[4];
		};

		struct DXT5AlphaBlock
		{
			uint8_t alpha0;
			uint8_t alpha1;

			uint8_t row[6];
		};

		struct DDSHeader
		{
			uint32_t dwSize;
			uint32_t dwFlags;
			uint32_t dwHeight;
			uint32_t dwWidth;
			uint32_t dwPitchOrLinearSize;
			uint32_t dwDepth;
			uint32_t dwMipMapCount;
			uint32_t dwReserved1[11];
			DDSPixelFormat ddspf;
			uint32_t dwCaps1;
			uint32_t dwCaps2;
			uint32_t dwReserved2[3];
		};

		class DDSSurface
		{
			friend class DDSTexture;
			friend class DDSImage;

		public:
			DDSSurface();
			DDSSurface(int w, int h, int d, int imgsize);
			DDSSurface(const DDSSurface& copy);
			DDSSurface &operator= (const DDSSurface& rhs);
			virtual ~DDSSurface();

			operator char*();

			void create(int w, int h, int d, int imgsize);
			void clear();

			int get_width()  { return width; }
			int get_height() { return height; }
			int get_depth()  { return depth; }
			int get_size()   { return size; }

		protected:
			int   width;
			int   height;
			int   depth;
			int   size;

			char* pixels;
		};

		class DDSTexture : public DDSSurface
		{
			friend class DDSImage;

		public:
			DDSTexture();
			DDSTexture(int w, int h, int d, int imgSize);
			DDSTexture(const DDSTexture& copy);
			DDSTexture& operator=(const DDSTexture& rhs);
			~DDSTexture();

			DDSSurface& get_mipmap(int index)
			{
				assert(index < (int)mipmaps.size());
				return mipmaps[index];
			}

			int get_num_mipmaps() { return (int)mipmaps.size(); }

		protected:
			std::vector<DDSSurface> mipmaps;
		};

		class DDSImage
		{
		public:
			DDSImage();
			~DDSImage();

			bool loadFromMemory(unsigned char* data, bool flipImage = true);
			bool load(const char* filename, bool flipImage = true);
			void clear();

			operator char*();
			DDSTexture& operator[](int index);

			bool upload_texture1D();
			bool upload_texture2D(int imageIndex = 0, unsigned int glTarget = 0x0DE1); // GLenum = GL_TEXTURE_2D
			bool upload_textureRectangle();
			bool upload_texture3D();
			bool upload_textureCubemap();

			int get_width()
			{
				assert(valid && images.size() > 0);

				return images[0].get_width();
			}

			int get_height()
			{
				assert(valid && images.size() > 0);

				return images[0].get_height();
			}

			int get_depth()
			{
				assert(valid && images.size() > 0);

				return images[0].get_depth();
			}

			int get_size()
			{
				assert(valid && images.size() > 0);

				return images[0].get_size();
			}

			int get_num_mipmaps()
			{
				assert(valid && images.size() > 0);

				return images[0].get_num_mipmaps();
			}

			inline DDSSurface& get_mipmap(int index)
			{
				assert(valid && images.size() > 0 && index < images[0].get_num_mipmaps());

				return images[0].get_mipmap(index);
			}

			int get_components() { return components; }
			int get_format()     { return format; }

			bool is_compressed() { return compressed; }
			bool is_cubemap()    { return cubemap; }
			bool is_volume()     { return volume; }
			bool is_valid()      { return valid; }

		private:
			int clamp_size(int size);
			int get_line_width(int width, int bpp);
			int size_dxtc(int width, int height);
			int size_rgb(int width, int height);
			void align_memory(DDSTexture* surface);

			void flip(char* image, int width, int height, int depth, int size);

			void swap(void* byte1, void* byte2, int size);
			void flip_blocks_dxtc1(DXTColBlock* line, int numBlocks);
			void flip_blocks_dxtc3(DXTColBlock* line, int numBlocks);
			void flip_blocks_dxtc5(DXTColBlock* line, int numBlocks);
			void flip_dxt5_alpha(DXT5AlphaBlock* block);

			int  format;
			int  components;
			bool compressed;
			bool cubemap;
			bool volume;
			bool valid;

			std::vector<DDSTexture> images;
		};
	}
}
#endif