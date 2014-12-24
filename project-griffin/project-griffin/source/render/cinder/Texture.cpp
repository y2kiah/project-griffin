/*
Copyright (c) 2010, The Barbarian Group
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and
the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include "../Texture.h"
#include "../dds.h"

#include "../../gl.h"
//#include "cinder/ImageIo.h"
#include <cstdio>

using namespace std;

namespace griffin {
	namespace gl {

//		class ImageSourceTexture;
//		class ImageTargetTexture;

/*		TextureDataExc::TextureDataExc(const std::string &log) throw()
		{
			strncpy(mMessage, log.c_str(), 16000);
		}
*/
		/////////////////////////////////////////////////////////////////////////////////
		// ImageTargetGLTexture
		template<typename T>
		class ImageTargetGLTexture : public ImageTarget {
		public:
			static shared_ptr<ImageTargetGLTexture> createRef(const Texture *aTexture, ImageIo::ChannelOrder &aChannelOrder, bool aIsGray, bool aHasAlpha);
			~ImageTargetGLTexture();

			virtual bool	hasAlpha() const { return mHasAlpha; }
			virtual void*	getRowPointer(int32_t row) { return mData + row * mRowInc; }

			const void*		getData() const { return mData; }

		private:
			ImageTargetGLTexture(const Texture *aTexture, ImageIo::ChannelOrder &aChannelOrder, bool aIsGray, bool aHasAlpha);
			const Texture		*mTexture;
			bool				mIsGray;
			bool				mHasAlpha;
			uint8_t				mPixelInc;
			T					*mData;
			int					mRowInc;
		};

		/////////////////////////////////////////////////////////////////////////////////
		// Texture::Format
		Texture::Format::Format()
		{
			mTarget = GL_TEXTURE_2D;
			mWrapS = GL_CLAMP_TO_EDGE;
			mWrapT = GL_CLAMP_TO_EDGE;
			mMinFilter = GL_LINEAR;
			mMagFilter = GL_LINEAR;
			mMipmapping = false;
			mInternalFormat = -1;
		}

		/////////////////////////////////////////////////////////////////////////////////
		// Texture::Obj
		Texture::Obj::~Obj()
		{
			if (mDeallocatorFunc)
				(*mDeallocatorFunc)(mDeallocatorRefcon);

			if ((mTextureID > 0) && (!mDoNotDispose)) {
				glDeleteTextures(1, &mTextureID);
			}
		}


		/////////////////////////////////////////////////////////////////////////////////
		// Texture
		Texture::Texture(int aWidth, int aHeight, Format format)
			: mObj(shared_ptr<Obj>(new Obj(aWidth, aHeight)))
		{
			if (format.mInternalFormat == -1)
				format.mInternalFormat = GL_RGBA;
			mObj->mInternalFormat = format.mInternalFormat;
			mObj->mTarget = format.mTarget;
			init((unsigned char*)0, 0, GL_RGBA, GL_UNSIGNED_BYTE, format);
		}

		Texture::Texture(const unsigned char *data, int dataFormat, int aWidth, int aHeight, Format format)
			: mObj(shared_ptr<Obj>(new Obj(aWidth, aHeight)))
		{
			if (format.mInternalFormat == -1)
				format.mInternalFormat = GL_RGBA;
			mObj->mInternalFormat = format.mInternalFormat;
			mObj->mTarget = format.mTarget;
			init(data, 0, dataFormat, GL_UNSIGNED_BYTE, format);
		}

/*		Texture::Texture(const Surface8u &surface, Format format)
			: mObj(shared_ptr<Obj>(new Obj(surface.getWidth(), surface.getHeight())))
		{
			if (format.mInternalFormat < 0)
				format.mInternalFormat = surface.hasAlpha() ? GL_RGBA : GL_RGB;
			mObj->mInternalFormat = format.mInternalFormat;
			mObj->mTarget = format.mTarget;

			GLint dataFormat;
			GLenum type;
			SurfaceChannelOrderToDataFormatAndType(surface.getChannelOrder(), &dataFormat, &type);

			init(surface.getData(), surface.getRowBytes() / surface.getChannelOrder().getPixelInc(), dataFormat, type, format);
		}

		Texture::Texture(const Surface32f &surface, Format format)
			: mObj(shared_ptr<Obj>(new Obj(surface.getWidth(), surface.getHeight())))
		{
			if (format.mInternalFormat < 0) {
				format.mInternalFormat = surface.hasAlpha() ? GL_RGBA32F_ARB : GL_RGB32F_ARB;
			}
			mObj->mInternalFormat = format.mInternalFormat;
			mObj->mTarget = format.mTarget;

			init(surface.getData(), surface.hasAlpha() ? GL_RGBA : GL_RGB, format);
		}

		Texture::Texture(const Channel8u &channel, Format format)
			: mObj(shared_ptr<Obj>(new Obj(channel.getWidth(), channel.getHeight())))
		{
			if (format.mInternalFormat < 0)
				format.mInternalFormat = GL_LUMINANCE;

			mObj->mInternalFormat = format.mInternalFormat;
			mObj->mTarget = format.mTarget;

			// if the data is not already contiguous, we'll need to create a block of memory that is
			if ((channel.getIncrement() != 1) || (channel.getRowBytes() != channel.getWidth() * sizeof(uint8_t))) {
				shared_ptr<uint8_t> data(new uint8_t[channel.getWidth() * channel.getHeight()], checked_array_deleter<uint8_t>());
				uint8_t *dest = data.get();
				const int8_t inc = channel.getIncrement();
				const int32_t width = channel.getWidth();
				for (int y = 0; y < channel.getHeight(); ++y) {
					const uint8_t *src = channel.getData(0, y);
					for (int x = 0; x < width; ++x) {
						*dest++ = *src;
						src += inc;
					}
				}

				init(data.get(), channel.getRowBytes() / channel.getIncrement(), GL_LUMINANCE, GL_UNSIGNED_BYTE, format);
			} else {
				init(channel.getData(), channel.getRowBytes() / channel.getIncrement(), GL_LUMINANCE, GL_UNSIGNED_BYTE, format);
			}
		}

		Texture::Texture(const Channel32f &channel, Format format)
			: mObj(shared_ptr<Obj>(new Obj(channel.getWidth(), channel.getHeight())))
		{
			if (format.mInternalFormat < 0) {
				format.mInternalFormat = GL_LUMINANCE32F_ARB;
			}

			mObj->mInternalFormat = format.mInternalFormat;
			mObj->mTarget = format.mTarget;

			// if the data is not already contiguous, we'll need to create a block of memory that is
			if ((channel.getIncrement() != 1) || (channel.getRowBytes() != channel.getWidth() * sizeof(float))) {
				shared_ptr<float> data(new float[channel.getWidth() * channel.getHeight()], checked_array_deleter<float>());
				float *dest = data.get();
				const int8_t inc = channel.getIncrement();
				const int32_t width = channel.getWidth();
				for (int y = 0; y < channel.getHeight(); ++y) {
					const float *src = channel.getData(0, y);
					for (int x = 0; x < width; ++x) {
						*dest++ = *src;
						src += inc;
					}
				}

				init(data.get(), GL_LUMINANCE, format);
			} else {
				init(channel.getData(), GL_LUMINANCE, format);
			}
		}

		Texture::Texture(ImageSourceRef imageSource, Format format)
			: mObj(shared_ptr<Obj>(new Obj))
		{
			init(imageSource, format);
		}
*/
		Texture::Texture(GLenum aTarget, GLuint aTextureID, int aWidth, int aHeight, bool aDoNotDispose)
			: mObj(shared_ptr<Obj>(new Obj))
		{
			mObj->mTarget = aTarget;
			mObj->mTextureID = aTextureID;
			mObj->mDoNotDispose = aDoNotDispose;
			mObj->mWidth = mObj->mCleanWidth = aWidth;
			mObj->mHeight = mObj->mCleanHeight = aHeight;

			if (mObj->mTarget == GL_TEXTURE_2D) {
				mObj->mMaxU = mObj->mMaxV = 1.0f;
			} else {
				mObj->mMaxU = (float)mObj->mWidth;
				mObj->mMaxV = (float)mObj->mHeight;
			}
		}

		void Texture::init(const unsigned char *data, int unpackRowLength, GLenum dataFormat, GLenum type, const Format &format)
		{
			mObj->mDoNotDispose = false;

			glGenTextures(1, &mObj->mTextureID);

			glBindTexture(mObj->mTarget, mObj->mTextureID);
			glTexParameteri(mObj->mTarget, GL_TEXTURE_WRAP_S, format.mWrapS);
			glTexParameteri(mObj->mTarget, GL_TEXTURE_WRAP_T, format.mWrapT);
			glTexParameteri(mObj->mTarget, GL_TEXTURE_MIN_FILTER, format.mMinFilter);
			glTexParameteri(mObj->mTarget, GL_TEXTURE_MAG_FILTER, format.mMagFilter);
			if (format.mMipmapping)
				glTexParameteri(mObj->mTarget, GL_GENERATE_MIPMAP, GL_TRUE);
			if (mObj->mTarget == GL_TEXTURE_2D) {
				mObj->mMaxU = mObj->mMaxV = 1.0f;
			} else {
				mObj->mMaxU = (float)mObj->mWidth;
				mObj->mMaxV = (float)mObj->mHeight;
			}

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, unpackRowLength);
			
			glTexImage2D(mObj->mTarget, 0, mObj->mInternalFormat, mObj->mWidth, mObj->mHeight, 0, dataFormat, type, data);

			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		}

		void Texture::init(const float *data, GLint dataFormat, const Format &format)
		{
			mObj->mDoNotDispose = false;

			glGenTextures(1, &mObj->mTextureID);

			glBindTexture(mObj->mTarget, mObj->mTextureID);
			glTexParameteri(mObj->mTarget, GL_TEXTURE_WRAP_S, format.mWrapS);
			glTexParameteri(mObj->mTarget, GL_TEXTURE_WRAP_T, format.mWrapT);
			glTexParameteri(mObj->mTarget, GL_TEXTURE_MIN_FILTER, format.mMinFilter);
			glTexParameteri(mObj->mTarget, GL_TEXTURE_MAG_FILTER, format.mMagFilter);
			if (format.mMipmapping)
				glTexParameteri(mObj->mTarget, GL_GENERATE_MIPMAP, GL_TRUE);
			if (mObj->mTarget == GL_TEXTURE_2D) {
				mObj->mMaxU = mObj->mMaxV = 1.0f;
			} else {
				mObj->mMaxU = (float)mObj->mWidth;
				mObj->mMaxV = (float)mObj->mHeight;
			}

			if (data) {
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glTexImage2D(mObj->mTarget, 0, mObj->mInternalFormat, mObj->mWidth, mObj->mHeight, 0, dataFormat, GL_FLOAT, data);
			} else
				glTexImage2D(mObj->mTarget, 0, mObj->mInternalFormat, mObj->mWidth, mObj->mHeight, 0, GL_LUMINANCE, GL_FLOAT, 0);  // init to black...
		}

/*		void Texture::init(ImageSourceRef imageSource, const Format &format)
		{
			mObj->mDoNotDispose = false;
			mObj->mTarget = format.mTarget;
			mObj->mWidth = mObj->mCleanWidth = imageSource->getWidth();
			mObj->mHeight = mObj->mCleanHeight = imageSource->getHeight();

			// Set the internal format based on the image's color space
			if (format.isAutoInternalFormat()) {
				switch (imageSource->getColorModel()) {
					case ImageIo::CM_RGB:
						if (imageSource->getDataType() == ImageIo::UINT8)
							mObj->mInternalFormat = (imageSource->hasAlpha()) ? GL_RGBA8 : GL_RGB8;
						else if (imageSource->getDataType() == ImageIo::UINT16)
							mObj->mInternalFormat = (imageSource->hasAlpha()) ? GL_RGBA16 : GL_RGB16;
						else if (imageSource->getDataType() == ImageIo::FLOAT32)
							mObj->mInternalFormat = (imageSource->hasAlpha()) ? GL_RGBA32F_ARB : GL_RGB32F_ARB;
						else
							mObj->mInternalFormat = (imageSource->hasAlpha()) ? GL_RGBA : GL_RGB;
						break;
					case ImageIo::CM_GRAY:
						if (imageSource->getDataType() == ImageIo::UINT8)
							mObj->mInternalFormat = (imageSource->hasAlpha()) ? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE8;
						else if (imageSource->getDataType() == ImageIo::UINT16)
							mObj->mInternalFormat = (imageSource->hasAlpha()) ? GL_LUMINANCE16_ALPHA16 : GL_LUMINANCE16;
						else if (imageSource->getDataType() == ImageIo::FLOAT32)
							mObj->mInternalFormat = (imageSource->hasAlpha()) ? GL_LUMINANCE_ALPHA32F_ARB : GL_LUMINANCE32F_ARB;
						else
							mObj->mInternalFormat = (imageSource->hasAlpha()) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
						break;
					default:
						throw ImageIoExceptionIllegalColorModel("Illegal color model.");
						break;
				}
			} else {
				mObj->mInternalFormat = format.mInternalFormat;
			}

			// setup an appropriate dataFormat/ImageTargetTexture based on the image's color space
			GLint dataFormat;
			ImageIo::ChannelOrder channelOrder;
			bool isGray = false;
			switch (imageSource->getColorModel()) {
				case ImageSource::CM_RGB:
					dataFormat = (imageSource->hasAlpha()) ? GL_RGBA : GL_RGB;
					channelOrder = (imageSource->hasAlpha()) ? ImageIo::RGBA : ImageIo::RGB;
					break;
				case ImageSource::CM_GRAY:
					dataFormat = (imageSource->hasAlpha()) ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
					channelOrder = (imageSource->hasAlpha()) ? ImageIo::YA : ImageIo::Y;
					isGray = true;
					break;
				default: // if this is some other color space, we'll have to punt and go w/ RGB
					dataFormat = (imageSource->hasAlpha()) ? GL_RGBA : GL_RGB;
					channelOrder = (imageSource->hasAlpha()) ? ImageIo::RGBA : ImageIo::RGB;
					break;
			}

			glGenTextures(1, &mObj->mTextureID);
			glBindTexture(mObj->mTarget, mObj->mTextureID);

			glTexParameteri(mObj->mTarget, GL_TEXTURE_WRAP_S, format.mWrapS);
			glTexParameteri(mObj->mTarget, GL_TEXTURE_WRAP_T, format.mWrapT);
			glTexParameteri(mObj->mTarget, GL_TEXTURE_MIN_FILTER, format.mMinFilter);
			glTexParameteri(mObj->mTarget, GL_TEXTURE_MAG_FILTER, format.mMagFilter);
			if (format.mMipmapping)
				glTexParameteri(mObj->mTarget, GL_GENERATE_MIPMAP, GL_TRUE);
			if (mObj->mTarget == GL_TEXTURE_2D) {
				mObj->mMaxU = mObj->mMaxV = 1.0f;
			} else {
				mObj->mMaxU = (float)mObj->mWidth;
				mObj->mMaxV = (float)mObj->mHeight;
			}

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			if (imageSource->getDataType() == ImageIo::UINT8) {
				shared_ptr<ImageTargetGLTexture<uint8_t> > target = ImageTargetGLTexture<uint8_t>::createRef(this, channelOrder, isGray, imageSource->hasAlpha());
				imageSource->load(target);
				glTexImage2D(mObj->mTarget, 0, mObj->mInternalFormat, mObj->mWidth, mObj->mHeight, 0, dataFormat, GL_UNSIGNED_BYTE, target->getData());
			} else if (imageSource->getDataType() == ImageIo::UINT16) {
				shared_ptr<ImageTargetGLTexture<uint16_t> > target = ImageTargetGLTexture<uint16_t>::createRef(this, channelOrder, isGray, imageSource->hasAlpha());
				imageSource->load(target);
				glTexImage2D(mObj->mTarget, 0, mObj->mInternalFormat, mObj->mWidth, mObj->mHeight, 0, dataFormat, GL_UNSIGNED_SHORT, target->getData());

			} else {
				shared_ptr<ImageTargetGLTexture<float> > target = ImageTargetGLTexture<float>::createRef(this, channelOrder, isGray, imageSource->hasAlpha());
				imageSource->load(target);
				glTexImage2D(mObj->mTarget, 0, mObj->mInternalFormat, mObj->mWidth, mObj->mHeight, 0, dataFormat, GL_FLOAT, target->getData());
			}
		}

		void Texture::update(const Surface &surface)
		{
			GLint dataFormat;
			GLenum type;
			SurfaceChannelOrderToDataFormatAndType(surface.getChannelOrder(), &dataFormat, &type);
			if ((surface.getWidth() != getWidth()) || (surface.getHeight() != getHeight()))
				throw TextureDataExc("Invalid Texture::update() surface dimensions");

			glBindTexture(mObj->mTarget, mObj->mTextureID);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, surface.getRowBytes() / surface.getPixelInc());
			glTexImage2D(mObj->mTarget, 0, getInternalFormat(), getWidth(), getHeight(), 0, dataFormat, type, surface.getData());
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		}

		void Texture::update(const Surface32f &surface)
		{
			GLint dataFormat;
			GLenum type;
			SurfaceChannelOrderToDataFormatAndType(surface.getChannelOrder(), &dataFormat, &type);
			if ((surface.getWidth() != getWidth()) || (surface.getHeight() != getHeight()))
				throw TextureDataExc("Invalid Texture::update() surface dimensions");

			glBindTexture(mObj->mTarget, mObj->mTextureID);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, surface.getRowBytes() / (surface.getPixelInc() * sizeof(float)));

			// @TODO: type does not seem to be pulling out the right value.. 
			glTexImage2D(mObj->mTarget, 0, getInternalFormat(), getWidth(), getHeight(), 0, dataFormat, GL_FLOAT, surface.getData());
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		}

		void Texture::update(const Surface &surface, const Area &area)
		{
			GLint dataFormat;
			GLenum type;
			SurfaceChannelOrderToDataFormatAndType(surface.getChannelOrder(), &dataFormat, &type);

			glBindTexture(mObj->mTarget, mObj->mTextureID);
			glTexSubImage2D(mObj->mTarget, 0, area.getX1(), area.getY1(), area.getWidth(), area.getHeight(), dataFormat, type, surface.getData(area.getUL()));
		}

		void Texture::update(const Channel32f &channel)
		{
			if ((channel.getWidth() != getWidth()) || (channel.getHeight() != getHeight()))
				throw TextureDataExc("Invalid Texture::update() channel dimensions");

			glBindTexture(mObj->mTarget, mObj->mTextureID);
			glTexSubImage2D(mObj->mTarget, 0, 0, 0, getWidth(), getHeight(), GL_LUMINANCE, GL_FLOAT, channel.getData());
		}

		void Texture::update(const Channel8u &channel, const Area &area)
		{
			glBindTexture(mObj->mTarget, mObj->mTextureID);
			// if the data is not already contiguous, we'll need to create a block of memory that is
			if ((channel.getIncrement() != 1) || (channel.getRowBytes() != channel.getWidth() * sizeof(uint8_t))) {
				shared_ptr<uint8_t> data(new uint8_t[area.getWidth() * area.getHeight()], checked_array_deleter<uint8_t>());
				uint8_t *dest = data.get();
				const int8_t inc = channel.getIncrement();
				const int32_t width = area.getWidth();
				for (int y = 0; y < area.getHeight(); ++y) {
					const uint8_t *src = channel.getData(area.getX1(), area.getY1() + y);
					for (int x = 0; x < width; ++x) {
						*dest++ = *src;
						src += inc;
					}
				}

				glTexSubImage2D(mObj->mTarget, 0, area.getX1(), area.getY1(), area.getWidth(), area.getHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, data.get());
			} else
				glTexSubImage2D(mObj->mTarget, 0, area.getX1(), area.getY1(), area.getWidth(), area.getHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, channel.getData(area.getUL()));
		}

		void Texture::SurfaceChannelOrderToDataFormatAndType(const SurfaceChannelOrder &sco, GLint *dataFormat, GLenum *type)
		{
			switch (sco.getCode()) {
				case SurfaceChannelOrder::RGB:
					*dataFormat = GL_RGB;
					*type = GL_UNSIGNED_BYTE;
					break;
				case SurfaceChannelOrder::RGBA:
				case SurfaceChannelOrder::RGBX:
					*dataFormat = GL_RGBA;
					*type = GL_UNSIGNED_BYTE;
					break;
				case SurfaceChannelOrder::BGRA:
				case SurfaceChannelOrder::BGRX:
					*dataFormat = GL_BGRA;
					*type = GL_UNSIGNED_BYTE;
					break;
				case SurfaceChannelOrder::BGR:
					*dataFormat = GL_BGR;
					*type = GL_UNSIGNED_BYTE;
					break;
				case SurfaceChannelOrder::ARGB:
					*dataFormat = GL_BGRA;
					*type = GL_UNSIGNED_INT_8_8_8_8;
					break;
				default:
					throw TextureDataExc("Invalid channel order"); // this is an unsupported channel order for a texture
					break;
			}
		}
*/
		bool Texture::dataFormatHasAlpha(GLint dataFormat)
		{
			switch (dataFormat) {
				case GL_RGBA:
				case GL_ALPHA:
				case GL_LUMINANCE_ALPHA:
				case GL_BGRA:
					return true;
					break;
				default:
					return false;
			}
		}

		bool Texture::dataFormatHasColor(GLint dataFormat)
		{
			switch (dataFormat) {
				case GL_ALPHA:
				case GL_LUMINANCE:
				case GL_LUMINANCE_ALPHA:
					return false;
					break;
			}

			return true;
		}

/*		Texture Texture::loadDds(IStreamRef ddsStream, Format format)
		{
			try {
				ddSurface ddsd;
				char filecode[4];
				ddsStream->readData(filecode, 4);
				if (strncmp(filecode, "DDS ", 4) != 0) {
					return Texture();
				}

				ddsStream->readData(&ddsd, 124); // sizeof(ddsd)

				// how big is it going to be including all mipmaps?
				uint32_t bufSize = (ddsd.dwMipMapCount > 1) ? (ddsd.dwLinearSize * 2) : (ddsd.dwLinearSize);
				shared_ptr<uint8_t> pixels(new uint8_t[bufSize + 1], checked_array_deleter<uint8_t>());
				ddsStream->readData(pixels.get(), bufSize);

				uint32_t width = ddsd.dwWidth;
				uint32_t height = ddsd.dwHeight;
				//int numComponents  = (ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT1) ? 3 : 4; 
				int numMipMaps = ddsd.dwMipMapCount;
				int dataFormat;
				switch (ddsd.ddpfPixelFormat.dwFourCC) {
					case FOURCC_DXT1:
						dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
						break;
					case FOURCC_DXT3:
						dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
						break;
					case FOURCC_DXT5:
						dataFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
						break;
					default:
						return Texture();
						break;
				}

				off_t offset = 0;
				uint32_t blockSize = (dataFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;

				// Create the texture
				GLenum target = format.mTarget;
				GLuint texID;
				glGenTextures(1, &texID);

				Texture result(target, texID, width, height, false);
				result.mObj->mWidth = width;
				result.mObj->mHeight = height;
				result.mObj->mInternalFormat = dataFormat;

				glBindTexture(result.mObj->mTarget, result.mObj->mTextureID);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				// load the mipmaps
				for (int i = 0; i <= numMipMaps && (width || height); ++i) {
					if (width == 0)
						width = 1;
					if (height == 0)
						height = 1;

					int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
					glCompressedTexImage2D(result.mObj->mTarget, i, dataFormat, width, height, 0, size, pixels.get() + offset);

					offset += size;
					width >>= 1;
					height >>= 1;
				}

				if (numMipMaps > 1) {
					glTexParameteri(result.mObj->mTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameteri(result.mObj->mTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				} else {
					glTexParameteri(result.mObj->mTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(result.mObj->mTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				}

				return result;
			} catch (...) {
				return Texture();
			}
		}
*/
		Texture	Texture::weakClone() const
		{
			gl::Texture result = Texture(mObj->mTarget, mObj->mTextureID, mObj->mWidth, mObj->mHeight, true);
			result.mObj->mInternalFormat = mObj->mInternalFormat;
			result.mObj->mFlipped = mObj->mFlipped;
			return result;
		}

		void Texture::setDeallocator(void(*aDeallocatorFunc)(void *), void *aDeallocatorRefcon)
		{
			mObj->mDeallocatorFunc = aDeallocatorFunc;
			mObj->mDeallocatorRefcon = aDeallocatorRefcon;
		}

		void Texture::setWrapS(GLenum wrapS)
		{
			glTexParameteri(mObj->mTarget, GL_TEXTURE_WRAP_S, wrapS);
		}

		void Texture::setWrapT(GLenum wrapT)
		{
			glTexParameteri(mObj->mTarget, GL_TEXTURE_WRAP_T, wrapT);
		}

		void Texture::setMinFilter(GLenum minFilter)
		{
			glTexParameteri(mObj->mTarget, GL_TEXTURE_MIN_FILTER, minFilter);
		}

		void Texture::setMagFilter(GLenum magFilter)
		{
			glTexParameteri(mObj->mTarget, GL_TEXTURE_MAG_FILTER, magFilter);
		}

		void Texture::setCleanTexCoords(float maxU, float maxV)
		{
			mObj->mMaxU = maxU;
			mObj->mMaxV = maxV;

			if (mObj->mTarget == GL_TEXTURE_2D) {
				mObj->mCleanWidth = getWidth() * maxU;
				mObj->mCleanHeight = getHeight() * maxV;
			} else {
				mObj->mCleanWidth = (int32_t)maxU;
				mObj->mCleanHeight = (int32_t)maxV;
			}
		}

		bool Texture::hasAlpha() const
		{
			switch (mObj->mInternalFormat) {
				case GL_RGBA8:
				case GL_RGBA16:
				case GL_RGBA32F_ARB:
				case GL_LUMINANCE8_ALPHA8:
				case GL_LUMINANCE16_ALPHA16:
				case GL_LUMINANCE_ALPHA32F_ARB:
				case GL_RGBA:
				case GL_LUMINANCE_ALPHA:
					return true;
					break;
				default:
					return false;
					break;
			}
		}

		float Texture::getLeft() const
		{
			return 0.0f;
		}

		float Texture::getRight() const
		{
			return mObj->mMaxU;
		}

		float Texture::getTop() const
		{
			return (mObj->mFlipped) ? getMaxV() : 0.0f;
		}

		GLint Texture::getInternalFormat() const
		{
			if (mObj->mInternalFormat == -1) {
				SaveTextureBindState(mObj->mTarget);
				bind();
				glGetTexLevelParameteriv(mObj->mTarget, 0, GL_TEXTURE_INTERNAL_FORMAT, &mObj->mInternalFormat);
			}

			return mObj->mInternalFormat;
		}

		GLint Texture::getWidth() const
		{
			if (mObj->mWidth == -1) {
				SaveTextureBindState(mObj->mTarget);
				bind();
				glGetTexLevelParameteriv(mObj->mTarget, 0, GL_TEXTURE_WIDTH, &mObj->mWidth);
				mObj->mCleanWidth = mObj->mWidth;
			}

			return mObj->mWidth;
		}

		GLint Texture::getHeight() const
		{
			if (mObj->mHeight == -1) {
				SaveTextureBindState(mObj->mTarget);
				bind();
				glGetTexLevelParameteriv(mObj->mTarget, 0, GL_TEXTURE_HEIGHT, &mObj->mHeight);
				mObj->mCleanHeight = mObj->mHeight;
			}

			return mObj->mHeight;
		}

		GLint Texture::getCleanWidth() const
		{
			if (mObj->mCleanWidth == -1) {
				SaveTextureBindState(mObj->mTarget);
				bind();
				glGetTexLevelParameteriv(mObj->mTarget, 0, GL_TEXTURE_WIDTH, &mObj->mWidth);
				mObj->mCleanWidth = mObj->mWidth;
			}

			return mObj->mCleanWidth;
		}

		GLint Texture::getCleanHeight() const
		{
			if (mObj->mCleanHeight == -1) {
				SaveTextureBindState(mObj->mTarget);
				bind();
				glGetTexLevelParameteriv(mObj->mTarget, 0, GL_TEXTURE_HEIGHT, &mObj->mHeight);
				mObj->mCleanHeight = mObj->mHeight;
			}

			return mObj->mCleanHeight;
		}

		Rectf Texture::getAreaTexCoords(const Area &area) const
		{
			Rectf result;

			if (mObj->mTarget == GL_TEXTURE_2D) {
				result.x1 = area.x1 / (float)getWidth();
				result.x2 = area.x2 / (float)getWidth();
				result.y1 = area.y1 / (float)getHeight();
				result.y2 = area.y2 / (float)getHeight();
			} else {
				result = Rectf(area);
			}

			if (mObj->mFlipped) {
				std::swap(result.y1, result.y2);
			}

			return result;
		}

		float Texture::getBottom() const
		{
			return (mObj->mFlipped) ? 0.0f : getMaxV();
		}

		float Texture::getMaxU() const
		{
			return mObj->mMaxU;
		}

		float Texture::getMaxV() const
		{
			return mObj->mMaxV;
		}

		void Texture::bind(GLuint textureUnit) const
		{
			glActiveTexture(GL_TEXTURE0 + textureUnit);
			glBindTexture(mObj->mTarget, mObj->mTextureID);
			glActiveTexture(GL_TEXTURE0);
		}

		void Texture::unbind(GLuint textureUnit) const
		{
			glActiveTexture(GL_TEXTURE0 + textureUnit);
			glBindTexture(mObj->mTarget, 0);
			glActiveTexture(GL_TEXTURE0);
		}

		void Texture::enableAndBind() const
		{
			glEnable(mObj->mTarget);
			glBindTexture(mObj->mTarget, mObj->mTextureID);
		}

		void Texture::disable() const
		{
			glDisable(mObj->mTarget);
		}

		/////////////////////////////////////////////////////////////////////////////////
		// TextureCache
/*		TextureCache::TextureCache(const Surface8u &prototypeSurface, const Texture::Format &format)
			: mObj(shared_ptr<Obj>(new Obj(prototypeSurface, format)))
		{

		}

		gl::Texture	TextureCache::cache(const Surface8u &data)
		{
			pair<int, Texture> *resultPair;

			// find an available slot and update that if possible
			bool found = false;
			for (vector<pair<int, Texture> >::iterator texIt = mObj->mTextures.begin(); texIt != mObj->mTextures.end(); ++texIt) {
				if (texIt->first == -1) { // this texture is available, let's use it!
					resultPair = &(*texIt);
					resultPair->second.update(data);
					found = true;
					break;
				}
			}

			// we didn't find an available slot, so let's make a new texture
			if (!found) {
				mObj->mTextures.push_back(make_pair(-1, gl::Texture(data, mObj->mFormat)));
				resultPair = &mObj->mTextures.back();
			}

			resultPair->first = mObj->mNextId++;
			Texture result = resultPair->second.weakClone();

			pair<shared_ptr<TextureCache::Obj>, int> *refcon = new pair<shared_ptr<TextureCache::Obj>, int>(mObj, resultPair->first);
			result.setDeallocator(Obj::TextureCacheDeallocator, refcon);
			return result;
		}

		TextureCache::Obj::Obj(const Surface8u &prototypeSurface, const Texture::Format &format)
			: mWidth(prototypeSurface.getWidth()), mHeight(prototypeSurface.getHeight()), mFormat(format), mNextId(0)
		{
		}
*/
		void TextureCache::Obj::markTextureAsFree(int id)
		{
			for (vector<pair<int, Texture> >::iterator texIt = mTextures.begin(); texIt != mTextures.end(); ++texIt) {
				if (texIt->first == id) { // this texture is available, let's use it!
					texIt->first = -1;
					break;
				}
			}
		}

		void TextureCache::Obj::TextureCacheDeallocator(void *aDeallocatorRefcon)
		{
			pair<shared_ptr<TextureCache::Obj>, int> *refconPair = reinterpret_cast<pair<shared_ptr<TextureCache::Obj>, int>* >(aDeallocatorRefcon);
			refconPair->first->markTextureAsFree(refconPair->second);
			delete refconPair;
		}

		/////////////////////////////////////////////////////////////////////////////////
		// ImageTargetGLTexture
		template<typename T>
		shared_ptr<ImageTargetGLTexture<T> > ImageTargetGLTexture<T>::createRef(const Texture *aTexture, ImageIo::ChannelOrder &aChannelOrder, bool aIsGray, bool aHasAlpha)
		{
			return shared_ptr<ImageTargetGLTexture<T> >(new ImageTargetGLTexture<T>(aTexture, aChannelOrder, aIsGray, aHasAlpha));
		}

		template<typename T>
		ImageTargetGLTexture<T>::ImageTargetGLTexture(const Texture *aTexture, ImageIo::ChannelOrder &aChannelOrder, bool aIsGray, bool aHasAlpha)
			: ImageTarget(), mTexture(aTexture), mIsGray(aIsGray), mHasAlpha(aHasAlpha)
		{
			if (mIsGray)
				mPixelInc = (mHasAlpha) ? 2 : 1;
			else
				mPixelInc = (mHasAlpha) ? 4 : 3;
			mRowInc = mTexture->getWidth() * mPixelInc;
			// allocate enough room to hold all these pixels
			mData = new T[mTexture->getHeight() * mRowInc];

			if (boost::is_same<T, uint8_t>::value)
				setDataType(ImageIo::UINT8);
			else if (boost::is_same<T, uint16_t>::value)
				setDataType(ImageIo::UINT16);
			else if (boost::is_same<T, float>::value)
				setDataType(ImageIo::FLOAT32);

			setChannelOrder(aChannelOrder);
			setColorModel(mIsGray ? ImageIo::CM_GRAY : ImageIo::CM_RGB);
		}

		template<typename T>
		ImageTargetGLTexture<T>::~ImageTargetGLTexture()
		{
			delete[] mData;
		}

		/////////////////////////////////////////////////////////////////////////////////
		// ImageSourceTexture
/*		class ImageSourceTexture : public ImageSource {
		public:
			ImageSourceTexture(const Texture &texture)
				: ImageSource()
			{
				mWidth = texture.getWidth();
				mHeight = texture.getHeight();

				GLint internalFormat = texture.getInternalFormat();
				GLenum format;
				switch (internalFormat) {
					case GL_RGB: setChannelOrder(ImageIo::RGB); setColorModel(ImageIo::CM_RGB); setDataType(ImageIo::FLOAT32); format = GL_RGB; break;
					case GL_RGBA: setChannelOrder(ImageIo::RGBA); setColorModel(ImageIo::CM_RGB); setDataType(ImageIo::FLOAT32); format = GL_RGBA; break;
					case GL_LUMINANCE: setChannelOrder(ImageIo::Y); setColorModel(ImageIo::CM_GRAY); setDataType(ImageIo::FLOAT32); format = GL_LUMINANCE; break;
					case GL_LUMINANCE_ALPHA: setChannelOrder(ImageIo::YA); setColorModel(ImageIo::CM_GRAY); setDataType(ImageIo::FLOAT32); format = GL_LUMINANCE_ALPHA; break;
					case GL_RGBA8: setChannelOrder(ImageIo::RGBA); setColorModel(ImageIo::CM_RGB); setDataType(ImageIo::UINT8); format = GL_RGBA; break;
					case GL_RGB8: setChannelOrder(ImageIo::RGB); setColorModel(ImageIo::CM_RGB); setDataType(ImageIo::UINT8); format = GL_RGB; break;
					case GL_BGR: setChannelOrder(ImageIo::RGB); setColorModel(ImageIo::CM_RGB); setDataType(ImageIo::FLOAT32); format = GL_RGB; break;
					case GL_LUMINANCE8: setChannelOrder(ImageIo::Y); setColorModel(ImageIo::CM_GRAY); setDataType(ImageIo::UINT8); format = GL_LUMINANCE; break;
					case GL_LUMINANCE8_ALPHA8: setChannelOrder(ImageIo::YA); setColorModel(ImageIo::CM_GRAY); setDataType(ImageIo::UINT8); format = GL_LUMINANCE_ALPHA; break;
					case GL_DEPTH_COMPONENT16: setChannelOrder(ImageIo::Y); setColorModel(ImageIo::CM_GRAY); setDataType(ImageIo::UINT16); format = GL_DEPTH_COMPONENT; break;
					case GL_DEPTH_COMPONENT24: setChannelOrder(ImageIo::Y); setColorModel(ImageIo::CM_GRAY); setDataType(ImageIo::FLOAT32); format = GL_DEPTH_COMPONENT; break;
					case GL_DEPTH_COMPONENT32: setChannelOrder(ImageIo::Y); setColorModel(ImageIo::CM_GRAY); setDataType(ImageIo::FLOAT32); format = GL_DEPTH_COMPONENT; break;
					case GL_RGBA32F_ARB: setChannelOrder(ImageIo::RGBA); setColorModel(ImageIo::CM_RGB); setDataType(ImageIo::FLOAT32); format = GL_RGBA; break;
					case GL_RGB32F_ARB: setChannelOrder(ImageIo::RGB); setColorModel(ImageIo::CM_RGB); setDataType(ImageIo::FLOAT32); format = GL_RGB; break;
					case GL_LUMINANCE32F_ARB: setChannelOrder(ImageIo::Y); setColorModel(ImageIo::CM_GRAY); setDataType(ImageIo::FLOAT32); format = GL_LUMINANCE; break;
					case GL_LUMINANCE_ALPHA32F_ARB: setChannelOrder(ImageIo::YA); setColorModel(ImageIo::CM_GRAY); setDataType(ImageIo::FLOAT32); format = GL_LUMINANCE_ALPHA; break;
					default: setChannelOrder(ImageIo::RGBA); setColorModel(ImageIo::CM_RGB); setDataType(ImageIo::FLOAT32); break;
				}

				GLenum dataType = GL_UNSIGNED_BYTE;
				int dataSize = 1;
				if (mDataType == ImageIo::UINT16) {
					dataType = GL_UNSIGNED_SHORT;
					dataSize = 2;
				} else if (mDataType == ImageIo::FLOAT32) {
					dataType = GL_FLOAT;
					dataSize = 4;
				}

				mRowBytes = mWidth * ImageIo::channelOrderNumChannels(mChannelOrder) * dataSize;
				mData = shared_ptr<uint8_t>(new uint8_t[mRowBytes * mHeight], boost::checked_array_delete<uint8_t>);
				cinder::gl::SaveTextureBindState(texture.getTarget());
				texture.bind();
				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				glGetTexImage(texture.getTarget(), 0, format, dataType, mData.get());
			}

			void load(ImageTargetRef target) {
				// get a pointer to the ImageSource function appropriate for handling our data configuration
				ImageSource::RowFunc func = setupRowFunc(target);

				const uint8_t *data = mData.get();
				for (int32_t row = 0; row < mHeight; ++row) {
					((*this).*func)(target, row, data);
					data += mRowBytes;
				}
			}

			shared_ptr<uint8_t>	mData;
			int32_t				mRowBytes;
		};

		Texture::operator ImageSourceRef() const
		{
			return shared_ptr<ImageSource>(new ImageSourceTexture(*this));
		}
*/
	}
}
