/*
 * Copyright (c) 2001-2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:	DarkWyrm <bpmagic@columbus.rr.com>
 *			Stephan Aßmus <superstippi@gmx.de>
 */

#include "PatternHandler.h"

#include <stdio.h>
#include <string.h>

#include <Point.h>

const Pattern kSolidHigh(0xFFFFFFFFFFFFFFFFLL);
const Pattern kSolidLow((uint64)0);
const Pattern kMixedColors(0xAAAAAAAAAAAAAAAALL);

const rgb_color kBlack = (rgb_color){ 0, 0, 0, 255 };
const rgb_color kWhite = (rgb_color){ 255, 255, 255, 255 };

/*!
	\brief Void constructor
	
	The pattern is set to B_SOLID_HIGH, high color is set to black, and 
	low color is set to white.
*/
PatternHandler::PatternHandler(void)
	: fPattern(kSolidHigh),
	  fHighColor(kBlack),
	  fLowColor(kWhite),
	  fXOffset(0),
	  fYOffset(0),
	  fColorsWhenCached(ULONGLONG_MAX)
{
	memset(fOpCopyColorCache, 255, 256 * sizeof(rgb_color));
}

/*!
	\brief Constructor initializes to given pattern
	\param pat Pattern to use.
	
	This initializes to the given pattern or B_SOLID_HIGH if the pattern 
	is NULL. High color is set to black, and low color is set to white.
*/
PatternHandler::PatternHandler(const int8* pat)
	: fPattern(pat ? Pattern(pat) : Pattern(kSolidHigh)),
	  fHighColor(kBlack),
	  fLowColor(kWhite),
	  fXOffset(0),
	  fYOffset(0),
	  fColorsWhenCached(ULONGLONG_MAX)
{
	memset(fOpCopyColorCache, 255, 256 * sizeof(rgb_color));
}

/*!
	\brief Constructor initializes to given pattern
	\param pat Pattern to use.
	
	This initializes to the given pattern or B_SOLID_HIGH if the pattern 
	is NULL. High color is set to black, and low color is set to white.
*/
PatternHandler::PatternHandler(const uint64& pat)
	: fPattern(pat),
	  fHighColor(kBlack),
	  fLowColor(kWhite),
	  fXOffset(0),
	  fYOffset(0),
	  fColorsWhenCached(ULONGLONG_MAX)
{
	memset(fOpCopyColorCache, 255, 256 * sizeof(rgb_color));
}

/*!
	\brief Constructor initializes to given pattern
	\param pat Pattern to use.
	
	This initializes to the given Pattern.
	High color is set to black, and low color is set to white.
*/
PatternHandler::PatternHandler(const Pattern& pat)
	: fPattern(pat),
	  fHighColor(kBlack),
	  fLowColor(kWhite),
	  fXOffset(0),
	  fYOffset(0),
	  fColorsWhenCached(ULONGLONG_MAX)
{
	memset(fOpCopyColorCache, 255, 256 * sizeof(rgb_color));
}

/*!
	\brief Constructor initializes to given PatternHandler
	\param other PatternHandler to copy.
	
	Copy constructor.
*/
PatternHandler::PatternHandler(const PatternHandler& other)
	: fPattern(other.fPattern),
	  fHighColor(other.fHighColor),
	  fLowColor(other.fLowColor),
	  fXOffset(other.fXOffset),
	  fYOffset(other.fYOffset),
	  fColorsWhenCached(ULONGLONG_MAX)
{
	memset(fOpCopyColorCache, 255, 256 * sizeof(rgb_color));
}

//! Destructor does nothing
PatternHandler::~PatternHandler(void)
{
}

/*!
	\brief Sets the pattern for the handler to the one given
	\param pat Pattern to use.
	
	This initializes to the given pattern or B_SOLID_HIGH if the pattern 
	is NULL.
*/
void
PatternHandler::SetPattern(const int8* pat)
{
	if (pat)
		fPattern.Set(pat);
	else
		fPattern = kSolidHigh;
}

/*!
	\brief Sets the pattern for the handler to the one given
	\param pat Pattern to use.
*/
void
PatternHandler::SetPattern(const uint64& pat)
{
	fPattern = pat;
}

/*!
	\brief Sets the pattern for the handler to the one given
	\param pat Pattern to use.
*/
void
PatternHandler::SetPattern(const Pattern& pat)
{
	fPattern = pat;
}

/*!
	\brief Sets the pattern for the handler to the one given
	\param pat R5 style pattern to use.
*/
void
PatternHandler::SetPattern(const pattern& pat)
{
	fPattern = pat;
}

/*!
	\brief Set the colors for the pattern to use
	\param high High color for the handler
	\param low Low color for the handler
*/
void
PatternHandler::SetColors(const rgb_color& high, const rgb_color& low)
{
	fHighColor = high;
	fLowColor = low;
}

/*!
	\brief Set the high color for the pattern to use
	\param color High color for the handler
*/
void
PatternHandler::SetHighColor(const rgb_color& color)
{
	fHighColor = color;
}

/*!
	\brief Set the low color for the pattern to use
	\param color Low color for the handler
*/
void
PatternHandler::SetLowColor(const rgb_color& color)
{
	fLowColor = color;
}

/*!
	\brief Obtains the color in the pattern at the specified coordinates
	\param pt Coordinates to get the color for
	\return Color for the coordinates
*/
rgb_color
PatternHandler::ColorAt(const BPoint &pt) const
{
	return ColorAt(pt.x, pt.y);
}

/*!
	\brief Obtains the color in the pattern at the specified coordinates
	\param x X coordinate to get the color for
	\param y Y coordinate to get the color for
	\return Color for the coordinates
*/
rgb_color
PatternHandler::ColorAt(float x, float y) const
{
	return ColorAt(int(x), int(y));
}

/*!
	\brief Obtains the value of the pattern at the specified coordinates
	\param pt Coordinates to get the value for
	\return Value for the coordinates - true if high, false if low.
*/
bool
PatternHandler::IsHighColor(const BPoint &pt) const
{
	return IsHighColor((int)pt.x, (int)pt.y);
}

/*!
	\brief Transfers the scrolling offset of a BView to affect the pattern.
	\param x Positive or negative horizontal offset
	\param y Positive or negative vertical offset
*/
void
PatternHandler::SetOffsets(int32 x, int32 y)
{
	fXOffset = x & 7;
	fYOffset = y & 7;
}


void
PatternHandler::MakeOpCopyColorCache()
{
	uint64 t = *(uint32*)&fHighColor;
	uint64 colors = (t << 32) | *(uint32*)&fLowColor;

	if (fColorsWhenCached == colors)
		return;

	fColorsWhenCached = colors;

	// ramp from low color to high color
	uint8 rA = fLowColor.red;
	uint8 gA = fLowColor.green;
	uint8 bA = fLowColor.blue;

	uint8 rB = fHighColor.red;
	uint8 gB = fHighColor.green;
	uint8 bB = fHighColor.blue;

	for (int32 i = 0; i < 256; i++) {
		// NOTE: rgb is twisted around, since this is
		// only used as uint32 in the end
		fOpCopyColorCache[i].red = (((bB - bA) * i) + (bA << 8)) >> 8;
		fOpCopyColorCache[i].green = (((gB - gA) * i) + (gA << 8)) >> 8;
		fOpCopyColorCache[i].blue = (((rB - rA) * i) + (rA << 8)) >> 8;
	}
}


