//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		Font.cpp
//	Author:			DarkWyrm (bpmagic@columbus.rr.com)
//	Description:	Class to manage font-handling capabilities
//------------------------------------------------------------------------------
#include <Rect.h>
#include <stdio.h>
#include <Font.h>
#include <PortLink.h>

//----------------------------------------------------------------------------------------
//		Globals
//----------------------------------------------------------------------------------------

// The actual objects which the globals point to
BFont be_plain_bfont;
BFont be_bold_bfont;
BFont be_fixed_bfont;

const BFont *be_plain_font=&be_plain_bfont;
const BFont *be_bold_font=&be_bold_bfont;
const BFont *be_fixed_font=&be_fixed_bfont;


/*!
	\brief Private function used by Be. Exists only for compatibility. Does nothing.
*/
void _font_control_(BFont *font, int32 cmd, void *data)
{
}

/*!
	\brief Returns the number of installed font families
	\return The number of installed font families
*/
int32 count_font_families(void)
{
	// TODO: Implement
}

/*!
	\brief Returns the number of styles available for a font family
	\return The number of styles available for a font family
*/
int32 count_font_styles(font_family name)
{
	// TODO: Implement
}

/*!
	\brief Retrieves the family name at the specified index
	\param index Unique font identifier code.
	\param name font_family string to receive the name of the family
	\param flags iF non-NULL, the values of the flags IS_FIXED and B_HAS_TUNED_FONT are returned
	\return B_ERROR if the index does not correspond to a font family
*/
status_t get_font_family(int32 index, font_family *name, uint32 *flags)
{
	// Fix over R5, which does not check for NULL font family names - it just crashes
	if(!name)
		return B_ERROR;
	
	// TODO: Implement
}

/*!
	\brief Retrieves the family name at the specified index
	\param index Unique font identifier code.
	\param name font_family string to receive the name of the family
	\param flags iF non-NULL, the values of the flags IS_FIXED and B_HAS_TUNED_FONT are returned
	\return B_ERROR if the index does not correspond to a font style
*/
status_t get_font_style(font_family family, int32 index, font_style *name, uint32 *flags)
{
	// Fix over R5, which does not check for NULL font style names - it just crashes
	if(!name)
		return B_ERROR;

	// TODO: Implement
}

/*!
	\brief Retrieves the family name at the specified index
	\param index Unique font identifier code.
	\param name font_family string to receive the name of the family
	\param face recipient of font face value, such as B_REGULAR_FACE
	\param flags iF non-NULL, the values of the flags IS_FIXED and B_HAS_TUNED_FONT are returned
	\return B_ERROR if the index does not correspond to a font style
	
	The face value returned by this function is not very reliable. At the same time, the value
	returned should be fairly reliable, returning the proper flag for 90%-99% of font names.
*/
status_t get_font_style(font_family family, int32 index, font_style *name,
			uint16 *face, uint32 *flags)
{
	// Fix over R5, which does not check for NULL font style names - it just crashes
	if(!name || !face)
		return B_ERROR;

	// TODO: Implement
}

/*!
	\brief Updates the font family list
	\param check_only If true, the function only checks to see if the font list has changed
	\return true if the font list has changed, false if not.

	Because of the differences in the R5 and OpenBeOS font subsystems, this function operates 
	slightly differently, resulting in more efficient operation. A global font list for all 
	applications is maintained, so calling this function will still be quite expensive, 
	but it should be unnecessary in most applications.
*/
bool update_font_families(bool check_only)
{
	// TODO: Implement
}

status_t get_font_cache_info(uint32 id, void *set)
{
	// TODO: Implement

	// Note that the only reliable data from this function will probably be the cache size
	// Depending on how the font cache is implemented, this function and the corresponding
	// set function will either see major revision or completely disappear in R2.
}

status_t set_font_cache_info(uint32 id, void *set)
{
	// TODO: Implement

	// Note that this function will likely only set the cache size in our implementation
	// because of (a) the lack of knowledge on R5's font system and (b) the fact that it
	// is a completely different font engine.
}


//----------------------------------------------------------------------------------------
//		BFont Class Definition
//----------------------------------------------------------------------------------------

BFont::BFont(void)
{
	fFamilyID=be_plain_font->fFamilyID;
	fStyleID=be_plain_font->fStyleID;
	fSize=be_plain_font->fSize;
	fShear=be_plain_font->fShear;
	fRotation=be_plain_font->fRotation;
	fSpacing=be_plain_font->fSpacing;
	fEncoding=be_plain_font->fEncoding;
	fFace=be_plain_font->fFace;
	fHeight=be_plain_font->fHeight;
}

BFont::BFont(const BFont &font)
{
	fFamilyID=font.fFamilyID;
	fStyleID=font.fStyleID;
	fSize=font.fSize;
	fShear=font.fShear;
	fRotation=font.fRotation;
	fSpacing=font.fSpacing;
	fEncoding=font.fEncoding;
	fFace=font.fFace;
	fHeight=font.fHeight;
}

BFont::BFont(const BFont *font)
{
	if(font)
	{
		fFamilyID=font->fFamilyID;
		fStyleID=font->fStyleID;
		fSize=font->fSize;
		fShear=font->fShear;
		fRotation=font->fRotation;
		fSpacing=font->fSpacing;
		fEncoding=font->fEncoding;
		fFace=font->fFace;
		fHeight=font->fHeight;
	}
	else
	{
		fFamilyID=be_plain_font->fFamilyID;
		fStyleID=be_plain_font->fStyleID;
		fSize=be_plain_font->fSize;
		fShear=be_plain_font->fShear;
		fRotation=be_plain_font->fRotation;
		fSpacing=be_plain_font->fSpacing;
		fEncoding=be_plain_font->fEncoding;
		fFace=be_plain_font->fFace;
		fHeight=be_plain_font->fHeight;
	}
	
}

/*!
	\brief Sets the font's family and style all at once
	\param family Font family to set
	\param style Font style to set
	\return B_ERROR if family or style do not exist or if style does not belong to family.
*/
status_t BFont::SetFamilyAndStyle(const font_family family, const font_style style)
{
	// R5 version always returns B_OK. That's a problem...
	
	// TODO: implement

	// Query server for the appropriate family and style IDs and then return the
	// appropriate value
	return B_ERROR;
}

/*!
	\brief Sets the font's family and style all at once
	\param code Unique font identifier obtained from the server.
*/
void BFont::SetFamilyAndStyle(uint32 code)
{
	fStyleID=code & 0xFFFF;
	fFamilyID=(code & 0xFFFF0000) >> 16;
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
status_t BFont::SetFamilyAndFace(const font_family family, uint16 face)
{
	if(face & (	B_ITALIC_FACE | B_UNDERSCORE_FACE | B_NEGATIVE_FACE | B_OUTLINED_FACE
			| B_STRIKEOUT_FACE | B_BOLD_FACE | B_REGULAR_FACE) != 0)
	{
		fFace=face;
	}

	// TODO: finish this function by adding the app_server Family query protocol code
	// Query server for family id for the specified family
	
	return B_OK;
}

void BFont::SetSize(float size)
{
	fSize=size;
}

void BFont::SetShear(float shear)
{
	fShear=shear;
}

void BFont::SetRotation(float rotation)
{
	fRotation=rotation;
}

void BFont::SetSpacing(uint8 spacing)
{
	fSpacing=spacing;
}

void BFont::SetEncoding(uint8 encoding)
{
	fEncoding=encoding;
}

void BFont::SetFace(uint16 face)
{
	fFace=face;
}

void BFont::SetFlags(uint32 flags)
{
	fFlags=flags;
}

void BFont::GetFamilyAndStyle(font_family *family, font_style *style) const
{
	// TODO: implement

	// Query server for the names of this stuff given the family and style IDs kept internally
}

uint32 BFont::FamilyAndStyle(void) const
{
	uint32 token;
	token=(fFamilyID << 16) | fStyleID;
	return 0L;
}

float BFont::Size(void) const
{
	return fSize;
}

float BFont::Shear(void) const
{
	return fShear;
}

float BFont::Rotation(void) const
{
	return fRotation;
}

uint8 BFont::Spacing(void) const
{
	return fSpacing;
}

uint8 BFont::Encoding(void) const
{
	return fEncoding;
}

uint16 BFont::Face(void) const
{
	return fFace;
}

uint32 BFont::Flags(void) const
{
	return fFlags;
}

font_direction BFont::Direction(void) const
{
	// TODO: Query the server for the value
	
	return B_FONT_LEFT_TO_RIGHT;
}
 
bool BFont::IsFixed(void) const
{
	// TODO: query server for whether this bad boy is fixed-width
	
	return false;
}

/*!
	\brief Returns true if the font is fixed-width and contains both full and half-width characters
	
	This was left unimplemented as of R5. It was a way to work with both Kanji and Roman 
	characters in the same fixed-width font.
*/
bool BFont::IsFullAndHalfFixed(void) const
{
	return false;
}

BRect BFont::BoundingBox(void) const
{
	// TODO: query server for bounding box
	return BRect(0,0,0,0);
}

unicode_block BFont::Blocks(void) const
{
	// TODO: Add Block support
	return unicode_block();
}

font_file_format BFont::FileFormat(void) const
{
	// TODO: this will not work until I extend FreeType to handle this kind of call
	return B_TRUETYPE_WINDOWS;
}

int32 BFont::CountTuned(void) const
{
	// TODO: query server for appropriate data
	return 0;
}

void BFont::GetTunedInfo(int32 index, tuned_font_info *info) const
{
	// TODO: implement
}

void BFont::TruncateString(BString *in_out,uint32 mode,float width) const
{
	// TODO: implement
}

void BFont::GetTruncatedStrings(const char *stringArray[], int32 numStrings, 
		uint32 mode, float width, BString resultArray[]) const
{
	// TODO: implement
}

void BFont::GetTruncatedStrings(const char *stringArray[], int32 numStrings, 
		uint32 mode, float width, char *resultArray[]) const
{
	// TODO: implement
}

float BFont::StringWidth(const char *string) const
{
	// TODO: implement
	return 0.0;
}

float BFont::StringWidth(const char *string, int32 length) const
{
	// TODO: implement
	return 0.0;
}

void BFont::GetStringWidths(const char *stringArray[], const int32 lengthArray[], 
		int32 numStrings, float widthArray[]) const
{
	// TODO: implement
}

void BFont::GetEscapements(const char charArray[], int32 numChars, float escapementArray[]) const
{
	// TODO: implement
}

void BFont::GetEscapements(const char charArray[], int32 numChars, escapement_delta *delta, 
		float escapementArray[]) const
{
	// TODO: implement
}

void BFont::GetEscapements(const char charArray[], int32 numChars, escapement_delta *delta, 
		BPoint escapementArray[]) const
{
	// TODO: implement
}

void BFont::GetEscapements(const char charArray[], int32 numChars, escapement_delta *delta, 
		BPoint escapementArray[], BPoint offsetArray[]) const
{
	// TODO: implement
}

void BFont::GetEdges(const char charArray[], int32 numBytes, edge_info edgeArray[]) const
{
	// TODO: implement
}

void BFont::GetHeight(font_height *height) const
{
	if(height)
	{
		*height=fHeight;
	}
}

void BFont::GetBoundingBoxesAsGlyphs(const char charArray[], int32 numChars, font_metric_mode mode,
		BRect boundingBoxArray[]) const
{
	// TODO: implement
}

void BFont::GetBoundingBoxesAsString(const char charArray[], int32 numChars, font_metric_mode mode,
		escapement_delta *delta, BRect boundingBoxArray[]) const
{
	// TODO: implement
}

void BFont::GetBoundingBoxesForStrings(const char *stringArray[], int32 numStrings,
		font_metric_mode mode, escapement_delta deltas[], BRect boundingBoxArray[]) const
{
	// TODO: implement
}

void BFont::GetGlyphShapes(const char charArray[], int32 numChars, BShape *glyphShapeArray[]) const
{
	// TODO: implement
}
   
void BFont::GetHasGlyphs(const char charArray[], int32 numChars, bool hasArray[]) const
{
	// TODO: implement
}

BFont &BFont::operator=(const BFont &font)
{
	fFamilyID=font.fFamilyID;
	fStyleID=font.fStyleID;
	fSize=font.fSize;
	fShear=font.fShear;
	fRotation=font.fRotation;
	fSpacing=font.fSpacing;
	fEncoding=font.fEncoding;
	fFace=font.fFace;
	fHeight=font.fHeight;
	return *this;
}

bool BFont::operator==(const BFont &font) const
{
	if(	fFamilyID!=font.fFamilyID ||
			fStyleID!=font.fStyleID ||
			fSize!=font.fSize ||
			fShear!=font.fShear ||
			fRotation!=font.fRotation ||
			fSpacing!=font.fSpacing ||
			fEncoding!=font.fEncoding ||
			fFace!=font.fFace ||
			fHeight.ascent!=font.fHeight.ascent ||
			fHeight.descent!=font.fHeight.descent ||
			fHeight.leading!=font.fHeight.leading //||
			)
		return false;
	return true;
}

bool BFont::operator!=(const BFont &font) const
{
	if(	fFamilyID!=font.fFamilyID ||
			fStyleID!=font.fStyleID ||
			fSize!=font.fSize ||
			fShear!=font.fShear ||
			fRotation!=font.fRotation ||
			fSpacing!=font.fSpacing ||
			fEncoding!=font.fEncoding ||
			fFace!=font.fFace ||
			fHeight.ascent!=font.fHeight.ascent ||
			fHeight.descent!=font.fHeight.descent ||
			fHeight.leading!=font.fHeight.leading //||
			)
		return true;
	return false;
}
 
void BFont::PrintToStream(void) const
{
	printf("FAMILY STYLE %f %f %f %f %f %f\n", fSize, fShear, fRotation, fHeight.ascent,
		fHeight.descent, fHeight.leading);
}

