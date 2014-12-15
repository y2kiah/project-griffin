/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.

 Copyright (c) Microsoft Open Technologies, Inc. All rights reserved.

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

#include "Font.h"

#include <vector>
#include <dwrite.h>
//#include "cinder/dx/FontEnumerator.h"

//#include "cinder/Utilities.h"
//#include "cinder/Unicode.h"

using std::vector;
using std::string;
using std::wstring;
using std::pair;

namespace griffin {

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// FontManager
	class FontManager
	{
	public:
		static FontManager*		instance();

		const vector<string>&	getNames(bool forceRefresh);
		Font					getDefault() const
		{
			if (!mDefault) {
				mDefault = Font("Arial", 12);
			}

			return mDefault;
		}
	private:
		FontManager();
		~FontManager();

		static FontManager	*sInstance;

		bool				mFontsEnumerated;
		vector<string>		mFontNames;
		mutable Font		mDefault;

		FT_Library			mLibrary;

		friend class Font;
	};

	FontManager *FontManager::sInstance = 0;

	FontManager::FontManager()
	{
		mFontsEnumerated = false;
		if (FT_Init_FreeType(&mLibrary)) {
			throw FontInvalidNameExc("Failed to initialize freetype");
		}
	}

	FontManager::~FontManager()
	{
		FT_Done_FreeType(mLibrary);
	}

	FontManager* FontManager::instance()
	{
		if (!FontManager::sInstance)
			FontManager::sInstance = new FontManager();

		return sInstance;
	}

/*	const vector<string>& FontManager::getNames(bool forceRefresh)
	{
		if ((!mFontsEnumerated) || forceRefresh) {
			mFontNames.clear();
			Platform::Array<Platform::String^>^ fontNames = FontEnumeration::FontEnumerator().ListSystemFonts();
			for(unsigned i = 0; i < fontNames->Length; ++i)
			{
				//mFontNames.push_back(std::string(fontNames[i]->Begin(), fontNames[i]->End())); //this doesn't work in release mode
				const wchar_t *start = fontNames[i]->Begin();
				const wchar_t *end = fontNames[i]->End();
				mFontNames.push_back(std::string(start, end));
				//int length = end - start;
				//char *str = new char[length + 1];
				//char *itr = str;
				//for(; start != end; ++start)
				//	*itr++ = *start;
				//*itr = 0;
				//mFontNames.push_back(std::string(str));
				//delete [] str;
			}
			mFontsEnumerated = true;
		}

		return mFontNames;
	}
*/
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Font
	Font::Font(const string &name, float size)
		: mObj(new Font::Obj(name, size))
	{
	}

/*	Font::Font(DataSourceRef dataSource, float size)
		: mObj(new Font::Obj(dataSource, size))
	{
	}

	const vector<string>& Font::getNames(bool forceRefresh)
	{
		return FontManager::instance()->getNames(forceRefresh);
	}
*/

	Font Font::getDefault()
	{
		return FontManager::instance()->getDefault();
	}

	const std::string& Font::getName() const
	{
		return mObj->mName;
	}

	float Font::getSize() const
	{
		return mObj->mSize;
	}

	std::string Font::getFullName() const
	{
		return mObj->mName;
	}

	float Font::getLeading() const
	{
		return (float)(mObj->mFace->height >> 6);
	}

	float Font::getAscent() const
	{
		return (float)(mObj->mFace->ascender >> 6);
	}

	float Font::getDescent() const
	{
		return (float)(mObj->mFace->descender >> 6);
	}

	size_t Font::getNumGlyphs() const
	{
		return mObj->mNumGlyphs;
	}

	Font::Glyph Font::getGlyphChar( char c ) const
	{
		return FT_Get_Char_Index(mObj->mFace, c);
	}

	Font::Glyph Font::getGlyphIndex( size_t idx ) const
	{
		size_t ct = 0;
		bool found = false;
		for( vector<pair<uint16_t,uint16_t> >::const_iterator rangeIt = mObj->mUnicodeRanges.begin(); rangeIt != mObj->mUnicodeRanges.end(); ++rangeIt ) {
			if( ct + rangeIt->second - rangeIt->first >= idx ) {
				ct = rangeIt->first + ( idx - ct );
				found = true;
				break;
			}
			else
				ct += rangeIt->second - rangeIt->first;
		}

		// this idx is invalid
		if (!found) {
			ct = 0;
		}

		return (Glyph)ct;
	}

	vector<Font::Glyph> Font::getGlyphs( const string &utf8String ) const
	{
		vector<Glyph> result;
		for(unsigned i = 0; i < utf8String.size(); ++i)
			result.push_back((Glyph)FT_Get_Char_Index(mObj->mFace, utf8String[i]));
		return result;
	}

/*	static int ftShape2dMoveTo(const FT_Vector *to, void *user)
	{
		Shape2d *shape = reinterpret_cast<Shape2d*>(user);
		shape->moveTo((float)to->x / 4096.f, (float)to->y / 4096.f);
		return 0;
	}

	static int ftShape2dLineTo(const FT_Vector *to, void *user)
	{
		Shape2d *shape = reinterpret_cast<Shape2d*>(user);
		shape->lineTo((float)to->x / 4096.f, (float)to->y / 4096.f);
		return 0;
	}

	static int ftShape2dConicTo(const FT_Vector *control, const FT_Vector *to, void *user)
	{
		Shape2d *shape = reinterpret_cast<Shape2d*>(user);
		shape->quadTo((float)control->x / 4096.f, (float)control->y / 4096.f, (float)to->x / 4096.f, (float)to->y / 4096.f);
		return 0;
	}

	static int ftShape2dCubicTo(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *user)
	{
		Shape2d *shape = reinterpret_cast<Shape2d*>(user);
		shape->curveTo((float)control1->x / 4096.f, (float)control1->y / 4096.f, (float)control2->x / 4096.f, (float)control2->y / 4096.f, (float)to->x / 4096.f, (float)to->y / 4096.f);
		return 0;
	}

	Shape2d Font::getGlyphShape( Glyph glyphIndex ) const
	{
		FT_Face face = mObj->mFace;
		FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
		FT_GlyphSlot glyph = face->glyph;
		FT_Outline outline = glyph->outline;
		FT_Outline_Funcs funcs;
		funcs.move_to = ftShape2dMoveTo;
		funcs.line_to = ftShape2dLineTo;
		funcs.conic_to = ftShape2dConicTo;
		funcs.cubic_to = ftShape2dCubicTo;
		funcs.shift = 6;
		funcs.delta = 0;

		Shape2d resultShape;
		FT_Outline_Decompose(&outline, &funcs, &resultShape);
		resultShape.close();
		resultShape.scale(Vec2f(1, -1));
		return resultShape;
	}
*/
	Rectf Font::getGlyphBoundingBox( Glyph glyphIndex ) const
	{
		FT_Load_Glyph(mObj->mFace, glyphIndex, FT_LOAD_DEFAULT);
		FT_GlyphSlot glyph = mObj->mFace->glyph;
		FT_Glyph_Metrics &metrics = glyph->metrics;
		return Rectf(
			(float)(metrics.horiBearingX >> 6),
			(float)((metrics.horiBearingY - metrics.height) >> 6),
			(float)((metrics.horiBearingX + metrics.width) >> 6),
			(float)(metrics.horiBearingY >> 6)
			);
	}


	Font::Obj::Obj(const string &aName, float aSize)
		: mName(aName), mSize(aSize)
	{
		//gotta go through a long tedious process just to get a font file

		//create the factory
		IDWriteFactory *writeFactory;
		if(!SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&writeFactory))))
			throw FontInvalidNameExc("Failed to create IDWriteFactory");

		//obtain the fonts owned by the machine
		IDWriteFontCollection *fontCollection;
		if(!SUCCEEDED(writeFactory->GetSystemFontCollection(&fontCollection, TRUE)))
			throw FontInvalidNameExc("Failed to get system fonts");

		//get the arial font itself
		UINT32 index;
		BOOL exists;
		std::wstring fontNameW;
		fontNameW.assign(aName.begin(), aName.end());
		if(!SUCCEEDED(fontCollection->FindFamilyName(fontNameW.c_str(), &index, &exists)))
			throw FontInvalidNameExc("Failed to locate the " + aName + " font family");
		if(exists == FALSE)
			throw FontInvalidNameExc("The " + aName + " font family doesn't exist");
		IDWriteFontFamily *fontFamily;
		if(!SUCCEEDED(fontCollection->GetFontFamily(index, &fontFamily)))
			throw FontInvalidNameExc("Failed to get the " + aName + " font family");
		IDWriteFont *matchingFont;
		if(!SUCCEEDED(fontFamily->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &matchingFont)))
			throw FontInvalidNameExc("Failed to get matching font for " + aName);

		//get the font face
		IDWriteFontFace *fontFace;
		if(!SUCCEEDED(matchingFont->CreateFontFace(&fontFace)))
			throw FontInvalidNameExc("Failed to get the " + aName + " font face");

		//get the font file making up this face
		IDWriteFontFile *fontFile;
		UINT32 numberOfFiles = 1;
		if(!SUCCEEDED(fontFace->GetFiles(&numberOfFiles, &fontFile)))
			throw FontInvalidNameExc("Failed to get the " + aName + " font file");

		//create the font file stream
		const void *fontFileReferenceKey;
		UINT32 fontFileReferenceKeySize;
		if(!SUCCEEDED(fontFile->GetReferenceKey(&fontFileReferenceKey, &fontFileReferenceKeySize)))
			throw FontInvalidNameExc("Failed to get the reference key for " + aName);
		IDWriteFontFileLoader *fontFileLoader;
		if(!SUCCEEDED(fontFile->GetLoader(&fontFileLoader)))
			throw FontInvalidNameExc("Failed to get the font file loader for " + aName);
		IDWriteFontFileStream *fontFileStream;
		if(!SUCCEEDED(fontFileLoader->CreateStreamFromKey(fontFileReferenceKey, fontFileReferenceKeySize, &fontFileStream)))
			throw FontInvalidNameExc("Failed to create font file stream for " + aName);

		//finally get the font file data and pass it to freetype
		UINT64 fileSize;
		if(!SUCCEEDED(fontFileStream->GetFileSize(&fileSize)))
			throw FontInvalidNameExc("Failed to get the file size for " + aName);
		const void *fragmentStart;
		void *fragmentContext;
		if(!SUCCEEDED(fontFileStream->ReadFileFragment(&fragmentStart, 0, fileSize, &fragmentContext)))
			throw FontInvalidNameExc("Failed to get the raw font file data for " + aName);
		mFileData = malloc((size_t)fileSize);
		memcpy(mFileData, fragmentStart, (size_t)fileSize);
		if(FT_New_Memory_Face(FontManager::instance()->mLibrary, reinterpret_cast<FT_Byte*>(mFileData), static_cast<FT_Long>(fileSize), 0, &mFace))
			throw FontInvalidNameExc("Failed to create a face for " + aName);
		FT_Set_Char_Size(mFace, 0, (int)aSize * 64, 0, 72);
		fontFileStream->ReleaseFileFragment(fragmentContext);

		//clean up all the DWrite stuff
		fontFileStream->Release();
		fontFileLoader->Release();
		fontFile->Release();
		fontFace->Release();
		matchingFont->Release();
		fontFamily->Release();
		fontCollection->Release();
		writeFactory->Release();
	}

/*	Font::Obj::Obj(DataSourceRef dataSource, float size)
		: mSize(size)
	{
		FT_New_Memory_Face(FontManager::instance()->mLibrary, (FT_Byte*)dataSource->getBuffer().getData(), dataSource->getBuffer().getDataSize(), 0, &mFace);
		FT_Set_Pixel_Sizes(mFace, 0, (int)size);
	}
*/
	Font::Obj::~Obj()
	{
		FT_Done_Face(mFace);
		free(mFileData);
	}

	void Font::Obj::finishSetup()
	{
	}

	FontInvalidNameExc::FontInvalidNameExc(const std::string &fontName) throw()
	{
		sprintf_s(mMessage, "%s", fontName.c_str());
	}

}