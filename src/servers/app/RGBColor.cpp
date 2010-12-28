/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */


#include "RGBColor.h"
#include "SystemPalette.h"

#include <stdio.h>
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

#if 0
/*!
	\brief Function for easy conversion of 16-bit colors to 32-bit
	\param col Pointer to an rgb_color.
	\param color RGB16 color

	This function will do nothing if passed a NULL 32-bit color.
*/
void
SetRGBColor16(rgb_color *col,uint16 color)
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
#endif

/*!
	\brief Finds the index of the closest matching color in a rgb_color palette array
	\param palette Array of 256 rgb_color objects
	\param color Color to match
	\return Index of the closest matching color

	Note that passing a NULL palette will always return 0 and passing an array of less
	than 256 rgb_colors will cause a crash.
*/
static uint8
FindClosestColor(const rgb_color *palette, rgb_color color)
{
	if (!palette)
		return 0;

	uint16 cindex = 0, cdelta = 765, delta = 765;

	for (uint16 i = 0; i < 256; i++) {
		const rgb_color *c = &(palette[i]);
		delta = abs(c->red-color.red) + abs(c->green-color.green)
			+ abs(c->blue-color.blue);

		if (delta == 0) {
			cindex = i;
			break;
		}

		if (delta < cdelta) {
			cindex = i;
			cdelta = delta;
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
static uint16
FindClosestColor15(rgb_color color)
{
	uint16 r16 = uint16(color.red * RATIO_8_TO_5_BIT);
	uint16 g16 = uint16(color.green * RATIO_8_TO_5_BIT);
	uint16 b16 = uint16(color.blue * RATIO_8_TO_5_BIT);

	// start with alpha value
	uint16 color16 = color.alpha > 127 ? 0x8000 : 0;

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
static uint16
FindClosestColor16(rgb_color color)
{
	uint16 r16 = uint16(color.red * RATIO_8_TO_5_BIT);
	uint16 g16 = uint16(color.green * RATIO_8_TO_6_BIT);
	uint16 b16 = uint16(color.blue * RATIO_8_TO_5_BIT);

	uint16 color16 = r16 << 11;
	color16 |= g16 << 5;
	color16 |= b16;

	return color16;
}


//	#pragma mark -


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
	SetColor(r, g, b, a);
}


/*!
	\brief Create an RGBColor from an rgb_color
	\param color color to initialize from
*/
RGBColor::RGBColor(const rgb_color &color)
{
	SetColor(color);
}

#if 0
/*!
	\brief Create an RGBColor from a 16-bit RGBA color
	\param color color to initialize from
*/
RGBColor::RGBColor(uint16 color)
{
	SetColor(color);
}
#endif

/*!
	\brief Create an RGBColor from an index color
	\param color color to initialize from
*/
RGBColor::RGBColor(uint8 color)
{
	SetColor(color);
}


/*!
	\brief Copy Contructor
	\param color color to initialize from
*/
RGBColor::RGBColor(const RGBColor &color)
{
	fColor32 = color.fColor32;
	fColor16 = color.fColor16;
	fColor15 = color.fColor15;
	fColor8 = color.fColor8;
	fUpdate8 = color.fUpdate8;
	fUpdate15 = color.fUpdate15;
	fUpdate16 = color.fUpdate16;
}


/*!
	\brief Create an RGBColor with the values(0,0,0,0)
*/
RGBColor::RGBColor()
{
	SetColor(0, 0, 0, 0);
}


/*!
	\brief Returns the color as the closest 8-bit color in the palette
	\return The palette index for the current color
*/
uint8
RGBColor::GetColor8() const
{
	if (fUpdate8) {
		fColor8 = FindClosestColor(SystemPalette(), fColor32);
		fUpdate8 = false;
	}

	return fColor8;
}


/*!
	\brief Returns the color as the closest 15-bit color
	\return 15-bit value of the current color plus 1-bit alpha
*/
uint16
RGBColor::GetColor15() const
{
	if (fUpdate15) {
		fColor15 = FindClosestColor15(fColor32);
		fUpdate15 = false;
	}

	return fColor15;
}


/*!
	\brief Returns the color as the closest 16-bit color
	\return 16-bit value of the current color
*/
uint16
RGBColor::GetColor16() const
{
	if (fUpdate16) {
		fColor16 = FindClosestColor16(fColor32);
		fUpdate16 = false;
	}

	return fColor16;
}


/*!
	\brief Returns the color as a 32-bit color
	\return current color, including alpha
*/
rgb_color
RGBColor::GetColor32() const
{
	return fColor32;
}


/*!
	\brief Set the object to specified values
	\param red red
	\param green green
	\param blue blue
	\param alpha alpha, defaults to 255
*/
void
RGBColor::SetColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	fColor32.red = r;
	fColor32.green = g;
	fColor32.blue = b;
	fColor32.alpha = a;

	fUpdate8 = fUpdate15 = fUpdate16 = true;
}


/*!
	\brief Set the object to specified values
	\param red red
	\param green green
	\param blue blue
	\param alpha alpha, defaults to 255
*/
void
RGBColor::SetColor(int r, int g, int b, int a)
{
	fColor32.red = (uint8)r;
	fColor32.green = (uint8)g;
	fColor32.blue = (uint8)b;
	fColor32.alpha = (uint8)a;

	fUpdate8 = fUpdate15 = fUpdate16 = true;
}

#if 0
/*!
	\brief Set the object to specified value
	\param col16 color to copy
*/
void
RGBColor::SetColor(uint16 col16)
{
	fColor16 = col16;
	SetRGBColor(&fColor32, col16);

	fUpdate8 = true;
	fUpdate15 = true;
	fUpdate16 = false;
}
#endif


/*!
	\brief Set the object to specified index in the palette
	\param col8 color to copy
*/
void
RGBColor::SetColor(uint8 col8)
{
	fColor8 = col8;
	fColor32 = SystemPalette()[col8];

	fUpdate8 = false;
	fUpdate15 = true;
	fUpdate16 = true;
}


/*!
	\brief Set the object to specified color
	\param color color to copy
*/
void
RGBColor::SetColor(const rgb_color &color)
{
	fColor32 = color;
	fUpdate8 = fUpdate15 = fUpdate16 = true;
}


/*!
	\brief Set the object to specified color
	\param color color to copy
*/
void
RGBColor::SetColor(const RGBColor &color)
{
	fColor32 = color.fColor32;
	fColor16 = color.fColor16;
	fColor15 = color.fColor15;
	fColor8 = color.fColor8;
	fUpdate8 = color.fUpdate8;
	fUpdate15 = color.fUpdate15;
	fUpdate16 = color.fUpdate16;
}


/*!
	\brief Set the object to specified color
	\param color color to copy
*/
const RGBColor&
RGBColor::operator=(const RGBColor &color)
{
	fColor32 = color.fColor32;
	fColor16 = color.fColor16;
	fColor15 = color.fColor15;
	fColor8 = color.fColor8;
	fUpdate8 = color.fUpdate8;
	fUpdate15 = color.fUpdate15;
	fUpdate16 = color.fUpdate16;

	return *this;
}


/*!
	\brief Set the object to specified color
	\param color color to copy
*/
const RGBColor&
RGBColor::operator=(const rgb_color &color)
{
	fColor32 = color;
	fUpdate8 = fUpdate15 = fUpdate16 = true;

	return *this;
}


/*!
	\brief Prints the 32-bit values of the color to standard out
*/
void
RGBColor::PrintToStream(void) const
{
	printf("RGBColor(%u,%u,%u,%u)\n",
		fColor32.red, fColor32.green, fColor32.blue, fColor32.alpha);
}


/*!
	\brief Overloaded comaparison
	\return true if all color elements are exactly equal
*/
bool
RGBColor::operator==(const rgb_color &color) const
{
	return fColor32.red == color.red
		&& fColor32.green == color.green
		&& fColor32.blue == color.blue
		&& fColor32.alpha == color.alpha;
}


/*!
	\brief Overloaded comaparison
	\return true if all color elements are exactly equal
*/
bool
RGBColor::operator==(const RGBColor &color) const
{
	return fColor32.red == color.fColor32.red
		&& fColor32.green == color.fColor32.green
		&& fColor32.blue == color.fColor32.blue
		&& fColor32.alpha == color.fColor32.alpha;
}


bool
RGBColor::operator!=(const rgb_color &color) const
{
	return fColor32.red != color.red
		|| fColor32.green != color.green
		|| fColor32.blue != color.blue
		|| fColor32.alpha != color.alpha;
}


bool
RGBColor::operator!=(const RGBColor &color) const
{
	return fColor32.red != color.fColor32.red
		|| fColor32.green != color.fColor32.green
		|| fColor32.blue != color.fColor32.blue
		|| fColor32.alpha != color.fColor32.alpha;
}


bool
RGBColor::IsTransparentMagic() const
{
	// TODO: validate this for B_CMAP8 for example
	return *this == B_TRANSPARENT_COLOR;
}
