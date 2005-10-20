//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		FontFamily.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	classes to represent font styles and families
//  
//------------------------------------------------------------------------------
#include "FontFamily.h"
#include "ServerFont.h"
#include FT_CACHE_H

FTC_Manager ftmanager;

/*!
	\brief Constructor
	\param filepath path to a font file
	\param face FreeType handle for the font file after it is loaded - it will be kept open until the FontStyle is destroied
*/
FontStyle::FontStyle(const char *filepath, FT_Face face)
	: fFTFace(face),
	  fFontFamily(NULL),
	  fName(face->style_name),
	  fPath(filepath),
	  fBounds(0, 0, 0, 0),
	  fID(0),
	  fFace(TranslateStyleToFace(face->style_name))
{
//	cachedface = new CachedFaceRec;
//	cachedface->file_path = filepath;

	fHeight.ascent = face->ascender;
	// FT2's descent numbers are negative. Be's is positive
	fHeight.descent = -face->descender;
	
	// FT2 doesn't provide a linegap, but according to the docs, we can
	// calculate it because height = ascending + descending + leading
	fHeight.leading = face->height - (fHeight.ascent + fHeight.descent);
	fHeight.units_per_em = face->units_per_EM;
}

/*!
	\brief Destructor
	
	Frees all data allocated on the heap. All child ServerFonts are marked as having
	a NULL style so that each ServerFont knows that it is running on borrowed time. 
	This is done because a FontStyle should be deleted only when it no longer has any
	dependencies.
*/
FontStyle::~FontStyle()
{
// TODO: what was the purpose of this?
//	delete cachedface;
// TODO: figure out if it is safe to call this:
//	FT_Done_Face(fFTFace);
}

/*!
	\brief Returns the name of the style as a string
	\return The style's name
*/
const char*
FontStyle::Name() const
{
	return fName.String();
}

font_height
FontStyle::GetHeight(const float &size) const
{
	font_height fh = { 0, 0, 0 };
	
	// font units are 26.6 format, so we get REALLY big numbers if
	// we don't do some shifting.
	// TODO: As it looks like that the "units_per_em" don't change
	// for the lifetime of FontStyle, can't we apply these
	// conversions in the constructor and get rid of "units_per_em" ?
	fh.ascent = (fHeight.ascent * size) / fHeight.units_per_em;
	fh.descent = (fHeight.descent * size) / fHeight.units_per_em;
	fh.leading = (fHeight.leading * size) / fHeight.units_per_em;
	return fh;
}

/*!
	\brief Returns the path to the style's font file 
	\return The style's font file path
*/
const char*
FontStyle::GetPath() const
{
	return fPath.String();
}

int32
FontStyle::GetFlags() const
{
	int32 flags=0;
	if(IsFixedWidth())
		flags|=B_IS_FIXED;
	
	if(TunedCount()>0)
		flags|=B_HAS_TUNED_FONT;
	
	return flags;
}

/*!
	\brief Converts an ASCII character to Unicode for the style
	\param c An ASCII character
	\return A Unicode value for the character
*/

// TODO: Re-enable when I understand how the FT2 Cache system changed from
// 2.1.4 to 2.1.8
/*
int16 FontStyle::ConvertToUnicode(uint16 c)
{
	FT_Face f;
	if(FTC_Manager_LookupFace(ftmanager,(FTC_FaceID)cachedface,&f)!=0)
		return 0;
	
	return FT_Get_Char_Index(f,c);
}
*/

uint16
FontStyle::TranslateStyleToFace(const char *name) const
{
	if(!name)
		return 0;
	
	BString str(name);
	
	
	if(str.IFindFirst("bold")!=B_ERROR)
		return B_BOLD_FACE;
	if(str.IFindFirst("italic")!=B_ERROR)
		return B_ITALIC_FACE;
	if(str.IFindFirst("oblique")!=B_ERROR)
		return B_ITALIC_FACE;
	
	return B_REGULAR_FACE;
}


/*!
	\brief Constructor
	\param namestr Name of the family
*/
FontFamily::FontFamily(const char *namestr, const uint16 &index)
{
	fName = namestr;
	fID = index;
	
	// will stay uninitialized until needed
	fFlags = -1;
}

/*!
	\brief Destructor
	
	Deletes all child styles. Note that a FontFamily should not be deleted unless
	its styles have no dependencies or some other really good reason, such as 
	system shutdown.
*/
FontFamily::~FontFamily()
{
	int32 count = fStyles.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (FontStyle*)fStyles.ItemAt(i);
}

/*!
	\brief Returns the name of the family
	\return The family's name
*/
const char*
FontFamily::Name() const
{
	return fName.String();
}

/*!
	\brief Adds the style to the family
	\param face FreeType face handle used to obtain info about the font
*/
bool
FontFamily::AddStyle(FontStyle *style)
{
	if (!style)
		return false;

	FontStyle *item;

	// Don't add if it already is in the family.	
	int32 count = fStyles.CountItems();
	for(int32 i = 0; i < count; i++) {
		item = (FontStyle*)fStyles.ItemAt(i);
		if (item->fName == style->fName)
			return false;
	}
	
	style->fFontFamily = this;
	
	if (fStyles.CountItems() > 0) {
		item = (FontStyle*)fStyles.ItemAt(fStyles.CountItems() - 1);
		style->fID = item->fID + 1;
	} else {
		style->fID = 0;
	}
	
	fStyles.AddItem(style);
	AddDependent();
	
	// force a refresh if a request for font flags is needed
	fFlags=-1;
	
	return true;
}

/*!
	\brief Removes a style from the family and deletes it
	\param style Name of the style to be removed from the family
*/
void
FontFamily::RemoveStyle(const char* style)
{
	int32 count = fStyles.CountItems();
	if (!style || count < 1)
		return;

	FontStyle *fs;
	for (int32 i = 0; i < count; i++) {
		fs = (FontStyle*)fStyles.ItemAt(i);
		if (fs && fs->fName.Compare(style) == 0) {
			if (fStyles.RemoveItem((void*)fs)) {
				delete fs;
				RemoveDependent();
				
				// force a refresh if a request for font flags is needed
				fFlags = -1;
				break;
			}
		}
	}
}

/*!
	\brief Removes a style from the family. The caller is responsible for freeing the object
	\param style The style to be removed from the family
*/
void
FontFamily::RemoveStyle(FontStyle* style)
{
	if (fStyles.RemoveItem((void*)style)) {
		RemoveDependent();
		
		// force a refresh if a request for font flags is needed
		fFlags = -1;
	}
}

/*!
	\brief Returns the number of styles in the family
	\return The number of styles in the family
*/
int32
FontFamily::CountStyles() const
{
	return fStyles.CountItems();
}

/*!
	\brief Determines whether the style belongs to the family
	\param style Name of the style being checked
	\return True if it belongs, false if not
*/
bool
FontFamily::HasStyle(const char *style) const
{
	int32 count = fStyles.CountItems();
	
	if (!style || count < 1)
		return false;
	
	FontStyle *fs;
	for (int32 i = 0; i < count; i++) {
		fs = (FontStyle*)fStyles.ItemAt(i);
		if( fs && fs->fName.Compare(style) == 0)
			return true;
	}
	return false;
}

/*! 
	\brief Returns the name of a style in the family
	\param index list index of the style to be found
	\return name of the style or NULL if the index is not valid
*/
FontStyle*
FontFamily::GetStyle(int32 index) const
{
	return (FontStyle*)fStyles.ItemAt(index);
}

/*!
	\brief Get the FontStyle object for the name given
	\param style Name of the style to be obtained
	\return The FontStyle object or NULL if none was found.
	
	The object returned belongs to the family and must not be deleted.
*/
FontStyle*
FontFamily::GetStyle(const char *style) const
{
	int32 count=fStyles.CountItems();
	if (!style || count < 1)
		return NULL;

	FontStyle *fs;
	for (int32 i = 0; i < count; i++) {
		fs = (FontStyle*)fStyles.ItemAt(i);
		if (fs && fs->fName.Compare(style) == 0)
			return fs;
	}
	return NULL;
}

int32
FontFamily::GetFlags()
{
	if (fFlags == -1) {
		fFlags = 0;
		
		for (int32 i = 0; i < fStyles.CountItems(); i++) {
			FontStyle* style = (FontStyle*)fStyles.ItemAt(i);
			if (style) {
				if (style->IsFixedWidth())
					fFlags |= B_IS_FIXED;
				if (style->TunedCount() > 0)
					fFlags |= B_HAS_TUNED_FONT;
			}
		}
	}
	
	return fFlags;
}
