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
#include "FontFamily.h"
#include "ServerFont.h"


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
	fstyle=style;
	fsize=size;
	frotation=rotation;
	fshear=shear;
	fflags=flags;
	fspacing=spacing;
	fdirection=B_FONT_LEFT_TO_RIGHT;
	fface=B_REGULAR_FACE;
	ftruncate=B_TRUNCATE_END;
	fencoding=B_UNICODE_UTF8;
	fbounds.Set(0,0,0,0);
	if(fstyle)
		fstyle->AddDependent();
}

ServerFont::ServerFont(void)
{
	fstyle=NULL;
	fsize=0.0;
	frotation=0.0;
	fshear=90.0;
	fflags=0;
	fspacing=B_STRING_SPACING;
	fdirection=B_FONT_LEFT_TO_RIGHT;
	fface=B_REGULAR_FACE;
	ftruncate=B_TRUNCATE_END;
	fencoding=B_UNICODE_UTF8;
	fbounds.Set(0,0,0,0);
}

/*! 
	\brief Copy Constructor
	\param font ServerFont to copy
*/
ServerFont::ServerFont(const ServerFont &font)
{
	fstyle=font.fstyle;
	fsize=font.fsize;
	frotation=font.frotation;
	fshear=font.fshear;
	fflags=font.fflags;
	fspacing=font.fspacing;
	fdirection=font.fdirection;
	fface=font.fface;
	ftruncate=font.ftruncate;
	fencoding=font.fencoding;
	fbounds.Set(0,0,0,0);
	if(fstyle)
		fstyle->AddDependent();
}

/*! 
	\brief Removes itself as a dependency of its owning style.
*/
ServerFont::~ServerFont(void)
{
	if(fstyle)
		fstyle->RemoveDependent();
}

/*! 
	\brief Returns a copy of the specified font
	\param The font to copy from.
	\return A copy of the specified font
*/
ServerFont& ServerFont::operator=(const ServerFont& font){
	fstyle		= font.fstyle;
	fsize		= font.fsize;
	frotation	= font.frotation;
	fshear		= font.fshear;
	fflags		= font.fflags;
	fspacing	= font.fspacing;
	fdirection	= B_FONT_LEFT_TO_RIGHT;
	fface		= B_REGULAR_FACE;
	ftruncate	= B_TRUNCATE_END;
	fencoding	= B_UNICODE_UTF8;
	fbounds.Set(0,0,0,0);
	if(fstyle)
		fstyle->AddDependent();
	return *this;
}

/*! 
	\brief Returns the number of strikes in the font
	\return The number of strikes in the font
*/
int32 ServerFont::CountTuned(void)
{
	if(fstyle)
		fstyle->TunedCount();

	return 0;
}

/*! 
	\brief Returns the file format of the font. Currently unimplemented.
	\return B_TRUETYPE_WINDOWS
*/
font_file_format ServerFont::FileFormat(void)
{
	// TODO: implement
	return 	B_TRUETYPE_WINDOWS;
}

/*! 
	\brief Returns a BRect which encloses the entire font
	\return A BRect which encloses the entire font
*/
BRect ServerFont::BoundingBox(void)
{
	return fbounds;
}

/*! 
	\brief Obtains the height values for characters in the font in its current state
	\param fh pointer to a font_height object to receive the values for the font
*/
void ServerFont::Height(font_height *fh)
{
	fh->ascent=fheight.ascent;
	fh->descent=fheight.descent;
	fh->leading=fheight.leading;
}
/*
 @log
	* added ServerFont::operator=(const ServerFont& font).
*/
