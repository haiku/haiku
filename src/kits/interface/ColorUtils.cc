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
//	File Name:		ColotUtils.cc
//	Author:			DarkWyrm (bpmagic@columbus.rr.com)
//	Description:	Useful global functions for working with rgb_color structures
//------------------------------------------------------------------------------
#include "ColorUtils.h"
#include <stdlib.h>

/*!
	\brief An approximation of 31/255, which is needed for converting from 32-bit
		colors to 16-bit and 15-bit.
*/
#define RATIO_8_TO_5_BIT .121568627451

/*!
	\brief An approximation of 63/255, which is needed for converting from 32-bit
		colors to 16-bit.
*/
#define RATIO_8_TO_6_BIT .247058823529

/*!
	\brief An approximation of 255/31, which is needed for converting from 16-bit
		and 15-bit colors to 32-bit.
*/
#define RATIO_5_TO_8_BIT 8.22580645161

/*!
	\brief An approximation of 255/63, which is needed for converting from 16-bit
		colors to 32-bit.
*/
#define RATIO_6_TO_8_BIT 4.04761904762

/*!
	\brief Function for easy assignment of values to rgb_color objects
	\param col Pointer to an rgb_color
	\param r red value
	\param g green value
	\param b blue value
	\param a alpha value, defaults to 255
	
	This function will do nothing if given a NULL color pointer.
*/
void SetRGBColor(rgb_color *col,uint8 r, uint8 g, uint8 b, uint8 a)
{
	if(col)
	{
		col->red=r;
		col->green=g;
		col->blue=b;
		col->alpha=a;
	}
}

/*!
	\brief Function for easy conversion of 15-bit colors to 32-bit
	\param col Pointer to an rgb_color.
	\param color RGBA16 color
	
	This function will do nothing if passed a NULL 32-bit color.
*/
void SetRGBColor15(rgb_color *col,uint16 color)
{
	if(!col)
		return;
	
	uint16 r16,g16,b16;
	
	// alpha's the easy part
	col->alpha=(color & 0x8000)?255:0;

	r16= (color >> 10) & 31;
	g16= (color >> 5) & 31;
	b16= color & 31;

	col->red=uint8(r16 * RATIO_5_TO_8_BIT);
	col->green=uint8(g16 * RATIO_5_TO_8_BIT);
	col->blue=uint8(b16 * RATIO_5_TO_8_BIT);
}

/*!
	\brief Function for easy conversion of 16-bit colors to 32-bit
	\param col Pointer to an rgb_color.
	\param color RGB16 color
	
	This function will do nothing if passed a NULL 32-bit color.
*/
void SetRGBColor16(rgb_color *col,uint16 color)
{
	if(!col)
		return;
	
	uint16 r16,g16,b16;
	
	// alpha's the easy part
	col->alpha=0;

	r16= (color >> 11) & 31;
	g16= (color >> 5) & 63;
	b16= color & 31;

	col->red=uint8(r16 * RATIO_5_TO_8_BIT);
	col->green=uint8(g16 * RATIO_6_TO_8_BIT);
	col->blue=uint8(b16 * RATIO_5_TO_8_BIT);
}

/*!
	\brief Function for easy conversion of 32-bit integer colors to an rgb_color structure
	\param col Pointer to an rgb_color.
	\param color 32-bit color as an integer
	
	This function will do nothing if passed a NULL 32-bit color.
*/
void SetRGBColor(rgb_color *col,uint32 color)
{
	if(!col)
		return;

	int8 *p8=(int8*)&color;
	col->blue=p8[0];
	col->red=p8[1];
	col->green=p8[2];
	col->alpha=p8[3];
}

/*!
	\brief Finds the index of the closest matching color in a rgb_color palette array
	\param palette Array of 256 rgb_color objects
	\param color Color to match
	\return Index of the closest matching color
	
	Note that passing a NULL palette will always return 0 and passing an array of less
	than 256 rgb_colors will cause a crash.
*/
uint8 FindClosestColor(const rgb_color *palette, rgb_color color)
{
	if(!palette)
		return 0;
	
	uint16 cindex=0,cdelta=765,delta=765;
	
	for(uint16 i=0;i<256;i++)
	{
		const rgb_color *c=&(palette[i]);
		delta=abs(c->red-color.red)+abs(c->green-color.green)+
			abs(c->blue-color.blue);

		if(delta==0)
		{
			cindex=i;
			break;
		}

		if(delta<cdelta)
		{
			cindex=i;
			cdelta=delta;
		}
	}

	return (uint8)cindex;
}

/*!
	\brief Constructs a RGBA15 color which best matches a given 32-bit color
	\param color Color to match
	\return The closest matching color's value
	
	Format is ARGB, 1:5:5:5
*/
uint16 FindClosestColor15(rgb_color color)
{
	uint16 r16,g16,b16;
	uint16 color16=0;
	
	r16=uint16(color.red * RATIO_8_TO_5_BIT);
	g16=uint16(color.green * RATIO_8_TO_5_BIT);
	b16=uint16(color.blue * RATIO_8_TO_5_BIT);

	// start with alpha value
	color16=(color.alpha>127)?0x8000:0;

	color16 |= r16 << 10;
	color16 |= g16 << 5;
	color16 |= b16;

	return color16;
}

/*!
	\brief Constructs a RGB16 color which best matches a given 32-bit color
	\param color Color to match
	\return The closest matching color's value
	
	Format is RGB, 5:6:5
*/
uint16 FindClosestColor16(rgb_color color)
{
	uint16 r16,g16,b16;
	uint16 color16=0;
	
	r16=uint16(color.red * RATIO_8_TO_5_BIT);
	g16=uint16(color.green * RATIO_8_TO_6_BIT);
	b16=uint16(color.blue * RATIO_8_TO_5_BIT);

	color16 |= r16 << 11;
	color16 |= g16 << 5;
	color16 |= b16;

	return color16;
}

// Function which could be used to . Position is
//  Any number outside
// this range will cause the function to fail and return the color (0,0,0,0)
// Alpha components are included in the calculations. 0 yields color #1
// and 1 yields color #2.

/*!
	\brief Function mostly for calculating gradient colors
	\param col Start color
	\param col2 End color
	\param position A floating point number such that 0.0 <= position <= 1.0. 0.0 results in the
		start color and 1.0 results in the end color.
	\return The blended color. If an invalid position was given, {0,0,0,0} is returned.
*/
rgb_color MakeBlendColor(rgb_color col, rgb_color col2, float position)
{
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

	return newcol;
}
