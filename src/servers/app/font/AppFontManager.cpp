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

#include <Debug.h>
#include <Entry.h>

#include "FontFamily.h"


//#define TRACE_FONT_MANAGER
#ifdef TRACE_FONT_MANAGER
#	define FTRACE(x) debug_printf x
#else
#	define FTRACE(x) ;
#endif


extern FT_Library gFreeTypeLibrary;


//	#pragma mark -


/*! Sets high id number to avoid collisions with GlobalFontManager
	The result of a collision would be that the global font is selected
	rather than the application font.
*/
AppFontManager::AppFontManager()
	: BLocker("AppFontManager")
{
	fNextID = UINT16_MAX;
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
		return B_ERROR;

	status = _AddFont(face, nodeRef, path, familyID, styleID);

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
		return B_ERROR;

	status = _AddFont(face, nodeRef, "", familyID, styleID);

	return status;
}


/*!	\brief Removes the FontFamily/FontStyle from the font manager.
*/
status_t
AppFontManager::RemoveUserFont(uint16 familyID, uint16 styleID)
{
	return _RemoveFont(familyID, styleID) != NULL ? B_OK : B_BAD_VALUE;
}


uint16
AppFontManager::_NextID()
{
	return fNextID--;
}
