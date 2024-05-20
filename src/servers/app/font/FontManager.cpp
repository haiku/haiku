/*
 * Copyright 2001-2016, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


/*!	Manages font families and styles */


#include "FontManager.h"

#include <new>

#include <Debug.h>

#include "FontFamily.h"


//#define TRACE_FONT_MANAGER
#ifdef TRACE_FONT_MANAGER
#	define FTRACE(x) printf x
#else
#	define FTRACE(x) ;
#endif


FT_Library gFreeTypeLibrary;


static int
compare_font_families(const FontFamily* a, const FontFamily* b)
{
	return strcmp(a->Name(), b->Name());
}


//	#pragma mark -


FontManager::FontManager()
	:
	fFamilies(20),
	fRevision(0),
	fNextID(0)
{
}


FontManager::~FontManager()
{
	_RemoveAllFonts();
}


/*!	\brief Finds and returns the first valid charmap in a font

	\param face Font handle obtained from FT_Load_Face()
	\return An FT_CharMap or NULL if unsuccessful
*/
FT_CharMap
FontManager::_GetSupportedCharmap(const FT_Face& face)
{
	for (int32 i = 0; i < face->num_charmaps; i++) {
		FT_CharMap charmap = face->charmaps[i];

		switch (charmap->platform_id) {
			case 3:
				// if Windows Symbol or Windows Unicode
				if (charmap->encoding_id == 0 || charmap->encoding_id == 1)
					return charmap;
				break;

			case 1:
				// if Apple Unicode
				if (charmap->encoding_id == 0)
					return charmap;
				break;

			case 0:
				// if Apple Roman
				if (charmap->encoding_id == 0)
					return charmap;
				break;

			default:
				break;
		}
	}

	return NULL;
}



/*!	\brief Counts the number of font families available
	\return The number of unique font families currently available
*/
int32
FontManager::CountFamilies()
{
	return fFamilies.CountItems();
}


/*!	\brief Counts the number of styles available in a font family
	\param family Name of the font family to scan
	\return The number of font styles currently available for the font family
*/
int32
FontManager::CountStyles(const char *familyName)
{
	FontFamily *family = GetFamily(familyName);
	if (family != NULL)
		return family->CountStyles();

	return 0;
}


/*!	\brief Counts the number of styles available in a font family
	\param family Name of the font family to scan
	\return The number of font styles currently available for the font family
*/
int32
FontManager::CountStyles(uint16 familyID)
{
	FontFamily *family = GetFamily(familyID);
	if (family != NULL)
		return family->CountStyles();

	return 0;
}


FontFamily*
FontManager::FamilyAt(int32 index) const
{
	ASSERT(IsLocked());

	return fFamilies.ItemAt(index);
}


/*!	\brief Locates a FontFamily object by name
	\param name The family to find
	\return Pointer to the specified family or NULL if not found.
*/
FontFamily*
FontManager::GetFamily(const char* name)
{
	if (name == NULL)
		return NULL;

	return _FindFamily(name);
}


FontFamily*
FontManager::GetFamily(uint16 familyID) const
{
	FontKey key(familyID, 0);
	FontStyle* style = fStyleHashTable.Get(key);
	if (style != NULL)
		return style->Family();

	// Try the slow route in case style 0 was removed
	return _FindFamily(familyID);
}


FontStyle*
FontManager::GetStyleByIndex(const char* familyName, int32 index)
{
	FontFamily* family = GetFamily(familyName);
	if (family != NULL)
		return family->StyleAt(index);

	return NULL;
}


FontStyle*
FontManager::GetStyleByIndex(uint16 familyID, int32 index)
{
	FontFamily* family = GetFamily(familyID);
	if (family != NULL)
		return family->StyleAt(index);

	return NULL;
}


/*!	\brief Retrieves the FontStyle object
	\param family ID for the font's family
	\param style ID of the font's style
	\return The FontStyle having those attributes or NULL if not available
*/
FontStyle*
FontManager::GetStyle(uint16 familyID, uint16 styleID) const
{
	ASSERT(IsLocked());

	FontKey key(familyID, styleID);
	FontStyle* style = fStyleHashTable.Get(key);
	if (style != NULL)
		return style;

	return fDelistedStyleHashTable.Get(key);
}


/*!	\brief Retrieves the FontStyle object that comes closest to the one
		specified.

	\param family The font's family or NULL in which case \a familyID is used
	\param style The font's style or NULL in which case \a styleID is used
	\param familyID will only be used if \a family is NULL (or empty)
	\param styleID will only be used if \a family and \a style are NULL (or empty)
	\param face is used to specify the style if both \a style is NULL or empty
		and styleID is 0xffff.

	\return The FontStyle having those attributes or NULL if not available
*/
FontStyle*
FontManager::GetStyle(const char* familyName, const char* styleName,
	uint16 familyID, uint16 styleID, uint16 face)
{
	ASSERT(IsLocked());

	FontFamily* family;

	if (styleID != 0xffff && (familyName == NULL || !familyName[0])
		&& (styleName == NULL || !styleName[0])) {
		return GetStyle(familyID, styleID);
	}

	// find family

	if (familyName != NULL && familyName[0])
		family = GetFamily(familyName);
	else
		family = GetFamily(familyID);

	if (family == NULL)
		return NULL;

	// find style

	if (styleName != NULL && styleName[0])
		return family->GetStyle(styleName);

	// try to get from face
	return family->GetStyleMatchingFace(face);
}


/*!	\brief If you don't find your preferred font style, but are anxious
		to have one fitting your needs, you may want to use this method.
*/
FontStyle*
FontManager::FindStyleMatchingFace(uint16 face) const
{
	int32 count = fFamilies.CountItems();

	for (int32 i = 0; i < count; i++) {
		FontFamily* family = fFamilies.ItemAt(i);
		FontStyle* style = family->GetStyleMatchingFace(face);
		if (style != NULL)
			return style;
	}

	return NULL;
}


/*!	\brief This call is used by the FontStyle class - and the FontStyle class
		only - to remove itself from the font manager.
	At this point, the style is already no longer available to the user.
*/
void
FontManager::RemoveStyle(FontStyle* style)
{
	ASSERT(IsLocked());

	FontFamily* family = style->Family();
	if (family == NULL)
		debugger("family is NULL!");

	family->RemoveStyle(style);
	fDelistedStyleHashTable.Remove(FontKey(family->ID(), style->ID()));
}


uint32
FontManager::Revision()
{
	return fRevision;
}


status_t
FontManager::_AddFont(FT_Face face, node_ref nodeRef, const char* path,
	uint16& familyID, uint16& styleID)
{
	ASSERT(IsLocked());

	BReference<FontFamily> family(_FindFamily(face->family_name));
	bool isNewFontFamily = !family.IsSet();

	if (family.IsSet() && family->HasStyle(face->style_name)) {
		// prevent adding the same style twice
		// (this indicates a problem with the installed fonts maybe?)
		FT_Done_Face(face);
		return B_NAME_IN_USE;
	}

	if (!family.IsSet()) {
		family.SetTo(new (std::nothrow) FontFamily(face->family_name, _NextID()), true);

		if (!family.IsSet() || !fFamilies.BinaryInsert(family, compare_font_families)) {
			FT_Done_Face(face);
			return B_NO_MEMORY;
		}
	}

	FTRACE(("\tadd style: %s, %s\n", face->family_name, face->style_name));

	// the FontStyle takes over ownership of the FT_Face object
	FontStyle* style = new (std::nothrow) FontStyle(nodeRef, path, face, this);

	if (style == NULL || !family->AddStyle(style)) {
		delete style;
		if (isNewFontFamily)
			fFamilies.RemoveItem(family);
		return B_NO_MEMORY;
	}

	familyID = style->Family()->ID();
	styleID = style->ID();

	fStyleHashTable.Put(FontKey(familyID, styleID), style);
	style->ReleaseReference();

	fRevision++;
	return B_OK;
}


FontStyle*
FontManager::_RemoveFont(uint16 familyID, uint16 styleID)
{
	ASSERT(IsLocked());

	FontKey key(familyID, styleID);
	FontStyle* style = fStyleHashTable.Get(key);
	if (style != NULL) {
		fDelistedStyleHashTable.Put(key, style);
		FontFamily* family = style->Family();
		if (family->RemoveStyle(style) && family->CountStyles() == 0)
			fFamilies.RemoveItem(family);
		fStyleHashTable.Remove(key);
	}

	fRevision++;
	return style;
}


void
FontManager::_RemoveAllFonts()
{
	fFamilies.MakeEmpty();

	// Disconnect the styles from their families before removing them; once we
	// get to this point, we are in the dtor and don't want them to call back.

	HashMap<FontKey, FontStyle*>::Iterator delisted = fDelistedStyleHashTable.GetIterator();
	while (delisted.HasNext())
		delisted.Next().value->_SetFontFamily(NULL, -1);
	fDelistedStyleHashTable.Clear();

	HashMap<FontKey, BReference<FontStyle> >::Iterator referenced = fStyleHashTable.GetIterator();
	while (referenced.HasNext())
		referenced.Next().value->_SetFontFamily(NULL, -1);
	fStyleHashTable.Clear();
}


FontFamily*
FontManager::_FindFamily(const char* name) const
{
	if (name == NULL)
		return NULL;

	FontFamily family(name, 0);
	return const_cast<FontFamily*>(fFamilies.BinarySearch(family,
		compare_font_families));
}


FontFamily*
FontManager::_FindFamily(uint16 familyID) const
{
	int32 count = fFamilies.CountItems();

	for (int32 i = 0; i < count; i++) {
		FontFamily* family = fFamilies.ItemAt(i);
		if (family->ID() == familyID)
			return family;
	}

	return NULL;
}


uint16
FontManager::_NextID()
{
	return fNextID++;
}
