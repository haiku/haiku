/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef FONT_SERVER_H
#define FONT_SERVER_H


#include <Font.h>
#include <Locker.h>
#include <OS.h>
#include <ObjectList.h>
#include <SupportDefs.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

class FontFamily;
class FontStyle;
class ServerFont;


/*!
	\class FontServer FontServer.h
	\brief Manager for the largest part of the font subsystem
*/
class FontServer : public BLocker {
	public:
		FontServer();
		~FontServer();

		status_t InitCheck() { return fInitStatus; }

		int32 CountFamilies();
		int32 CountStyles(const char *family);
		void RemoveFamily(const char *family);
		void ScanSystemFolders();
		status_t ScanDirectory(const char *path);
		void SaveList();

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

	protected:
		uint16 TranslateStyleToFace(const char *name) const;

		FT_CharMap _GetSupportedCharmap(const FT_Face &face);

	private:
		status_t	fInitStatus;
		BObjectList<FontFamily> fFamilies;
		ServerFont	*fPlain, *fBold, *fFixed;
		bool		fNeedUpdate;
};

extern FTC_Manager ftmanager; 
extern FT_Library ftlib;
extern FontServer *gFontServer;

#endif	/* FONTSERVER_H_ */
