/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef APP_FONT_MANAGER_H
#define APP_FONT_MANAGER_H


#include "FontManager.h"


#include <ft2build.h>
#include FT_FREETYPE_H

struct node_ref;


// font areas should be less than 20MB
#define MAX_FONT_DATA_SIZE_BYTES	20 * 1024 * 1024
#define MAX_USER_FONTS				128

/*!
	\class AppFontManager AppFontManager.h
	\brief Manager for application-added fonts in the font subsystem
*/
class AppFontManager : public FontManagerBase {
public:
								AppFontManager();

			status_t			AddUserFontFromFile(const char* path,
									uint16& familyID, uint16& styleID);
			status_t			AddUserFontFromMemory(const FT_Byte* fontAddress,
									size_t size, uint16& familyID, uint16& styleID);
			status_t			RemoveUserFont(uint16 familyID, uint16 styleID);

private:
			status_t			_AddUserFont(FT_Face face, node_ref nodeRef,
									const char* path,
									uint16& familyID, uint16& styleID);
private:

			int32				fNextID;
};


#endif	/* APP_FONT_MANAGER_H */
