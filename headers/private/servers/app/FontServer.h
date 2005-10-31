/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef FONTSERVER_H_
#define FONTSERVER_H_

#include <OS.h>
#include <List.h>
#include <SupportDefs.h>
#include <Font.h>
#include <Locker.h>

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

	/*!
		\brief Determines whether the font server has started up properly
		\return true if so, false if not.
	*/
	bool IsInitialized() { return fInit; }
	int32 CountFamilies();
	int32 CountStyles(const char *family);
	void RemoveFamily(const char *family);
	void ScanSystemFolders();
	status_t ScanDirectory(const char *path);
	void SaveList();

	const char *GetFamilyName(uint16 id) const;
	const char *GetStyleName(const char *family, uint16 id) const;

	FontStyle *GetStyle(const char *family, const char *style, uint16 face = 0);
	FontStyle *GetStyle(const char *family, uint16 id) const;
	FontStyle *GetStyle(uint16 familyID, uint16 styleID);
	FontFamily *GetFamily(uint16 familyID) const;
	FontFamily *GetFamily(const char *name) const;

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
	bool		fInit;
	BList		fFamilies;
	ServerFont	*fPlain, *fBold, *fFixed;
	bool		fNeedUpdate;
};

extern FTC_Manager ftmanager; 
extern FT_Library ftlib;
extern FontServer *gFontServer;

#endif	/* FONTSERVER_H_ */
