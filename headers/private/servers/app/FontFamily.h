//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		FontFamily.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	classes to represent font styles and families
//  
//------------------------------------------------------------------------------
#ifndef FONT_FAMILY_H_
#define FONT_FAMILY_H_

#include <String.h>
#include <Rect.h>
#include <Font.h>
#include <List.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "SharedObject.h"

class FontFamily;
class ServerFont;

enum font_format
{
	FONT_TRUETYPE=0,
	FONT_TYPE_1,
	FONT_OPENTYPE,
	FONT_BDF,
	FONT_CFF,
	FONT_CID,
	FONT_PCF,
	FONT_PFR,
	FONT_SFNT,
	FONT_TYPE_42,
	FONT_WINFONT,
};


//! data structure used by the FreeType cache manager
typedef struct CachedFaceRec_
{
	BString file_path; 
	int face_index; 
} CachedFaceRec, *CachedFace;

/*!
	\class FontStyle FontFamily.h
	\brief Object used to represent a font style
	
	FontStyle objects help abstract a lot of the font engine details while
	still offering plenty of information the style in question.
*/
class FontStyle : public SharedObject
{
public:
	FontStyle(const char *filepath, FT_Face face);
	~FontStyle(void);
	ServerFont *Instantiate(float size, float rotation=0.0, float shear=90.0);

/*!
	\fn bool FontStyle::IsFixedWidth(void)
	\brief Determines whether the font's character width is fixed
	\return true if fixed, false if not
*/
	bool IsFixedWidth(void) { return is_fixedwidth; }
/*!
	\fn bool FontStyle::IsScalable(void)
	\brief Determines whether the font can be scaled to any size
	\return true if scalable, false if not
*/
	bool IsScalable(void) { return is_scalable; }
/*!
	\fn bool FontStyle::HasKerning(void)
	\brief Determines whether the font has kerning information
	\return true if kerning info is available, false if not
*/
	bool HasKerning(void) { return has_kerning; }
/*!
	\fn bool FontStyle::HasTuned(void)
	\brief Determines whether the font contains strikes
	\return true if it has strikes included, false if not
*/
	bool HasTuned(void) { return has_bitmaps; }
/*!
	\fn bool FontStyle::TunedCount(void)
	\brief Returns the number of strikes the style contains
	\return The number of strikes the style contains
*/
	int32 TunedCount(void) { return tunedcount; }
/*!
	\fn bool FontStyle::GlyphCount(void)
	\brief Returns the number of glyphs in the style
	\return The number of glyphs the style contains
*/
	uint16 GlyphCount(void) { return glyphcount; }
/*!
	\fn bool FontStyle::CharMapCount(void)
	\brief Returns the number of character maps the style contains
	\return The number of character maps the style contains
*/
	uint16 CharMapCount(void) { return charmapcount; }
	const char *Name(void);
	FontFamily *Family(void) { return family; }

	// TODO: Re-enable when I understand how the FT2 Cache system changed from
	// 2.1.4 to 2.1.8
//	FT_Face GetFace(void);

	const char *GetPath(void);

	// TODO: Re-enable when I understand how the FT2 Cache system changed from
	// 2.1.4 to 2.1.8
//	int16 ConvertToUnicode(uint16 c);
protected:
	friend class FontFamily;
	FontFamily *family;
	uint16 glyphcount, charmapcount;
	BString *name, *path;
	BList *instances;
	bool is_fixedwidth, is_scalable, has_kerning, has_bitmaps;
	int32 tunedcount;
	CachedFace cachedface;
	uint8 format;
	BRect fbounds;
};

/*!
	\class FontFamily FontFamily.h
	\brief Class representing a collection of similar styles
	
	FontFamily objects bring together many styles of the same face, such as
	Arial Roman, Arial Italic, Arial Bold, etc.
*/
class FontFamily : public SharedObject
{
public:
	FontFamily(const char *namestr);
	~FontFamily(void);
	const char *Name(void);
	void AddStyle(const char *path, FT_Face face);
	void RemoveStyle(const char *style);
	bool HasStyle(const char *style);
	int32 CountStyles(void);
	const char *GetStyle(int32 index);
	FontStyle *GetStyle(const char *style);
protected:
	BString *name;
	BList *styles;
};

#endif
