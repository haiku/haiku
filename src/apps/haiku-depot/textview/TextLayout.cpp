/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextLayout.h"

#include <new>

#include <AutoDeleter.h>
#include <utf8_functions.h>
#include <View.h>

#include "List.h"


enum {
	CHAR_CLASS_DEFAULT,
	CHAR_CLASS_WHITESPACE,
	CHAR_CLASS_GRAPHICAL,
	CHAR_CLASS_QUOTE,
	CHAR_CLASS_PUNCTUATION,
	CHAR_CLASS_PARENS_OPEN,
	CHAR_CLASS_PARENS_CLOSE,
	CHAR_CLASS_END_OF_TEXT
};


static uint32
get_char_classification(uint32 charCode)
{
	// TODO: Should check against a list of characters containing also
	// word breakers from other languages.

	switch (charCode) {
		case '\0':
			return CHAR_CLASS_END_OF_TEXT;

		case ' ':
		case '\t':
		case '\n':
			return CHAR_CLASS_WHITESPACE;

		case '=':
		case '+':
		case '@':
		case '#':
		case '$':
		case '%':
		case '^':
		case '&':
		case '*':
		case '\\':
		case '|':
		case '<':
		case '>':
		case '/':
		case '~':
			return CHAR_CLASS_GRAPHICAL;

		case '\'':
		case '"':
			return CHAR_CLASS_QUOTE;

		case ',':
		case '.':
		case '?':
		case '!':
		case ';':
		case ':':
		case '-':
			return CHAR_CLASS_PUNCTUATION;

		case '(':
		case '[':
		case '{':
			return CHAR_CLASS_PARENS_OPEN;

		case ')':
		case ']':
		case '}':
			return CHAR_CLASS_PARENS_CLOSE;

		default:
			return CHAR_CLASS_DEFAULT;
	}
}


class Style : public BReferenceable {
public:
	Style(const BFont& font, float ascent, float descent, float width,
			rgb_color fgColor, rgb_color bgColor, bool strikeOut,
			rgb_color strikeOutColor, bool underline,
			uint32 underlineStyle, rgb_color underlineColor)
		:
		font(font),

		ascent(ascent),
		descent(descent),
		width(width),

		fgColor(fgColor),
		bgColor(bgColor),

		strikeOut(strikeOut),
		strikeOutColor(strikeOutColor),

		underline(underline),
		underlineStyle(underlineStyle),
		underlineColor(underlineColor)
	{
	}

public:
	BFont					font;

	// The following three values override glyph metrics unless -1
	float					ascent;
	float					descent;
	float					width;

	rgb_color				fgColor;
	rgb_color				bgColor;

	bool					strikeOut;
	rgb_color				strikeOutColor;

	bool					underline;
	uint32					underlineStyle;
	rgb_color				underlineColor;
};


typedef BReference<Style> StyleRef;


class GlyphInfo {
public:
	GlyphInfo(uint32 charCode, float x, float y, float advanceX,
			float maxAscend, float maxDescend, int lineIndex,
			const StyleRef& style)
		:
		charCode(charCode),
		x(x),
		y(y),
		advanceX(advanceX),
		maxAscend(maxAscend),
		maxDescend(maxDescend),
		lineIndex(lineIndex),
		style(style)
	{
	}
	
	GlyphInfo(const GlyphInfo& other)
		:
		charCode(other.charCode),
		x(other.x),
		y(other.y),
		advanceX(other.advanceX),
		maxAscend(other.maxAscend),
		maxDescend(other.maxDescend),
		lineIndex(other.lineIndex),
		style(other.style)
	{
	}

	GlyphInfo& operator=(const GlyphInfo& other)
	{
		charCode = other.charCode;
		x = other.x;
		y = other.y;
		advanceX = other.advanceX;
		maxAscend = other.maxAscend;
		maxDescend = other.maxDescend;
		lineIndex = other.lineIndex;
		style = other.style;
		return *this;
	}

	bool operator==(const GlyphInfo& other) const
	{
		return charCode == other.charCode
			&& x == other.x
			&& y == other.y
			&& advanceX == other.advanceX
			&& maxAscend == other.maxAscend
			&& maxDescend == other.maxDescend
			&& lineIndex == other.lineIndex
			&& style == other.style;
	}

	bool operator!=(const GlyphInfo& other) const
	{
		return !(*this == other);
	}

public:
	uint32					charCode;

	float					x;
	float					y;
	float					advanceX;

	float					maxAscend;
	float					maxDescend;

	int						lineIndex;

	StyleRef				style;
};


class Line {
public:
	Line()
		:
		textOffset(0),
		y(0.0f),
		height(0.0f)
	{
	}
	
	Line(int textOffset, float y, float height)
		:
		textOffset(textOffset),
		y(y),
		height(height)
	{
	}
	
	Line(const Line& other)
		:
		textOffset(other.textOffset),
		y(other.y),
		height(other.height)
	{
	}

	Line& operator=(const Line& other)
	{
		textOffset = other.textOffset;
		y = other.y;
		height = other.height;
		return *this;
	}

	bool operator==(const Line& other) const
	{
		return textOffset == other.textOffset
			&& y == other.y
			&& height == other.height;
	}

	bool operator!=(const Line& other) const
	{
		return !(*this == other);
	}

public:
	int		textOffset;
	float	y;
	float	height;
	float	maxAscent;
	float	maxDescent;
};


inline bool
can_end_line(const GlyphInfo* buffer, int offset, int count)
{
	if (offset == count - 1)
		return true;

	if (offset < 0 || offset > count)
		return false;

	uint32 charCode = buffer[offset].charCode;
	uint32 classification = get_char_classification(charCode);

	// wrapping is always allowed at end of text and at newlines
	if (classification == CHAR_CLASS_END_OF_TEXT || charCode == '\n')
		return true;

	uint32 nextCharCode = buffer[offset + 1].charCode;
	uint32 nextClassification = get_char_classification(nextCharCode);

	// never separate a punctuation char from its preceding word
	if (classification == CHAR_CLASS_DEFAULT
		&& nextClassification == CHAR_CLASS_PUNCTUATION) {
		return false;
	}

	if ((classification == CHAR_CLASS_WHITESPACE
			&& nextClassification != CHAR_CLASS_WHITESPACE)
		|| (classification != CHAR_CLASS_WHITESPACE
			&& nextClassification == CHAR_CLASS_WHITESPACE)) {
		return true;
	}

	// allow wrapping after whitespace, unless more whitespace (except for
	// newline) follows
	if (classification == CHAR_CLASS_WHITESPACE
		&& (nextClassification != CHAR_CLASS_WHITESPACE
			|| nextCharCode == '\n')) {
		return true;
	}

	// allow wrapping after punctuation chars, unless more punctuation, closing
	// parenthesis or quotes follow
	if (classification == CHAR_CLASS_PUNCTUATION
		&& nextClassification != CHAR_CLASS_PUNCTUATION
		&& nextClassification != CHAR_CLASS_PARENS_CLOSE
		&& nextClassification != CHAR_CLASS_QUOTE) {
		return true;
	}

	// allow wrapping after quotes, graphical chars and closing parenthesis only
	// if whitespace follows (not perfect, but seems to do the right thing most
	// of the time)
	if ((classification == CHAR_CLASS_QUOTE
			|| classification == CHAR_CLASS_GRAPHICAL
			|| classification == CHAR_CLASS_PARENS_CLOSE)
		&& nextClassification == CHAR_CLASS_WHITESPACE) {
		return true;
	}

	return false;
}


class TextLayout::Layout {
public:
	Layout(TextLayout* textLayout)
		:
		fTextLayout(textLayout)
	{
	}

	void ComputeLayout()
	{
		ComputeLayout(fTextLayout->fText, fTextLayout->fDefaultFont,
			fTextLayout->fWidth);
	}

	void ComputeLayout(const BString& text, const BFont& font, float width)
	{
		fLines.Clear();
		
		int charCount = text.CountChars();
		if (charCount == 0)
			return;
		
		// Allocate arrays
		float* escapementArray = new (std::nothrow) float[charCount];
		if (escapementArray == NULL)
			return;
		ArrayDeleter<float> escapementDeleter(escapementArray);

		uint32* charArray = new (std::nothrow) uint32[charCount];
		if (charArray == NULL)
			return;
		ArrayDeleter<uint32> charDeleter(charArray);

		// Fetch glyph spacing information
		font.GetEscapements(text.String(), charCount, escapementArray);

		// Convert to glyph buffer
		const char* c = text.String();
		for (int i = 0; i < charCount; i++) {
			charArray[i] = UTF8ToCharCode(&c);
		}

//		float x = 0.0f;
//		float y = 0.0f;
		for (int i = 0; i < charCount; i++) {
			// TODO: ...
		}
	}

	float Height() const
	{
		float height = fTextLayout->fDefaultFont.Size();
		if (fLines.CountItems() > 0) {
			const Line& lastLine = fLines.LastItem();
			height = lastLine.y + lastLine.height;
		}
		return height;
	}

	void Draw(BView* view, const BPoint& offset) const
	{
		// TODO: ...
	}

private:
	TextLayout*			fTextLayout;
	List<Line, false>	fLines;
};


// #pragma mark - TextLayout


TextLayout::TextLayout()
	:
	fText(),

	fDefaultFont(be_plain_font),
	fWidth(0.0f),

	fLayoutValid(false),
	fLayout(new (std::nothrow) Layout(this))
{
}


TextLayout::TextLayout(const TextLayout& other)
	:
	fText(other.fText),

	fDefaultFont(other.fDefaultFont),
	fWidth(other.fWidth),

	fLayoutValid(false),
	fLayout(new (std::nothrow) Layout(this))
{
}


TextLayout::~TextLayout()
{
	delete fLayout;
}


void
TextLayout::SetText(const BString& text)
{
	if (fText == text)
		return;

	fText = text;
	fLayoutValid = false;
}


void
TextLayout::SetFont(const BFont& font)
{
	if (fDefaultFont == font)
		return;

	fDefaultFont = font;
	fLayoutValid = false;
}


void
TextLayout::SetWidth(float width)
{
	if (fWidth == width)
		return;

	fWidth = width;
	fLayoutValid = false;
}


float
TextLayout::Height()
{
	if (_ValidateLayout())
		return fLayout->Height();
	return 0.0f;
}


void
TextLayout::Draw(BView* view, const BPoint& offset)
{
	if (_ValidateLayout())
		fLayout->Draw(view, offset);
}


// #pragma mark - private


bool
TextLayout::_ValidateLayout()
{
	if (!fLayoutValid && fLayout != NULL) {
		fLayout->ComputeLayout();
		return true;
	}
	return false;
}

