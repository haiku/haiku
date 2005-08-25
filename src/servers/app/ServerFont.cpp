/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Jérôme Duval, jerome.duval@free.fr
 */


#include <ByteOrder.h>
#include <Shape.h>
#include <String.h>
#include <UTF8.h>

#include "Angle.h"
#include "FontServer.h"
#include "moreUTF8.h"
#include "truncate_string.h"

#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include "ServerFont.h"


// functions needed to convert a freetype vector graphics to a BShape
inline BPoint
VectorToPoint(FT_Vector *vector)
{
	BPoint result;
	result.x = float(int32(vector->x)) / 2097152;
	result.y = -float(int32(vector->y)) / 2097152;
	return result;
}

int
MoveToFunc(FT_Vector *to, void *user)
{
	((BShape *)user)->MoveTo(VectorToPoint(to));
	return 0;
}

int
LineToFunc(FT_Vector *to, void *user)
{
	((BShape *)user)->LineTo(VectorToPoint(to));
	return 0;
}

int
ConicToFunc(FT_Vector *control, FT_Vector *to, void *user)
{
	BPoint controls[3];
	
	controls[0] = VectorToPoint(control);
	controls[1] = VectorToPoint(to);
	controls[2] = controls[1];
	
	((BShape *)user)->BezierTo(controls);
	return 0;
}

int
CubicToFunc(FT_Vector *control1, FT_Vector *control2, FT_Vector *to, void *user)
{
	BPoint controls[3];
	
	controls[0] = VectorToPoint(control1);
	controls[1] = VectorToPoint(control2);
	controls[2] = VectorToPoint(to);
	
	((BShape *)user)->BezierTo(controls);
	return 0;
}


// is_white_space
inline bool
is_white_space(uint16 glyph)
{
	// TODO: handle them all!
	if (glyph == ' ' || glyph == B_TAB)
		return true;
	return false;
}


//	#pragma mark -


/*! 
	\brief Constructor
	\param style Style object to which the ServerFont belongs
	\param size Character size in points
	\param rotation Rotation in degrees
	\param shear Shear (slant) in degrees. 45 <= shear <= 135
	\param flags Style flags as defined in <Font.h>
	\param spacing String spacing flag as defined in <Font.h>
*/
ServerFont::ServerFont(FontStyle *style, float size,
					   float rotation, float shear,
					   uint16 flags, uint8 spacing)
	: fStyle(style),
	  fSize(size),
	  fRotation(rotation),
	  fShear(shear),
	  fBounds(0, 0, 0, 0),
	  fFlags(flags),
	  fSpacing(spacing),
	  fDirection(B_FONT_LEFT_TO_RIGHT),
	  fFace(B_REGULAR_FACE),
	  fEncoding(B_UNICODE_UTF8)

{
	if (fStyle)
		fStyle->AddDependent();
}

// TODO: fStyle should not be NULL. There should be another FontStyle
// constructor, that initializes without actually interfacing with
// freetype, so that a ServerFont can be guaranteed to be "valid".

ServerFont::ServerFont()
	: fStyle(NULL),
	  fSize(0.0),
	  fRotation(0.0),
	  fShear(90.0),
	  fBounds(0, 0, 0, 0),
	  fFlags(0),
	  fSpacing(B_STRING_SPACING),
	  fDirection(B_FONT_LEFT_TO_RIGHT),
	  fFace(B_REGULAR_FACE),
	  fEncoding(B_UNICODE_UTF8)
{
}

/*! 
	\brief Copy Constructor
	\param font ServerFont to copy
*/
ServerFont::ServerFont(const ServerFont &font)
	: fStyle(font.fStyle),
	  fSize(font.fSize),
	  fRotation(font.fRotation),
	  fShear(font.fShear),
	  fBounds(0, 0, 0, 0),
	  fFlags(font.fFlags),
	  fSpacing(font.fSpacing),
	  fDirection(font.fDirection),
	  fFace(font.fFace),
	  fEncoding(font.fEncoding)
{
	if (fStyle)
		fStyle->AddDependent();
}

/*! 
	\brief Removes itself as a dependency of its owning style.
*/
ServerFont::~ServerFont()
{
	if (fStyle)
		fStyle->RemoveDependent();
}

/*! 
	\brief Returns a copy of the specified font
	\param The font to copy from.
	\return A copy of the specified font
*/
ServerFont&
ServerFont::operator=(const ServerFont& font)
{
	fSize		= font.fSize;
	fRotation	= font.fRotation;
	fShear		= font.fShear;
	fFlags		= font.fFlags;
	fSpacing	= font.fSpacing;
	fDirection	= font.fDirection;
	fFace		= font.fFace;
	fEncoding	= font.fEncoding;
	fBounds		= font.fBounds;

	_SetStyle(font.fStyle);

	return *this;
}

/*! 
	\brief Returns the number of strikes in the font
	\return The number of strikes in the font
*/
int32
ServerFont::CountTuned()
{
	if (fStyle)
		return fStyle->TunedCount();

	return 0;
}

/*! 
	\brief Returns the file format of the font. Currently unimplemented.
	\return B_TRUETYPE_WINDOWS
*/
font_file_format
ServerFont::FileFormat()
{
	// TODO: implement ServerFont::FileFormat
	return B_TRUETYPE_WINDOWS;
}

const char*
ServerFont::GetStyle() const
{
	if (fStyle)
		return fStyle->Name();

	return NULL;
}

const char*
ServerFont::GetFamily() const
{
	if (fStyle)
		return fStyle->Family()->Name();

	return NULL;
}

/*!
	\brief Sets the ServerFont instance to whatever font is specified
	\param familyID ID number of the family to set
	\param styleID ID number of the style to set
	\return B_OK if successful, B_ERROR if not
*/
status_t
ServerFont::SetFamilyAndStyle(uint16 familyID, uint16 styleID)
{
	FontStyle* style = NULL;

	if (gFontServer->Lock()) {
		 style = gFontServer->GetStyle(familyID, styleID);
		gFontServer->Unlock();
	}

	if (!style)
		return B_ERROR;

	_SetStyle(style);

	return B_OK;
}

/*!
	\brief Sets the ServerFont instance to whatever font is specified
	\param fontID the combination of family and style ID numbers
	\return B_OK if successful, B_ERROR if not
*/
status_t
ServerFont::SetFamilyAndStyle(uint32 fontID)
{
	uint16 style = fontID & 0xFFFF;
	uint16 family = (fontID & 0xFFFF0000) >> 16;
	
	return SetFamilyAndStyle(family, style);
}

/*!
	\brief Gets the ID values for the ServerFont instance in one shot
	\return the combination of family and style ID numbers
*/
uint32
ServerFont::GetFamilyAndStyle(void) const
{
	return (FamilyID() << 16) | StyleID();
}


BShape **
ServerFont::GetGlyphShapes(const char charArray[], int32 numChars) const
{
	if (!charArray || numChars <= 0)
		return NULL;
	
	FT_Outline_Funcs funcs;
	funcs.move_to = MoveToFunc;
	funcs.line_to = LineToFunc;
	funcs.conic_to = ConicToFunc;
	funcs.cubic_to = CubicToFunc;
	
	FT_Face face = fStyle->GetFTFace();
	if (!face)
		return NULL;
	
	FT_Set_Char_Size(face, 0, int32(fSize * 64), 72, 72);
	
	Angle rotation(fRotation);
	Angle shear(fShear);
	
	// First, rotate
	FT_Matrix rmatrix;
	rmatrix.xx = (FT_Fixed)( rotation.Cosine()*0x10000);
	rmatrix.xy = (FT_Fixed)(-rotation.Sine()*0x10000);
	rmatrix.yx = (FT_Fixed)( rotation.Sine()*0x10000);
	rmatrix.yy = (FT_Fixed)( rotation.Cosine()*0x10000);
	
	// Next, shear
	FT_Matrix smatrix;
	smatrix.xx = (FT_Fixed)(0x10000); 
	smatrix.xy = (FT_Fixed)(-shear.Cosine()*0x10000);
	smatrix.yx = (FT_Fixed)(0);
	smatrix.yy = (FT_Fixed)(0x10000);
	
	// Multiply togheter
	FT_Matrix_Multiply(&rmatrix, &smatrix);
	
	//FT_Vector pen;
	//FT_Set_Transform(face, &smatrix, &pen);
	
	BShape **shapes = (BShape **)malloc(sizeof(BShape *) * numChars);
	for (int i = 0; i < numChars; i++) {
		shapes[i] = new BShape();
		shapes[i]->Clear();
		// TODO : this is wrong (the nth char isn't charArray[i])
		FT_Load_Char(face, charArray[i], FT_LOAD_NO_BITMAP);
		FT_Outline outline = face->glyph->outline;
		FT_Outline_Decompose(&outline, &funcs, shapes[i]);
		shapes[i]->Close();
	}
	
	return shapes;
}


void
ServerFont::GetHasGlyphs(const char charArray[], int32 numChars, bool hasArray[]) const
{
	if (!fStyle || !charArray || numChars <= 0 || !hasArray)
		return;

		FT_Face face = fStyle->GetFTFace();
	if (!face)
		return;

	FT_Set_Char_Size(face, 0, int32(fSize * 64), 72, 72);

	// UTF8 handling...this can probably be smarter
	// Here is what I do in the AGGTextRenderer to handle UTF8...
	// It is probably highly inefficient, so it should be reviewed.
	int32 numBytes = UTF8CountBytes(charArray, numChars);
	int32 convertedLength = numBytes * 2;
	char* convertedBuffer = new char[convertedLength];

	int32 state = 0;
	status_t ret;
	if ((ret = convert_from_utf8(B_UNICODE_CONVERSION, 
								 charArray, &numBytes,
								 convertedBuffer, &convertedLength,
								 &state, B_SUBSTITUTE)) >= B_OK
		&& (ret = swap_data(B_INT16_TYPE, convertedBuffer, convertedLength,
							B_SWAP_BENDIAN_TO_HOST)) >= B_OK) {

		uint16* glyphIndex = (uint16*)convertedBuffer;
		// just to be sure
		numChars = min_c((uint32)numChars, convertedLength / sizeof(uint16));

		for (int i = 0; i < numChars; i++) {
			hasArray[i] = FT_Get_Char_Index(face, glyphIndex[i]) > 0;
		}
	}
	delete[] convertedBuffer;
}


// GetEdges
void
ServerFont::GetEdges(const char charArray[], int32 numChars, edge_info edgeArray[]) const
{
	if (!fStyle || !charArray || numChars <= 0 || !edgeArray)
		return;

	FT_Face face = fStyle->GetFTFace();
	if (!face)
		return;

	FT_Set_Char_Size(face, 0, int32(fSize * 64), 72, 72);

	// UTF8 handling...this can probably be smarter
	// Here is what I do in the AGGTextRenderer to handle UTF8...
	// It is probably highly inefficient, so it should be reviewed.
	int32 numBytes = UTF8CountBytes(charArray, numChars);
	int32 convertedLength = numBytes * 2;
	char* convertedBuffer = new char[convertedLength];

	int32 state = 0;
	status_t ret;
	if ((ret = convert_from_utf8(B_UNICODE_CONVERSION, 
								 charArray, &numBytes,
								 convertedBuffer, &convertedLength,
								 &state, B_SUBSTITUTE)) >= B_OK
		&& (ret = swap_data(B_INT16_TYPE, convertedBuffer, convertedLength,
							B_SWAP_BENDIAN_TO_HOST)) >= B_OK) {

		uint16* glyphIndex = (uint16*)convertedBuffer;
		// just to be sure
		numChars = min_c((uint32)numChars, convertedLength / sizeof(uint16));

		for (int i = 0; i < numChars; i++) {
			FT_Load_Char(face, glyphIndex[i], FT_LOAD_NO_BITMAP);
			if (face->glyph) {
				edgeArray[i].left = float(face->glyph->metrics.horiBearingX /64) / fSize;
				edgeArray[i].right = float((face->glyph->metrics.horiBearingX 
					+ face->glyph->metrics.width - face->glyph->metrics.horiAdvance)/64) /fSize;
			}
		}
	}
	delete[] convertedBuffer;
}


// GetEscapements
BPoint*
ServerFont::GetEscapements(const char charArray[], int32 numChars,
						   BPoint offsetArray[]) const
{
	if (!fStyle || !charArray || numChars <= 0 || !offsetArray)
		return NULL;
	
	FT_Face face = fStyle->GetFTFace();
	if (!face)
		return NULL;
	
	FT_Set_Char_Size(face, 0, int32(fSize * 64), 72, 72);
	
	Angle rotation(fRotation);
	Angle shear(fShear);
	
	// First, rotate
	FT_Matrix rmatrix;
	rmatrix.xx = (FT_Fixed)( rotation.Cosine()*0x10000);
	rmatrix.xy = (FT_Fixed)(-rotation.Sine()*0x10000);
	rmatrix.yx = (FT_Fixed)( rotation.Sine()*0x10000);
	rmatrix.yy = (FT_Fixed)( rotation.Cosine()*0x10000);
	
	// Next, shear
	FT_Matrix smatrix;
	smatrix.xx = (FT_Fixed)(0x10000); 
	smatrix.xy = (FT_Fixed)(-shear.Cosine()*0x10000);
	smatrix.yx = (FT_Fixed)(0);
	smatrix.yy = (FT_Fixed)(0x10000);
	
	// Multiply togheter
	FT_Matrix_Multiply(&rmatrix, &smatrix);
	
	//FT_Vector pen;
	//FT_Set_Transform(face, &smatrix, &pen);
	
	// TODO: I'm not sure if this the correct interpretation
	// of the BeBook. Have actual tests been done here?

	// TODO: handle UTF8... see below!!
	BPoint *escapements = (BPoint *)malloc(sizeof(BPoint) * numChars);
	for (int i = 0; i < numChars; i++) {
		// TODO : this is wrong (the nth char isn't charArray[i])
		FT_Load_Char(face, charArray[i], FT_LOAD_NO_BITMAP);
//		escapements[i].x = float(face->glyph->metrics.width / 64) / fSize;
//		escapements[i].y = 0;
		escapements[i].x = float(face->glyph->metrics.horiAdvance / 64) / fSize;
		escapements[i].y = float(face->glyph->metrics.vertAdvance / 64) / fSize;
		escapements[i] += offsetArray[i];
	}
	
	return escapements;
}


// GetEscapements
bool
ServerFont::GetEscapements(const char charArray[], int32 numChars,
						   float widthArray[], escapement_delta delta) const
{
	if (!fStyle || !charArray || numChars <= 0)
		return false;
	
	FT_Face face = fStyle->GetFTFace();
	if (!face)
		return false;

	FT_Set_Char_Size(face, 0, int32(fSize * 64), 72, 72);

	// UTF8 handling...this can probably be smarter
	// Here is what I do in the AGGTextRenderer to handle UTF8...
	// It is probably highly inefficient, so it should be reviewed.
	int32 numBytes = UTF8CountBytes(charArray, numChars);
	int32 convertedLength = numBytes * 2;
	char* convertedBuffer = new char[convertedLength];

	int32 state = 0;
	status_t ret;
	if ((ret = convert_from_utf8(B_UNICODE_CONVERSION, 
								 charArray, &numBytes,
								 convertedBuffer, &convertedLength,
								 &state, B_SUBSTITUTE)) >= B_OK
		&& (ret = swap_data(B_INT16_TYPE, convertedBuffer, convertedLength,
							B_SWAP_BENDIAN_TO_HOST)) >= B_OK) {

		uint16* glyphIndex = (uint16*)convertedBuffer;
		// just to be sure
		numChars = min_c((uint32)numChars, convertedLength / sizeof(uint16));

		for (int i = 0; i < numChars; i++) {
			FT_Load_Char(face, glyphIndex[i], FT_LOAD_NO_BITMAP);
			if (face->glyph) {
				widthArray[i] = ((float)face->glyph->metrics.horiAdvance / 64.0) / fSize;
				widthArray[i] += is_white_space(glyphIndex[i]) ? delta.space : delta.nonspace;
			}
		}
	}
	delete[] convertedBuffer;

	return ret >= B_OK;
}


bool
ServerFont::GetBoundingBoxesAsString(const char charArray[], int32 numChars, BRect rectArray[],
		font_metric_mode mode, escapement_delta delta)
{
	if (!fStyle || !charArray || numChars <= 0 || !rectArray)
		return false;

	FT_Face face = fStyle->GetFTFace();
	if (!face)
		return false;
	
	FT_Set_Char_Size(face, 0, int32(fSize * 64), 72, 72);

	return true;
}


bool
ServerFont::GetBoundingBoxesAsStrings(char *charArray[], int32 lengthArray[], 
	int32 numStrings, BRect rectArray[], font_metric_mode mode, escapement_delta deltaArray[])
{
	if (!fStyle || !charArray || !lengthArray|| numStrings <= 0 || !rectArray || !deltaArray)
		return false;

	FT_Face face = fStyle->GetFTFace();
	if (!face)
		return false;

	FT_Set_Char_Size(face, 0, int32(fSize * 64), 72, 72);

	return true;
}


// StringWidth
float
ServerFont::StringWidth(const char* string, int32 numBytes) const
{
	if (!fStyle || !string || numBytes <= 0)
		return 0.0;

	FT_Face face = fStyle->GetFTFace();
	if (!face)
		return 0.0;

	float width = 0.0;

	int32 convertedLength = numBytes * 2;
	char* convertedBuffer = new char[convertedLength];

	int32 state = 0;
	status_t ret;
	if ((ret = convert_from_utf8(B_UNICODE_CONVERSION, 
								 string, &numBytes,
								 convertedBuffer, &convertedLength,
								 &state, B_SUBSTITUTE)) >= B_OK
		&& (ret = swap_data(B_INT16_TYPE, convertedBuffer, convertedLength,
							B_SWAP_BENDIAN_TO_HOST)) >= B_OK) {

		uint16* glyphIndex = (uint16*)convertedBuffer;
		// just to be sure
		int numChars = convertedLength / sizeof(uint16);

		for (int i = 0; i < numChars; i++) {
			FT_Load_Char(face, glyphIndex[i], FT_LOAD_NO_BITMAP);
			width += (float)face->glyph->metrics.horiAdvance / 64.0;
		}
	}
	delete[] convertedBuffer;

	return width;
}

/*! 
	\brief Returns a BRect which encloses the entire font
	\return A BRect which encloses the entire font
*/
BRect
ServerFont::BoundingBox()
{
	// TODO: fBounds is nowhere calculated!
	return fBounds;
}

/*! 
	\brief Obtains the height values for characters in the font in its current state
	\param fh pointer to a font_height object to receive the values for the font
*/
void
ServerFont::Height(font_height *fh) const
{
	if (fh && fStyle)
		*fh = fStyle->GetHeight(fSize);
}

// TruncateString
void
ServerFont::TruncateString(BString* inOut, uint32 mode, float width) const
{
	if (inOut) {
		// the width of the "â€¦" glyph
		float ellipsisWidth = StringWidth(B_UTF8_ELLIPSIS, 3);
		const char* string = inOut->String();
		int32 length = strlen(string);
		// temporary array to hold result
		char* result = new char[length + 3];
		// count the individual glyphs
		int32 numChars = UTF8CountChars(string, length);
		// get the escapement of each glyph in font units
		float* escapementArray = new float[numChars];
		static escapement_delta delta = (escapement_delta){ 0.0, 0.0 };
		GetEscapements(string, numChars, escapementArray, delta);

		truncate_string(string, mode, width, result,
						escapementArray, fSize, ellipsisWidth, length, numChars);

		inOut->SetTo(result);

		delete[] escapementArray;
		delete[] result;
	}
}

// _SetStyle
void
ServerFont::_SetStyle(FontStyle* style)
{
	if (style != fStyle) {
		// detach from old style
		if (fStyle)
			fStyle->RemoveDependent();

		fStyle = style;
	
		// attach to new style
		if (fStyle)
			fStyle->AddDependent();
	}
}
