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

RGBColor::RGBColor(uint8 r, uint8 g, uint8 b, uint8 a=255)
{
	SetColor(r,g,b,a);
}

RGBColor::RGBColor(int r, int g, int b, int a=255)
{
	SetColor(r,g,b,a);
}

RGBColor::RGBColor(rgb_color col)
{
	SetColor(col);
}

RGBColor::RGBColor(uint16 col)
{
	SetColor(col);
}

RGBColor::RGBColor(uint8 col)
{
	SetColor(col);
}

RGBColor::RGBColor(const RGBColor &col)
{
	color32=col.color32;
	color16=col.color16;
	color8=col.color8;
}

RGBColor::RGBColor(void)
{
	SetColor(0,0,0,0);
}

uint8 RGBColor::GetColor8(void)
{
	return color8;
}

uint16 RGBColor::GetColor16(void)
{
	return color16;
}

rgb_color RGBColor::GetColor32(void)
{
	return color32;
}

void RGBColor::SetColor(uint8 r, uint8 g, uint8 b, uint8 a=255)
{
	color32.red=r;
	color32.green=g;
	color32.blue=b;
	color32.alpha=a;
}

void RGBColor::SetColor(int r, int g, int b, int a=255)
{
	color32.red=(uint8)r;
	color32.green=(uint8)g;
	color32.blue=(uint8)b;
	color32.alpha=(uint8)a;
}

void RGBColor::SetColor(uint16 col16)
{
	color16=col16;
	
	// TODO: convert and set the 32-bit and 8-bit values
}

void RGBColor::SetColor(uint8 col8)
{
	// Pared-down version from what is used in the app_server to
	// eliminate some dependencies
	color8=col8;
	
	// TODO: convert and set the 32-bit and 16-bit values
}

void RGBColor::SetColor(const rgb_color &color)
{
	// Pared-down version from what is used in the app_server to
	// eliminate some dependencies
	color32=color;

	
	// TODO: convert and set the 16-bit and 8-bit values
}

void RGBColor::SetColor(const RGBColor &col)
{
	color32=col.color32;
	color16=col.color16;
	color8=col.color8;
}


RGBColor & RGBColor::operator=(const RGBColor &col)
{
	color32=col.color32;
	color16=col.color16;
	color8=col.color8;
	return *this;
}

RGBColor & RGBColor::operator=(const rgb_color &col)
{
	color32=col;

	// TODO: convert and set the 16-bit and 8-bit values
	return *this;
}

/*!	\brief Returns a color blended between the object's value and 
	another color.
	
	\param The other color to be blended with.
	\param A weighted percentage of the second color to use. 0 <= value <= 1.0
	\return The blended color
*/
RGBColor RGBColor::MakeBlendColor(RGBColor color, float position)
{
	rgb_color col=color32;
	rgb_color col2=color.color32;
	
	rgb_color newcol={0,0,0,0};
	float mod=0;
	int16 delta;
	if(position<0 || position>1)
		return newcol;

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

void RGBColor::PrintToStream(void)
{
	printf("RGBColor (%u,%u,%u,%u)\n", color32.red,color32.green,color32.blue,color32.alpha);
}

bool RGBColor::operator==(const rgb_color &col)
{
	return (color32.red==col.red && color32.green==col.green
		&& color32.blue==col.blue)?true:false;
}

bool RGBColor::operator==(const RGBColor &col)
{
	return (color32.red==col.color32.red && color32.green==col.color32.green
		&& color32.blue==col.color32.blue)?true:false;
}
