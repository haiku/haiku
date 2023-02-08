/*
 * Copyright 2001-2016, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


/*!	Manages user font families and styles */


#include "AppFontManager.h"

#include <new>
#include <stdint.h>
#include <syslog.h>

#include <Autolock.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>

#include "FontFamily.h"
#include "FontManager.h"
#include "ServerConfig.h"
#include "ServerFont.h"


//#define TRACE_FONT_MANAGER
#ifdef TRACE_FONT_MANAGER
#	define FTRACE(x) debug_printf x
#else
#	define FTRACE(x) ;
#endif


//	#pragma mark -


/*! Sets high id number to avoid collisions with GlobalFontManager
	The result of a collision would be that the global font is selected
	rather than the application font.
*/
AppFontManager::AppFontManager()
	: FontManagerBase(false, "AppFontManager")
{
	fNextID = UINT16_MAX;
}


status_t
AppFontManager::_AddUserFont(FT_Face face, node_ref nodeRef, const char* path,
	uint16& familyID, uint16& styleID)
{
	FontFamily* family = _FindFamily(face->family_name);
	if (family != NULL
		&& family->HasStyle(face->style_name)) {
		// prevent adding the same style twice
		// (this indicates a problem with the installed fonts maybe?)
		FT_Done_Face(face);
		return B_NAME_IN_USE;
	}

	if (family == NULL) {
		family = new (std::nothrow) FontFamily(face->family_name, fNextID--);

		if (family == NULL
			|| !fFamilies.BinaryInsert(family, compare_font_families)) {
			delete family;
			FT_Done_Face(face);
			return B_NO_MEMORY;
		}
	}

	FTRACE(("\tadd style: %s, %s\n", face->family_name, face->style_name));

	// the FontStyle takes over ownership of the FT_Face object
	FontStyle* style = new (std::nothrow) FontStyle(nodeRef, path, face);

	if (style == NULL || !family->AddStyle(style, this)) {
		delete style;
		delete family;
		return B_NO_MEMORY;
	}

	familyID = style->Family()->ID();
	styleID = style->ID();

	fStyleHashTable.Put(FontKey(familyID, styleID), style);
	style->ReleaseReference();

	return B_OK;
}


/*!	\brief Adds the FontFamily/FontStyle that is represented by this path.
*/
status_t
AppFontManager::AddUserFontFromFile(const char* path,
	uint16& familyID, uint16& styleID)
{
	ASSERT(IsLocked());

	BEntry entry;
	status_t status = entry.SetTo(path);
	if (status != B_OK)
		return status;

	node_ref nodeRef;
	status = entry.GetNodeRef(&nodeRef);
	if (status < B_OK)
		return status;

	FT_Face face;
	FT_Error error = FT_New_Face(gFreeTypeLibrary, path, 0, &face);
	if (error != 0)
		return error;

	status = _AddUserFont(face, nodeRef, path, familyID, styleID);

	return status;
}


/*!	\brief Adds the FontFamily/FontStyle that is represented by the area in memory.
*/
status_t
AppFontManager::AddUserFontFromMemory(const FT_Byte* fontAddress, size_t size,
	uint16& familyID, uint16& styleID)
{
	ASSERT(IsLocked());

	node_ref nodeRef;
	status_t status;

	FT_Face face;
	FT_Error error = FT_New_Memory_Face(gFreeTypeLibrary, fontAddress, size, 0,
		&face);
	if (error != 0)
		return error;

	status = _AddUserFont(face, nodeRef, "", familyID, styleID);

	return status;
}


/*!	\brief Removes the FontFamily/FontStyle from the font manager.
*/
status_t
AppFontManager::RemoveUserFont(uint16 familyID, uint16 styleID)
{
	ASSERT(IsLocked());

	FontKey fKey(familyID, styleID);
	FontStyle* styleRef = fStyleHashTable.Remove(fKey);

	return styleRef != NULL ? B_OK : B_BAD_VALUE;
}
