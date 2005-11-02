/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H


#include <Font.h>
#include <Locker.h>
#include <OS.h>
#include <ObjectList.h>
#include <SupportDefs.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

class BPath;

class FontFamily;
class FontStyle;
class ServerFont;


/*!
	\class FontManager FontManager.h
	\brief Manager for the largest part of the font subsystem
*/
class FontManager : public BLocker {
	public:
		FontManager();
		~FontManager();

		status_t InitCheck() { return fInitStatus; }

		int32 CountFamilies();
		int32 CountStyles(const char *family);
		void RemoveFamily(const char *family);
		void ScanSystemFolders();
		status_t ScanDirectory(const char *path);

		FontFamily* GetFamilyByIndex(int32 index) const;
		FontFamily *GetFamily(uint16 familyID) const;
		FontFamily *GetFamily(const char *name) const;

		FontStyle *GetStyleByIndex(const char *family, int32 index) const;
		FontStyle *GetStyle(const char *family, const char *style, uint16 familyID = 0xffff,
						uint16 styleID = 0xffff, uint16 face = 0);
		FontStyle *GetStyle(const char *family, uint16 styleID);
		FontStyle *GetStyle(uint16 familyID, uint16 styleID);

		ServerFont *GetSystemPlain();
		ServerFont *GetSystemBold();
		ServerFont *GetSystemFixed();

		bool SetSystemPlain(const char *family, const char *style, float size);
		bool SetSystemBold(const char *family, const char *style, float size);
		bool SetSystemFixed(const char *family, const char *style, float size);

		bool FontsNeedUpdated() { return fNeedUpdate; }
		/*!
			\brief Called when the fonts list has been updated
		*/
		void FontsUpdated() { fNeedUpdate = false; }

	private:
		void _AddFont(BPath &path);

		FT_CharMap _GetSupportedCharmap(const FT_Face &face);

	private:
		status_t	fInitStatus;
		BObjectList<FontFamily> fFamilies;
		ServerFont	*fPlain, *fBold, *fFixed;
		bool		fNeedUpdate;
		int32		fNextID;
};

extern FT_Library gFreeTypeLibrary;
extern FontManager *gFontManager;

#endif	/* FONT_MANAGER_H */
