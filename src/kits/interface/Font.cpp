#include <Rect.h>
#include <stdio.h>
#include "Font.h"

class BFontPrivate
{
public:
	BFontPrivate(void);
	BFontPrivate &operator=(const BFontPrivate &fontdata);
	
	BRect fBox;
	font_family fFamily;
	font_style fStyle;
	bool fFixed;
	font_file_format fFormat;
	font_direction fDirection;
	int32 fPrivateFlags;
};

BFontPrivate::BFontPrivate(void)
{
	fBox=BRect(0,0,0,0);
	fFixed=false;
	fFormat=B_TRUETYPE_WINDOWS;
	fDirection=B_FONT_LEFT_TO_RIGHT;
}

BFontPrivate & BFontPrivate::operator=(const BFontPrivate &fontdata)
{
	fBox=fontdata.fBox;
	*fFamily=*(fontdata.fFamily);
	*fStyle=*(fontdata.fStyle);
	fFixed=fontdata.fFixed;
	fFormat=fontdata.fFormat;
	fDirection=fontdata.fDirection;
	return *this;
}


BFont::BFont(void)
{
	private_data=new BFontPrivate();
	if(be_plain_font)
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
		private_data->fPrivateFlags=be_plain_font->private_data->fPrivateFlags;
	}
	else
	{
		fFamilyID=0;
		fStyleID=0;
		fSize=0.0;
		fShear=90.0;
		fRotation=0.0;
		fSpacing=B_CHAR_SPACING;
		fEncoding=B_UNICODE_UTF8;
		fFace=B_REGULAR_FACE;
		fHeight.ascent=0.0;
		fHeight.descent=0.0;
		fHeight.leading=0.0;
		private_data->fPrivateFlags=0;
	}
}

BFont::BFont(const BFont &font)
{
	private_data=new BFontPrivate();
	fFamilyID=font.fFamilyID;
	fStyleID=font.fStyleID;
	fSize=font.fSize;
	fShear=font.fShear;
	fRotation=font.fRotation;
	fSpacing=font.fSpacing;
	fEncoding=font.fEncoding;
	fFace=font.fFace;
	fHeight=font.fHeight;
	private_data->fPrivateFlags=font.private_data->fPrivateFlags;
}

BFont::BFont(const BFont *font)
{
	private_data=new BFontPrivate();
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
		private_data->fPrivateFlags=font->private_data->fPrivateFlags;
	}
	else
	{
		if(be_plain_font)
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
			private_data->fPrivateFlags=be_plain_font->private_data->fPrivateFlags;
		}
		else
		{
			fFamilyID=0;
			fStyleID=0;
			fSize=0.0;
			fShear=90.0;
			fRotation=0.0;
			fSpacing=B_CHAR_SPACING;
			fEncoding=B_UNICODE_UTF8;
			fFace=B_REGULAR_FACE;
			fHeight.ascent=0.0;
			fHeight.descent=0.0;
			fHeight.leading=0.0;
			private_data->fPrivateFlags=0;
		}
	}
	
}

/* XXX TODO: R5 doesn't have a destructor, so we get linking errors when objects compiled with old headers with the new library
			 (but now we leak memory here)
BFont::~BFont(void)
{
	delete private_data;
}
*/

status_t BFont::SetFamilyAndStyle(const font_family family, const font_style style)
{
	// TODO: implement
	// TODO: find out what codes are returned by this function. Be Book says this returns nothing

	// Query server for the appropriate family and style IDs and then return the
	// appropriate value
	return B_ERROR;
}

void BFont::SetFamilyAndStyle(uint32 code)
{
	fStyleID=code & 0xFFFF;
	fFamilyID=(code & 0xFFFF0000) >> 16;
}

status_t BFont::SetFamilyAndFace(const font_family family, uint16 face)
{
	// TODO: find out what codes are returned by this function. Be Book says this returns nothing
	fFace=face;

	// TODO: finish this function by adding the app_server Family query protocol code
	if(family)
	{
		// Query server for family id for the specified family
	}
	
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
	return B_FONT_LEFT_TO_RIGHT;
}
 
bool BFont::IsFixed(void) const
{
	// TODO: query server for whether this bad boy is fixed-width
	
	return false;
}

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
	private_data->fPrivateFlags=font.private_data->fPrivateFlags;
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
			fHeight.leading!=font.fHeight.leading ||
			private_data->fPrivateFlags!=font.private_data->fPrivateFlags )
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
			fHeight.leading!=font.fHeight.leading ||
			private_data->fPrivateFlags!=font.private_data->fPrivateFlags )
		return true;
	return false;
}
 
void BFont::PrintToStream(void) const
{
	printf("FAMILY STYLE %f %f %f %f %f %f\n", fSize, fShear, fRotation, fHeight.ascent,
		fHeight.descent, fHeight.leading);
}

