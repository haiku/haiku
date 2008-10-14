/*
 * Copyright 2001-2008, Haiku.
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

#include <Entry.h>


static BLocker sFontLock("font lock");


/*!
	\brief Constructor
	\param filepath path to a font file
	\param face FreeType handle for the font file after it is loaded - it will
		   be kept open until the FontStyle is destroyed
*/
FontStyle::FontStyle(node_ref& nodeRef, const char* path, FT_Face face)
	:
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
	// our hash values are unique (unless you have more than 65536 font
	// families installed...)
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
	if (IsFullAndHalfFixed())
		flags |= B_PRIVATE_FONT_IS_FULL_AND_HALF_FIXED;
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


void
FontStyle::_SetFontFamily(FontFamily* family, uint16 id)
{
	fFamily = family;
	fID = id;
}


uint16
FontStyle::_TranslateStyleToFace(const char* name) const
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

	if (string.IFindFirst("condensed") >= 0)
		face |= B_CONDENSED_FACE;

	if (string.IFindFirst("light") >= 0)
		face |= B_LIGHT_FACE;

	if (string.IFindFirst("heavy") >= 0
		|| string.IFindFirst("black") >= 0)
		face |= B_HEAVY_FACE;

	if (face == 0)
		return B_REGULAR_FACE;

	return face;
}


