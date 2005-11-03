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
#include <Path.h>
#include <String.h>

#include <new>


static FTC_Manager ftmanager;
FT_Library gFreeTypeLibrary;
FontManager *gFontManager = NULL;

//#define PRINT_FONT_LIST


#if 0
/*!
	\brief Access function to request a face via the FreeType font cache
*/
static FT_Error
face_requester(FTC_FaceID face_id, FT_Library library,
	FT_Pointer request_data, FT_Face *aface)
{ 
	CachedFace face = (CachedFace) face_id;
	return FT_New_Face(gFreeTypeLibrary, face->file_path.String(), face->face_index,aface); 
} 
#endif


//	#pragma mark -


//! Does basic set up so that directories can be scanned
FontManager::FontManager()
	: BLooper("Font Manager"),
	fFamilies(20),
	fDefaultFont(NULL),
	fNextID(0)
{
	fInitStatus = FT_Init_FreeType(&gFreeTypeLibrary) == 0 ? B_OK : B_ERROR;

	if (fInitStatus == B_OK) {
		_ScanSystemFonts();
		
		if (CountFamilies() != 0)
			_SetDefaultFont();
		else
			fInitStatus = B_ENTRY_NOT_FOUND;
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
}


void
FontManager::MessageReceived(BMessage* message)
{
	// nothing to do here yet
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
		fFamilies.RemoveItem(family);
		delete family;
	}
}


/*!
	\brief Sets the font that will be used when you create an empty
		ServerFont without specifying a style.
*/
void
FontManager::_SetDefaultFont()
{
	FontStyle* style = GetStyle(DEFAULT_PLAIN_FONT_FAMILY, DEFAULT_PLAIN_FONT_STYLE);
	if (style == NULL) {
		style = FindStyleMatchingFace(B_REGULAR_FACE);
		if (style == NULL)
			style = FamilyAt(0)->StyleAt(0);
	}

	fDefaultFont = new ServerFont(*style, DEFAULT_PLAIN_FONT_SIZE);
}


//! Scans the four default system font folders
void
FontManager::_ScanSystemFonts()
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


/*!
	\brief Adds the FontFamily/FontStyle that is represented by this path.
*/
void
FontManager::_AddFont(BPath &path)
{
	FT_Face face;
	FT_Error error = FT_New_Face(gFreeTypeLibrary, path.Path(), 0, &face);
	if (error != 0)
		return;

    FontFamily *family = GetFamily(face->family_name);
	if (family != NULL && family->HasStyle(face->style_name)) {
		// prevent adding the same style twice
		// (this indicates a problem with the installed fonts maybe?)
		FT_Done_Face(face);
		return;
	}

	if (family == NULL) {
		family = new (nothrow) FontFamily(face->family_name, fNextID++);
		if (family == NULL
			|| !fFamilies.AddItem(family)) {
			delete family;
			FT_Done_Face(face);
			return;
		}
	}

#ifdef PRINT_FONT_LIST
	printf("\tFont Style: %s, %s\n", face->family_name, face->style_name);
#endif

	// the FontStyle takes over ownership of the FT_Face object
    FontStyle *style = new FontStyle(path.Path(), face);
	if (!family->AddStyle(style))
		delete style;
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
FontManager::_AddPath(BEntry& entry)
{
	status_t status = _ScanDirectory(entry);
	if (status != B_OK)
		return status;

	node_ref nodeRef;
	if (entry.GetNodeRef(&nodeRef) != B_OK)
		return status;

	// TODO: don't add it twice!
	// TODO: monitor this directory!
	return B_OK;
}


/*!
	\brief Scan a folder for all valid fonts
	\param directoryPath Path of the folder to scan.
*/
status_t
FontManager::_ScanDirectory(BEntry& entry)
{
	// This bad boy does all the real work. It loads each entry in the
	// directory. If a valid font file, it adds both the family and the style.
	// Both family and style are stored internally as BStrings. Once everything

	BDirectory dir;
	status_t status = dir.SetTo(&entry);
	if (status != B_OK)
		return status;

	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		status = entry.GetPath(&path);
		if (status < B_OK)
			continue;

		if (entry.IsDirectory()) {
			// scan this directory recursively
			_AddPath(entry);
			continue;
		}

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

	fNeedUpdate = true;
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


/*!
	\brief Counts the number of font families available
	\return The number of unique font families currently available
*/
int32
FontManager::CountFamilies() const
{
	return fFamilies.CountItems();
}


/*!
	\brief Counts the number of styles available in a font family
	\param family Name of the font family to scan
	\return The number of font styles currently available for the font family
*/
int32
FontManager::CountStyles(const char *familyName) const
{
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


/*!
	\brief Locates a FontFamily object by name
	\param name The family to find
	\return Pointer to the specified family or NULL if not found.
*/
FontFamily*
FontManager::GetFamily(const char* name) const
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


FontFamily*
FontManager::GetFamily(uint16 familyID) const
{
	for (int32 i = 0; i < fFamilies.CountItems(); i++) {
		FontFamily *family = (FontFamily*)fFamilies.ItemAt(i);
		if (family->ID() == familyID)
			return family;
	}

	return NULL;
}


FontStyle*
FontManager::GetStyleByIndex(const char* familyName, int32 index) const
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
FontManager::GetStyle(uint16 familyID, uint16 styleID)
{
	FontFamily *family = GetFamily(familyID);
	if (family)
		return family->GetStyleByID(styleID);

	return NULL;
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
FontManager::DefaultFont() const
{
	return fDefaultFont;
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

