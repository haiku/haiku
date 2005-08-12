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
	FontServer(void);
	~FontServer(void);

	/*!
		\brief Determines whether the font server has started up properly
		\return true if so, false if not.
	*/
	bool IsInitialized(void) { return fInit; }
	int32 CountFamilies(void);
	int32 CountStyles(const char *family);
	void RemoveFamily(const char *family);
	void ScanSystemFolders(void);
	status_t ScanDirectory(const char *path);
	void SaveList(void);

	const char *GetFamilyName(uint16 id) const;
	const char *GetStyleName(const char *family, uint16 id) const;

	FontStyle *GetStyle(const char *family, const char *face);
	FontStyle *GetStyle(const char *family, uint16 id) const;
	FontStyle *GetStyle(const uint16 &familyid, const uint16 &styleid);
	FontFamily *GetFamily(const uint16 &familyid) const;
	FontFamily *GetFamily(const char *name) const;

	ServerFont *GetSystemPlain(void);
	ServerFont *GetSystemBold(void);
	ServerFont *GetSystemFixed(void);

	bool SetSystemPlain(const char *family, const char *style, float size);
	bool SetSystemBold(const char *family, const char *style, float size);
	bool SetSystemFixed(const char *family, const char *style, float size);

	bool FontsNeedUpdated(void) { return fNeedUpdate; }
	/*!
		\brief Called when the fonts list has been updated
	*/
	void FontsUpdated(void) { fNeedUpdate = false; }

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
