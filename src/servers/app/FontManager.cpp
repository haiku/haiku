/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Manages font families and styles */


#include <FontFamily.h>
#include <FontManager.h>
#include <ServerConfig.h>
#include <ServerFont.h>

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>

#include <new>

//#define PRINT_FONT_LIST


static FTC_Manager ftmanager;
FT_Library gFreeTypeLibrary;
FontManager *gFontManager = NULL;

struct FontManager::font_directory {
	node_ref	directory;
	uid_t		user;
	gid_t		group;
	uint32		revision;
	BObjectList<FontStyle> styles;

	bool AlreadyScanned() const { return revision != 0; }
};

struct FontManager::font_mapping {
	BString	family;
	BString	style;
	BPath	path;
};


//	#pragma mark -


//! Does basic set up so that directories can be scanned
FontManager::FontManager()
	: BLooper("Font Manager"),
	fFamilies(20),
	fScanned(false),
	fNextID(0)
{
	fInitStatus = FT_Init_FreeType(&gFreeTypeLibrary) == 0 ? B_OK : B_ERROR;

	if (fInitStatus == B_OK) {
		_AddSystemPaths();
		_LoadRecentFontMappings();

		fInitStatus = _SetDefaultFonts();
	}

/*
	Fire up the font caching subsystem.
	The three zeros tell FreeType to use the defaults, which are 2 faces,
	4 face sizes, and a maximum of 200000 bytes. I will probably change
	these numbers in the future to maximize performance for your "average"
	application.
*/
//	if (FTC_Manager_New(gFreeTypeLibrary, 0, 0, 0, &face_requester, NULL, &ftmanager) != 0)
//		fInit = false;
}


//! Frees items allocated in the constructor and shuts down FreeType
FontManager::~FontManager()
{
	FTC_Manager_Done(ftmanager);
	FT_Done_FreeType(gFreeTypeLibrary);

	// we need to make sure the hash table doesn't delete the font styles;
	// that's already done by the lists
	fStyleHashTable.MakeEmpty(false);
}


void
FontManager::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
			// TODO: monitor fonts and update font_directories
			break;
	}
}


void
FontManager::SaveRecentFontMappings()
{
}


bool
FontManager::_LoadRecentFontMappings()
{
	return false;
}


/*!
	\brief Removes a font family from the font list
	\param family The family to remove
*/
void
FontManager::_RemoveFamily(const char *familyName)
{
	FontFamily *family = GetFamily(familyName);
	if (family) {
		// remove styles from hash
		for (int32 i = 0; i < family->CountStyles(); i++) {
			FontStyle* style = family->StyleAt(i);

			fStyleHashTable.RemoveItem(*style);
		}

		fFamilies.RemoveItem(family);
		delete family;
	}
}


FontStyle*
FontManager::_GetDefaultStyle(const char *familyName, const char *styleName,
	const char *fallbackFamily, const char *fallbackStyle,
	uint16 fallbackFace)
{
	// try to find a matching font

	FontStyle* style = GetStyle(familyName, styleName);
	if (style == NULL) {
		style = GetStyle(fallbackFamily, fallbackStyle);
		if (style == NULL) {
			style = FindStyleMatchingFace(fallbackFace);
			if (style == NULL && FamilyAt(0) != NULL)
				style = FamilyAt(0)->StyleAt(0);
		}
	}

	return style;
}


/*!
	\brief Sets the fonts that will be used when you create an empty
		ServerFont without specifying a style, as well as the default
		Desktop fonts if there are no settings available.
*/
status_t
FontManager::_SetDefaultFonts()
{
	FontStyle* style = _GetDefaultStyle(DEFAULT_PLAIN_FONT_FAMILY,
		DEFAULT_PLAIN_FONT_STYLE, FALLBACK_PLAIN_FONT_FAMILY,
		DEFAULT_PLAIN_FONT_STYLE,
		B_REGULAR_FACE);
	if (style == NULL)
		return B_ERROR;

	fDefaultPlainFont = new (nothrow) ServerFont(*style, DEFAULT_PLAIN_FONT_SIZE);
	if (fDefaultPlainFont == NULL)
		return B_NO_MEMORY;

	style = _GetDefaultStyle(DEFAULT_BOLD_FONT_FAMILY, DEFAULT_BOLD_FONT_STYLE,
		FALLBACK_BOLD_FONT_FAMILY, DEFAULT_BOLD_FONT_STYLE, B_BOLD_FACE);

	fDefaultBoldFont = new (nothrow) ServerFont(*style, DEFAULT_BOLD_FONT_SIZE);
	if (fDefaultBoldFont == NULL)
		return B_NO_MEMORY;

	style = _GetDefaultStyle(DEFAULT_FIXED_FONT_FAMILY, DEFAULT_FIXED_FONT_STYLE,
		FALLBACK_FIXED_FONT_FAMILY, DEFAULT_FIXED_FONT_STYLE, B_REGULAR_FACE);

	fDefaultFixedFont = new (nothrow) ServerFont(*style, DEFAULT_FIXED_FONT_SIZE);
	if (fDefaultFixedFont == NULL)
		return B_NO_MEMORY;

	fDefaultFixedFont->SetSpacing(B_FIXED_SPACING);
	return B_OK;
}


void
FontManager::_AddSystemPaths()
{
	BPath path;
	if (find_directory(B_BEOS_FONTS_DIRECTORY, &path) == B_OK)
		_AddPath(path.Path());

	// We don't scan these in test mode to help shave off some startup time
#if !TEST_MODE
	if (find_directory(B_COMMON_FONTS_DIRECTORY, &path) == B_OK)
		_AddPath(path.Path());
#endif
}


void
FontManager::_ScanFontsIfNecessary()
{
	if (!fScanned)
		_ScanFonts();
}


//! Scans all currently known font directories
void
FontManager::_ScanFonts()
{
	if (fScanned)
		return;

	for (int32 i = fDirectories.CountItems(); i-- > 0;) {
		font_directory* directory = fDirectories.ItemAt(i);

		if (directory->AlreadyScanned())
			continue;

		_ScanFontDirectory(*directory);
	}

	fScanned = true;
}


/*!
	\brief Adds the FontFamily/FontStyle that is represented by this path.
*/
status_t
FontManager::_AddFont(BPath &path)
{
	FT_Face face;
	FT_Error error = FT_New_Face(gFreeTypeLibrary, path.Path(), 0, &face);
	if (error != 0)
		return B_ERROR;

    FontFamily *family = _FindFamily(face->family_name);
	if (family != NULL && family->HasStyle(face->style_name)) {
		// prevent adding the same style twice
		// (this indicates a problem with the installed fonts maybe?)
		FT_Done_Face(face);
		return B_OK;
	}

	if (family == NULL) {
		family = new (nothrow) FontFamily(face->family_name, fNextID++);
		if (family == NULL
			|| !fFamilies.AddItem(family)) {
			delete family;
			FT_Done_Face(face);
			return B_NO_MEMORY;
		}
	}

#ifdef PRINT_FONT_LIST
	printf("\tFont Style: %s, %s\n", face->family_name, face->style_name);
#endif

	// the FontStyle takes over ownership of the FT_Face object
    FontStyle *style = new FontStyle(path.Path(), face);
	if (!family->AddStyle(style))
		delete style;

	fStyleHashTable.AddItem(style);
	return B_OK;
}


status_t
FontManager::_AddPath(const char* path)
{
	BEntry entry;
	status_t status = entry.SetTo(path);
	if (status != B_OK)
		return status;

	return _AddPath(entry);
}


status_t
FontManager::_AddPath(BEntry& entry, font_directory** _newDirectory)
{
	node_ref nodeRef;
	status_t status = entry.GetNodeRef(&nodeRef);
	if (status != B_OK)
		return status;

	// check if we are already know this directory

	for (int32 i = fDirectories.CountItems(); i-- > 0;) {
		font_directory* directory = fDirectories.ItemAt(i);

		if (directory->directory == nodeRef) {
			if (_newDirectory)
				*_newDirectory = NULL;
			return B_OK;
		}
	}

	// it's a new one, so let's add it

	font_directory* directory = new (nothrow) font_directory;
	if (directory == NULL)
		return B_NO_MEMORY;

	struct stat stat;
	status = entry.GetStat(&stat);
	if (status != B_OK) {
		delete directory;
		return status;
	}

	directory->directory = nodeRef;
	directory->user = stat.st_uid;
	directory->group = stat.st_gid;
	directory->revision = 0;

	status = watch_node(&nodeRef, B_WATCH_DIRECTORY, this);
	if (status != B_OK) {
		// we cannot watch this directory - while this is unfortunate,
		// it's not a critical error
		printf("could not watch directory %ld:%Ld\n", nodeRef.device, nodeRef.node);
			// TODO: should go into syslog()
	}

	fDirectories.AddItem(directory);

	if (_newDirectory)
		*_newDirectory = directory;

	fScanned = false;
	return B_OK;
}


/*!
	\brief Scan a folder for all valid fonts
	\param directoryPath Path of the folder to scan.
*/
status_t
FontManager::_ScanFontDirectory(font_directory& fontDirectory)
{
	// This bad boy does all the real work. It loads each entry in the
	// directory. If a valid font file, it adds both the family and the style.

	BDirectory directory;
	status_t status = directory.SetTo(&fontDirectory.directory);
	if (status != B_OK)
		return status;

	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		if (entry.IsDirectory()) {
			// scan this directory recursively
			font_directory* newDirectory;
			if (_AddPath(entry, &newDirectory) == B_OK && newDirectory != NULL)
				_ScanFontDirectory(*newDirectory);

			continue;
		}

		BPath path;
		status = entry.GetPath(&path);
		if (status < B_OK)
			continue;

// TODO: Commenting this out makes my "Unicode glyph lookup"
// work with our default fonts. The real fix is to select the
// Unicode char map (if supported), and/or adjust the
// utf8 -> glyph-index mapping everywhere to handle other
// char maps. We could also ignore fonts that don't support
// the Unicode lookup as a temporary "solution".
#if 0
		FT_CharMap charmap = _GetSupportedCharmap(face);
		if (!charmap) {
		    FT_Done_Face(face);
		    continue;
    	}

		face->charmap = charmap;
#endif

		_AddFont(path);
			// takes over ownership of the FT_Face object
	}

	fontDirectory.revision = 1;
	return B_OK;
}


/*!
	\brief Finds and returns the first valid charmap in a font
	
	\param face Font handle obtained from FT_Load_Face()
	\return An FT_CharMap or NULL if unsuccessful
*/
FT_CharMap
FontManager::_GetSupportedCharmap(const FT_Face& face)
{
	for (int32 i = 0; i < face->num_charmaps; i++) {
		FT_CharMap charmap = face->charmaps[i];

		switch (charmap->platform_id) {
			case 3:
				// if Windows Symbol or Windows Unicode
				if (charmap->encoding_id == 0 || charmap->encoding_id == 1)
					return charmap;
				break;

			case 1:
				// if Apple Unicode
				if (charmap->encoding_id == 0)
					return charmap;
				break;

			case 0:
				// if Apple Roman
				if (charmap->encoding_id == 0)
					return charmap;
				break;

			default:
				break;
		}
	}

	return NULL;
}


int32
FontManager::CheckRevision(uid_t user)
{
	BAutolock locker(this);
	int32 revision = 0;

	_ScanFontsIfNecessary();

	for (int32 i = 0; i < fDirectories.CountItems(); i++) {
		font_directory* directory = fDirectories.ItemAt(i);

		// TODO: for now, add all directories
		revision += directory->revision;
	}

	return revision;
}


/*!
	\brief Counts the number of font families available
	\return The number of unique font families currently available
*/
int32
FontManager::CountFamilies()
{
	_ScanFontsIfNecessary();

	return fFamilies.CountItems();
}


/*!
	\brief Counts the number of styles available in a font family
	\param family Name of the font family to scan
	\return The number of font styles currently available for the font family
*/
int32
FontManager::CountStyles(const char *familyName)
{
	_ScanFontsIfNecessary();

	FontFamily *family = GetFamily(familyName);
	if (family)
		return family->CountStyles();

	return 0;
}


FontFamily*
FontManager::FamilyAt(int32 index) const
{
	return fFamilies.ItemAt(index);
}


FontFamily*
FontManager::_FindFamily(const char* name) const
{
	if (name == NULL)
		return NULL;

	int32 count = fFamilies.CountItems();

	for (int32 i = 0; i < count; i++) {
		FontFamily* family = fFamilies.ItemAt(i);
		if (!strcmp(family->Name(), name))
			return family;
	}

	return NULL;
}


/*!
	\brief Locates a FontFamily object by name
	\param name The family to find
	\return Pointer to the specified family or NULL if not found.
*/
FontFamily*
FontManager::GetFamily(const char* name)
{
	if (name == NULL)
		return NULL;

	FontFamily* family = _FindFamily(name);
	if (family != NULL)
		return family;

	if (fScanned)
		return NULL;

	// try again
	family = _FindFamily(name);
	if (family != NULL)
		return family;

	// try font mappings before failing
	for (int32 i = 0; i < fMappings.CountItems(); i++) {
		font_mapping* mapping = fMappings.ItemAt(i);

		if (mapping->family == name) {
			if (_AddFont(mapping->path) == B_OK)
				return _FindFamily(name);
		}
	}

	_ScanFonts();
	return _FindFamily(name);
}


FontFamily*
FontManager::GetFamily(uint16 familyID) const
{
	FontKey key(familyID, 0);
	FontStyle* style = (FontStyle*)fStyleHashTable.GetValue(key);
	if (style != NULL)
		return style->Family();

	return NULL;
}


FontStyle*
FontManager::GetStyleByIndex(const char* familyName, int32 index)
{
	FontFamily* family = GetFamily(familyName);
	if (family != NULL)
		return family->StyleAt(index);

	return NULL;
}


/*!
	\brief Retrieves the FontStyle object that comes closest to the one specified

	\param family The font's family or NULL in which case \a familyID is used
	\param style The font's style or NULL in which case \a styleID is used
	\param familyID will only be used if \a family is NULL (or empty)
	\param styleID will only be used if \a style is NULL (or empty)
	\param face is used to specify the style if both \a style is NULL or empty
		and styleID is 0xffff.

	\return The FontStyle having those attributes or NULL if not available
*/
FontStyle*
FontManager::GetStyle(const char* familyName, const char* styleName, uint16 familyID,
	uint16 styleID, uint16 face)
{
	FontFamily* family;

	// find family

	if (familyName != NULL && familyName[0])
		family = GetFamily(familyName);
	else
		family = GetFamily(familyID);

	if (family == NULL)
		return NULL;

	// find style

	if (styleName != NULL && styleName[0])
		return family->GetStyle(styleName);

	if (styleID != 0xffff)
		return family->GetStyleByID(styleID);

	// try to get from face
	return family->GetStyleMatchingFace(face);
}


/*!
	\brief Retrieves the FontStyle object
	\param family ID for the font's family
	\param style ID of the font's style
	\return The FontStyle having those attributes or NULL if not available
*/
FontStyle*
FontManager::GetStyle(uint16 familyID, uint16 styleID) const
{
	FontKey key(familyID, styleID);
	return (FontStyle*)fStyleHashTable.GetValue(key);
}


/*!
	\brief If you don't find your preferred font style, but are anxious
		to have one fitting your needs, you may want to use this method.
*/
FontStyle*
FontManager::FindStyleMatchingFace(uint16 face) const
{
	int32 count = fFamilies.CountItems();

	for (int32 i = 0; i < count; i++) {
		FontFamily* family = fFamilies.ItemAt(i);
		FontStyle* style = family->GetStyleMatchingFace(face);
		if (style != NULL)
			return style;
	}

	return NULL;
}


const ServerFont*
FontManager::DefaultPlainFont() const
{
	return fDefaultPlainFont;
}


const ServerFont*
FontManager::DefaultBoldFont() const
{
	return fDefaultBoldFont;
}


const ServerFont*
FontManager::DefaultFixedFont() const
{
	return fDefaultFixedFont;
}


void
FontManager::AttachUser(uid_t userID)
{
	BAutolock locker(this);

/*
	BPath path;
	status_t status = find_directory(B_USER_FONTS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	_AddPath(path.Path());
*/
}


void
FontManager::DetachUser(uid_t userID)
{
	BAutolock locker(this);
}

