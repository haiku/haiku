//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		ServerFont.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Shadow BFont class
//  
//------------------------------------------------------------------------------
#include "ServerFont.h"
#include "FontServer.h"

/*! 
	\brief Constructor
	\param style Style object to which the ServerFont belongs
	\param size Character size in points
	\param rotation Rotation in degrees
	\param shear Shear (slant) in degrees. 45 <= shear <= 135
	\param flags Style flags as defined in <Font.h>
	\param spacing String spacing flag as defined in <Font.h>
*/
ServerFont::ServerFont(FontStyle *style, float size, float rotation, float shear,
	uint16 flags, uint8 spacing)
{
	fStyle=style;
	fSize=size;
	frotation=rotation;
	fshear=shear;
	fFlags=flags;
	fSpacing=spacing;
	fDirection=B_FONT_LEFT_TO_RIGHT;
	fFace=B_REGULAR_FACE;
	fEncoding=B_UNICODE_UTF8;
	fBounds.Set(0,0,0,0);
	if(fStyle)
		fStyle->AddDependent();
}

ServerFont::ServerFont(void)
{
	fStyle=NULL;
	fSize=0.0;
	frotation=0.0;
	fshear=90.0;
	fFlags=0;
	fSpacing=B_STRING_SPACING;
	fDirection=B_FONT_LEFT_TO_RIGHT;
	fFace=B_REGULAR_FACE;
	fEncoding=B_UNICODE_UTF8;
	fBounds.Set(0,0,0,0);
}

/*! 
	\brief Copy Constructor
	\param font ServerFont to copy
*/
ServerFont::ServerFont(const ServerFont &font)
{
	fStyle=font.fStyle;
	fSize=font.fSize;
	frotation=font.frotation;
	fshear=font.fshear;
	fFlags=font.fFlags;
	fSpacing=font.fSpacing;
	fDirection=font.fDirection;
	fFace=font.fFace;
	fEncoding=font.fEncoding;
	fBounds.Set(0,0,0,0);
	if(fStyle)
		fStyle->AddDependent();
}

/*! 
	\brief Removes itself as a dependency of its owning style.
*/
ServerFont::~ServerFont(void)
{
	if(fStyle)
		fStyle->RemoveDependent();
}

/*! 
	\brief Returns a copy of the specified font
	\param The font to copy from.
	\return A copy of the specified font
*/
ServerFont& ServerFont::operator=(const ServerFont& font){
	fStyle		= font.fStyle;
	fSize		= font.fSize;
	frotation	= font.frotation;
	fshear		= font.fshear;
	fFlags		= font.fFlags;
	fSpacing	= font.fSpacing;
	fDirection	= B_FONT_LEFT_TO_RIGHT;
	fFace		= B_REGULAR_FACE;
	fEncoding	= B_UNICODE_UTF8;
	fBounds.Set(0,0,0,0);
	if(fStyle)
		fStyle->AddDependent();
	return *this;
}

/*! 
	\brief Returns the number of strikes in the font
	\return The number of strikes in the font
*/
int32 ServerFont::CountTuned(void)
{
	if(fStyle)
		fStyle->TunedCount();

	return 0;
}

/*! 
	\brief Returns the file format of the font. Currently unimplemented.
	\return B_TRUETYPE_WINDOWS
*/
font_file_format ServerFont::FileFormat(void)
{
	// TODO: implement ServerFont::FileFormat
	return B_TRUETYPE_WINDOWS;
}

/*! 
	\brief Returns a BRect which encloses the entire font
	\return A BRect which encloses the entire font
*/
BRect ServerFont::BoundingBox(void)
{
	return fBounds;
}

const char *ServerFont::GetStyle(void) const
{
	return fStyle->Name();
}

const char *ServerFont::GetFamily(void) const
{
	return fStyle->Family()->Name();
}

/*! 
	\brief Obtains the height values for characters in the font in its current state
	\param fh pointer to a font_height object to receive the values for the font
*/
void ServerFont::Height(font_height *fh)
{
	if(!fh)
		return;
	
	if(fStyle)
		*fh=fStyle->GetHeight(fSize);
}

/*!
	\brief Sets the ServerFont instance to whatever font is specified
	\param familyID ID number of the family to set
	\param styleID ID number of the style to set
	\return B_OK if successful, B_ERROR if not
*/
status_t ServerFont::SetFamilyAndStyle(const uint16 &familyID,const uint16 &styleID)
{
	fontserver->Lock();
	FontStyle *sty=fontserver->GetStyle(familyID,styleID);
	fontserver->Unlock();
	if(!sty)
		return B_ERROR;
	
	fStyle=sty;
	return B_OK;
}

/*!
	\brief Sets the ServerFont instance to whatever font is specified
	\param fontID the combination of family and style ID numbers
	\return B_OK if successful, B_ERROR if not
*/
status_t ServerFont::SetFamilyAndStyle(const uint32 &fontID)
{
	uint16 style = fontID & 0xFFFF;
	uint16 family = (fontID & 0xFFFF0000) >> 16;
	
	fontserver->Lock();
	FontStyle *sty=fontserver->GetStyle(family,style);
	fontserver->Unlock();
	if(!sty)
		return B_ERROR;
	
	fStyle=sty;
	return B_OK;
}

/*!
	\brief Gets the ID values for the ServerFont instance in one shot
	\return the combination of family and style ID numbers
*/
uint32 ServerFont::GetFamilyAndStyle(void)
{
	uint32 famsty=0;
	
	famsty|=FamilyID() << 16;
	famsty|=StyleID();
	
	return famsty;
}

