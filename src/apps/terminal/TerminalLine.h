/*
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Siarzhuk Zharski, zharik@gmx.li
 */
#ifndef TERMINAL_LINE_H
#define TERMINAL_LINE_H

#include <GraphicsDefs.h>
#include <SupportDefs.h>

#include "TermConst.h"

#include "UTF8Char.h"


struct Attributes {
	uint32 state;
	uint32 foreground;
	uint32 background;
	uint32 underline;
	int underlineStyle;

	Attributes() : state(0), foreground(0), background(0), underline(0), underlineStyle(0) {}

	inline void Reset()
	{
		state = 0;
		foreground = 0;
		background = 0;
		underline = 0;
		underlineStyle = 0;
	}

	inline bool IsWidth() const { return (state & A_WIDTH) == A_WIDTH; }
	inline bool IsBold() const { return (state & BOLD) == BOLD; }
	inline bool IsUnder() const { return (state & UNDERLINE) == UNDERLINE; }
	inline bool IsInverse() const { return (state & INVERSE) == INVERSE; }
	inline bool IsMouse() const { return (state & MOUSE) == MOUSE; }
	inline bool IsForeSet() const { return (state & FORESET) == FORESET; }
	inline bool IsBackSet() const { return (state & BACKSET) == BACKSET; }
	inline bool IsUnderSet() const { return (state & UNDERSET) == UNDERSET; }
	inline bool IsFont() const { return (state & FONT) == FONT; }
	inline bool IsCR() const { return (state & DUMPCR) == DUMPCR; }

	inline void SetDirectForeground(uint8 red, uint8 green, uint8 blue)
	{
		foreground = 0x80000000 | (red << 16) | (green << 8) | blue;
		state &= ~FORECOLOR;
		state |= FORESET;
	}

	inline void SetDirectBackground(uint8 red, uint8 green, uint8 blue)
	{
		background = 0x80000000 | (red << 16) | (green << 8) | blue;
		state &= ~BACKCOLOR;
		state |= BACKSET;
	}

	inline void SetDirectUnderline(uint8 red, uint8 green, uint8 blue)
	{
		underline = 0x80000000 | (red << 16) | (green << 8) | blue;
		state |= UNDERSET;
	}

	inline void SetIndexedForeground(uint32 index)
	{
		state &= ~FORECOLOR;
		state |= FORESET;
		state |= FORECOLORED(index);
		foreground = 0;
	}

	inline void SetIndexedBackground(uint32 index)
	{
		state &= ~BACKCOLOR;
		state |= BACKSET;
		state |= BACKCOLORED(index);
		background = 0;
	}

	inline void SetIndexedUnderline(uint32 index)
	{
		state |= UNDERSET;
		underline = index;
	}

	inline void SetUnder(int style)
	{
		underlineStyle = style;
		state |= UNDERLINE;
	}

	inline void UnsetForeground()
	{
		state &= ~FORESET;
		foreground = 0;
	}

	inline void UnsetBackground()
	{
		state &= ~BACKSET;
		background = 0;
	}

	inline void UnsetUnderline()
	{
		state &= ~UNDERSET;
		underline = 0;
	}

	inline void UnsetUnder()
	{
		underlineStyle = 0;
		state &= ~UNDERLINE;
	}

	inline rgb_color
	ForegroundColor(const rgb_color* indexedColors) const
	{
		if ((foreground & 0x80000000) != 0)
			return make_color((foreground >> 16) & 0xFF,
				(foreground >> 8) & 0xFF,
				foreground & 0xFF);
		else
			return indexedColors[(state & FORECOLOR) >> 16];
	}

	inline rgb_color
	BackgroundColor(const rgb_color* indexedColors) const
	{
		if ((background & 0x80000000) != 0)
			return make_color((background >> 16) & 0xFF,
				(background >> 8) & 0xFF,
				background & 0xFF);
		else
			return indexedColors[(state & BACKCOLOR) >> 24];
	}

	inline rgb_color
	UnderlineColor(const rgb_color* indexedColors) const
	{
		if ((underline & 0x80000000) != 0)
			return make_color((underline >> 16) & 0xFF,
				(underline >> 8) & 0xFF,
				underline & 0xFF);
		else
			return indexedColors[underline];
	}

	inline int
	UnderlineStyle() const
	{
		return underlineStyle;
	}

	inline Attributes&
	operator&=(uint32 value) { state &= value; return *this; }

	inline Attributes&
	operator|=(uint32 value) { state |= value; return *this; }

	inline uint32
	operator|(uint32 value) { return state | value; }

	inline uint32
	operator&(uint32 value) { return state & value; }

	inline bool
	operator==(const Attributes& other) const
	{
		return state == other.state
			&& foreground == other.foreground
			&& background == other.background
			&& underline == other.underline
			&& underlineStyle == other.underlineStyle;
	}

	inline bool
	operator!=(const Attributes& other) const
	{
		return state != other.state
			|| foreground != other.foreground
			|| background != other.background
			|| underline != other.underline
			|| underlineStyle != other.underlineStyle;
	}
};


struct TerminalCell {
	UTF8Char			character;
	Attributes			attributes;

	inline bool
	operator!=(const Attributes& other) const
	{
		return (attributes.state & CHAR_ATTRIBUTES)
				!= (other.state & CHAR_ATTRIBUTES)
			|| attributes.foreground != other.foreground
			|| attributes.background != other.background;
	}
};


struct TerminalLine {
	uint16			length;
	bool			softBreak;	// soft line break
	Attributes		attributes;
	TerminalCell	cells[1];

	inline void Clear()
	{
		Clear(Attributes());
	}

	inline void Clear(size_t count)
	{
		Clear(Attributes(), count);
	}

	inline void Clear(Attributes attr, size_t count = 0)
	{
		length = 0;
		attributes = attr;
		softBreak = false;
		for (size_t i = 0; i < count; i++)
			cells[i].attributes = attr;
	}
};


struct AttributesRun {
	Attributes	attributes;
	uint16	offset;			// character offset
	uint16	length;			// length of the run in characters
};


struct HistoryLine {
	AttributesRun*	attributesRuns;
	uint16			attributesRunCount;	// number of attribute runs
	uint16			byteLength : 15;	// number of bytes in the line
	bool			softBreak : 1;		// soft line break;
	Attributes		attributes;

	AttributesRun* AttributesRuns() const
	{
		return attributesRuns;
	}

	char* Chars() const
	{
		return (char*)(attributesRuns + attributesRunCount);
	}

	int32 BufferSize() const
	{
		return attributesRunCount * sizeof(AttributesRun) + byteLength;
	}
};


#endif	// TERMINAL_LINE_H
