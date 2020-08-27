/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H


#include <AutoDeleter.h>
#include <HashMap.h>
#include <Looper.h>
#include <ObjectList.h>
#include <Referenceable.h>

#include <ft2build.h>
#include FT_FREETYPE_H

class BEntry;
class BPath;
struct node_ref;

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
	virtual						~FontManager();

			status_t			InitCheck() { return fInitStatus; }
			void				SaveRecentFontMappings();

	virtual	void				MessageReceived(BMessage* message);

			int32				CheckRevision(uid_t user);
			int32				CountFamilies();

			int32				CountStyles(const char* family);
			int32				CountStyles(uint16 familyID);
			FontFamily*			FamilyAt(int32 index) const;

			FontFamily*			GetFamily(uint16 familyID) const;
			FontFamily*			GetFamily(const char* name);

			FontStyle*			GetStyleByIndex(const char* family,
									int32 index);
			FontStyle*			GetStyleByIndex(uint16 familyID, int32 index);
			FontStyle*			GetStyle(const char* family, const char* style,
									uint16 familyID = 0xffff,
									uint16 styleID = 0xffff, uint16 face = 0);
			FontStyle*			GetStyle(const char *family, uint16 styleID);
			FontStyle*			GetStyle(uint16 familyID,
									uint16 styleID) const;
			FontStyle*			FindStyleMatchingFace(uint16 face) const;

			void				RemoveStyle(FontStyle* style);
				// This call must not be used by anything else than class
				// FontStyle.

			const ServerFont*	DefaultPlainFont() const;
			const ServerFont*	DefaultBoldFont() const;
			const ServerFont*	DefaultFixedFont() const;

			void				AttachUser(uid_t userID);
			void				DetachUser(uid_t userID);

private:
			struct font_directory;
			struct font_mapping;

			void				_AddDefaultMapping(const char* family,
									const char* style, const char* path);
			bool				_LoadRecentFontMappings();
			status_t			_AddMappedFont(const char* family,
									const char* style = NULL);
			FontStyle*			_GetDefaultStyle(const char* familyName,
									const char* styleName,
									const char* fallbackFamily,
									const char* fallbackStyle,
									uint16 fallbackFace);
			status_t			_SetDefaultFonts();
			void				_PrecacheFontFile(const ServerFont* font);
			void				_AddSystemPaths();
			font_directory*		_FindDirectory(node_ref& nodeRef);
			void				_RemoveDirectory(font_directory* directory);
			status_t			_CreateDirectories(const char* path);
			status_t			_AddPath(const char* path);
			status_t			_AddPath(BEntry& entry,
									font_directory** _newDirectory = NULL);

			void				_RemoveStyle(font_directory& directory,
									FontStyle* style);
			void				_RemoveStyle(dev_t device, uint64 directory,
									uint64 node);
			FontFamily*			_FindFamily(const char* family) const;

			void				_ScanFontsIfNecessary();
			void				_ScanFonts();
			status_t			_ScanFontDirectory(font_directory& directory);
			status_t			_AddFont(font_directory& directory,
									BEntry& entry);

			FT_CharMap			_GetSupportedCharmap(const FT_Face& face);

private:
			struct FontKey {
				FontKey(uint16 family, uint16 style)
					: familyID(family), styleID(style) {}

				uint32 GetHashCode() const
				{
					return familyID | (styleID << 16UL);
				}

				bool operator==(const FontKey& other) const
				{
					return familyID == other.familyID
						&& styleID == other.styleID;
				}

				uint16 familyID, styleID;
			};

private:
			status_t			fInitStatus;

			typedef BObjectList<font_directory>		DirectoryList;
			typedef BObjectList<font_mapping>		MappingList;
			typedef BObjectList<FontFamily>			FamilyList;

			DirectoryList		fDirectories;
			MappingList			fMappings;
			FamilyList			fFamilies;

			HashMap<FontKey, BReference<FontStyle> > fStyleHashTable;

			ObjectDeleter<ServerFont>
								fDefaultPlainFont;
			ObjectDeleter<ServerFont>
								fDefaultBoldFont;
			ObjectDeleter<ServerFont>
								fDefaultFixedFont;

			bool				fScanned;
			int32				fNextID;
};

extern FT_Library gFreeTypeLibrary;
extern FontManager* gFontManager;

#endif	/* FONT_MANAGER_H */
