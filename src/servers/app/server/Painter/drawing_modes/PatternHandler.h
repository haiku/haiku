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
//	File Name:		PatternHandler.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Class for easy calculation and use of patterns
//  
//------------------------------------------------------------------------------
#ifndef PATTERNHADLER_H
#define PATTERNHADLER_H

#include <string.h>
//#include <SupportDefs.h>
//#include "rgb_color.h"
#include <GraphicsDefs.h>

class Pattern {
 public:

								Pattern(void) {} 

								Pattern(const uint64& p)
									{ fPattern.type64 = p; }

								Pattern(const int8* p)
									{ fPattern.type64 = *((const uint64*)p); }
	
								Pattern(const Pattern& src)
									{ fPattern.type64 = src.fPattern.type64; }

			const int8*			GetInt8(void) const
									{ return fPattern.type8; }

			uint64				GetInt64(void) const
									{ return fPattern.type64; }

			void				Set(const int8* p)
									{ fPattern.type64 = *((const uint64*)p); }

			void				Set(const uint64& p)
									{ fPattern.type64 = p; }

			Pattern&			operator=(const Pattern& from)
									{ fPattern.type64 = from.fPattern.type64; return *this; }

			Pattern&			operator=(const int64 &from)
									{ fPattern.type64 = from; return *this; }

			Pattern&			operator=(const pattern &from)
									{ memcpy(&fPattern.type64, &from, sizeof(pattern)); return *this; }
 private:

	typedef union
	{
		uint64	type64;
		int8	type8[8];
	} pattern_union;

			pattern_union		fPattern;
};

extern const Pattern kSolidHigh;
extern const Pattern kSolidLow;
extern const Pattern kMixedColors;

/*!
	\brief Class for easy calculation and use of patterns
	
	PatternHandlers are designed specifically for DisplayDriver subclasses. 
	Pattern support can be easily added by setting the pattern to use via 
	SetTarget, and then merely retrieving the value for the coordinates 
	specified. 
*/
class PatternHandler {
 public:
								PatternHandler(void);
								PatternHandler(const int8* p);
								PatternHandler(const uint64& p);
								PatternHandler(const Pattern& p);
								PatternHandler(const PatternHandler& other);
			virtual				~PatternHandler(void);

			void				SetPattern(const int8* p);
			void				SetPattern(const uint64& p);
			void				SetPattern(const Pattern& p);
			void				SetPattern(const pattern& p);
			void				SetColors(const rgb_color& high, const rgb_color& low);
			void				SetHighColor(const rgb_color& color);
			void				SetLowColor(const rgb_color& color);

			rgb_color			HighColor() const
									{ return fHighColor; }
			rgb_color			LowColor() const
									{ return fLowColor; }

			rgb_color			ColorAt(const BPoint& pt) const;
			rgb_color			ColorAt(float x, float y) const;
	inline	rgb_color			ColorAt(int x, int y) const;

			bool				IsHighColor(const BPoint& pt) const;
	inline	bool				IsHighColor(int x, int y) const;

			const pattern*		GetR5Pattern(void) const
									{ return (const pattern*)fPattern.GetInt8(); }
 private:
			Pattern				fPattern;
			rgb_color			fHighColor;
			rgb_color			fLowColor;
};

/*!
	\brief Obtains the color in the pattern at the specified coordinates
	\param x X coordinate to get the color for
	\param y Y coordinate to get the color for
	\return Color for the coordinates
*/
inline	rgb_color
PatternHandler::ColorAt(int x, int y) const
{
	return IsHighColor(x, y) ? fHighColor : fLowColor;
}

/*!
	\brief Obtains the value of the pattern at the specified coordinates
	\param pt Coordinates to get the value for
	\return Value for the coordinates - true if high, false if low.
*/
inline	bool
PatternHandler::IsHighColor(int x, int y) const
{
	// TODO: Does this work correctly for
	// negative coordinates?!?
	const int8* ptr = fPattern.GetInt8();
	int32 value = ptr[y % 8] & (1 << (7 - (x % 8)) );

	return (value == 0) ? false : true;
}

#endif
