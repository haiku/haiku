/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H


#include <AutoDeleter.h>
#include <HashMap.h>
#include <Looper.h>
#include <ObjectList.h>
#include <Referenceable.h>

#include <ft2build.h>
#include FT_FREETYPE_H


class BEntry;
class BPath;
struct node_ref;

class FontFamily;
class FontStyle;
class ServerFont;


/*!
	\class FontManager FontManager.h
	\brief Base class interface used by GlobalFontManager and AppFontManager
*/
class FontManagerBase : public BLooper {
public:
								FontManagerBase(bool init_freetype,
									const char* className = "FontManagerBase");
	virtual						~FontManagerBase();

			status_t			InitCheck() { return fInitStatus; }
			void  				SetInitStatus(status_t new_status)
									{ fInitStatus = new_status; }

	virtual	void				MessageReceived(BMessage* message);

	virtual	int32				CountFamilies();

	virtual	int32				CountStyles(const char* family);
	virtual	int32				CountStyles(uint16 familyID);
			FontFamily*			FamilyAt(int32 index) const;

	virtual	FontFamily*			GetFamily(uint16 familyID) const;
	virtual	FontFamily*			GetFamily(const char* name);

			FontStyle*			GetStyleByIndex(const char* family,
									int32 index);
			FontStyle*			GetStyleByIndex(uint16 familyID, int32 index);

	virtual	FontStyle*			GetStyle(uint16 familyID,
									uint16 styleID) const;
	virtual	FontStyle*			GetStyle(const char* familyName,
									const char* styleName,
									uint16 familyID = 0xffff,
									uint16 styleID = 0xffff,
									uint16 face = 0);
			FontStyle*			FindStyleMatchingFace(uint16 face) const;

			void				RemoveStyle(FontStyle* style);
				// This call must not be used by anything else than class
				// FontStyle.


			FT_CharMap			_GetSupportedCharmap(const FT_Face& face);

protected:
			FontFamily*			_FindFamily(const char* family) const;

			static int 			compare_font_families(const FontFamily* a,
									const FontFamily* b);

			struct FontKey {
				FontKey(uint16 family, uint16 style)
					: familyID(family), styleID(style) {}

				uint32 GetHashCode() const
				{
					return familyID | (styleID << 16UL);
				}

				bool operator==(const FontKey& other) const
				{
					return familyID == other.familyID
						&& styleID == other.styleID;
				}

				uint16 familyID, styleID;
			};

			status_t			fInitStatus;

			typedef BObjectList<FontFamily>			FamilyList;
			FamilyList			fFamilies;

			HashMap<FontKey, BReference<FontStyle> > fStyleHashTable;

			uint16				fNextID;
			bool  				fHasFreetypeLibrary;
};

extern FT_Library gFreeTypeLibrary;

#endif	/* FONT_MANAGER_H */
