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
//	File Name:		PatternHandler.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class for easy calculation and use of patterns
//  
//------------------------------------------------------------------------------
#include <Point.h>
#include "PatternHandler.h"

const Pattern pat_solidhigh(0xFFFFFFFFFFFFFFFFLL);
const Pattern pat_solidlow((uint64)0);
const Pattern pat_mixedcolors(0xAAAAAAAAAAAAAAAALL);

/*!
	\brief Void constructor
	
	The pattern is set to B_SOLID_HIGH, high color is set to black, and 
	low color is set to white.
*/
PatternHandler::PatternHandler(void)
{
	_pat=0xFFFFFFFFFFFFFFFFLL;
	_high=new RGBColor(0,0,0,255);
	_low=new RGBColor(255,255,255,255);
}

/*!
	\brief Constructor initializes to given pattern
	\param pat Pattern to use.
	
	This initializes to the given pattern or B_SOLID_HIGH if the pattern 
	is NULL. High color is set to black, and low color is set to white.
*/
PatternHandler::PatternHandler(int8 *pat)
{
	if(pat)
		_pat.Set(pat);
	else
		_pat=0xFFFFFFFFFFFFFFFFLL;

	_high=new RGBColor(0,0,0,255);
	_low=new RGBColor(255,255,255,255);
}

/*!
	\brief Constructor initializes to given pattern
	\param pat Pattern to use.
	
	This initializes to the given pattern or B_SOLID_HIGH if the pattern 
	is NULL. High color is set to black, and low color is set to white.
*/
PatternHandler::PatternHandler(const uint64 &pat)
{
	_pat=pat;
	_high=new RGBColor(0,0,0,255);
	_low=new RGBColor(255,255,255,255);
}

PatternHandler::PatternHandler(const Pattern &pat)
{
	_pat=pat;
	_high=new RGBColor(0,0,0,255);
	_low=new RGBColor(255,255,255,255);
}

//! Destructor frees internal color variables
PatternHandler::~PatternHandler(void)
{
	delete _high;
	delete _low;
}

/*!
	\brief Sets the pattern for the handler to the one given
	\param pat Pattern to use.
	
	This initializes to the given pattern or B_SOLID_HIGH if the pattern 
	is NULL.
*/
void PatternHandler::SetTarget(int8 *pat)
{
	if(pat)
		_pat.Set(pat);
	else
		_pat=0xFFFFFFFFFFFFFFFFLL;
}

/*!
	\brief Sets the pattern for the handler to the one given
	\param pat Pattern to use.
*/
void PatternHandler::SetTarget(const uint64 &pat)
{
	_pat=pat;
}

/*!
	\brief Sets the pattern for the handler to the one given
	\param pat Pattern to use.
*/
void PatternHandler::SetTarget(const Pattern &pat)
{
	_pat=pat;
}

/*!
	\brief Set the colors for the pattern to use
	\param high High color for the handler
	\param low Low color for the handler
*/
void PatternHandler::SetColors(const RGBColor &high, const RGBColor &low)
{
	*_high=high;
	*_low=low;
}

/*!
	\brief Obtains the color in the pattern at the specified coordinates
	\param pt Coordinates to get the color for
	\return Color for the coordinates
*/
RGBColor PatternHandler::GetColor(const BPoint &pt)
{
	return GetColor(pt.x,pt.y);
}

/*!
	\brief Obtains the color in the pattern at the specified coordinates
	\param x X coordinate to get the color for
	\param y Y coordinate to get the color for
	\return Color for the coordinates
*/
RGBColor PatternHandler::GetColor(const float &x, const float &y)
{
	const int8 *ptr=_pat.GetInt8();
	int32 value=ptr[(uint32)y%8] & (1 << (7-((uint32)x%8)) );

	return (value==0)?*_low:*_high;
}

/*!
	\brief Obtains the value of the pattern at the specified coordinates
	\param x X coordinate to get the value for
	\param y Y coordinate to get the value for
	\return Value for the coordinates - true if high, false if low.
*/
bool PatternHandler::GetValue(const float &x, const float &y)
{
	const int8 *ptr=_pat.GetInt8();
	int32 value=ptr[(uint32)y%8] & (1 << (7-((uint32)x%8)) );

	return (value==0)?false:true;
}

/*!
	\brief Obtains the value of the pattern at the specified coordinates
	\param pt Coordinates to get the value for
	\return Value for the coordinates - true if high, false if low.
*/
bool PatternHandler::GetValue(const BPoint &pt)
{
	return GetValue(pt.x,pt.y);
}

