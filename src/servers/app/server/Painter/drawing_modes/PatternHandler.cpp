//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
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
//	File Name:		PatternHandler.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Class for easy calculation and use of patterns
//  
//------------------------------------------------------------------------------
#include <Point.h>
#include "PatternHandler.h"

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
	  fLowColor(kWhite)
{
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
	  fLowColor(kWhite)
{
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
	  fLowColor(kWhite)
{
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
	  fLowColor(kWhite)
{
}

/*!
	\brief Constructor initializes to given PatternHandler
	\param other PatternHandler to copy.
	
	Copy constructor.
*/
PatternHandler::PatternHandler(const PatternHandler& other)
	: fPattern(other.fPattern),
	  fHighColor(other.fHighColor),
	  fLowColor(other.fLowColor)
{
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
void PatternHandler::SetPattern(const int8* pat)
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
void PatternHandler::SetPattern(const uint64& pat)
{
	fPattern = pat;
}

/*!
	\brief Sets the pattern for the handler to the one given
	\param pat Pattern to use.
*/
void PatternHandler::SetPattern(const Pattern& pat)
{
	fPattern = pat;
}

/*!
	\brief Sets the pattern for the handler to the one given
	\param pat R5 style pattern to use.
*/
void PatternHandler::SetPattern(const pattern& pat)
{
	fPattern = pat;
}

/*!
	\brief Set the colors for the pattern to use
	\param high High color for the handler
	\param low Low color for the handler
*/
void PatternHandler::SetColors(const rgb_color& high, const rgb_color& low)
{
	fHighColor = high;
	fLowColor = low;
}

/*!
	\brief Set the high color for the pattern to use
	\param color High color for the handler
*/
void PatternHandler::SetHighColor(const rgb_color& color)
{
	fHighColor = color;
}

/*!
	\brief Set the low color for the pattern to use
	\param color Low color for the handler
*/
void PatternHandler::SetLowColor(const rgb_color& color)
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
bool PatternHandler::IsHighColor(const BPoint &pt) const
{
	return IsHighColor((int)pt.x, (int)pt.y);
}

