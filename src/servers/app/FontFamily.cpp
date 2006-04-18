/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Classes to represent font styles and families */


#include "FontFamily.h"
#include "ServerFont.h"
#include "FontManager.h"

#include <FontPrivate.h>
#include FT_CACHE_H

#include <Entry.h>


const uint32 kInvalidFamilyFlags = ~0UL;

static BLocker sFontLock("font lock");


static int
font_score(const FontStyle* style)
{
	int score = 0;
	if (style->Face() & B_REGULAR_FACE)
		score += 10;
	else {
		if (style->Face() & B_BOLD_FACE)
			score += 5;
		if (style->Face() & B_ITALIC_FACE)
			score--;
	}

	return score;
}


static int
compare_font_styles(const FontStyle* a, const FontStyle* b)
{
	// Regular fonts come first, then bold, then italics
	return font_score(b) - font_score(a);
}


//	#pragma mark -


/*!
	\brief Constructor
	\param filepath path to a font file
	\param face FreeType handle for the font file after it is loaded - it will be kept open until the FontStyle is destroied
*/
FontStyle::FontStyle(node_ref& nodeRef, const char* path, FT_Face face)
	: //BLocker(BString("FontStyle_").Append(face->style_name).String()),
	fFreeTypeFace(face),
	fName(face->style_name),
	fPath(path),
	fNodeRef(nodeRef),
	fFamily(NULL),
	fID(0),
	fBounds(0, 0, 0, 0),
	fFace(_TranslateStyleToFace(face->style_name))
{
	fName.Truncate(B_FONT_STYLE_LENGTH);
		// make sure this style can be found using the Be API

	fHeight.ascent = (double)face->ascender / face->units_per_EM;
	fHeight.descent = (double)-face->descender / face->units_per_EM;
		// FT2's descent numbers are negative. Be's is positive

	// FT2 doesn't provide a linegap, but according to the docs, we can
	// calculate it because height = ascending + descending + leading
	fHeight.leading = (double)(face->height - face->ascender + face->descender)
		/ face->units_per_EM;
}


FontStyle::~FontStyle()
{
	// make sure the font server is ours
	if (fFamily != NULL && gFontManager->Lock()) {
		gFontManager->RemoveStyle(this);
		gFontManager->Unlock();
	}

	FT_Done_Face(fFreeTypeFace);
}


uint32
FontStyle::Hash() const
{
	return (ID() << 16) | fFamily->ID();
}


bool
FontStyle::CompareTo(Hashable& other) const
{
	// our hash values are unique (unless you have more than 65536 font families installed...)
	return Hash() == other.Hash();
}


bool
FontStyle::Lock()
{
	return sFontLock.Lock();
}


void
FontStyle::Unlock()
{
	sFontLock.Unlock();
}


void
FontStyle::GetHeight(float size, font_height& height) const
{
	height.ascent = fHeight.ascent * size;
	height.descent = fHeight.descent * size;
	height.leading = fHeight.leading * size;
}


/*!
	\brief Returns the path to the style's font file 
	\return The style's font file path
*/
const char*
FontStyle::Path() const
{
	return fPath.Path();
}


/*!
	\brief Updates the path of the font style in case the style
		has been moved around.
*/
void
FontStyle::UpdatePath(const node_ref& parentNodeRef)
{
	entry_ref ref;
	ref.device = parentNodeRef.device;
	ref.directory = parentNodeRef.node;
	ref.set_name(fPath.Leaf());

	fPath.SetTo(&ref);
}


/*!
	\brief Unlike BFont::Flags() this returns the extra flags field as used
		in the private part of BFont.
*/
uint32
FontStyle::Flags() const
{
	uint32 flags = uint32(Direction()) << B_PRIVATE_FONT_DIRECTION_SHIFT;

	if (IsFixedWidth())
		flags |= B_IS_FIXED;
	if (TunedCount() > 0)
		flags |= B_HAS_TUNED_FONT;
	if (HasKerning())
		flags |= B_PRIVATE_FONT_HAS_KERNING;

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


status_t
FontStyle::UpdateFace(FT_Face face)
{
	if (!sFontLock.IsLocked()) {
		debugger("UpdateFace() called without having locked FontStyle!");
		return B_ERROR;
	}

	// we only accept the face if it hasn't change its style

	BString name = face->style_name;
	name.Truncate(B_FONT_STYLE_LENGTH);

	if (name != fName)
		return B_BAD_VALUE;

	FT_Done_Face(fFreeTypeFace);
	fFreeTypeFace = face;
	return B_OK;
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


void
FontStyle::_SetFontFamily(FontFamily* family, uint16 id)
{
	fFamily = family;
	fID = id;
}


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
	
	Deletes all attached styles. Note that a FontFamily must only be deleted
	by the font manager.
*/
FontFamily::~FontFamily()
{
	for (int32 i = fStyles.CountItems(); i-- > 0;) {
		FontStyle* style = fStyles.RemoveItemAt(i);

		// we remove us before deleting the style, so that the font manager
		// is not contacted to remove the style from us
		style->_SetFontFamily(NULL, -1);
		delete style;
	}
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

	if (!fStyles.BinaryInsert(style, compare_font_styles))
		return false;

	style->_SetFontFamily(this, fNextID++);

	// force a refresh if a request for font flags is needed
	fFlags = kInvalidFamilyFlags;

	return true;
}


/*!
	\brief Removes a style from the family.

	The font style will not be deleted.
*/
bool
FontFamily::RemoveStyle(FontStyle* style)
{
	if (!gFontManager->IsLocked()) {
		debugger("FontFamily::RemoveStyle() called without having the font manager locked!");
		return false;
	}

	if (!fStyles.RemoveItem(style))
		return false;

	style->_SetFontFamily(NULL, -1);

	// force a refresh if a request for font flags is needed
	fFlags = kInvalidFamilyFlags;
	return true;
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


uint32
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
