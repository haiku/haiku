/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Classes to represent font styles and families */


#include "FontFamily.h"
#include "ServerFont.h"
#include FT_CACHE_H


const int32 kInvalidFamilyFlags = -1;


/*!
	\brief Constructor
	\param filepath path to a font file
	\param face FreeType handle for the font file after it is loaded - it will be kept open until the FontStyle is destroied
*/
FontStyle::FontStyle(const char *path, FT_Face face)
	:
	fFTFace(face),
	fFontFamily(NULL),
	fName(face->style_name),
	fPath(path),
	fBounds(0, 0, 0, 0),
	fID(0),
	fFace(_TranslateStyleToFace(face->style_name))
{
	fName.Truncate(B_FONT_STYLE_LENGTH);
		// make sure this style can be found using the Be API

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
	FT_Done_Face(fFTFace);
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
FontStyle::Path() const
{
	return fPath.String();
}


int32
FontStyle::Flags() const
{
	int32 flags = 0;

	if (IsFixedWidth())
		flags |= B_IS_FIXED;
	if (TunedCount() > 0)
		flags |= B_HAS_TUNED_FONT;

	return flags;
}


/*!
	\brief Updates the given face to match the one from this style
	
	The specified font face often doesn't match the exact face of
	a style. This method will preserve the attributes of the face
	that this style does not alter, and will only update the
	attributes that matter to this style.
	The font renderer could then emulate the other face attributes
	taking this style as a base.
*/
uint16
FontStyle::PreservedFace(uint16 face) const
{
	// TODO: make this better
	face &= ~(B_REGULAR_FACE | B_BOLD_FACE | B_ITALIC_FACE);
	face |= Face();

	return face;
}


/*!
	\brief Converts an ASCII character to Unicode for the style
	\param c An ASCII character
	\return A Unicode value for the character
*/

// TODO: Re-enable when I understand how the FT2 Cache system changed from
// 2.1.4 to 2.1.8
/*
int16
FontStyle::ConvertToUnicode(uint16 c)
{
	FT_Face f;
	if(FTC_Manager_LookupFace(ftmanager,(FTC_FaceID)cachedface,&f)!=0)
		return 0;
	
	return FT_Get_Char_Index(f,c);
}
*/

uint16
FontStyle::_TranslateStyleToFace(const char *name) const
{
	if (name == NULL)
		return 0;

	BString string(name);
	uint16 face = 0;

	if (string.IFindFirst("bold") >= 0)
		face |= B_BOLD_FACE;

	if (string.IFindFirst("italic") >= 0
		|| string.IFindFirst("oblique") >= 0)
		face |= B_ITALIC_FACE;

	if (face == 0)
		return B_REGULAR_FACE;

	return face;
}


//	#pragma mark -


/*!
	\brief Constructor
	\param namestr Name of the family
*/
FontFamily::FontFamily(const char *name, uint16 id)
	:
	fName(name),
	fID(id),
	fNextID(0),
	fFlags(kInvalidFamilyFlags)
{
	fName.Truncate(B_FONT_FAMILY_LENGTH);
		// make sure this family can be found using the Be API
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
		delete fStyles.ItemAt(i);
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

	// Don't add if it already is in the family.	
	int32 count = fStyles.CountItems();
	for (int32 i = 0; i < count; i++) {
		FontStyle *item = fStyles.ItemAt(i);
		if (!strcmp(item->Name(), style->Name()))
			return false;
	}

	style->_SetFontFamily(this);
	style->_SetID(fNextID++);

	fStyles.AddItem(style);
	AddDependent();

	// force a refresh if a request for font flags is needed
	fFlags = kInvalidFamilyFlags;

	return true;
}


/*!
	\brief Removes a style from the family and deletes it
	\param style Name of the style to be removed from the family
*/
void
FontFamily::RemoveStyle(const char* styleName)
{
	FontStyle *style = GetStyle(styleName);
	if (style == NULL)
		return;

	fStyles.RemoveItem(style);
	delete style;

	RemoveDependent();

	// force a refresh if a request for font flags is needed
	fFlags = kInvalidFamilyFlags;
}


/*!
	\brief Removes a style from the family. The caller is responsible for freeing the object
	\param style The style to be removed from the family
*/
void
FontFamily::RemoveStyle(FontStyle* style)
{
	if (fStyles.RemoveItem(style)) {
		RemoveDependent();

		// force a refresh if a request for font flags is needed
		fFlags = kInvalidFamilyFlags;
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
FontFamily::HasStyle(const char *styleName) const
{
	return GetStyle(styleName) != NULL;
}


/*! 
	\brief Returns the name of a style in the family
	\param index list index of the style to be found
	\return name of the style or NULL if the index is not valid
*/
FontStyle*
FontFamily::StyleAt(int32 index) const
{
	return fStyles.ItemAt(index);
}


/*!
	\brief Get the FontStyle object for the name given
	\param style Name of the style to be obtained
	\return The FontStyle object or NULL if none was found.
	
	The object returned belongs to the family and must not be deleted.
*/
FontStyle*
FontFamily::GetStyle(const char *styleName) const
{
	int32 count = fStyles.CountItems();
	if (!styleName || count < 1)
		return NULL;

	for (int32 i = 0; i < count; i++) {
		FontStyle *style = fStyles.ItemAt(i);
		if (!strcmp(style->Name(), styleName))
			return style;
	}

	return NULL;
}


FontStyle*
FontFamily::GetStyleByID(uint16 id) const
{
	for (int32 i = 0; i < fStyles.CountItems(); i++) {
		FontStyle* style = fStyles.ItemAt(i);
		if (style->ID() == id)
			return style;
	}

	return NULL;
}


FontStyle*
FontFamily::GetStyleMatchingFace(uint16 face) const
{
	// we currently only use bold/italic/regular faces
	face &= B_BOLD_FACE | B_ITALIC_FACE | B_REGULAR_FACE;

	for (int32 i = 0; i < fStyles.CountItems(); i++) {
		FontStyle* style = fStyles.ItemAt(i);
		
		if (style->Face() == face)
			return style;
	}

	return NULL;
}


int32
FontFamily::Flags()
{
	if (fFlags == kInvalidFamilyFlags) {
		fFlags = 0;

		for (int32 i = 0; i < fStyles.CountItems(); i++) {
			FontStyle* style = fStyles.ItemAt(i);

			if (style->IsFixedWidth())
				fFlags |= B_IS_FIXED;
			if (style->TunedCount() > 0)
				fFlags |= B_HAS_TUNED_FONT;
		}
	}
	
	return fFlags;
}
