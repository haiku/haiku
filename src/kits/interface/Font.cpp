/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <stdio.h>
#include <stdlib.h>

#include <AppServerLink.h>
#include <Message.h>
#include <PortLink.h>
#include <Rect.h>
#include <ServerProtocol.h>
#include <Shape.h>
#include <String.h>

#include <moreUTF8.h>
#include <truncate_string.h>

#include <Font.h>


const float kUninitializedAscent = INFINITY;

// The actual objects which the globals point to
static BFont sPlainFont;
static BFont sBoldFont;
static BFont sFixedFont;

const BFont *be_plain_font = &sPlainFont;
const BFont *be_bold_font = &sBoldFont;
const BFont *be_fixed_font = &sFixedFont;


extern "C" void
_init_global_fonts()
{
	_font_control_(&sPlainFont, AS_SET_SYSFONT_PLAIN, NULL);
	_font_control_(&sBoldFont, AS_SET_SYSFONT_BOLD, NULL);
	_font_control_(&sFixedFont, AS_SET_SYSFONT_FIXED, NULL);
}


/*!
	\brief Private function originally used by Be. Now used for initialization
	\param font The font to initialize
	\param cmd message code to send to the app_server
	\param data unused
	
	While it is not known what Be used it for, Haiku uses it to initialize the
	three system fonts when the interface kit is initialized when an app starts.
*/

void
_font_control_(BFont *font, int32 cmd, void *data)
{
	if (!font
		|| (cmd != AS_SET_SYSFONT_PLAIN && cmd != AS_SET_SYSFONT_BOLD
			&& cmd != AS_SET_SYSFONT_FIXED)) {
		// this shouldn't ever happen, but just in case....
		return;
	}

	BPrivate::AppServerLink link;
	link.StartMessage(cmd);

	int32 code;
	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK) {
		printf("DEBUG: Couldn't initialize font in _font_control()\n");
		return;
	}

	// there really isn't that much data that we need to set for such cases -- most
	// of them need to be set to the defaults. The stuff that can change are family,
	// style/face, size, and height.
	// NOTE: this code assumes it's only called 
	link.Read<uint16>(&font->fFamilyID);
	link.Read<uint16>(&font->fStyleID);
	link.Read<float>(&font->fSize);
	link.Read<uint16>(&font->fFace);
	link.Read<uint32>(&font->fFlags);

	font->fHeight.ascent = kUninitializedAscent;
}


/*!
	\brief Private function used to replace the R5 hack which sets a system font
	\param which string denoting which font to set
	\param family the new family for the system font
	\param style the new style for the system font
	\param size the size for the system font to have
	
	R5 used a global area offset table to set the system fonts in the Font
	preferences panel. Bleah.
*/
void
_set_system_font_(const char *which, font_family family, font_style style, 
	float size)
{
	if (!which)
		return;
	
	if (!strcmp(which, "plain")
		|| !strcmp(which, "bold")
		|| !strcmp(which, "fixed")) {
		BPrivate::AppServerLink link;

		link.StartMessage(AS_SET_SYSTEM_FONT);
		link.AttachString(which);
		link.AttachString(family);
		link.AttachString(style);
		link.Attach<float>(size);
		link.Flush();
	}
}


/*!
	\brief Returns the number of installed font families
	\return The number of installed font families
*/

int32
count_font_families(void)
{
	int32 code, count;
	BPrivate::AppServerLink link;

	link.StartMessage(AS_COUNT_FONT_FAMILIES);

	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return -1;

	link.Read<int32>(&count);
	return count;
}


/*!
	\brief Returns the number of styles available for a font family
	\return The number of styles available for a font family
*/

int32
count_font_styles(font_family name)
{
	int32 code, count;
	BPrivate::AppServerLink link;

	link.StartMessage(AS_COUNT_FONT_STYLES);
	link.Attach(name, sizeof(font_family));

	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return -1;

	link.Read<int32>(&count);
	return count;
}


/*!
	\brief Retrieves the family name at the specified index
	\param index Unique font identifier code.
	\param name font_family string to receive the name of the family
	\param flags if non-NULL, the values of the flags IS_FIXED and B_HAS_TUNED_FONT are returned
	\return B_ERROR if the index does not correspond to a font family
*/

status_t
get_font_family(int32 index, font_family *name, uint32 *flags)
{
	// Fix over R5, which does not check for NULL font family names - it just crashes
	if (!name)
		return B_BAD_VALUE;

	int32 code;
	BPrivate::AppServerLink link;
	
	link.StartMessage(AS_GET_FAMILY_NAME);
	link.Attach<int32>(index);

	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return B_ERROR;

	link.Read<font_family>(name);
	
	uint32 value;
	link.Read<uint32>(&value);
	if (flags)
		*flags = value;

	return B_OK;
}


/*!
	\brief Retrieves the family name at the specified index
	\param index Unique font identifier code.
	\param name font_family string to receive the name of the family
	\param flags if non-NULL, the values of the flags IS_FIXED and B_HAS_TUNED_FONT are returned
	\return B_ERROR if the index does not correspond to a font style
*/

status_t
get_font_style(font_family family, int32 index, font_style *_name,
	uint32 *_flags)
{
	return get_font_style(family, index, _name, NULL, _flags);
}


/*!
	\brief Retrieves the family name at the specified index
	\param index Unique font identifier code.
	\param name font_family string to receive the name of the family
	\param face recipient of font face value, such as B_REGULAR_FACE
	\param flags if non-NULL, the values of the flags IS_FIXED and B_HAS_TUNED_FONT are returned
	\return B_ERROR if the index does not correspond to a font style
	
	The face value returned by this function is not very reliable. At the same time, the value
	returned should be fairly reliable, returning the proper flag for 90%-99% of font names.
*/

status_t
get_font_style(font_family family, int32 index, font_style *_name,
	uint16 *_face, uint32 *_flags)
{
	if (_name == NULL)
		return B_BAD_VALUE;

	// TODO: maybe cache the whole font list locally?

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_STYLE_NAME);
	link.AttachString(family);
	link.Attach<int32>(index);

	int32 status = B_ERROR;
	if (link.FlushWithReply(status) != B_OK
		|| status != B_OK)
		return status;

	link.ReadString(*_name, sizeof(font_style));

	uint16 face;
	uint32 flags;
	link.Read<uint16>(&face);
	link.Read<uint32>(&flags);

	if (_face)
		*_face = face;
	if (_flags)
		*_flags = flags;

	return B_OK;
}


/*!
	\brief Updates the font family list
	\param check_only If true, the function only checks to see if the font list has changed
	\return true if the font list has changed, false if not.
*/

bool
update_font_families(bool checkOnly)
{
	int32 code;
	bool value;
	BPrivate::AppServerLink link;

	// TODO: get some kind of change counter or timestamp and use node-monitoring
	//	for fonts in the app_server

	link.StartMessage(AS_QUERY_FONTS_CHANGED);
	link.Attach<bool>(checkOnly);

	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return false;

	link.Read<bool>(&value);
	return value;
}


status_t
get_font_cache_info(uint32 id, void *set)
{
	// TODO: Implement

	// Note that the only reliable data from this function will probably be the cache size
	// Depending on how the font cache is implemented, this function and the corresponding
	// set function will either see major revision or completely disappear in R2.
	return B_ERROR;
}


status_t
set_font_cache_info(uint32 id, void *set)
{
	// TODO: Implement

	// Note that this function will likely only set the cache size in our implementation
	// because of (a) the lack of knowledge on R5's font system and (b) the fact that it
	// is a completely different font engine.
	return B_ERROR;
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
	fSpacing(0),
	fEncoding(0),
	fFace(0),
	fFlags(0)
{
	if (be_plain_font != NULL && this != &sPlainFont)
		*this = *be_plain_font;
	else {
		fHeight.ascent = 7.0;
		fHeight.descent = 2.0;
		fHeight.leading = 13.0;
	}
}


BFont::BFont(const BFont &font)
{
	*this = font;
}


BFont::BFont(const BFont *font)
{
	if (font)
		*this = *font;
	else
		*this = *be_plain_font;
}


/*!
	\brief Sets the font's family and style all at once
	\param family Font family to set
	\param style Font style to set
	\return B_NAME_NOT_FOUND if family or style do not exist.
*/

status_t
BFont::SetFamilyAndStyle(const font_family family, const font_style style)
{
	if (family == NULL && style == NULL)
		return B_BAD_VALUE;

	BPrivate::AppServerLink link;

	link.StartMessage(AS_SET_FAMILY_AND_STYLE);
	link.AttachString(family);
	link.AttachString(style);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(0xffff);
	link.Attach<uint16>(fFace);

	int32 status = B_ERROR;
	if (link.FlushWithReply(status) != B_OK
		|| status != B_OK)
		return status;

	link.Read<uint16>(&fFamilyID);
	link.Read<uint16>(&fStyleID);
	link.Read<uint16>(&fFace);
	fHeight.ascent = kUninitializedAscent;

	return B_OK;
}


/*!
	\brief Sets the font's family and style all at once
	\param code Unique font identifier obtained from the server.
*/

void
BFont::SetFamilyAndStyle(uint32 fontcode)
{
	// R5 has a bug here: the face is not updated even though the IDs are set. This
	// is a problem because the face flag includes Regular/Bold/Italic information in
	// addition to stuff like underlining and strikethrough. As a result, this will
	// need a trip to the server and, thus, be slower than R5's in order to be correct

	uint16 family, style;
	style = fontcode & 0xFFFF;
	family = (fontcode & 0xFFFF0000) >> 16;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_FAMILY_AND_STYLE);
	link.AttachString(NULL);	// no family and style name
	link.AttachString(NULL);
	link.Attach<uint16>(family);
	link.Attach<uint16>(style);
	link.Attach<uint16>(fFace);

	int32 code;
	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	link.Read<uint16>(&fFamilyID);
	link.Read<uint16>(&fStyleID);
	link.Read<uint16>(&fFace);
	fHeight.ascent = kUninitializedAscent;
}


/*!
	\brief Sets the font's family and face all at once
	\param family Font family to set
	\param face Font face to set.
	\return B_ERROR if family does not exists or face is an invalid value.
	
	To comply with the BeBook, this function will only set valid values - i.e. passing a 
	nonexistent family will cause only the face to be set. Additionally, if a particular 
	face does not exist in a family, the closest match will be chosen.
*/

status_t
BFont::SetFamilyAndFace(const font_family family, uint16 face)
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_FAMILY_AND_STYLE);
	link.AttachString(family);
	link.AttachString(NULL);	// no style given
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(0xffff);
	link.Attach<uint16>(face);

	int32 status = B_ERROR;
	if (link.FlushWithReply(status) != B_OK
		|| status != B_OK)
		return status;

	link.Read<uint16>(&fFamilyID);
	link.Read<uint16>(&fStyleID);
	link.Read<uint16>(&fFace);
	fHeight.ascent = kUninitializedAscent;

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
BFont::GetFamilyAndStyle(font_family *family, font_style *style) const
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
	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK) {
		// the least we can do is to clear the buffers
		memset(family, 0, sizeof(font_family));
		memset(style, 0, sizeof(font_style));
		return;
	}

	link.Read<font_family>(family);
	link.Read<font_style>(style);
}


uint32
BFont::FamilyAndStyle(void) const
{
	return (fFamilyID << 16UL) | fStyleID;
}


float
BFont::Size(void) const
{
	return fSize;
}


float
BFont::Shear(void) const
{
	return fShear;
}


float
BFont::Rotation(void) const
{
	return fRotation;
}


uint8
BFont::Spacing(void) const
{
	return fSpacing;
}


uint8
BFont::Encoding(void) const
{
	return fEncoding;
}


uint16
BFont::Face(void) const
{
	return fFace;
}


uint32
BFont::Flags(void) const
{
	return fFlags;
}


font_direction
BFont::Direction(void) const
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_FONT_DIRECTION);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);

	int32 code;
	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return B_FONT_LEFT_TO_RIGHT;

	font_direction fdir;
	link.Read<font_direction>(&fdir);
	return fdir;
}
 

bool
BFont::IsFixed(void) const
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_QUERY_FONT_FIXED);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);

	int32 code;
	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return false;

	bool fixed;
	link.Read<bool>(&fixed);
	return fixed;
}


/*!
	\brief Returns true if the font is fixed-width and contains both full and half-width characters
	
	This was left unimplemented as of R5. It was a way to work with both Kanji and Roman 
	characters in the same fixed-width font.
*/

bool
BFont::IsFullAndHalfFixed(void) const
{
	return false;
}


BRect
BFont::BoundingBox(void) const
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
BFont::Blocks(void) const
{
	// TODO: Add Block support
	return unicode_block();
}


font_file_format
BFont::FileFormat(void) const
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
BFont::CountTuned(void) const
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
BFont::GetTunedInfo(int32 index, tuned_font_info *info) const
{
	if (!info)
		return;

	BPrivate::AppServerLink link;
	link.StartMessage(AS_GET_TUNED_INFO);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<uint32>(index);

	int32 code;
	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	link.Read<tuned_font_info>(info);
}


void
BFont::TruncateString(BString *inOut, uint32 mode, float width) const
{
	// NOTE: Careful, we cannot directly use "inOut->String()" as result
	// array, because the string length increases by 3 bytes in the worst case scenario.
	const char *string = inOut->String();
	GetTruncatedStrings(&string, 1, mode, width, inOut);
}


void
BFont::GetTruncatedStrings(const char *stringArray[], int32 numStrings, 
	uint32 mode, float width, BString resultArray[]) const
{
	if (stringArray && resultArray && numStrings > 0) {
		// allocate storage, see BeBook for "+ 3" (make space for ellipsis)
		char** truncatedStrings = new char*[numStrings];
		for (int32 i = 0; i < numStrings; i++) {
			truncatedStrings[i] = new char[strlen(stringArray[i]) + 3];
		}

		GetTruncatedStrings(stringArray, numStrings, mode, width, truncatedStrings);

		// copy the strings into the BString array and free each one
		for (int32 i = 0; i < numStrings; i++) {
			resultArray[i].SetTo(truncatedStrings[i]);
			delete[] truncatedStrings[i];
		}
		delete[] truncatedStrings;
	}
}


void
BFont::GetTruncatedStrings(const char *stringArray[], int32 numStrings, 
	uint32 mode, float width, char *resultArray[]) const
{
	if (stringArray && numStrings > 0) {
		// the width of the "…" glyph
		float ellipsisWidth = StringWidth(B_UTF8_ELLIPSIS);
		for (int32 i = 0; i < numStrings; i++) {
			int32 length = strlen(stringArray[i]);
			// count the individual glyphs
			int32 numChars = UTF8CountChars(stringArray[i], length);
			// get the escapement of each glyph in font units
			float* escapementArray = new float[numChars];
			GetEscapements(stringArray[i], numChars, NULL, escapementArray);

			truncate_string(stringArray[i], mode, width, resultArray[i],
							escapementArray, fSize, ellipsisWidth, length, numChars);

			delete[] escapementArray;
		}
	}
}


float
BFont::StringWidth(const char *string) const
{
	if (!string)
		return 0.0;

	int32 length = strlen(string);
	float width;
	GetStringWidths(&string, &length, 1, &width);

	return width;
}


float
BFont::StringWidth(const char *string, int32 length) const
{
	if (!string || length < 1)
		return 0.0;

	float width;
	GetStringWidths(&string, &length, 1, &width);

	return width;
}


void
BFont::GetStringWidths(const char *stringArray[], const int32 lengthArray[], 
	int32 numStrings, float widthArray[]) const
{
	if (!stringArray || !lengthArray || numStrings < 1 || !widthArray)
		return;

	int32 code;
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_STRING_WIDTHS);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<float>(fSize);
	link.Attach<uint8>(fSpacing);
	link.Attach<int32>(numStrings);

	for (int32 i = 0; i < numStrings; i++) {
		link.Attach<int32>(lengthArray[i]);
		link.AttachString(stringArray[i]);
	}

	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	link.Read(widthArray, sizeof(float) * numStrings);
}


void
BFont::GetEscapements(const char charArray[], int32 numChars, float escapementArray[]) const
{
	GetEscapements(charArray, numChars, NULL, escapementArray);
}


void
BFont::GetEscapements(const char charArray[], int32 numChars, escapement_delta *delta, 
	float escapementArray[]) const
{
	if (!charArray || numChars < 1 || !escapementArray)
		return;

	// NOTE: The R5 implementation crashes if delta == NULL!

	int32 code;
	BPrivate::AppServerLink link;
	
	link.StartMessage(AS_GET_ESCAPEMENTS_AS_FLOATS);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<float>(fSize);
	link.Attach<float>(fRotation);
	link.Attach<uint32>(fFlags);

	link.Attach<float>(delta ? delta->nonspace : 0.0);
	link.Attach<float>(delta ? delta->space : 0.0);
	
	// TODO: Should we not worry about the port capacity here?!?
	link.Attach<int32>(numChars);

	uint32 bytesInBuffer = UTF8CountBytes(charArray, numChars);
	link.Attach<int32>(bytesInBuffer);
	link.Attach(charArray, bytesInBuffer);

	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	link.Read(escapementArray, numChars * sizeof(float));
}


void
BFont::GetEscapements(const char charArray[], int32 numChars, escapement_delta *delta, 
	BPoint escapementArray[]) const
{
	GetEscapements(charArray, numChars, delta, escapementArray, NULL);
}


void
BFont::GetEscapements(const char charArray[], int32 numChars, escapement_delta *delta, 
	BPoint escapementArray[], BPoint offsetArray[]) const
{
	if (!charArray || numChars < 1 || !escapementArray)
		return;

	int32 code;
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_ESCAPEMENTS);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<float>(fSize);
	link.Attach<float>(fRotation);
	link.Attach<uint32>(fFlags);

	link.Attach<int32>(numChars);

	// TODO: Support UTF8 characters
	if (offsetArray) {
		for (int32 i = 0; i < numChars; i++) {
			link.Attach<char>(charArray[i]);
			link.Attach<BPoint>(offsetArray[i]);
		}
	} else {
		BPoint dummypt(0, 0);

		for (int32 i = 0; i < numChars; i++) {
			link.Attach<char>(charArray[i]);
			link.Attach<BPoint>(dummypt);
		}
	}

	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	link.Read(escapementArray, sizeof(BPoint) * numChars);
}


void
BFont::GetEdges(const char charArray[], int32 numChars, edge_info edgeArray[]) const
{
	if (!charArray || numChars < 1 || !edgeArray)
		return;

	int32 code;
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_EDGES);
	link.Attach<int32>(numChars);

	uint32 bytesInBuffer = UTF8CountBytes(charArray, numChars);
	link.Attach<int32>(bytesInBuffer);
	link.Attach(charArray, bytesInBuffer);

	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	link.Read(edgeArray, sizeof(edge_info) * numChars);
}


void
BFont::GetHeight(font_height *_height) const
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
		if (link.FlushWithReply(code) != B_OK
			|| code != B_OK)
			return;

		// Who put that "const" to this method? :-)
		// We made fHeight mutable for this, but we should drop the "const" when we can
		link.Read<font_height>(&fHeight);
	}

	*_height = fHeight;
}


void
BFont::GetBoundingBoxesAsGlyphs(const char charArray[], int32 numChars, font_metric_mode mode,
	BRect boundingBoxArray[]) const
{
	_GetBoundingBoxes_(charArray, numChars, mode, false, NULL, boundingBoxArray);
}


void
BFont::GetBoundingBoxesAsString(const char charArray[], int32 numChars, font_metric_mode mode,
	escapement_delta *delta, BRect boundingBoxArray[]) const
{
	_GetBoundingBoxes_(charArray, numChars, mode, true, delta, boundingBoxArray);
}


void
BFont::_GetBoundingBoxes_(const char charArray[], int32 numChars, font_metric_mode mode,
	bool string_escapement, escapement_delta *delta, BRect boundingBoxArray[]) const
{
	if (!charArray || numChars < 1 || !boundingBoxArray)
		return;

	int32 code;
	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_BOUNDINGBOXES_CHARS);
	link.Attach<uint16>(fFamilyID);
	link.Attach<uint16>(fStyleID);
	link.Attach<float>(fSize);
	link.Attach<float>(fRotation);
	link.Attach<float>(fShear);
	link.Attach<uint8>(fSpacing);
	
	link.Attach<uint32>(fFlags);
	link.Attach<font_metric_mode>(mode);
	link.Attach<bool>(string_escapement);

	if (delta) {
		link.Attach<escapement_delta>(*delta);
	} else {
		escapement_delta emptyDelta = {0, 0};
		link.Attach<escapement_delta>(emptyDelta);
	}

	link.Attach<int32>(numChars);
	uint32 bytesInBuffer = UTF8CountBytes(charArray, numChars);
	link.Attach<int32>(bytesInBuffer);
	link.Attach(charArray, bytesInBuffer);

	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	link.Read(boundingBoxArray, sizeof(BRect) * numChars);
}


void
BFont::GetBoundingBoxesForStrings(const char *stringArray[], int32 numStrings,
	font_metric_mode mode, escapement_delta deltas[], BRect boundingBoxArray[]) const
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

	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	link.Read(boundingBoxArray, sizeof(BRect) * numStrings);
}


void
BFont::GetGlyphShapes(const char charArray[], int32 numChars, BShape *glyphShapeArray[]) const
{
	// TODO: implement code specifically for passing BShapes to and from the server
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
	link.Attach<uint32>(fFlags);
	
	link.Attach<int32>(numChars);
	link.Attach(charArray, numChars);

	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	for (int32 i = 0; i < numChars; i++)
		link.ReadShape(glyphShapeArray[i]);
}


void
BFont::GetHasGlyphs(const char charArray[], int32 numChars, bool hasArray[]) const
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

	if (link.FlushWithReply(code) != B_OK
		|| code != B_OK)
		return;

	link.Read(hasArray, sizeof(bool) * numChars);
}


BFont
&BFont::operator=(const BFont &font)
{
	fFamilyID = font.fFamilyID;
	fStyleID = font.fStyleID;
	fSize = font.fSize;
	fShear = font.fShear;
	fRotation = font.fRotation;
	fSpacing = font.fSpacing;
	fEncoding = font.fEncoding;
	fFace = font.fFace;
	fHeight = font.fHeight;
	fFlags = font.fFlags;
	return *this;
}


bool
BFont::operator==(const BFont &font) const
{
	return fFamilyID == font.fFamilyID
		&& fStyleID == font.fStyleID
		&& fSize == font.fSize
		&& fShear == font.fShear
		&& fRotation == font.fRotation
		&& fSpacing == font.fSpacing
		&& fEncoding == font.fEncoding
		&& fFace == font.fFace;
}


bool
BFont::operator!=(const BFont &font) const
{
	return fFamilyID != font.fFamilyID
		|| fStyleID != font.fStyleID
		|| fSize != font.fSize
		|| fShear != font.fShear
		|| fRotation != font.fRotation
		|| fSpacing != font.fSpacing
		|| fEncoding != font.fEncoding
		|| fFace != font.fFace;
}


void
BFont::PrintToStream(void) const
{
	printf("FAMILY STYLE %f %f %f %f %f %f\n", fSize, fShear, fRotation, fHeight.ascent,
		fHeight.descent, fHeight.leading);
}

