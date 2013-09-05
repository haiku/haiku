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

#include "ParagraphLayout.h"

#include <new>
#include <stdio.h>

#include <AutoDeleter.h>
#include <utf8_functions.h>
#include <View.h>


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
can_end_line(const GlyphInfoList& glyphInfos, int offset)
{
	int count = glyphInfos.CountItems();

	if (offset == count - 1)
		return true;

	if (offset < 0 || offset > count)
		return false;

	uint32 charCode = glyphInfos.ItemAtFast(offset).charCode;
	uint32 classification = get_char_classification(charCode);

	// wrapping is always allowed at end of text and at newlines
	if (classification == CHAR_CLASS_END_OF_TEXT || charCode == '\n')
		return true;

	uint32 nextCharCode = glyphInfos.ItemAtFast(offset + 1).charCode;
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


// #pragma mark - ParagraphLayout


ParagraphLayout::ParagraphLayout()
	:
	fTextSpans(),
	fParagraphStyle(),

	fWidth(0.0f),
	fLayoutValid(false),

	fGlyphInfos(),
	fLineInfos()
{
}


ParagraphLayout::ParagraphLayout(const Paragraph& paragraph)
	:
	fTextSpans(paragraph.TextSpans()),
	fParagraphStyle(paragraph.Style()),

	fWidth(0.0f),
	fLayoutValid(false),

	fGlyphInfos(),
	fLineInfos()
{
	_Init();
}


ParagraphLayout::ParagraphLayout(const ParagraphLayout& other)
	:
	fTextSpans(other.fTextSpans),
	fParagraphStyle(other.fParagraphStyle),
	
	fWidth(other.fWidth),
	fLayoutValid(false),

	fGlyphInfos(),
	fLineInfos()
{
	_Init();
}


ParagraphLayout::~ParagraphLayout()
{
}


void
ParagraphLayout::SetParagraph(const Paragraph& paragraph)
{
	fTextSpans = paragraph.TextSpans();
	fParagraphStyle = paragraph.Style();

	_Init();

	fLayoutValid = false;
}


void
ParagraphLayout::SetWidth(float width)
{
	if (fWidth != width) {
		fWidth = width;
		fLayoutValid = false;
	}
}


float
ParagraphLayout::Height()
{
	_ValidateLayout();

	float height = 0.0f;

	if (fLineInfos.CountItems() > 0) {
		const LineInfo& lastLine = fLineInfos.LastItem();
		height = lastLine.y + lastLine.height;
	}

	return height;
}


void
ParagraphLayout::Draw(BView* view, const BPoint& offset)
{
	_ValidateLayout();
	
	int lineCount = fLineInfos.CountItems();
	for (int i = 0; i < lineCount; i++) {
		const LineInfo& line = fLineInfos.ItemAtFast(i);
		_DrawLine(view, line);
	}
}


// #pragma mark - private


void
ParagraphLayout::_ValidateLayout()
{
	if (!fLayoutValid)
		_Layout();
}


void
ParagraphLayout::_Layout()
{
	fLineInfos.Clear();

	float x = fParagraphStyle.FirstLineInset();
	float y = 0.0f;
	int lineIndex = 0;
	int lineStart = 0;

	int glyphCount = fGlyphInfos.CountItems();
	for (int i = 0; i < glyphCount; i++) {
		GlyphInfo glyph = fGlyphInfos.ItemAtFast(i);
		
		uint32 charClassification = get_char_classification(glyph.charCode);

		float advanceX = glyph.width;
		float advanceY = 0.0f;

		bool nextLine = false;
		bool lineBreak = false;

//		if (glyph.charCode == '\t') {
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

		glyph.advanceX = advanceX;

		if (glyph.charCode == '\n') {
			nextLine = true;
			lineBreak = true;
			glyph.x = x;
			fGlyphInfos.Replace(i, glyph);
		} else if (fWidth > 0.0f && x + advanceX > fWidth) {
			fGlyphInfos.Replace(i, glyph);
			if (charClassification == CHAR_CLASS_WHITESPACE) {
				advanceX = 0.0f;
			} else if (i > lineStart) {
				nextLine = true;
				// The current glyph extends outside the width, we need to wrap
				// to the next line. See what previous offset can be the end
				// of the line.
				int lineEnd = i - 1;
				while (lineEnd > lineStart
					&& !can_end_line(fGlyphInfos, lineEnd)) {
					lineEnd--;
				}

				if (lineEnd > lineStart) {
					// Found a place to perform a line break.
					i = lineEnd + 1;

					// Adjust the glyph info to point at the changed buffer
					// position
					glyph = fGlyphInfos.ItemAtFast(i);
					advanceX = glyph.width;
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
			_FinalizeLine(lineStart, lineEnd, lineIndex, y, lineHeight);

			// Start position of the next line
			x = fParagraphStyle.LineInset();
			y += lineHeight + fParagraphStyle.LineSpacing();

			if (lineBreak)
				lineStart = i + 1;
			else
				lineStart = i;

			lineIndex++;
		}

		if (!lineBreak && i < glyphCount) {
			glyph.x = x;
			fGlyphInfos.Replace(i, glyph);
		}

		x += advanceX;
		y += advanceY;
	}

	// The last line may not have been appended and initialized yet.
	if (lineStart <= glyphCount - 1) {
		float lineHeight;
		_FinalizeLine(lineStart, glyphCount - 1, lineIndex, y, lineHeight);
	}
}


void
ParagraphLayout::_Init()
{
	fGlyphInfos.Clear();

	int spanCount = fTextSpans.CountItems();
	for (int i = 0; i < spanCount; i++) {
		const TextSpan& span = fTextSpans.ItemAtFast(i);
		if (!_AppendGlyphInfos(span)) {
			fprintf(stderr, "%p->ParagraphLayout::_Init() - Out of memory\n",
				this);
			return;
		}
	}
}


bool
ParagraphLayout::_AppendGlyphInfos(const TextSpan& span)
{
	int charCount = span.CharCount();
	if (charCount == 0)
		return true;
	
	const BString& text = span.Text();
	const BFont& font = span.Style().Font();

	// Allocate arrays
	float* escapementArray = new (std::nothrow) float[charCount];
	if (escapementArray == NULL)
		return false;
	ArrayDeleter<float> escapementDeleter(escapementArray);

	// Fetch glyph spacing information
	font.GetEscapements(text, charCount, escapementArray);

	// Append to glyph buffer and convert escapement scale
	float size = font.Size();
	const char* c = text.String();
	for (int i = 0; i < charCount; i++) {
		if (!_AppendGlyphInfo(UTF8ToCharCode(&c), escapementArray[i] * size,
				span.Style())) {
			return false;
		}
	}

	return true;
}


bool
ParagraphLayout::_AppendGlyphInfo(uint32 charCode, float width,
	const CharacterStyle& style)
{
	if (style.Width() >= 0.0f) {
		// Use the metrics provided by the CharacterStyle and override
		// the font provided metrics passed in "width"
		width = style.Width();
	}

	width += style.GlyphSpacing();
	
	GlyphInfo glyph(charCode, 0.0f, width, width, 0, style);
	return fGlyphInfos.Add(glyph);
}


void
ParagraphLayout::_FinalizeLine(int lineStart, int lineEnd, int lineIndex,
	float y, float& lineHeight)
{
	lineHeight = 0.0f;
	float maxAscent = 0.0f;
	float maxDescent = 0.0f;

	for (int i = lineStart; i <= lineEnd; i++) {
		GlyphInfo glyph = fGlyphInfos.ItemAtFast(i);
		glyph.lineIndex = lineIndex;
		fGlyphInfos.Replace(i, glyph);

		const CharacterStyle& style = glyph.style;
		
		if (style.Font().Size() > lineHeight)
			lineHeight = style.Font().Size();
		
		if (style.Ascent() > maxAscent)
			maxAscent = style.Ascent();

		if (style.Descent() > maxDescent)
			maxDescent = style.Descent();
	}

	fLineInfos.Add(LineInfo(lineStart, y, lineHeight, maxAscent, maxDescent));
}


void
ParagraphLayout::_DrawLine(BView* view, const LineInfo& line) const
{
	int textOffset = line.textOffset;
	int spanCount = line.layoutedSpans.CountItems();
	for (int i = 0; i < spanCount; i++) {
		const TextSpan& span = line.layoutedSpans.ItemAtFast(i);
		_DrawSpan(view, span, textOffset);
		textOffset += span.CharCount();
	}
}


void
ParagraphLayout::_DrawSpan(BView* view, const TextSpan& span,
	int32 textOffset) const
{
	const GlyphInfo& glyph = fGlyphInfos.ItemAtFast(textOffset);
	const LineInfo& line = fLineInfos.ItemAtFast(glyph.lineIndex);
	
	float x = glyph.x;
	float y = line.y + line.maxAscent;

	const CharacterStyle& style = span.Style();

	view->SetFont(&style.Font());
	view->SetHighColor(style.ForegroundColor());

	// TODO: Implement other style properties
	// TODO: Implement correct glyph spacing

	view->DrawString(span.Text(), BPoint(x, y));
}
