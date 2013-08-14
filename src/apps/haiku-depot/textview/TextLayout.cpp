/*
 * Copyright 2001-2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Marc Flerackers, mflerackers@androme.be
 *		Hiroshi Lockheimer (BTextView is based on his STEEngine)
 *		Oliver Tappe, zooey@hirschkaefer.de
 */

#include "TextLayout.h"

#include <new>
#include <stdio.h>

#include <AutoDeleter.h>
#include <utf8_functions.h>
#include <View.h>

#include "GlyphInfo.h"
#include "List.h"
#include "TextStyle.h"


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


inline uint32
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


// #pragma mark - TextLayout


TextLayout::TextLayout()
	:
	fText(),

	fDefaultFont(),
	fAscent(0.0f),
	fDescent(0.0f),
	fWidth(0.0f),
	fGlyphSpacing(0.0f),
	fFirstLineInset(0.0f),
	fLineInset(0.0f),
	fLineSpacing(0.0f),

	fLayoutValid(false),
	fGlyphInfoBuffer(NULL),
	fGlyphInfoCount(0)
{
	// Init fAscent and fDescent
	SetFont(BFont(be_plain_font));
}


TextLayout::TextLayout(const TextLayout& other)
	:
	fText(other.fText),

	fDefaultFont(other.fDefaultFont),
	fAscent(other.fAscent),
	fDescent(other.fDescent),
	fWidth(other.fWidth),
	fGlyphSpacing(other.fGlyphSpacing),
	fFirstLineInset(other.fFirstLineInset),
	fLineInset(other.fLineInset),
	fLineSpacing(other.fLineSpacing),

	fLayoutValid(false),
	fGlyphInfoBuffer(NULL),
	fGlyphInfoCount(0)
{
}


TextLayout::~TextLayout()
{
	delete[] fGlyphInfoBuffer;
}


void
TextLayout::SetText(const BString& text)
{
	if (fText != text) {
		fText = text;
		fLayoutValid = false;
	}
}


void
TextLayout::SetFont(const BFont& font)
{
	if (fDefaultFont != font || fAscent == 0.0f) {
		fDefaultFont = font;

		font_height fontHeight;
		font.GetHeight(&fontHeight);

		fAscent = fontHeight.ascent;
		fDescent = fontHeight.descent;

		fLayoutValid = false;
	}
}


void
TextLayout::SetGlyphSpacing(float glyphSpacing)
{
	if (fGlyphSpacing != glyphSpacing) {
		fGlyphSpacing = glyphSpacing;
		fLayoutValid = false;
	}
}


void
TextLayout::SetFirstLineInset(float inset)
{
	if (fFirstLineInset != inset) {
		fFirstLineInset = inset;
		fLayoutValid = false;
	}
}


void
TextLayout::SetLineInset(float inset)
{
	if (fLineInset != inset) {
		fLineInset = inset;
		fLayoutValid = false;
	}
}


void
TextLayout::SetLineSpacing(float spacing)
{
	if (fLineSpacing != spacing) {
		fLineSpacing = spacing;
		fLayoutValid = false;
	}
}


void
TextLayout::SetWidth(float width)
{
	if (fWidth != width) {
		fWidth = width;
		fLayoutValid = false;
	}
}


float
TextLayout::Height()
{
	_ValidateLayout();

	float height = fDefaultFont.Size();

	if (fLineInfos.CountItems() > 0) {
		const LineInfo& lastLine = fLineInfos.LastItem();
		height = lastLine.y + lastLine.height;
	}

	return height;
}


void
TextLayout::Draw(BView* view, const BPoint& offset)
{
	_ValidateLayout();
	
	// TODO: Support styles and all
	view->SetFont(&fDefaultFont);
	view->SetHighColor(0, 0, 0);
	
	int lineCount = fLineInfos.CountItems();
	for (int i = 0; i < lineCount; i++) {
		const LineInfo& line = fLineInfos.ItemAtFast(i);
		
		int startOffset = line.textOffset;
		int endOffset;
		if (i < lineCount - 1) {
			const LineInfo& nextLine = fLineInfos.ItemAtFast(i + 1);
			endOffset = nextLine.textOffset;
		} else {
			endOffset = fGlyphInfoCount;
		}
		
		BString string;
		fText.CopyCharsInto(string, startOffset, endOffset - startOffset);
		
		float x = fGlyphInfoBuffer[startOffset].x;
		float y = fGlyphInfoBuffer[startOffset].y;

//printf("%p->%.1f,%.1f: '%s'\n", this, x, y, string.String());
		view->DrawString(string, BPoint(x, y));
	}
}


// #pragma mark - private


void
TextLayout::_ValidateLayout()
{
	if (!fLayoutValid)
		_Layout();
}


void
TextLayout::_Layout()
{
	fLineInfos.Clear();
	delete[] fGlyphInfoBuffer;
	fGlyphInfoBuffer = NULL;

	fGlyphInfoCount = fText.CountChars();
	if (fGlyphInfoCount == 0)
		return;
	
	// Allocate arrays
	float* escapementArray = new (std::nothrow) float[fGlyphInfoCount];
	if (escapementArray == NULL)
		return;
	ArrayDeleter<float> escapementDeleter(escapementArray);

	// Fetch glyph spacing information
	fDefaultFont.GetEscapements(fText.String(), fGlyphInfoCount,
		escapementArray);

	// Convert to glyph buffer
	fGlyphInfoBuffer = new (std::nothrow) GlyphInfo[fGlyphInfoCount];
	
	// Init glyph buffer and convert escapement scale
	float size = fDefaultFont.Size();
	const char* c = fText.String();
	for (int i = 0; i < fGlyphInfoCount; i++) {
		fGlyphInfoBuffer[i].charCode = UTF8ToCharCode(&c);
		escapementArray[i] *= size;
	}


	float x = fFirstLineInset;
	float y = 0.0f;
	int lineIndex = 0;
	int lineStart = 0;

	for (int i = 0; i < fGlyphInfoCount; i++) {
		uint32 charClassification = get_char_classification(
			fGlyphInfoBuffer[i].charCode);

		float advanceX = 0.0f;
		float advanceY = 0.0f;
		_GetGlyphAdvance(i, advanceX, advanceY, escapementArray);

		bool nextLine = false;
		bool lineBreak = false;

//		if (fGlyphInfoBuffer[i].charCode == '\t') {
//			// Figure out tab width, it's the width between the last two tab
//			// stops.
//			float tabWidth = 0.0f;
//			if (fTabCount > 0)
//				tabWidth = fTabBuffer[fTabCount - 1];
//			if (fTabCount > 1)
//				tabWidth -= fTabBuffer[fTabCount - 2];
//
//			// Try to find a tab stop that is farther than the current x
//			// offset
//			double tabOffset = 0.0;
//			for (unsigned tabIndex = 0; tabIndex < fTabCount; tabIndex++) {
//				tabOffset = fTabBuffer[tabIndex];
//				if (tabOffset > x)
//					break;
//			}
//
//			// If no tab stop has been found, make the tab stop a multiple of
//			// the tab width
//			if (tabOffset <= x && tabWidth > 0.0)
//				tabOffset = ((int) (x / tabWidth) + 1) * tabWidth;
//
//			if (tabOffset - x > 0.0)
//				advanceX = tabOffset - x;
//		}

		fGlyphInfoBuffer[i].advanceX = advanceX;

		if (fGlyphInfoBuffer[i].charCode == '\n') {
			nextLine = true;
			lineBreak = true;
			fGlyphInfoBuffer[i].x = x;
			fGlyphInfoBuffer[i].y = y;
		} else if (fWidth > 0.0f && x + advanceX > fWidth) {
			if (charClassification == CHAR_CLASS_WHITESPACE) {
				advanceX = 0.0f;
			} else if (i > lineStart) {
				nextLine = true;
				// The current glyph extends outside the width, we need to wrap
				// to the next line. See what previous offset can be the end
				// of the line.
				int lineEnd = i - 1;
				while (lineEnd > lineStart
					&& !can_end_line(fGlyphInfoBuffer, lineEnd,
						fGlyphInfoCount)) {
					lineEnd--;
				}

				if (lineEnd > lineStart) {
					// Found a place to perform a line break.
					i = lineEnd + 1;
					// Adjust the glyph info to point at the changed buffer
					// position
					_GetGlyphAdvance(i, advanceX, advanceY, escapementArray);
				} else {
					// Just break where we are.
				}
			}
		}

		if (nextLine) {
			// * Initialize the max ascent/descent of all preceding glyph infos
			// on the current/last line
			// * Adjust the baseline offset according to the max ascent
			// * Fill in the line index.
			unsigned lineEnd;
			if (lineBreak)
				lineEnd = i;
			else
				lineEnd = i - 1;

			float lineHeight = 0.0;
			_FinalizeLine(lineStart, lineEnd, lineIndex, y,
				lineHeight);

			// Start position of the next line
			x = fLineInset;
			y += lineHeight + fLineSpacing;

			if (lineBreak)
				lineStart = i + 1;
			else
				lineStart = i;

			lineIndex++;
		}

		if (!lineBreak && i < fGlyphInfoCount) {
			fGlyphInfoBuffer[i].x = x;
			fGlyphInfoBuffer[i].y = y;
		}

		x += advanceX;
		y += advanceY;
	}

	// The last line may not have been appended and initialized yet.
	if (lineStart <= fGlyphInfoCount - 1) {
		float lineHeight;
		_FinalizeLine(lineStart, fGlyphInfoCount - 1, lineIndex, y, lineHeight);
	}
}


void
TextLayout::_GetGlyphAdvance(int offset, float& advanceX, float& advanceY,
	float escapementArray[]) const
{
	float additionalGlyphSpacing = 0.0f;
	if (fGlyphInfoBuffer[offset].style.Get() != NULL)
		additionalGlyphSpacing = fGlyphInfoBuffer[offset].style->glyphSpacing;
	else
		additionalGlyphSpacing = fGlyphSpacing;

	if (fGlyphInfoBuffer[offset].style.Get() != NULL
		&& fGlyphInfoBuffer[offset].style->width > 0.0f) {
		// Use the metrics provided by the TextStyle
		advanceX = fGlyphInfoBuffer[offset].style->width
			+ additionalGlyphSpacing;
	} else {
		advanceX = escapementArray[offset] + additionalGlyphSpacing;
		advanceY = 0.0f;
	}
}


void
TextLayout::_FinalizeLine(int lineStart, int lineEnd, int lineIndex, float y,
	float& lineHeight)
{
	lineHeight = 0.0f;
	float maxAscent = 0.0f;
	float maxDescent = 0.0f;

	for (int i = lineStart; i <= lineEnd; i++) {
		if (fGlyphInfoBuffer[i].style.Get() != NULL) {
			if (fGlyphInfoBuffer[i].style->font.Size() > lineHeight)
				lineHeight = fGlyphInfoBuffer[i].style->font.Size();
			if (fGlyphInfoBuffer[i].style->ascent > maxAscent)
				maxAscent = fGlyphInfoBuffer[i].style->ascent;
			if (fGlyphInfoBuffer[i].style->descent > maxDescent)
				maxDescent = fGlyphInfoBuffer[i].style->descent;
		} else {
			if (fDefaultFont.Size() > lineHeight)
				lineHeight = fDefaultFont.Size();
			if (fAscent > maxAscent)
				maxAscent = fAscent;
			if (fDescent > maxDescent)
				maxDescent = fDescent;
		}
	}

	fLineInfos.Add(LineInfo(lineStart, y, lineHeight, maxAscent, maxDescent));

	for (int i = lineStart; i <= lineEnd; i++) {
		fGlyphInfoBuffer[i].maxAscend = maxAscent;
		fGlyphInfoBuffer[i].maxDescend = maxDescent;
		fGlyphInfoBuffer[i].lineIndex = lineIndex;
		fGlyphInfoBuffer[i].y += maxAscent;
	}
}
