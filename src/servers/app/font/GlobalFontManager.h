/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef GLOBAL_FONT_MANAGER_H
#define GLOBAL_FONT_MANAGER_H


#include "FontManager.h"

#include <AutoDeleter.h>
#include <Looper.h>
#include <ObjectList.h>


class BEntry;
class BPath;
struct node_ref;


class FontFamily;
class FontStyle;
class ServerFont;


/*!
	\class GlobalFontManager GlobalFontManager.h
	\brief Manager for system fonts within the font subsystem
*/
class GlobalFontManager : public FontManager, public BLooper {
public:
								GlobalFontManager();
	virtual						~GlobalFontManager();

			bool				Lock() { return BLooper::Lock(); }
			void				Unlock() { BLooper::Unlock(); }
			bool				IsLocked() const { return BLooper::IsLocked(); }

			status_t			InitCheck() { return fInitStatus; }

			void				SaveRecentFontMappings();

	virtual	void				MessageReceived(BMessage* message);

	virtual	int32				CountFamilies();

	virtual	int32				CountStyles(const char* family);
	virtual	int32				CountStyles(uint16 familyID);

			const ServerFont*	DefaultPlainFont() const;
			const ServerFont*	DefaultBoldFont() const;
			const ServerFont*	DefaultFixedFont() const;

	virtual	FontFamily*			GetFamily(uint16 familyID) const;
	virtual	FontFamily*			GetFamily(const char* name);

	virtual	FontStyle*			GetStyle(uint16 familyID, uint16 styleID) const;
	virtual	FontStyle*			GetStyle(const char* familyName,
									const char* styleName,
									uint16 familyID = 0xffff,
									uint16 styleID = 0xffff,
									uint16 face = 0);

	virtual	uint32				Revision();

private:
			struct font_directory;
			struct font_mapping;

			void				_AddDefaultMapping(const char* family,
									const char* style, const char* path);
			bool				_LoadRecentFontMappings();
			status_t			_AddMappedFont(const char* family,
									const char* style = NULL);
			void				_PrecacheFontFile(const ServerFont* font);
			void				_AddSystemPaths();
			void				_AddUserPaths();
			font_directory*		_FindDirectory(node_ref& nodeRef);
			void				_RemoveDirectory(font_directory* directory);
			status_t			_CreateDirectories(const char* path);
			status_t			_AddPath(const char* path);
			status_t			_AddPath(BEntry& entry,
									font_directory** _newDirectory = NULL);

			void				_ScanFontsIfNecessary();
			void				_ScanFonts();
			status_t			_ScanFontDirectory(font_directory& directory);
			status_t			_AddFont(font_directory& directory,
									BEntry& entry);
			void 				_RemoveStyle(font_directory& directory,
									FontStyle* style);
			void 				_RemoveStyle(dev_t device, uint64 directory,
									uint64 node);
			FontStyle*			_GetDefaultStyle(const char* familyName,
									const char* styleName,
									const char* fallbackFamily,
									const char* fallbackStyle,
									uint16 fallbackFace);
			status_t			_SetDefaultFonts();

private:
			status_t			fInitStatus;

			typedef BObjectList<font_directory>		DirectoryList;
			typedef BObjectList<font_mapping>		MappingList;

			DirectoryList		fDirectories;
			MappingList			fMappings;

			ObjectDeleter<ServerFont>
								fDefaultPlainFont;
			ObjectDeleter<ServerFont>
								fDefaultBoldFont;
			ObjectDeleter<ServerFont>
								fDefaultFixedFont;

			bool				fScanned;

};


extern GlobalFontManager* gFontManager;

#endif	/* GLOBAL_FONT_MANAGER_H */
