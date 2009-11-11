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

#include "FontManager.h"

#include <FontPrivate.h>


const uint32 kInvalidFamilyFlags = ~0UL;


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
	\param style pointer to FontStyle object to be added
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


FontStyle*
FontFamily::_FindStyle(const char* name) const
{
	int32 count = fStyles.CountItems();
	if (!name || count < 1)
		return NULL;

	for (int32 i = 0; i < count; i++) {
		FontStyle *style = fStyles.ItemAt(i);
		if (!strcmp(style->Name(), name))
			return style;
	}

	return NULL;
}


/*!
	\brief Determines whether the style belongs to the family
	\param style Name of the style being checked
	\return True if it belongs, false if not
*/
bool
FontFamily::HasStyle(const char *styleName) const
{
	return _FindStyle(styleName) != NULL;
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
FontFamily::GetStyle(const char *name) const
{
	if (name == NULL || !name[0])
		return NULL;

	FontStyle* style = _FindStyle(name);
	if (style != NULL)
		return style;

	// try alternative names

	if (!strcmp(name, "Roman") || !strcmp(name, "Regular")
		|| !strcmp(name, "Book")) {
		style = _FindStyle("Roman");
		if (style == NULL) {
			style = _FindStyle("Regular");
			if (style == NULL)
				style = _FindStyle("Book");
		}
		return style;
	}

	BString alternative = name;
	if (alternative.FindFirst("Italic") >= 0) {
		alternative.ReplaceFirst("Italic", "Oblique");
		return _FindStyle(alternative.String());
	}
	if (alternative.FindFirst("Oblique") >= 0) {
		alternative.ReplaceFirst("Oblique", "Italic");
		return _FindStyle(alternative.String());
	}

	return NULL;
}


FontStyle*
FontFamily::GetStyleByID(uint16 id) const
{
	int32 count = fStyles.CountItems();
	for (int32 i = 0; i < count; i++) {
		FontStyle* style = fStyles.ItemAt(i);
		if (style->ID() == id)
			return style;
	}

	return NULL;
}


FontStyle*
FontFamily::GetStyleMatchingFace(uint16 face) const
{
	// TODO: support other faces (strike through, underlined, outlines...)
	face &= B_BOLD_FACE | B_ITALIC_FACE | B_REGULAR_FACE | B_CONDENSED_FACE
		| B_LIGHT_FACE | B_HEAVY_FACE;

	int32 count = fStyles.CountItems();
	for (int32 i = 0; i < count; i++) {
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

		int32 count = fStyles.CountItems();
		for (int32 i = 0; i < count; i++) {
			FontStyle* style = fStyles.ItemAt(i);

			if (style->IsFixedWidth())
				fFlags |= B_IS_FIXED;
			if (style->IsFullAndHalfFixed())
				fFlags |= B_PRIVATE_FONT_IS_FULL_AND_HALF_FIXED;
			if (style->TunedCount() > 0)
				fFlags |= B_HAS_TUNED_FONT;
		}
	}

	return fFlags;
}
