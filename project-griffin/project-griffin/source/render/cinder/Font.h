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

#pragma once

//#include "cinder/Shape2d.h"
//#include "cinder/DataSource.h"
#include <ft2build.h>
#include <cstdint>
#include <memory>

#include "render/Rect.h"

// Note: generic is a reserved word in winrt c++/cx
// need to redefine it for freetype.h
#define generic GenericFromFreeTypeLibrary
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#undef generic

#include FT_GLYPH_H

namespace griffin {

	//! Represents an instance of a font at a point size. \ImplShared
	class Font {
	public:
		typedef uint16_t		Glyph;

		/** \brief constructs a null Font **/
		Font() {}
		/** \brief Constructs a Font from its name (Postscript name preferred) and its \a size in points
			\note Assumes a point size relative to 72dpi on Cocoa but 96dpi on Windows. This creates rough parity between the platforms on type size, but in Windows this renders fonts smaller than normal. **/
		Font(const std::string &aName, float size);
		/** \brief Constructs a Font from a DataSource representing font data (such as a .ttf file) and its \a size in points.
			\note Assumes a point size relative to 72dpi on Cocoa but 96dpi on Windows. This creates rough parity between the platforms on type size, but in Windows this renders fonts smaller than normal. **/
//		Font(DataSourceRef dataSource, float size);

		const std::string&		getName() const;
		std::string				getFullName() const;
		float					getSize() const;

		float					getLeading() const;
		float					getAscent() const;
		float					getDescent() const;
		size_t					getNumGlyphs() const;

		Glyph					getGlyphIndex(size_t idx) const;
		Glyph					getGlyphChar(char utf8Char) const;
		std::vector<Glyph>		getGlyphs(const std::string &utf8String) const;
		//! Returns a cinder::Shape2d representing the shape of the glyph at \a glyphIndex
//		Shape2d					getGlyphShape(Glyph glyphIndex) const;
		//! Returns the bounding box of a Glyph, relative to the baseline as the origin
		Rectf					getGlyphBoundingBox(Glyph glyph) const;

		FT_Face					getFace() const { return mObj->mFace; }

//		static const std::vector<std::string>&		getNames(bool forceRefresh = false);
		static Font				getDefault();

	private:
		class Obj {
		public:
			Obj(const std::string &aName, float aSize);
//			Obj(DataSourceRef dataSource, float size);
			~Obj();

			void		finishSetup();


			std::string				mName;
			float					mSize;
			std::vector<std::pair<uint16_t, uint16_t> >	mUnicodeRanges;
			void *mFileData;
			FT_Face mFace;
			size_t					mNumGlyphs;
		};

		std::shared_ptr<Obj>		mObj;

	public:
		//@{
		//! Emulates shared_ptr-like behavior
		typedef std::shared_ptr<Obj> Font::*unspecified_bool_type;
		operator unspecified_bool_type() const { return (mObj.get() == 0) ? 0 : &Font::mObj; }
		void reset() { mObj.reset(); }
		//@}
	};

	class FontInvalidNameExc : public std::exception {
	public:
		FontInvalidNameExc() throw() {}
		FontInvalidNameExc(const std::string &fontName) throw();
		virtual const char* what() const throw() { return mMessage; }
	private:
		char mMessage[2048];
	};

	class FontGlyphFailureExc : public std::exception {
	};

}