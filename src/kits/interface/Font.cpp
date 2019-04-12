/*
 * Copyright 2001-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 */


#include <AppServerLink.h>
#include <FontPrivate.h>
#include <ObjectList.h>
#include <ServerProtocol.h>
#include <truncate_string.h>
#include <utf8_functions.h>

#include <Autolock.h>
#include <Font.h>
#include <Locker.h>
#include <Message.h>
#include <PortLink.h>
#include <Rect.h>
#include <Shape.h>
#include <String.h>

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


using namespace std;

const float kUninitializedAscent = INFINITY;
const uint32 kUninitializedExtraFlags = 0xffffffff;

// The actual objects which the globals point to
static BFont sPlainFont;
static BFont sBoldFont;
static BFont sFixedFont;

const BFont* be_plain_font = &sPlainFont;
const BFont* be_bold_font = &sBoldFont;
const BFont* be_fixed_font = &sFixedFont;


struct style {
	BString	name;
	uint16	face;
	uint32	flags;
};

struct family {
	BString	name;
	uint32	flags;
	BObjectList<style> styles;
};

namespace {

class FontList : public BLocker {
public:
								FontList();
	virtual						~FontList();

	static	FontList*			Default();

			bool				UpdatedOnServer();

			status_t			FamilyAt(int32 index, font_family* _family,
									uint32* _flags);
			status_t			StyleAt(font_family family, int32 index,
									font_style* _style, uint16* _face,
									uint32* _flags);

			int32				CountFamilies();
			int32				CountStyles(font_family family);

private:
			status_t			_UpdateIfNecessary();
			status_t			_Update();
			int32				_RevisionOnServer();
			family*				_FindFamily(font_family name);
	static	void				_InitSingleton();

private:
			BObjectList<family>	fFamilies;
			family*				fLastFamily;
			bigtime_t			fLastUpdate;
			int32				fRevision;

	static	pthread_once_t		sDefaultInitOnce;
	static	FontList*			sDefaultInstance;
};

pthread_once_t FontList::sDefaultInitOnce = PTHREAD_ONCE_INIT;
FontList* FontList::sDefaultInstance = NULL;

}	// unnamed namespace


//	#pragma mark -


static int
compare_families(const family* a, const family* b)
{
	// TODO: compare font names according to the user's locale settings
	return strcmp(a->name.String(), b->name.String());
}


namespace {

FontList::FontList()
	: BLocker("font list"),
	fLastFamily(NULL),
	fLastUpdate(0),
	fRevision(0)
{
}


FontList::~FontList()
{
}


/*static*/ FontList*
FontList::Default()
{
	if (sDefaultInstance == NULL)
		pthread_once(&sDefaultInitOnce, &_InitSingleton);

	return sDefaultInstance;
}


bool
FontList::UpdatedOnServer()
{
	return _RevisionOnServer() != fRevision;
}


status_t
FontList::FamilyAt(int32 index, font_family* _family, uint32* _flags)
{
	BAutolock locker(this);

	status_t status = _UpdateIfNecessary();
	if (status < B_OK)
		return status;

	::family* family = fFamilies.ItemAt(index);
	if (family == NULL)
		return B_BAD_VALUE;

	memcpy(*_family, family->name.String(), family->name.Length() + 1);
	if (_flags)
		*_flags = family->flags;
	return B_OK;
}


status_t
FontList::StyleAt(font_family familyName, int32 index, font_style* _style,
	uint16* _face, uint32* _flags)
{
	BAutolock locker(this);

	status_t status = _UpdateIfNecessary();
	if (status < B_OK)
		return status;

	::family* family = _FindFamily(familyName);
	if (family == NULL)
		return B_BAD_VALUE;

	::style* style = family->styles.ItemAt(index);
	if (style == NULL)
		return B_BAD_VALUE;

	memcpy(*_style, style->name.String(), style->name.Length() + 1);
	if (_face)
		*_face = style->face;
	if (_flags)
		*_flags = style->flags;
	return B_OK;
}


int32
FontList::CountFamilies()
{
	BAutolock locker(this);

	_UpdateIfNecessary();
	return fFamilies.CountItems();
}


int32
FontList::CountStyles(font_family familyName)
{
	BAutolock locker(this);

	_UpdateIfNecessary();

	::family* family = _FindFamily(familyName);
	if (family == NULL)
		return 0;

	return family->styles.CountItems();
}


status_t
FontList::_Update()
{
	// check version

	int32 revision = _RevisionOnServer();
	fLastUpdate = system_time();

	// are we up-to-date already?
	if (revision == fRevision)
		return B_OK;

	fFamilies.MakeEmpty();
	fLastFamily = NULL;

	BPrivate::AppServerLink link;

	for (int32 index = 0;; index++) {
		link.StartMessage(AS_GET_FAMILY_AND_STYLES);
		link.Attach<int32>(index);

		int32 status;
		if (link.FlushWithReply(status) != B_OK
			|| status != B_OK)
			break;

		::family* family = new (nothrow) ::family;
		if (family == NULL)
			return B_NO_MEMORY;

		link.ReadString(family->name);
		link.Read<uint32>(&family->flags);

		int32 styleCount;
		link.Read<int32>(&styleCount);

		for (int32 i = 0; i < styleCount; i++) {
			::style* style = new (nothrow) ::style;
			if (style == NULL) {
				delete family;
				return B_NO_MEMORY;
			}

			link.ReadString(style->name);
			link.Read<uint16>(&style->face);
			link.Read<uint32>(&style->flags);

			family->styles.AddItem(style);
		}

		fFamilies.BinaryInsert(family, compare_families);
	}

	fRevision = revision;

	// if the font list has been changed in the mean time, just update again
	if (UpdatedOnServer())
		_Update();

	return B_OK;
}


status_t
FontList::_UpdateIfNecessary()
{
	// an updated font list is at least valid for 1 second
	if (fLastUpdate > system_time() - 1000000)
		return B_OK;

	return _Update();
}


int32
FontList::_RevisionOnServer()
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_FONT_LIST_REVISION);

	int32 code;
	if (link.FlushWithReply(code) != B_OK || code != B_OK)
		return B_ERROR;

	int32 revision;
	link.Read<int32>(&revision);

	return revision;
}


family*
FontList::_FindFamily(font_family name)
{
	if (fLastFamily != NULL && fLastFamily->name == name)
		return fLastFamily;

	::family family;
	family.name = name;
	fLastFamily = const_cast< ::family*>(fFamilies.BinarySearch(family,
		compare_families));
	return fLastFamily;
}


/*static*/ void
FontList::_InitSingleton()
{
	sDefaultInstance = new FontList;
}

}	// unnamed namespace


//	#pragma mark -


void
_init_global_fonts_()
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_SYSTEM_FONTS);

	int32 code;
	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK) {
		printf("DEBUG: Couldn't initialize global fonts!\n");
		return;
	}

	char type[B_OS_NAME_LENGTH];

	while (link.ReadString(type, sizeof(type)) >= B_OK && type[0]) {
		BFont dummy;
		BFont* font = &dummy;

		if (!strcmp(type, "plain"))
			font = &sPlainFont;
		else if (!strcmp(type, "bold"))
			font = &sBoldFont;
		else if (!strcmp(type, "fixed"))
			font = &sFixedFont;

		link.Read<uint16>(&font->fFamilyID);
		link.Read<uint16>(&font->fStyleID);
		link.Read<float>(&font->fSize);
		link.Read<uint16>(&font->fFace);
		link.Read<uint32>(&font->fFlags);

		font->fHeight.ascent = kUninitializedAscent;
		font->fExtraFlags = kUninitializedExtraFlags;
	}
}


void
_font_control_(BFont* font, int32 cmd, void* data)
{
}


status_t
get_font_cache_info(uint32 id, void* set)
{
	return B_ERROR;
}


status_t
set_font_cache_info(uint32 id, void* set)
{
	return B_ERROR;
}


// Private function used to replace the R5 hack which sets a system font
void
_set_system_font_(const char* which, font_family family, font_style style,
	float size)
{
	// R5 used a global area offset table to set the system fonts in the Font
	// preferences panel. Bleah.
	BPrivate::AppServerLink link;

	link.StartMessage(AS_SET_SYSTEM_FONT);
	link.AttachString(which, B_OS_NAME_LENGTH);
	link.AttachString(family, sizeof(font_family));
	link.AttachString(style, sizeof(font_style));
	link.Attach<float>(size);
	link.Flush();
}


status_t
_get_system_default_font_(const char* which, font_family family,
	font_style style, float* _size)
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_SYSTEM_DEFAULT_FONT);
	link.AttachString(which, B_OS_NAME_LENGTH);

	int32 status = B_ERROR;
	if (link.FlushWithReply(status) != B_OK
		|| status < B_OK)
		return status;

	link.ReadString(family, sizeof(font_family));
	link.ReadString(style, sizeof(font_style));
	link.Read<float>(_size);
	return B_OK;
}


// Returns the number of installed font families
int32
count_font_families()
{
	return FontList::Default()->CountFamilies();
}


// Returns the number of styles available for a font family
int32
count_font_styles(font_family family)
{
	return FontList::Default()->CountStyles(family);
}


// Retrieves the family name at the specified index
status_t
get_font_family(int32 index, font_family* _name, uint32* _flags)
{
	if (_name == NULL)
		return B_BAD_VALUE;

	return FontList::Default()->FamilyAt(index, _name, _flags);
}


// Retrieves the family name at the specified index
status_t
get_font_style(font_family family, int32 index, font_style* _name,
	uint32* _flags)
{
	return get_font_style(family, index, _name, NULL, _flags);
}


// Retrieves the family name at the specified index
status_t
get_font_style(font_family family, int32 index, font_style* _name,
	uint16* _face, uint32* _flags)
{
	// The face value returned by this function is not very reliable. At the
	// same time, the value returned should be fairly reliable, returning the
	// proper flag for 90%-99% of font names.

	if (_name == NULL)
		return B_BAD_VALUE;

	return FontList::Default()->StyleAt(family, index, _name, _face, _flags);
}


// Updates the font family list
bool
update_font_families(bool /*checkOnly*/)
{
	return FontList::Default()->UpdatedOnServer();
}


//	#pragma mark -


BFont::BFont()
	:
	// initialise for be_plain_font (avoid circular definition)
	fFamilyID(0),
	fStyleID(0),
	fSize(10.0),
	fShear(90.0),
	fRotation(0.0),
	fFalseBoldWidth(0.0),
	fSpacing(B_BITMAP_SPACING),
	fEncoding(B_UNICODE_UTF8),
	fFace(0),
	fFlags(0),
	fExtraFlags(kUninitializedExtraFlags)
{
	if (be_plain_font != NULL && this != &sPlainFont)
		*this = *be_plain_font;
	else {
		fHeight.ascent = 7.0;
		fHeight.descent = 2.0;
		fHeight.leading = 13.0;
	}
}


BFont::BFont(const BFont& font)
{
	*this = font;
}


BFont::BFont(const BFont* font)
{
	if (font != NULL)
		*this = *font;
	else
		*this = *be_plain_font;
}


// Sets the font's family and style all at once
status_t
BFont::SetFamilyAndStyle(const font_family family, const font_style style)
{
	if (family == NULL && style == NULL)
		return B_BAD_VALUE;

	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_FAMILY_AND_STYLE_IDS);
	link.AttachString(family, sizeof(font_family));
	link.AttachString(style, sizeof(font_style));
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(0xffff);
	link.Attach<uint16>(fFace);

	int32 status = B_ERROR;
	if (link.FlushWithReply(status) != B_OK || status != B_OK)
		return status;

	link.Read<uint16>(&fFamilyID);
	link.Read<uint16>(&fStyleID);
	link.Read<uint16>(&fFace);
	fHeight.ascent = kUninitializedAscent;
	fExtraFlags = kUninitializedExtraFlags;

	return B_OK;
}


// Sets the font's family and style all at once
void
BFont::SetFamilyAndStyle(uint32 code)
{
	// R5 has a bug here: the face is not updated even though the IDs are set.
	// This is a problem because the face flag includes Regular/Bold/Italic
	// information in addition to stuff like underlining and strikethrough.
	// As a result, this will need a trip to the server and, thus, be slower
	// than R5's in order to be correct

	uint16 family, style;
	style = code & 0xFFFF;
	family = (code & 0xFFFF0000) >> 16;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_FAMILY_AND_STYLE_IDS);
	link.AttachString(NULL);	// no family and style name
	link.AttachString(NULL);
	link.Attach<uint16>(family);
	link.Attach<uint16>(style);
	link.Attach<uint16>(fFace);

	int32 fontcode;
	if (link.FlushWithReply(fontcode) != B_OK || fontcode != B_OK)
		return;

	link.Read<uint16>(&fFamilyID);
	link.Read<uint16>(&fStyleID);
	link.Read<uint16>(&fFace);
	fHeight.ascent = kUninitializedAscent;
	fExtraFlags = kUninitializedExtraFlags;
}


// Sets the font's family and face all at once
status_t
BFont::SetFamilyAndFace(const font_family family, uint16 face)
{
	// To comply with the BeBook, this function will only set valid values
	// i.e. passing a nonexistent family will cause only the face to be set.
	// Additionally, if a particular  face does not exist in a family, the
	// closest match will be chosen.

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_FAMILY_AND_STYLE_IDS);
	link.AttachString(family, sizeof(font_family));
	link.AttachString(NULL);	// no style given
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(0xffff);
	link.Attach<uint16>(face);

	int32 status = B_ERROR;
	if (link.FlushWithReply(status) != B_OK || status != B_OK)
		return status;

	link.Read<uint16>(&fFamilyID);
	link.Read<uint16>(&fStyleID);
	link.Read<uint16>(&fFace);
	fHeight.ascent = kUninitializedAscent;
	fExtraFlags = kUninitializedExtraFlags;

	return B_OK;
}


void
BFont::SetSize(float size)
{
	fSize = size;
	fHeight.ascent = kUninitializedAscent;
}


void
BFont::SetShear(float shear)
{
	fShear = shear;
	fHeight.ascent = kUninitializedAscent;
}


void
BFont::SetRotation(float rotation)
{
	fRotation = rotation;
	fHeight.ascent = kUninitializedAscent;
}


void
BFont::SetFalseBoldWidth(float width)
{
	fFalseBoldWidth = width;
}


void
BFont::SetSpacing(uint8 spacing)
{
	fSpacing = spacing;
}


void
BFont::SetEncoding(uint8 encoding)
{
	fEncoding = encoding;
}


void
BFont::SetFace(uint16 face)
{
	if (face == fFace)
		return;

	SetFamilyAndFace(NULL, face);
}


void
BFont::SetFlags(uint32 flags)
{
	fFlags = flags;
}


void
BFont::GetFamilyAndStyle(font_family* family, font_style* style) const
{
	if (family == NULL && style == NULL)
		return;

	// it's okay to call this function with either family or style set to NULL

	font_family familyBuffer;
	font_style styleBuffer;

	if (family == NULL)
		family = &familyBuffer;
	if (style == NULL)
		style = &styleBuffer;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_FAMILY_AND_STYLE);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);

	int32 code;
	if (link.FlushWithReply(code) != B_OK || code != B_OK) {
		// the least we can do is to clear the buffers
		memset(*family, 0, sizeof(font_family));
		memset(*style, 0, sizeof(font_style));
		return;
	}

	link.ReadString(*family, sizeof(font_family));
	link.ReadString(*style, sizeof(font_style));
}


uint32
BFont::FamilyAndStyle() const
{
	return (fFamilyID << 16UL) | fStyleID;
}


float
BFont::Size() const
{
	return fSize;
}


float
BFont::Shear() const
{
	return fShear;
}


float
BFont::Rotation() const
{
	return fRotation;
}


float
BFont::FalseBoldWidth() const
{
	return fFalseBoldWidth;
}


uint8
BFont::Spacing() const
{
	return fSpacing;
}


uint8
BFont::Encoding() const
{
	return fEncoding;
}


uint16
BFont::Face() const
{
	return fFace;
}


uint32
BFont::Flags() const
{
	return fFlags;
}


font_direction
BFont::Direction() const
{
	_GetExtraFlags();
	return (font_direction)(fExtraFlags >> B_PRIVATE_FONT_DIRECTION_SHIFT);
}


bool
BFont::IsFixed() const
{
	_GetExtraFlags();
	return (fExtraFlags & B_IS_FIXED) != 0;
}


// Returns whether or not the font is fixed-width and contains both
// full and half-width characters.
bool
BFont::IsFullAndHalfFixed() const
{
	// This was left unimplemented as of R5. It is a way to work with both
	// Kanji and Roman characters in the same fixed-width font.

	_GetExtraFlags();
	return (fExtraFlags & B_PRIVATE_FONT_IS_FULL_AND_HALF_FIXED) != 0;
}


BRect
BFont::BoundingBox() const
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_FONT_BOUNDING_BOX);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);

	int32 code;
	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return BRect(0, 0, 0 ,0);

	BRect box;
	link.Read<BRect>(&box);
	return box;
}


unicode_block
BFont::Blocks() const
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_UNICODE_BLOCKS);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);

	int32 status;
	if (link.FlushWithReply(status) != B_OK
		|| status != B_OK) {
		return unicode_block(~0LL, ~0LL);
	}

	unicode_block blocksForFont;
	link.Read<unicode_block>(&blocksForFont);

	return blocksForFont;
}

bool
BFont::IncludesBlock(uint32 start, uint32 end) const
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_HAS_UNICODE_BLOCK);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<uint32>(start);
	link.Attach<uint32>(end);

	int32 status;
	if (link.FlushWithReply(status) != B_OK
		|| status != B_OK) {
		return false;
	}

	bool hasBlock;
	link.Read<bool>(&hasBlock);

	return hasBlock;
}


font_file_format
BFont::FileFormat() const
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_FONT_FILE_FORMAT);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);

	int32 status;
	if (link.FlushWithReply(status) != B_OK
		|| status != B_OK) {
		// just take a safe bet...
		return B_TRUETYPE_WINDOWS;
	}

	uint16 format;
	link.Read<uint16>(&format);

	return (font_file_format)format;
}


int32
BFont::CountTuned() const
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_TUNED_COUNT);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);

	int32 code;
	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return -1;

	int32 count;
	link.Read<int32>(&count);
	return count;
}


void
BFont::GetTunedInfo(int32 index, tuned_font_info* info) const
{
	if (info == NULL)
		return;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_TUNED_INFO);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<uint32>(index);

	int32 code;
	if (link.FlushWithReply(code) != B_OK || code != B_OK)
		return;

	link.Read<tuned_font_info>(info);
}


void
BFont::TruncateString(BString* inOut, uint32 mode, float width) const
{
	if (mode == B_NO_TRUNCATION)
		return;

	// NOTE: Careful, we cannot directly use "inOut->String()" as result
	// array, because the string length increases by 3 bytes in the worst
	// case scenario.
	const char* string = inOut->String();
	GetTruncatedStrings(&string, 1, mode, width, inOut);
}


void
BFont::GetTruncatedStrings(const char* stringArray[], int32 numStrings,
	uint32 mode, float width, BString resultArray[]) const
{
	if (stringArray != NULL && numStrings > 0) {
		// the width of the "…" glyph
		float ellipsisWidth = StringWidth(B_UTF8_ELLIPSIS);

		for (int32 i = 0; i < numStrings; i++) {
			resultArray[i] = stringArray[i];
			int32 numChars = resultArray[i].CountChars();

			// get the escapement of each glyph in font units
			float* escapementArray = new float[numChars];
			GetEscapements(stringArray[i], numChars, NULL, escapementArray);

			truncate_string(resultArray[i], mode, width, escapementArray,
				fSize, ellipsisWidth, numChars);

			delete[] escapementArray;
		}
	}
}


void
BFont::GetTruncatedStrings(const char* stringArray[], int32 numStrings,
	uint32 mode, float width, char* resultArray[]) const
{
	if (stringArray != NULL && numStrings > 0) {
		for (int32 i = 0; i < numStrings; i++) {
			BString* strings = new BString[numStrings];
			GetTruncatedStrings(stringArray, numStrings, mode, width, strings);

			for (int32 i = 0; i < numStrings; i++)
				strcpy(resultArray[i], strings[i].String());

			delete[] strings;
		}
	}
}


float
BFont::StringWidth(const char* string) const
{
	if (string == NULL)
		return 0.0;

	int32 length = strlen(string);
	float width;
	GetStringWidths(&string, &length, 1, &width);

	return width;
}


float
BFont::StringWidth(const char* string, int32 length) const
{
	if (!string || length < 1)
		return 0.0f;

	float width = 0.0f;
	GetStringWidths(&string, &length, 1, &width);

	return width;
}


void
BFont::GetStringWidths(const char* stringArray[], const int32 lengthArray[],
	int32 numStrings, float widthArray[]) const
{
	if (stringArray == NULL || lengthArray == NULL || numStrings < 1
		|| widthArray == NULL) {
		return;
	}

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_STRING_WIDTHS);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<float>(fSize);
	link.Attach<uint8>(fSpacing);
	link.Attach<int32>(numStrings);

	// TODO: all strings into a single array???
	// we do have a maximum message length, and it could be easily touched
	// here...
	for (int32 i = 0; i < numStrings; i++)
		link.AttachString(stringArray[i], lengthArray[i]);

	status_t status;
	if (link.FlushWithReply(status) != B_OK || status != B_OK)
		return;

	link.Read(widthArray, sizeof(float) * numStrings);
}


void
BFont::GetEscapements(const char charArray[], int32 numChars,
	float escapementArray[]) const
{
	GetEscapements(charArray, numChars, NULL, escapementArray);
}


void
BFont::GetEscapements(const char charArray[], int32 numChars,
	escapement_delta* delta, float escapementArray[]) const
{
	if (charArray == NULL || numChars < 1 || escapementArray == NULL)
		return;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_ESCAPEMENTS_AS_FLOATS);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<float>(fSize);
	link.Attach<uint8>(fSpacing);
	link.Attach<float>(fRotation);
	link.Attach<uint32>(fFlags);

	link.Attach<float>(delta ? delta->nonspace : 0.0f);
	link.Attach<float>(delta ? delta->space : 0.0f);
	link.Attach<int32>(numChars);

	// TODO: Should we not worry about the port capacity here?!?
	uint32 bytesInBuffer = UTF8CountBytes(charArray, numChars);
	link.Attach<int32>(bytesInBuffer);
	link.Attach(charArray, bytesInBuffer);

	int32 code;
	if (link.FlushWithReply(code) != B_OK || code != B_OK)
		return;

	link.Read(escapementArray, numChars * sizeof(float));
}


void
BFont::GetEscapements(const char charArray[], int32 numChars,
	escapement_delta* delta, BPoint escapementArray[]) const
{
	GetEscapements(charArray, numChars, delta, escapementArray, NULL);
}


void
BFont::GetEscapements(const char charArray[], int32 numChars,
	escapement_delta* delta, BPoint escapementArray[],
	BPoint offsetArray[]) const
{
	if (charArray == NULL || numChars < 1 || escapementArray == NULL)
		return;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_ESCAPEMENTS);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<float>(fSize);
	link.Attach<uint8>(fSpacing);
	link.Attach<float>(fRotation);
	link.Attach<uint32>(fFlags);

	link.Attach<float>(delta ? delta->nonspace : 0.0);
	link.Attach<float>(delta ? delta->space : 0.0);
	link.Attach<bool>(offsetArray != NULL);
	link.Attach<int32>(numChars);

	// TODO: Should we not worry about the port capacity here?!?
	uint32 bytesInBuffer = UTF8CountBytes(charArray, numChars);
	link.Attach<int32>(bytesInBuffer);
	link.Attach(charArray, bytesInBuffer);

	int32 code;
	if (link.FlushWithReply(code) != B_OK || code != B_OK)
		return;

	link.Read(escapementArray, sizeof(BPoint) * numChars);
	if (offsetArray)
		link.Read(offsetArray, sizeof(BPoint) * numChars);
}


void
BFont::GetEdges(const char charArray[], int32 numChars,
	edge_info edgeArray[]) const
{
	if (!charArray || numChars < 1 || !edgeArray)
		return;

	int32 code;
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_EDGES);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<int32>(numChars);

	uint32 bytesInBuffer = UTF8CountBytes(charArray, numChars);
	link.Attach<int32>(bytesInBuffer);
	link.Attach(charArray, bytesInBuffer);

	if (link.FlushWithReply(code) != B_OK || code != B_OK)
		return;

	link.Read(edgeArray, sizeof(edge_info) * numChars);
}


void
BFont::GetHeight(font_height* _height) const
{
	if (_height == NULL)
		return;

	if (fHeight.ascent == kUninitializedAscent) {
		// we don't have the font height cached yet
		BPrivate::AppServerLink link;

		link.StartMessage(AS_GET_FONT_HEIGHT);
		link.Attach<uint16>(fFamilyID);
		link.Attach<uint16>(fStyleID);
		link.Attach<float>(fSize);

		int32 code;
		if (link.FlushWithReply(code) != B_OK || code != B_OK)
			return;

		// Who put that "const" to this method? :-)
		// We made fHeight mutable for this, but we should drop the "const"
		// when we can
		link.Read<font_height>(&fHeight);
	}

	*_height = fHeight;
}


void
BFont::GetBoundingBoxesAsGlyphs(const char charArray[], int32 numChars,
	font_metric_mode mode, BRect boundingBoxArray[]) const
{
	_GetBoundingBoxes(charArray, numChars, mode, false, NULL,
		boundingBoxArray, false);
}


void
BFont::GetBoundingBoxesAsString(const char charArray[], int32 numChars,
	font_metric_mode mode, escapement_delta* delta,
	BRect boundingBoxArray[]) const
{
	_GetBoundingBoxes(charArray, numChars, mode, true, delta,
		boundingBoxArray, true);
}


void
BFont::_GetBoundingBoxes(const char charArray[], int32 numChars,
	font_metric_mode mode, bool string_escapement, escapement_delta* delta,
	BRect boundingBoxArray[], bool asString) const
{
	if (charArray == NULL || numChars < 1 || boundingBoxArray == NULL)
		return;

	int32 code;
	BPrivate::AppServerLink link;

	link.StartMessage(asString
		? AS_GET_BOUNDINGBOXES_STRING : AS_GET_BOUNDINGBOXES_CHARS);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<float>(fSize);
	link.Attach<float>(fRotation);
	link.Attach<float>(fShear);
	link.Attach<float>(fFalseBoldWidth);
	link.Attach<uint8>(fSpacing);

	link.Attach<uint32>(fFlags);
	link.Attach<font_metric_mode>(mode);
	link.Attach<bool>(string_escapement);

	if (delta != NULL) {
		link.Attach<escapement_delta>(*delta);
	} else {
		escapement_delta emptyDelta = {0, 0};
		link.Attach<escapement_delta>(emptyDelta);
	}

	link.Attach<int32>(numChars);
	uint32 bytesInBuffer = UTF8CountBytes(charArray, numChars);
	link.Attach<int32>(bytesInBuffer);
	link.Attach(charArray, bytesInBuffer);

	if (link.FlushWithReply(code) != B_OK || code != B_OK)
		return;

	link.Read(boundingBoxArray, sizeof(BRect) * numChars);
}


void
BFont::GetBoundingBoxesForStrings(const char* stringArray[], int32 numStrings,
	font_metric_mode mode, escapement_delta deltas[],
	BRect boundingBoxArray[]) const
{
	if (!stringArray || numStrings < 1 || !boundingBoxArray)
		return;

	int32 code;
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_BOUNDINGBOXES_STRINGS);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<float>(fSize);
	link.Attach<float>(fRotation);
	link.Attach<float>(fShear);
	link.Attach<float>(fFalseBoldWidth);
	link.Attach<uint8>(fSpacing);
	link.Attach<uint32>(fFlags);
	link.Attach<font_metric_mode>(mode);
	link.Attach<int32>(numStrings);

	if (deltas) {
		for (int32 i = 0; i < numStrings; i++) {
			link.AttachString(stringArray[i]);
			link.Attach<escapement_delta>(deltas[i]);
		}
	} else {
		escapement_delta emptyDelta = {0, 0};

		for (int32 i = 0; i < numStrings; i++) {
			link.AttachString(stringArray[i]);
			link.Attach<escapement_delta>(emptyDelta);
		}
	}

	if (link.FlushWithReply(code) != B_OK || code != B_OK)
		return;

	link.Read(boundingBoxArray, sizeof(BRect) * numStrings);
}


void
BFont::GetGlyphShapes(const char charArray[], int32 numChars,
	BShape* glyphShapeArray[]) const
{
	// TODO: implement code specifically for passing BShapes to and
	// from the server
	if (!charArray || numChars < 1 || !glyphShapeArray)
		return;

	int32 code;
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_GLYPH_SHAPES);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<float>(fSize);
	link.Attach<float>(fShear);
	link.Attach<float>(fRotation);
	link.Attach<float>(fFalseBoldWidth);
	link.Attach<uint32>(fFlags);
	link.Attach<int32>(numChars);

	uint32 bytesInBuffer = UTF8CountBytes(charArray, numChars);
	link.Attach<int32>(bytesInBuffer);
	link.Attach(charArray, bytesInBuffer);

	if (link.FlushWithReply(code) != B_OK || code != B_OK)
		return;

	for (int32 i = 0; i < numChars; i++)
		link.ReadShape(glyphShapeArray[i]);
}


void
BFont::GetHasGlyphs(const char charArray[], int32 numChars,
	bool hasArray[]) const
{
	if (!charArray || numChars < 1 || !hasArray)
		return;

	int32 code;
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_HAS_GLYPHS);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<int32>(numChars);

	uint32 bytesInBuffer = UTF8CountBytes(charArray, numChars);
	link.Attach<int32>(bytesInBuffer);
	link.Attach(charArray, bytesInBuffer);

	if (link.FlushWithReply(code) != B_OK || code != B_OK)
		return;

	link.Read(hasArray, sizeof(bool) * numChars);
}


BFont&
BFont::operator=(const BFont& font)
{
	fFamilyID = font.fFamilyID;
	fStyleID = font.fStyleID;
	fSize = font.fSize;
	fShear = font.fShear;
	fRotation = font.fRotation;
	fFalseBoldWidth = font.fFalseBoldWidth;
	fSpacing = font.fSpacing;
	fEncoding = font.fEncoding;
	fFace = font.fFace;
	fHeight = font.fHeight;
	fFlags = font.fFlags;
	fExtraFlags = font.fExtraFlags;

	return *this;
}


bool
BFont::operator==(const BFont& font) const
{
	return fFamilyID == font.fFamilyID
		&& fStyleID == font.fStyleID
		&& fSize == font.fSize
		&& fShear == font.fShear
		&& fRotation == font.fRotation
		&& fFalseBoldWidth == font.fFalseBoldWidth
		&& fSpacing == font.fSpacing
		&& fEncoding == font.fEncoding
		&& fFace == font.fFace;
}


bool
BFont::operator!=(const BFont& font) const
{
	return fFamilyID != font.fFamilyID
		|| fStyleID != font.fStyleID
		|| fSize != font.fSize
		|| fShear != font.fShear
		|| fRotation != font.fRotation
		|| fFalseBoldWidth != font.fFalseBoldWidth
		|| fSpacing != font.fSpacing
		|| fEncoding != font.fEncoding
		|| fFace != font.fFace;
}


void
BFont::PrintToStream() const
{
	font_family family;
	font_style style;
	GetFamilyAndStyle(&family, &style);

	printf("BFont { %s (%d), %s (%d) 0x%x %f/%f %fpt (%f %f %f), %d }\n",
		family, fFamilyID, style, fStyleID, fFace, fShear, fRotation, fSize,
		fHeight.ascent, fHeight.descent, fHeight.leading, fEncoding);
}


void
BFont::_GetExtraFlags() const
{
	// TODO: this has to be const in order to allow other font getters to
	// stay const as well
	if (fExtraFlags != kUninitializedExtraFlags)
		return;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_EXTRA_FONT_FLAGS);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) != B_OK || status != B_OK) {
		// use defaut values for the flags
		fExtraFlags = (uint32)B_FONT_LEFT_TO_RIGHT
			<< B_PRIVATE_FONT_DIRECTION_SHIFT;
		return;
	}

	link.Read<uint32>(&fExtraFlags);
}
