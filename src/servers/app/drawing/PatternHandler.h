/*
 * Copyright (c) 2001-2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:	DarkWyrm <bpmagic@columbus.rr.com>
 *			Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef PATTERNHANDLER_H
#define PATTERNHANDLER_H


#include <stdio.h>
#include <string.h>

#include <GraphicsDefs.h>

class BPoint;


class Pattern {
 public:

								Pattern() {} 

								Pattern(const uint64& p)
									{ fPattern.type64 = p; }

								Pattern(const int8* p)
									{ fPattern.type64 = *((const uint64*)p); }
	
								Pattern(const Pattern& src)
									{ fPattern.type64 = src.fPattern.type64; }

								Pattern(const pattern& src)
									{ fPattern.type64 = *(uint64*)src.data; }

	inline	const int8*			GetInt8() const
									{ return fPattern.type8; }

	inline	uint64				GetInt64() const
									{ return fPattern.type64; }

	inline	const ::pattern&	GetPattern() const
									{ return *(const ::pattern*)&fPattern.type64; }

	inline	void				Set(const int8* p)
									{ fPattern.type64 = *((const uint64*)p); }

	inline	void				Set(const uint64& p)
									{ fPattern.type64 = p; }

			Pattern&			operator=(const Pattern& from)
									{ fPattern.type64 = from.fPattern.type64; return *this; }

			Pattern&			operator=(const int64 &from)
									{ fPattern.type64 = from; return *this; }

			Pattern&			operator=(const pattern &from)
									{ memcpy(&fPattern.type64, &from, sizeof(pattern)); return *this; }

			bool				operator==(const Pattern& other) const
									{ return fPattern.type64 == other.fPattern.type64; }

			bool				operator==(const pattern& other) const
									{ return fPattern.type64 == *(uint64*)other.data; }

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
	virtual						~PatternHandler(void);

			void				SetPattern(const int8* p);
			void				SetPattern(const uint64& p);
			void				SetPattern(const Pattern& p);
			void				SetPattern(const pattern& p);

			void				SetColors(const rgb_color& high,
									const rgb_color& low);
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
	inline	bool				IsSolidHigh() const
									{ return fPattern == B_SOLID_HIGH; }
	inline	bool				IsSolidLow() const
									{ return fPattern == B_SOLID_LOW; }
	inline	bool				IsSolid() const
									{ return IsSolidHigh() || IsSolidLow(); }

			const pattern*		GetR5Pattern(void) const
									{ return (const pattern*)fPattern.GetInt8(); }
			const Pattern&		GetPattern(void) const
									{ return fPattern; }

			void				SetOffsets(int32 x, int32 y);

			void				MakeOpCopyColorCache();
	inline	const rgb_color*	OpCopyColorCache() const
									{ return fOpCopyColorCache; }

 private:
			Pattern				fPattern;
			rgb_color			fHighColor;
			rgb_color			fLowColor;

			uint16				fXOffset;
			uint16				fYOffset;

			rgb_color			fOpCopyColorCache[256];
			uint64				fColorsWhenCached;
};

/*!
	\brief Obtains the color in the pattern at the specified coordinates
	\param x X coordinate to get the color for
	\param y Y coordinate to get the color for
	\return Color for the coordinates
*/
inline rgb_color
PatternHandler::ColorAt(int x, int y) const
{
	return IsHighColor(x, y) ? fHighColor : fLowColor;
}

/*!
	\brief Obtains the value of the pattern at the specified coordinates
	\param pt Coordinates to get the value for
	\return Value for the coordinates - true if high, false if low.
*/

inline bool
PatternHandler::IsHighColor(int x, int y) const
{
	x -= fXOffset;
	y -= fYOffset;
	const int8* ptr = fPattern.GetInt8();
	int32 value = ptr[y & 7] & (1 << (7 - (x & 7)) );
	
	return value != 0;
}

#endif
