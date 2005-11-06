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


#include <Looper.h>
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
class FontManager : public BLooper {
	public:
		FontManager();
		virtual ~FontManager();

		status_t InitCheck() { return fInitStatus; }
		void SaveRecentFontMappings();

		virtual void MessageReceived(BMessage* message);

		int32 CheckUpdate(uid_t user);
		int32 CountFamilies();

		int32 CountStyles(const char *family);
		FontFamily* FamilyAt(int32 index) const;

		FontFamily *GetFamily(uint16 familyID) const;
		FontFamily *GetFamily(const char *name);

		FontStyle *GetStyleByIndex(const char *family, int32 index);
		FontStyle *GetStyle(const char *family, const char *style, uint16 familyID = 0xffff,
						uint16 styleID = 0xffff, uint16 face = 0);
		FontStyle *GetStyle(const char *family, uint16 styleID);
		FontStyle *GetStyle(uint16 familyID, uint16 styleID) const;
		FontStyle* FindStyleMatchingFace(uint16 face) const;

		const ServerFont* DefaultFont() const;

		void AttachUser(uid_t userID);
		void DetachUser(uid_t userID);

	private:
		struct font_directory;
		struct font_mapping;

		bool _LoadRecentFontMappings();
		status_t _SetDefaultFont();
		void _AddSystemPaths();
		status_t _AddPath(const char* path);
		status_t _AddPath(BEntry& entry, font_directory** _newDirectory = NULL);

		FontFamily* _FindFamily(const char* family) const;
		void _RemoveFamily(const char* family);

		void _ScanFontsIfNecessary();
		void _ScanFonts();
		status_t _ScanFontDirectory(font_directory& directory);
		status_t _AddFont(BPath& path);

		FT_CharMap _GetSupportedCharmap(const FT_Face &face);

	private:
		status_t	fInitStatus;
		BObjectList<font_directory> fDirectories;
		BObjectList<font_mapping> fMappings;
		BObjectList<FontFamily> fFamilies;
		ServerFont	*fDefaultFont;
		bool		fScanned;
		int32		fNextID;
};

extern FT_Library gFreeTypeLibrary;
extern FontManager *gFontManager;

#endif	/* FONT_MANAGER_H */
