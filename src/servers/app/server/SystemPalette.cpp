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
//	File Name:		SystemPalette.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	One global function to generate the palette which is 
//					the default BeOS System palette and the variable to go with it	
//  
//------------------------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "SystemPalette.h"

/*!
	\var rgb_color system_palette[256]
	\brief The global array of colors for the system palette.
	
	Whenever the system's color palette is referenced, this is the variable used.
*/
const rgb_color system_palette[] = {
 {   0,   0,   0, 255 }, {   8,   8,   8, 255 }, {  16,  16,  16, 255 },
 {  24,  24,  24, 255 }, {  32,  32,  32, 255 }, {  40,  40,  40, 255 },
 {  48,  48,  48, 255 }, {  56,  56,  56, 255 }, {  64,  64,  64, 255 },
 {  72,  72,  72, 255 }, {  80,  80,  80, 255 }, {  88,  88,  88, 255 },
 {  96,  96,  96, 255 }, { 104, 104, 104, 255 }, { 112, 112, 112, 255 },
 { 120, 120, 120, 255 }, { 128, 128, 128, 255 }, { 136, 136, 136, 255 },
 { 144, 144, 144, 255 }, { 152, 152, 152, 255 }, { 160, 160, 160, 255 },
 { 168, 168, 168, 255 }, { 176, 176, 176, 255 }, { 184, 184, 184, 255 },
 { 192, 192, 192, 255 }, { 200, 200, 200, 255 }, { 208, 208, 208, 255 },
 { 216, 216, 216, 255 }, { 224, 224, 224, 255 }, { 232, 232, 232, 255 },
 { 240, 240, 240, 255 }, { 248, 248, 248, 255 }, {   0,   0, 255, 255 },
 {   0,   0, 229, 255 }, {   0,   0, 204, 255 }, {   0,   0, 179, 255 },
 {   0,   0, 154, 255 }, {   0,   0, 129, 255 }, {   0,   0, 105, 255 },
 {   0,   0,  80, 255 }, {   0,   0,  55, 255 }, {   0,   0,  30, 255 },
 { 255,   0,   0, 255 }, { 228,   0,   0, 255 }, { 203,   0,   0, 255 },
 { 178,   0,   0, 255 }, { 153,   0,   0, 255 }, { 128,   0,   0, 255 },
 { 105,   0,   0, 255 }, {  80,   0,   0, 255 }, {  55,   0,   0, 255 },
 {  30,   0,   0, 255 }, {   0, 255,   0, 255 }, {   0, 228,   0, 255 },
 {   0, 203,   0, 255 }, {   0, 178,   0, 255 }, {   0, 153,   0, 255 },
 {   0, 128,   0, 255 }, {   0, 105,   0, 255 }, {   0,  80,   0, 255 },
 {   0,  55,   0, 255 }, {   0,  30,   0, 255 }, {   0, 152,  51, 255 },
 { 255, 255, 255, 255 }, { 203, 255, 255, 255 }, { 203, 255, 203, 255 },
 { 203, 255, 152, 255 }, { 203, 255, 102, 255 }, { 203, 255,  51, 255 },
 { 203, 255,   0, 255 }, { 152, 255, 255, 255 }, { 152, 255, 203, 255 },
 { 152, 255, 152, 255 }, { 152, 255, 102, 255 }, { 152, 255,  51, 255 },
 { 152, 255,   0, 255 }, { 102, 255, 255, 255 }, { 102, 255, 203, 255 },
 { 102, 255, 152, 255 }, { 102, 255, 102, 255 }, { 102, 255,  51, 255 },
 { 102, 255,   0, 255 }, {  51, 255, 255, 255 }, {  51, 255, 203, 255 },
 {  51, 255, 152, 255 }, {  51, 255, 102, 255 }, {  51, 255,  51, 255 },
 {  51, 255,   0, 255 }, { 255, 152, 255, 255 }, { 255, 152, 203, 255 },
 { 255, 152, 152, 255 }, { 255, 152, 102, 255 }, { 255, 152,  51, 255 },
 { 255, 152,   0, 255 }, {   0, 102, 255, 255 }, {   0, 102, 203, 255 },
 { 203, 203, 255, 255 }, { 203, 203, 203, 255 }, { 203, 203, 152, 255 },
 { 203, 203, 102, 255 }, { 203, 203,  51, 255 }, { 203, 203,   0, 255 },
 { 152, 203, 255, 255 }, { 152, 203, 203, 255 }, { 152, 203, 152, 255 },
 { 152, 203, 102, 255 }, { 152, 203,  51, 255 }, { 152, 203,   0, 255 },
 { 102, 203, 255, 255 }, { 102, 203, 203, 255 }, { 102, 203, 152, 255 },
 { 102, 203, 102, 255 }, { 102, 203,  51, 255 }, { 102, 203,   0, 255 },
 {  51, 203, 255, 255 }, {  51, 203, 203, 255 }, {  51, 203, 152, 255 },
 {  51, 203, 102, 255 }, {  51, 203,  51, 255 }, {  51, 203,   0, 255 },
 { 255, 102, 255, 255 }, { 255, 102, 203, 255 }, { 255, 102, 152, 255 },
 { 255, 102, 102, 255 }, { 255, 102,  51, 255 }, { 255, 102,   0, 255 },
 {   0, 102, 152, 255 }, {   0, 102, 102, 255 }, { 203, 152, 255, 255 },
 { 203, 152, 203, 255 }, { 203, 152, 152, 255 }, { 203, 152, 102, 255 },
 { 203, 152,  51, 255 }, { 203, 152,   0, 255 }, { 152, 152, 255, 255 },
 { 152, 152, 203, 255 }, { 152, 152, 152, 255 }, { 152, 152, 102, 255 },
 { 152, 152,  51, 255 }, { 152, 152,   0, 255 }, { 102, 152, 255, 255 },
 { 102, 152, 203, 255 }, { 102, 152, 152, 255 }, { 102, 152, 102, 255 },
 { 102, 152,  51, 255 }, { 102, 152,   0, 255 }, {  51, 152, 255, 255 },
 {  51, 152, 203, 255 }, {  51, 152, 152, 255 }, {  51, 152, 102, 255 },
 {  51, 152,  51, 255 }, {  51, 152,   0, 255 }, { 230, 134,   0, 255 },
 { 255,  51, 203, 255 }, { 255,  51, 152, 255 }, { 255,  51, 102, 255 },
 { 255,  51,  51, 255 }, { 255,  51,   0, 255 }, {   0, 102,  51, 255 },
 {   0, 102,   0, 255 }, { 203, 102, 255, 255 }, { 203, 102, 203, 255 },
 { 203, 102, 152, 255 }, { 203, 102, 102, 255 }, { 203, 102,  51, 255 },
 { 203, 102,   0, 255 }, { 152, 102, 255, 255 }, { 152, 102, 203, 255 },
 { 152, 102, 152, 255 }, { 152, 102, 102, 255 }, { 152, 102,  51, 255 },
 { 152, 102,   0, 255 }, { 102, 102, 255, 255 }, { 102, 102, 203, 255 },
 { 102, 102, 152, 255 }, { 102, 102, 102, 255 }, { 102, 102,  51, 255 },
 { 102, 102,   0, 255 }, {  51, 102, 255, 255 }, {  51, 102, 203, 255 },
 {  51, 102, 152, 255 }, {  51, 102, 102, 255 }, {  51, 102,  51, 255 },
 {  51, 102,   0, 255 }, { 255,   0, 255, 255 }, { 255,   0, 203, 255 },
 { 255,   0, 152, 255 }, { 255,   0, 102, 255 }, { 255,   0,  51, 255 },
 { 255, 175,  19, 255 }, {   0,  51, 255, 255 }, {   0,  51, 203, 255 },
 { 203,  51, 255, 255 }, { 203,  51, 203, 255 }, { 203,  51, 152, 255 },
 { 203,  51, 102, 255 }, { 203,  51,  51, 255 }, { 203,  51,   0, 255 },
 { 152,  51, 255, 255 }, { 152,  51, 203, 255 }, { 152,  51, 152, 255 },
 { 152,  51, 102, 255 }, { 152,  51,  51, 255 }, { 152,  51,   0, 255 },
 { 102,  51, 255, 255 }, { 102,  51, 203, 255 }, { 102,  51, 152, 255 },
 { 102,  51, 102, 255 }, { 102,  51,  51, 255 }, { 102,  51,   0, 255 },
 {  51,  51, 255, 255 }, {  51,  51, 203, 255 }, {  51,  51, 152, 255 },
 {  51,  51, 102, 255 }, {  51,  51,  51, 255 }, {  51,  51,   0, 255 },
 { 255, 203, 102, 255 }, { 255, 203, 152, 255 }, { 255, 203, 203, 255 },
 { 255, 203, 255, 255 }, {   0,  51, 152, 255 }, {   0,  51, 102, 255 },
 {   0,  51,  51, 255 }, {   0,  51,   0, 255 }, { 203,   0, 255, 255 },
 { 203,   0, 203, 255 }, { 203,   0, 152, 255 }, { 203,   0, 102, 255 },
 { 203,   0,  51, 255 }, { 255, 227,  70, 255 }, { 152,   0, 255, 255 },
 { 152,   0, 203, 255 }, { 152,   0, 152, 255 }, { 152,   0, 102, 255 },
 { 152,   0,  51, 255 }, { 152,   0,   0, 255 }, { 102,   0, 255, 255 },
 { 102,   0, 203, 255 }, { 102,   0, 152, 255 }, { 102,   0, 102, 255 },
 { 102,   0,  51, 255 }, { 102,   0,   0, 255 }, {  51,   0, 255, 255 },
 {  51,   0, 203, 255 }, {  51,   0, 152, 255 }, {  51,   0, 102, 255 },
 {  51,   0,  51, 255 }, {  51,   0,   0, 255 }, { 255, 203,  51, 255 },
 { 255, 203,   0, 255 }, { 255, 255,   0, 255 }, { 255, 255,  51, 255 },
 { 255, 255, 102, 255 }, { 255, 255, 152, 255 }, { 255, 255, 203, 255 },
 { 255, 255, 255, 255 }
};

/*!
	\brief Takes a palette array and places the BeOS System	palette in it.
	\param palette 256-element rgb_color array
*/
void GenerateSystemPalette(rgb_color *palette)
{
	int i,j,index=0;
	int indexvals1[]={ 255,229,204,179,154,129,105,80,55,30 },
		indexvals2[]={ 255,203,152,102,51,0 };
		// ff, cb, 98, 66, 33
	rgb_color *currentcol;
	
	// Grays 0,0,0 -> 248,248,248 by 8's	
	for(i=0; i<=248; i+=8,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=i;
		currentcol->green=i;
		currentcol->blue=i;
		currentcol->alpha=255;
	}
	
	// Blues, following indexvals1
	for(i=0; i<10; i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=0;
		currentcol->green=0;
		currentcol->blue=indexvals1[i];
		currentcol->alpha=255;
	}

	// Reds, following indexvals1 - 1
	for(i=0; i<10; i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=indexvals1[i] - 1;
		currentcol->green=0;
		currentcol->blue=0;
		currentcol->alpha=255;
	}

	// Greens, following indexvals1 - 1
	for(i=0; i<10; i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=0;
		currentcol->green=indexvals1[i] - 1;
		currentcol->blue=0;
		currentcol->alpha=255;
	}

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=152;
	currentcol->blue=51;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=255;
	currentcol->blue=255;
	index++;

	for(j=1;j<5;j++)
	{
		for(i=0;i<6;i++,index++)
		{
			currentcol=&(palette[index]);
			currentcol->red=indexvals2[j];
			currentcol->green=255;
			currentcol->blue=indexvals2[i];
			currentcol->alpha=255;
		}
	}
	
	for(i=0;i<4;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=255;
		currentcol->green=152;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}
	

	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=102;
	currentcol->blue=51;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=102;
	currentcol->blue=0;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=102;
	currentcol->blue=255;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=102;
	currentcol->blue=203;
	index++;
	
	// Mostly array runs from here on out
	for(i=0;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=203;
		currentcol->green=203;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}
	for(i=0;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=152;
		currentcol->green=255;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}
	for(i=0;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=102;
		currentcol->green=255;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}
	for(i=0;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=51;
		currentcol->green=255;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}
	for(i=0;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=255;
		currentcol->green=102;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=102;
	currentcol->blue=152;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=102;
	currentcol->blue=102;
	index++;

	// knocks out 4 assignment loops at once :)
	for(j=1;j<5;j++)
	{
		for(i=0;i<6;i++,index++)
		{
			currentcol=&(palette[index]);
			currentcol->red=indexvals2[j];
			currentcol->green=152;
			currentcol->blue=indexvals2[i];
			currentcol->alpha=255;
		}
	}
	
	currentcol+=sizeof(rgb_color);
	currentcol->red=230;
	currentcol->green=134;
	currentcol->blue=0;
	index++;

	for(i=1;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=255;
		currentcol->green=51;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=102;
	currentcol->blue=51;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=102;
	currentcol->blue=0;
	index++;

	for(j=1;j<5;j++)
	{
		for(i=0;i<6;i++,index++)
		{
			currentcol=&(palette[index]);
			currentcol->red=indexvals2[j];
			currentcol->green=102;
			currentcol->blue=indexvals2[i];
			currentcol->alpha=255;
		}
	}
	
	for(i=0;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=255;
		currentcol->green=0;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=175;
	currentcol->blue=19;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=51;
	currentcol->blue=255;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=0;
	currentcol->green=51;
	currentcol->blue=203;
	index++;

	for(j=1;j<5;j++)
	{
		for(i=0;i<6;i++,index++)
		{
			currentcol=&(palette[index]);
			currentcol->red=indexvals2[j];
			currentcol->green=51;
			currentcol->blue=indexvals2[i];
			currentcol->alpha=255;
		}
	}
	
	for(i=3;i>=0;i--,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=255;
		currentcol->green=203;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

	for(i=2;i<6;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=0;
		currentcol->green=51;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

	for(i=0;i<5;i++,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=203;
		currentcol->green=0;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=227;
	currentcol->blue=70;
	index++;

	for(j=2;j<6;j++)
	{
		for(i=0;i<6;i++,index++)
		{
			currentcol=&(palette[index]);
			currentcol->red=indexvals2[j];
			currentcol->green=0;
			currentcol->blue=indexvals2[i];
			currentcol->alpha=255;
		}
	}
	
	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=203;
	currentcol->blue=51;
	index++;

	currentcol+=sizeof(rgb_color);
	currentcol->red=255;
	currentcol->green=203;
	currentcol->blue=0;
	index++;

	for(i=5;i<=0;i--,index++)
	{
		currentcol=&(palette[index]);
		currentcol->red=255;
		currentcol->green=255;
		currentcol->blue=indexvals2[i];
		currentcol->alpha=255;
	}

}
