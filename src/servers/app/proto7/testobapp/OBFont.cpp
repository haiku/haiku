#include <Rect.h>
#include <stdio.h>
#include <string.h>
#include "ClientFontList.h"
#include "OBFont.h"

class OBFont;

extern ClientFontList *client_font_list;

class OBFontPrivate
{
public:
	OBFontPrivate(void);
	OBFontPrivate &operator=(const OBFontPrivate &fontdata);
	
	BRect fBox;
	font_family fFamily;
	font_style fStyle;
	bool fFixed;
	font_file_format fFormat;
	font_direction fDirection;
	int32 fPrivateFlags;
};

OBFontPrivate::OBFontPrivate(void)
{
	fBox=BRect(0,0,0,0);
	strcpy(fFamily,"\0");
	strcpy(fStyle,"\0");
	fFixed=false;
	fFormat=B_TRUETYPE_WINDOWS;
	fDirection=B_FONT_LEFT_TO_RIGHT;
	fPrivateFlags=0;
}

OBFontPrivate & OBFontPrivate::operator=(const OBFontPrivate &fontdata)
{
	fBox=fontdata.fBox;
	*fFamily=*(fontdata.fFamily);
	*fStyle=*(fontdata.fStyle);
	fFixed=fontdata.fFixed;
	fFormat=fontdata.fFormat;
	fDirection=fontdata.fDirection;
	fPrivateFlags=fontdata.fPrivateFlags;
	return *this;
}

OBFont::OBFont(void)
{
	private_data=new OBFontPrivate();
	if(be_plain_font)
	{
		fFamilyID=obe_plain_font->fFamilyID;
		fStyleID=obe_plain_font->fStyleID;
		fSize=obe_plain_font->fSize;
		fShear=obe_plain_font->fShear;
		fRotation=obe_plain_font->fRotation;
		fSpacing=obe_plain_font->fSpacing;
		fEncoding=obe_plain_font->fEncoding;
		fFace=obe_plain_font->fFace;
		fHeight=obe_plain_font->fHeight;
		private_data=obe_plain_font->private_data;
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

OBFont::OBFont(const OBFont &font)
{
	private_data=new OBFontPrivate();
	fFamilyID=font.fFamilyID;
	fStyleID=font.fStyleID;
	fSize=font.fSize;
	fShear=font.fShear;
	fRotation=font.fRotation;
	fSpacing=font.fSpacing;
	fEncoding=font.fEncoding;
	fFace=font.fFace;
	fHeight=font.fHeight;
		private_data=font.private_data;
}

OBFont::OBFont(const OBFont *font)
{
	private_data=new OBFontPrivate();
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
		private_data=font->private_data;
	}
	else
	{
		if(obe_plain_font)
		{
			fFamilyID=obe_plain_font->fFamilyID;
			fStyleID=obe_plain_font->fStyleID;
			fSize=obe_plain_font->fSize;
			fShear=obe_plain_font->fShear;
			fRotation=obe_plain_font->fRotation;
			fSpacing=obe_plain_font->fSpacing;
			fEncoding=obe_plain_font->fEncoding;
			fFace=obe_plain_font->fFace;
			fHeight=obe_plain_font->fHeight;
			private_data=obe_plain_font->private_data;
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

OBFont::~OBFont(void)
{
	delete private_data;
}

status_t OBFont::SetFamilyAndStyle(const font_family family, const font_style style)
{
	// TODO: implement
	// TODO: find out what codes are returned by this function. Be Book says this returns nothing

	// Query server for the appropriate family and style IDs and then return the
	// appropriate value
	return B_ERROR;
}

void OBFont::SetFamilyAndStyle(uint32 code)
{
	fStyleID=code & 0xFFFF;
	fFamilyID=(code & 0xFFFF0000) >> 16;
}

status_t OBFont::SetFamilyAndFace(const font_family family, uint16 face)
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

void OBFont::SetSize(float size)
{
	fSize=size;
}

void OBFont::SetShear(float shear)
{
	fShear=shear;
}

void OBFont::SetRotation(float rotation)
{
	fRotation=rotation;
}

void OBFont::SetSpacing(uint8 spacing)
{
	fSpacing=spacing;
}

void OBFont::SetEncoding(uint8 encoding)
{
	fEncoding=encoding;
}

void OBFont::SetFace(uint16 face)
{
	fFace=face;
}

void OBFont::SetFlags(uint32 flags)
{
	fFlags=flags;
}

void OBFont::GetFamilyAndStyle(font_family *family, font_style *style) const
{
	if(!family | !style)
		return;

	// TODO: implement

	// Query server for the names of this stuff given the family and style IDs kept internally
	strcpy((char *)family,private_data->fFamily);
	strcpy((char *)style,private_data->fStyle);
}

uint32 OBFont::FamilyAndStyle(void) const
{
	uint32 token;
	token=(fFamilyID << 16) | fStyleID;
	return 0L;
}

float OBFont::Size(void) const
{
	return fSize;
}

float OBFont::Shear(void) const
{
	return fShear;
}

float OBFont::Rotation(void) const
{
	return fRotation;
}

uint8 OBFont::Spacing(void) const
{
	return fSpacing;
}

uint8 OBFont::Encoding(void) const
{
	return fEncoding;
}

uint16 OBFont::Face(void) const
{
	return fFace;
}

uint32 OBFont::Flags(void) const
{
	return fFlags;
}

font_direction OBFont::Direction(void) const
{
	return B_FONT_LEFT_TO_RIGHT;
}
 
bool OBFont::IsFixed(void) const
{
	// TODO: query server for whether this bad boy is fixed-width
	
	return false;
}

bool OBFont::IsFullAndHalfFixed(void) const
{
	return false;
}

BRect OBFont::BoundingBox(void) const
{
	// TODO: query server for bounding box
	return BRect(0,0,0,0);
}

unicode_block OBFont::Blocks(void) const
{
	// TODO: Add Block support
	return unicode_block();
}

font_file_format OBFont::FileFormat(void) const
{
	// TODO: this will not work until I extend FreeType to handle this kind of call
	return B_TRUETYPE_WINDOWS;
}

int32 OBFont::CountTuned(void) const
{
	// TODO: query server for appropriate data
	return 0;
}

void OBFont::GetTunedInfo(int32 index, tuned_font_info *info) const
{
	// TODO: implement
}

void OBFont::TruncateString(BString *in_out,uint32 mode,float width) const
{
	// TODO: implement
}

void OBFont::GetTruncatedStrings(const char *stringArray[], int32 numStrings, 
		uint32 mode, float width, BString resultArray[]) const
{
	// TODO: implement
}

void OBFont::GetTruncatedStrings(const char *stringArray[], int32 numStrings, 
		uint32 mode, float width, char *resultArray[]) const
{
	// TODO: implement
}

float OBFont::StringWidth(const char *string) const
{
	// TODO: implement
	return 0.0;
}

float OBFont::StringWidth(const char *string, int32 length) const
{
	// TODO: implement
	return 0.0;
}

void OBFont::GetStringWidths(const char *stringArray[], const int32 lengthArray[], 
		int32 numStrings, float widthArray[]) const
{
	// TODO: implement
}

void OBFont::GetEscapements(const char charArray[], int32 numChars, float escapementArray[]) const
{
	// TODO: implement
}

void OBFont::GetEscapements(const char charArray[], int32 numChars, escapement_delta *delta, 
		float escapementArray[]) const
{
	// TODO: implement
}

void OBFont::GetEscapements(const char charArray[], int32 numChars, escapement_delta *delta, 
		BPoint escapementArray[]) const
{
	// TODO: implement
}

void OBFont::GetEscapements(const char charArray[], int32 numChars, escapement_delta *delta, 
		BPoint escapementArray[], BPoint offsetArray[]) const
{
	// TODO: implement
}

void OBFont::GetEdges(const char charArray[], int32 numBytes, edge_info edgeArray[]) const
{
	// TODO: implement
}

void OBFont::GetHeight(font_height *height) const
{
	if(height)
	{
		*height=fHeight;
	}
}

void OBFont::GetBoundingBoxesAsGlyphs(const char charArray[], int32 numChars, font_metric_mode mode,
		BRect boundingBoxArray[]) const
{
	// TODO: implement
}

void OBFont::GetBoundingBoxesAsString(const char charArray[], int32 numChars, font_metric_mode mode,
		escapement_delta *delta, BRect boundingBoxArray[]) const
{
	// TODO: implement
}

void OBFont::GetBoundingBoxesForStrings(const char *stringArray[], int32 numStrings,
		font_metric_mode mode, escapement_delta deltas[], BRect boundingBoxArray[]) const
{
	// TODO: implement
}

void OBFont::GetGlyphShapes(const char charArray[], int32 numChars, BShape *glyphShapeArray[]) const
{
	// TODO: implement
}
   
void OBFont::GetHasGlyphs(const char charArray[], int32 numChars, bool hasArray[]) const
{
	// TODO: implement
}

OBFont &OBFont::operator=(const OBFont &font)
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

bool OBFont::operator==(const OBFont &font) const
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

bool OBFont::operator!=(const OBFont &font) const
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
 
void OBFont::PrintToStream(void) const
{
	printf("FAMILY STYLE %f %f %f %f %f %f\n", fSize, fShear, fRotation, fHeight.ascent,
		fHeight.descent, fHeight.leading);
}

int32 ob_count_font_families(void)
{
	return client_font_list->CountFamilies();
}

status_t ob_get_font_family(int32 index, font_family *name, uint32 *flags=NULL)
{
	
	return client_font_list->GetFamily(index,name,flags);
}

int32 ob_count_font_styles(font_family name)
{
	return client_font_list->CountStyles(name);
}

status_t ob_get_font_style(font_family family, int32 index, font_style *name, uint32 *flags=NULL)
{
	return client_font_list->GetStyle(family,index,name,flags);
}

status_t ob_get_font_style(font_family family, int32 index, font_style *name, 
		uint16 *face, uint32 *flags=NULL)
{
	return client_font_list->GetStyle(family,index,name,flags,face);
}

bool ob_update_font_families(bool check_only)
{
	return client_font_list->Update(check_only);
}

status_t ob_get_font_cache_info(uint32 id, void *set)
{
	// lame duck undocumented function
	return B_OK;
}

status_t ob_set_font_cache_info(uint32 id, void *set)
{
	// lame duck undocumented function
	return B_OK;
}

