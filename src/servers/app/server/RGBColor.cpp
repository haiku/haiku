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
//	File Name:		RGBColor.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Color encapsulation class for the app_server
//  
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// Local Includes --------------------------------------------------------------
#include "RGBColor.h"
#include "SystemPalette.h"
#include <ColorUtils.h>

/*!
	\brief Create an RGBColor from specified values
	\param red red
	\param green green
	\param blue blue
	\param alpha alpha, defaults to 255
*/
RGBColor::RGBColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	SetColor(r,g,b,a);
}

/*!
	\brief Create an RGBColor from specified values
	\param red red
	\param green green
	\param blue blue
	\param alpha alpha, defaults to 255
*/
RGBColor::RGBColor(int r, int g, int b, int a)
{
	SetColor(r,g,b,a);
}

/*!
	\brief Create an RGBColor from an rgb_color
	\param col color to initialize from
*/
RGBColor::RGBColor(const rgb_color &col)
{
	SetColor(col);
}

/*!
	\brief Create an RGBColor from a 16-bit RGBA color
	\param col color to initialize from
*/
RGBColor::RGBColor(uint16 col)
{
	SetColor(col);
}

/*!
	\brief Create an RGBColor from an index color
	\param col color to initialize from
*/
RGBColor::RGBColor(uint8 col)
{
	SetColor(col);
}

/*!
	\brief Copy Contructor
	\param col color to initialize from
*/
RGBColor::RGBColor(const RGBColor &col)
{
	color32=col.color32;
	color16=col.color16;
	color8=col.color8;
	update8=col.update8;
	update16=col.update16;
}

/*!
	\brief Create an RGBColor with the values(0,0,0,0)
*/
RGBColor::RGBColor(void)
{
	SetColor(0,0,0,0);
	update8=update16=false;
}

/*!
	\brief Returns the color as the closest 8-bit color in the palette
	\return The palette index for the current color
*/
uint8 RGBColor::GetColor8(void)
{
	if(update8)
	{
		color8=FindClosestColor(system_palette, color32);
		update8=false;
	}

	return color8;
}

/*!
	\brief Returns the color as the closest 15-bit color
	\return 15-bit value of the current color plus 1-bit alpha
*/
uint16 RGBColor::GetColor15(void)
{
	if(update15)
	{
		color15=FindClosestColor15(color32);
		update15=false;
	}
	
	return color15;
}

/*!
	\brief Returns the color as the closest 16-bit color
	\return 16-bit value of the current color
*/
uint16 RGBColor::GetColor16(void)
{
	if(update16)
	{
		color16=FindClosestColor16(color32);
		update16=false;
	}
	
	return color16;
}

/*!
	\brief Returns the color as a 32-bit color
	\return current color, including alpha
*/
rgb_color RGBColor::GetColor32(void) const
{
	return color32;
}

/*!
	\brief Set the object to specified values
	\param red red
	\param green green
	\param blue blue
	\param alpha alpha, defaults to 255
*/
void RGBColor::SetColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	color32.red=r;
	color32.green=g;
	color32.blue=b;
	color32.alpha=a;
	
	update8=update16=true;
}

/*!
	\brief Set the object to specified values
	\param red red
	\param green green
	\param blue blue
	\param alpha alpha, defaults to 255
*/
void RGBColor::SetColor(int r, int g, int b, int a)
{
	color32.red=(uint8)r;
	color32.green=(uint8)g;
	color32.blue=(uint8)b;
	color32.alpha=(uint8)a;

	update8=update16=true;
}

/*!
	\brief Set the object to specified value
	\param col16 color to copy
*/
void RGBColor::SetColor(uint16 col16)
{
	color16=col16;
	SetRGBColor(&color32,col16);

	update8=true;
	update16=false;
}

/*!
	\brief Set the object to specified index in the palette
	\param col8 color to copy
*/
void RGBColor::SetColor(uint8 col8)
{
	color8=col8;
	color32=system_palette[col8];
	
	update8=false;
	update16=true;
}

/*!
	\brief Set the object to specified color
	\param color color to copy
*/
void RGBColor::SetColor(const rgb_color &color)
{
	color32=color;
	update8=update16=true;
}

/*!
	\brief Set the object to specified color
	\param col color to copy
*/
void RGBColor::SetColor(const RGBColor &col)
{
	color32=col.color32;
	color16=col.color16;
	color8=col.color8;
	update8=col.update8;
	update16=col.update16;
}

/*!
	\brief Set the object to specified color
	\param col color to copy
*/
const RGBColor & RGBColor::operator=(const RGBColor &col)
{
	color32=col.color32;
	color16=col.color16;
	color8=col.color8;
	update8=col.update8;
	update16=col.update16;
	return *this;
}

/*!
	\brief Set the object to specified color
	\param col color to copy
*/
const RGBColor & RGBColor::operator=(const rgb_color &col)
{
	color32=col;
	update8=update16=true;

	return *this;
}

/*!
	\brief Returns a color blended between the object's value and 
	another color.
	
	\param color The other color to be blended with.
	\param position A weighted percentage of the second color to use. 0 <= value <= 1.0
	\return The blended color
	
	If the position passed to this function is invalid, the starting
	color will be returned.
*/
RGBColor RGBColor::MakeBlendColor(const RGBColor &color, const float &position)
{
	rgb_color col=color32;
	rgb_color col2=color.color32;
	
	rgb_color newcol={0,0,0,0};
	float mod=0;
	int16 delta;
	if(position<0 || position>1)
		return *this;

	delta=int16(col2.red)-int16(col.red);
	mod=col.red + (position * delta);
	newcol.red=uint8(mod);
	if(mod>255 )
		newcol.red=255;
	if(mod<0 )
		newcol.red=0;

delta=int16(col2.green)-int16(col.green);
	mod=col.green + (position * delta);
	newcol.green=uint8(mod);
	if(mod>255 )
		newcol.green=255;
	if(mod<0 )
		newcol.green=0;

	delta=int16(col2.blue)-int16(col.blue);
	mod=col.blue + (position * delta);
	newcol.blue=uint8(mod);
	if(mod>255 )
		newcol.blue=255;
	if(mod<0 )
		newcol.blue=0;

	delta=int8(col2.alpha)-int8(col.alpha);
	mod=col.alpha + (position * delta);
	newcol.alpha=uint8(mod);
	if(mod>255 )
		newcol.alpha=255;
	if(mod<0 )
		newcol.alpha=0;

	return RGBColor(newcol);
}

/*!
	\brief Prints the 32-bit values of the color to standard out
*/
void RGBColor::PrintToStream(void) const
{
	printf("RGBColor (%u,%u,%u,%u)\n", color32.red,color32.green,color32.blue,color32.alpha);
}

/*!
	\brief Overloaded comaparison
	\return true if all color elements are exactly equal
*/
bool RGBColor::operator==(const rgb_color &col)
{
	return (color32.red==col.red && color32.green==col.green
		&& color32.blue==col.blue && color32.alpha==col.alpha)?true:false;
}

/*!
	\brief Overloaded comaparison
	\return true if all color elements are exactly equal
*/
bool RGBColor::operator==(const RGBColor &col)
{
	return (color32.red==col.color32.red && color32.green==col.color32.green
		&& color32.blue==col.color32.blue 
		&& color32.alpha==col.color32.alpha)?true:false;
}
