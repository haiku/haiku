//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		FontServer.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Handles the largest part of the font subsystem
//  
//------------------------------------------------------------------------------
#ifndef FONTSERVER_H_
#define FONTSERVER_H_

#include <OS.h>
#include <List.h>
#include <SupportDefs.h>
#include <Font.h>
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
class FontServer
{
public:
	FontServer(void);
	~FontServer(void);
	void Lock(void);
	void Unlock(void);
	
	/*!
		\brief Determines whether the font server has started up properly
		\return true if so, false if not.
	*/
	bool IsInitialized(void) { return init; }
	int32 CountFamilies(void);
	int32 CountStyles(const char *family);
	void RemoveFamily(const char *family);
	status_t ScanDirectory(const char *path);
	void SaveList(void);
	FontStyle *GetStyle(font_family family, font_style face);
	ServerFont *GetSystemPlain(void);
	ServerFont *GetSystemBold(void);
	ServerFont *GetSystemFixed(void);
	bool SetSystemPlain(const char *family, const char *style, float size);
	bool SetSystemBold(const char *family, const char *style, float size);
	bool SetSystemFixed(const char *family, const char *style, float size);
	bool FontsNeedUpdated(void) { return need_update; }
	/*!
		\brief Called when the fonts list has been updated
	*/
	void FontsUpdated(void) { need_update=false; }
protected:
	FontFamily *_FindFamily(const char *name);
	FT_CharMap _GetSupportedCharmap(const FT_Face &face);
	bool init;
	sem_id lock;
	BList *families;
	ServerFont *plain, *bold, *fixed;
	bool need_update;
};

extern FTC_Manager ftmanager; 
extern FT_Library ftlib;
extern FontServer *fontserver;

#endif
