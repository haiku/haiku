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

		virtual void MessageReceived(BMessage* message);

		int32 CountFamilies() const;
		int32 CountStyles(const char *family) const;
		FontFamily* FamilyAt(int32 index) const;

		FontFamily *GetFamily(uint16 familyID) const;
		FontFamily *GetFamily(const char *name) const;

		FontStyle *GetStyleByIndex(const char *family, int32 index) const;
		FontStyle *GetStyle(const char *family, const char *style, uint16 familyID = 0xffff,
						uint16 styleID = 0xffff, uint16 face = 0);
		FontStyle *GetStyle(const char *family, uint16 styleID);
		FontStyle *GetStyle(uint16 familyID, uint16 styleID);
		FontStyle* FindStyleMatchingFace(uint16 face) const;

		ServerFont* GetFont(const char *family, const char *style, float size);
		ServerFont* GetFont(uint16 face, float size);
		const ServerFont* DefaultFont() const;

		bool FontsNeedUpdated() { return fNeedUpdate; }
		/*!
			\brief Called when the fonts list has been updated
		*/
		void FontsUpdated() { fNeedUpdate = false; }

		void AttachUser(uid_t userID);
		void DetachUser(uid_t userID);

	private:
		void _SetDefaultFont();
		void _ScanSystemFonts();
		status_t _AddPath(const char* path);
		status_t _AddPath(BEntry& entry);

		void _RemoveFamily(const char *family);

		status_t _ScanDirectory(BEntry &entry);
		void _AddFont(BPath& path);

		FT_CharMap _GetSupportedCharmap(const FT_Face &face);

	private:
		status_t	fInitStatus;
		BObjectList<node_ref> fFontDirectories;
		BObjectList<FontFamily> fFamilies;
		ServerFont	*fDefaultFont;
		bool		fNeedUpdate;
		int32		fNextID;
};

extern FT_Library gFreeTypeLibrary;
extern FontManager *gFontManager;

#endif	/* FONT_MANAGER_H */
