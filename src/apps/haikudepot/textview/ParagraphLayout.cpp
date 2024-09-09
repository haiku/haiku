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
 *      Andrew Lindesay, apl@lindesay.co.nz
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
can_end_line(const std::vector<GlyphInfo>& glyphInfos, int offset)
{
	int count = static_cast<int>(glyphInfos.size());

	if (offset == count - 1)
		return true;

	if (offset < 0 || offset > count)
		return false;

	uint32 charCode = glyphInfos[offset].charCode;
	uint32 classification = get_char_classification(charCode);

	// wrapping is always allowed at end of text and at newlines
	if (classification == CHAR_CLASS_END_OF_TEXT || charCode == '\n')
		return true;

	uint32 nextCharCode = glyphInfos[offset + 1].charCode;
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
	fTextSpans(),
	fParagraphStyle(paragraph.Style()),

	fWidth(0.0f),
	fLayoutValid(false),

	fGlyphInfos(),
	fLineInfos()
{
	_AppendTextSpans(paragraph);
	_Init();
}


ParagraphLayout::ParagraphLayout(const ParagraphLayout& other)
	:
	fTextSpans(other.fTextSpans),
	fParagraphStyle(other.fParagraphStyle),

	fWidth(other.fWidth),
	fLayoutValid(false),

	fGlyphInfos(other.fGlyphInfos),
	fLineInfos()
{
}


ParagraphLayout::~ParagraphLayout()
{
}


void
ParagraphLayout::SetParagraph(const Paragraph& paragraph)
{
	fTextSpans.clear();
	_AppendTextSpans(paragraph);
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

	if (!fLineInfos.empty()) {
		const LineInfo& lastLine = fLineInfos[fLineInfos.size() - 1];
		height = lastLine.y + lastLine.height;
	}

	return height;
}


void
ParagraphLayout::Draw(BView* view, const BPoint& offset)
{
	_ValidateLayout();

	int lineCount = static_cast<int>(fLineInfos.size());
	for (int i = 0; i < lineCount; i++) {
		const LineInfo& line = fLineInfos[i];
		_DrawLine(view, offset, line);
	}

	const Bullet& bullet = fParagraphStyle.Bullet();
	if (bullet.Spacing() > 0.0f && bullet.String().Length() > 0) {
		// Draw bullet at offset
		BPoint bulletPos(offset);
		bulletPos.x += fParagraphStyle.FirstLineInset()
			+ fParagraphStyle.LineInset();
		bulletPos.y += fLineInfos[0].maxAscent;
		view->DrawString(bullet.String(), bulletPos);
	}
}


int32
ParagraphLayout::CountGlyphs() const
{
	return static_cast<int32>(fGlyphInfos.size());
}


int32
ParagraphLayout::CountLines()
{
	_ValidateLayout();
	return static_cast<int32>(fLineInfos.size());
}


int32
ParagraphLayout::LineIndexForOffset(int32 textOffset)
{
	_ValidateLayout();

	if (fGlyphInfos.empty())
		return 0;

	if (textOffset >= static_cast<int32>(fGlyphInfos.size())) {
		const GlyphInfo& glyph = fGlyphInfos[fGlyphInfos.size() - 1];
		return glyph.lineIndex;
	}

	if (textOffset < 0)
		textOffset = 0;

	const GlyphInfo& glyph = fGlyphInfos[textOffset];
	return glyph.lineIndex;
}


int32
ParagraphLayout::FirstOffsetOnLine(int32 lineIndex)
{
	_ValidateLayout();

	if (lineIndex < 0)
		lineIndex = 0;
	int32 countLineInfos = static_cast<int32>(fLineInfos.size());
	if (lineIndex >= countLineInfos)
		lineIndex = countLineInfos - 1;

	return fLineInfos[lineIndex].textOffset;
}


int32
ParagraphLayout::LastOffsetOnLine(int32 lineIndex)
{
	_ValidateLayout();

	if (lineIndex < 0)
		lineIndex = 0;

	if (lineIndex >= static_cast<int32>(fLineInfos.size()) - 1)
		return CountGlyphs() - 1;

	return fLineInfos[lineIndex + 1].textOffset - 1;
}


void
ParagraphLayout::GetLineBounds(int32 lineIndex, float& x1, float& y1,
	float& x2, float& y2)
{
	_ValidateLayout();

	if (fGlyphInfos.empty()) {
		_GetEmptyLayoutBounds(x1, y1, x2, y2);
		return;
	}

	if (lineIndex < 0)
		lineIndex = 0;
	int32 countLineInfos = static_cast<int32>(fLineInfos.size());
	if (lineIndex >= countLineInfos)
		lineIndex = countLineInfos - 1;

	const LineInfo& lineInfo = fLineInfos[lineIndex];
	int32 firstGlyphIndex = lineInfo.textOffset;

	int32 lastGlyphIndex;
	if (lineIndex < countLineInfos - 1)
		lastGlyphIndex = fLineInfos[lineIndex + 1].textOffset - 1;
	else
		lastGlyphIndex = static_cast<int32>(fGlyphInfos.size()) - 1;

	const GlyphInfo& firstInfo = fGlyphInfos[firstGlyphIndex];
	const GlyphInfo& lastInfo = fGlyphInfos[lastGlyphIndex];

	x1 = firstInfo.x;
	y1 = lineInfo.y;
	x2 = lastInfo.x + lastInfo.width;
	y2 = lineInfo.y + lineInfo.height;
}


void
ParagraphLayout::GetTextBounds(int32 textOffset, float& x1, float& y1,
	float& x2, float& y2)
{
	_ValidateLayout();

	if (fGlyphInfos.empty()) {
		_GetEmptyLayoutBounds(x1, y1, x2, y2);
		return;
	}

	if (textOffset >= static_cast<int32>(fGlyphInfos.size())) {
		const GlyphInfo& glyph = fGlyphInfos[fGlyphInfos.size() - 1];
		const LineInfo& line = fLineInfos[glyph.lineIndex];

		x1 = glyph.x + glyph.width;
		x2 = x1;
		y1 = line.y;
		y2 = y1 + line.height;

		return;
	}

	if (textOffset < 0)
		textOffset = 0;

	const GlyphInfo& glyph = fGlyphInfos[textOffset];
	const LineInfo& line = fLineInfos[glyph.lineIndex];

	x1 = glyph.x;
	x2 = x1 + glyph.width;
	y1 = line.y;
	y2 = y1 + line.height;
}


int32
ParagraphLayout::TextOffsetAt(float x, float y, bool& rightOfCenter)
{
	_ValidateLayout();

	rightOfCenter = false;

	int32 lineCount = static_cast<int32>(fLineInfos.size());
	if (fGlyphInfos.empty() || lineCount == 0
		|| fLineInfos[0].y > y) {
		// Above first line or empty text
		return 0;
	}

	int32 lineIndex = 0;
	LineInfo lastLineInfo = fLineInfos[fLineInfos.size() - 1];
	if (floorf(lastLineInfo.y + lastLineInfo.height + 0.5) > y) {
		// TODO: Optimize, can binary search line here:
		for (; lineIndex < lineCount; lineIndex++) {
			const LineInfo& line = fLineInfos[lineIndex];
			float lineBottom = floorf(line.y + line.height + 0.5);
			if (lineBottom > y)
				break;
		}
	} else {
		lineIndex = lineCount - 1;
	}

	// Found line
	const LineInfo& line = fLineInfos[lineIndex];
	int32 textOffset = line.textOffset;
	int32 end;
	if (lineIndex < lineCount - 1)
		end = fLineInfos[lineIndex + 1].textOffset - 1;
	else
		end = fGlyphInfos.size() - 1;

	// TODO: Optimize, can binary search offset here:
	for (; textOffset <= end; textOffset++) {
		const GlyphInfo& glyph = fGlyphInfos[textOffset];
		float x1 = glyph.x;
		if (x1 > x)
			return textOffset;

		// x2 is the location at the right bounding box of the glyph
		float x2 = x1 + glyph.width;

		// x3 is the location of the next glyph, which may be different from
		// x2 in case the line is justified.
		float x3;
		if (textOffset < end - 1)
			x3 = fGlyphInfos[textOffset + 1].x;
		else
			x3 = x2;

		if (x3 > x) {
			rightOfCenter = x > (x1 + x2) / 2.0f;
			return textOffset;
		}
	}

	// Account for trailing line break at end of line, the
	// returned offset should be before that.
	rightOfCenter = fGlyphInfos[end].charCode != '\n';

	return end;
}


// #pragma mark - private


void
ParagraphLayout::_Init()
{
	fGlyphInfos.clear();

	std::vector<TextSpan>::const_iterator it;
	for (it = fTextSpans.begin(); it != fTextSpans.end(); it++) {
		const TextSpan& span = *it;
		if (!_AppendGlyphInfos(span)) {
			fprintf(stderr, "%p->ParagraphLayout::_Init() - Out of memory\n",
				this);
			return;
		}
	}
}


void
ParagraphLayout::_ValidateLayout()
{
	if (!fLayoutValid) {
		_Layout();
		fLayoutValid = true;
	}
}


void
ParagraphLayout::_Layout()
{
	fLineInfos.clear();

	const Bullet& bullet = fParagraphStyle.Bullet();

	float x = fParagraphStyle.LineInset() + fParagraphStyle.FirstLineInset()
		+ bullet.Spacing();
	float y = 0.0f;
	int lineIndex = 0;
	int lineStart = 0;

	int glyphCount = static_cast<int>(fGlyphInfos.size());
	for (int i = 0; i < glyphCount; i++) {
		GlyphInfo glyph = fGlyphInfos[i];

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

		if (glyph.charCode == '\n') {
			nextLine = true;
			lineBreak = true;
			glyph.x = x;
			fGlyphInfos[i] = glyph;
		} else if (fWidth > 0.0f && x + advanceX > fWidth) {
			fGlyphInfos[i] = glyph;
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
					glyph = fGlyphInfos[i];
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
			x = fParagraphStyle.LineInset() + bullet.Spacing();
			y += lineHeight + fParagraphStyle.LineSpacing();

			if (lineBreak)
				lineStart = i + 1;
			else
				lineStart = i;

			lineIndex++;
		}

		if (!lineBreak && i < glyphCount) {
			glyph.x = x;
			fGlyphInfos[i] = glyph;
		}

		x += advanceX;
		y += advanceY;
	}

	// The last line may not have been appended and initialized yet.
	if (lineStart <= glyphCount - 1 || glyphCount == 0) {
		float lineHeight;
		_FinalizeLine(lineStart, glyphCount - 1, lineIndex, y, lineHeight);
	}

	_ApplyAlignment();
}


void
ParagraphLayout::_ApplyAlignment()
{
	Alignment alignment = fParagraphStyle.Alignment();
	bool justify = fParagraphStyle.Justify();

	if (alignment == ALIGN_LEFT && !justify)
		return;

	int glyphCount = static_cast<int>(fGlyphInfos.size());
	if (glyphCount == 0)
		return;

	int lineIndex = -1;
	float spaceLeft = 0.0f;
	float charSpace = 0.0f;
	float whiteSpace = 0.0f;
	bool seenChar = false;

	// Iterate all glyphs backwards. On the last character of the next line,
	// the position of the character determines the available space to be
	// distributed (spaceLeft).
	for (int i = glyphCount - 1; i >= 0; i--) {
		GlyphInfo glyph = fGlyphInfos[i];

		if (glyph.lineIndex != lineIndex) {
			bool lineBreak = glyph.charCode == '\n' || i == glyphCount - 1;
			lineIndex = glyph.lineIndex;

			// The position of the last character determines the available
			// space.
			spaceLeft = fWidth - glyph.x;

			// If the character is visible, the width of the character needs to
			// be subtracted from the available space, otherwise it would be
			// pushed outside the line.
			uint32 charClassification = get_char_classification(glyph.charCode);
			if (charClassification != CHAR_CLASS_WHITESPACE)
				spaceLeft -= glyph.width;

			charSpace = 0.0f;
			whiteSpace = 0.0f;
			seenChar = false;

			if (lineBreak || !justify) {
				if (alignment == ALIGN_CENTER)
					spaceLeft /= 2.0f;
				else if (alignment == ALIGN_LEFT)
					spaceLeft = 0.0f;
			} else {
				// Figure out how much chars and white space chars are on the
				// line. Don't count trailing white space.
				int charCount = 0;
				int spaceCount = 0;
				for (int j = i; j >= 0; j--) {
					const GlyphInfo& previousGlyph = fGlyphInfos[j];
					if (previousGlyph.lineIndex != lineIndex) {
						j++;
						break;
					}
					uint32 classification = get_char_classification(
						previousGlyph.charCode);
					if (classification == CHAR_CLASS_WHITESPACE) {
						if (charCount > 0)
							spaceCount++;
						else if (j < i)
							spaceLeft += glyph.width;
					} else {
						charCount++;
					}
				}

				// The first char is not shifted when justifying, so it doesn't
				// contribute.
				if (charCount > 0)
					charCount--;

				// Check if it looks better if both whitespace and chars get
				// some space distributed, in case there are only 1 or two
				// space chars on the line.
				float spaceLeftForSpace = spaceLeft;
				float spaceLeftForChars = spaceLeft;

				if (spaceCount > 0) {
					float spaceCharRatio = (float) spaceCount / charCount;
					if (spaceCount < 3 && spaceCharRatio < 0.4f) {
						spaceLeftForSpace = spaceLeft * 2.0f * spaceCharRatio;
						spaceLeftForChars = spaceLeft - spaceLeftForSpace;
					} else
						spaceLeftForChars = 0.0f;
				}

				if (spaceCount > 0)
					whiteSpace = spaceLeftForSpace / spaceCount;
				if (charCount > 0)
					charSpace = spaceLeftForChars / charCount;

				LineInfo line = fLineInfos[lineIndex];
				line.extraGlyphSpacing = charSpace;
				line.extraWhiteSpacing = whiteSpace;

				fLineInfos[lineIndex] = line;
			}
		}

		// Each character is pushed towards the right by the space that is
		// still available. When justification is performed, the shift is
		// gradually decreased. This works since the iteration is backwards
		// and the characters on the right are pushed farthest.
		glyph.x += spaceLeft;

		unsigned classification = get_char_classification(glyph.charCode);

		if (i < glyphCount - 1) {
			GlyphInfo nextGlyph = fGlyphInfos[i + 1];
			if (nextGlyph.lineIndex == lineIndex) {
				uint32 nextClassification
					= get_char_classification(nextGlyph.charCode);
				if (nextClassification == CHAR_CLASS_WHITESPACE
					&& classification != CHAR_CLASS_WHITESPACE) {
					// When a space character is right of a regular character,
					// add the additional space to the space instead of the
					// character
					float shift = (nextGlyph.x - glyph.x) - glyph.width;
					nextGlyph.x -= shift;
					fGlyphInfos[i + 1] = nextGlyph;
				}
			}
		}

		fGlyphInfos[i] = glyph;

		// The shift (spaceLeft) is reduced depending on the character
		// classification.
		if (classification == CHAR_CLASS_WHITESPACE) {
			if (seenChar)
				spaceLeft -= whiteSpace;
		} else {
			seenChar = true;
			spaceLeft -= charSpace;
		}
	}
}


bool
ParagraphLayout::_AppendGlyphInfos(const TextSpan& span)
{
	int charCount = span.CountChars();
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

	try {
		fGlyphInfos.push_back(GlyphInfo(charCode, 0.0f, width, 0));
	}
	catch (std::bad_alloc& ba) {
		fprintf(stderr, "bad_alloc occurred adding glyph info to a "
			"paragraph\n");
		return false;
	}

	return true;
}


bool
ParagraphLayout::_FinalizeLine(int lineStart, int lineEnd, int lineIndex,
	float y, float& lineHeight)
{
	LineInfo line(lineStart, y, 0.0f, 0.0f, 0.0f);

	int spanIndex = -1;
	int spanStart = 0;
	int spanEnd = 0;

	for (int i = lineStart; i <= lineEnd; i++) {
		// Mark line index in glyph
		GlyphInfo glyph = fGlyphInfos[i];
		glyph.lineIndex = lineIndex;
		fGlyphInfos[i] = glyph;

		// See if the next sub-span needs to be added to the LineInfo
		bool addSpan = false;

		while (i >= spanEnd) {
			spanIndex++;
			const TextSpan& span = fTextSpans[spanIndex];
			spanStart = spanEnd;
			spanEnd += span.CountChars();
			addSpan = true;
		}

		if (addSpan) {
			const TextSpan& span = fTextSpans[spanIndex];
			TextSpan subSpan = span.SubSpan(i - spanStart,
				(lineEnd - spanStart + 1) - (i - spanStart));
			line.layoutedSpans.push_back(subSpan);
			_IncludeStyleInLine(line, span.Style());
		}
	}

	if (fGlyphInfos.empty() && !fTextSpans.empty()) {
		// When the layout contains no glyphs, but there is at least one
		// TextSpan in the paragraph, use the font info from that span
		// to calculate the height of the first LineInfo.
		const TextSpan& span = fTextSpans[0];
		line.layoutedSpans.push_back(span);
		_IncludeStyleInLine(line, span.Style());
	}

	lineHeight = line.height;

	try {
		fLineInfos.push_back(line);
	}
	catch (std::bad_alloc& ba) {
		fprintf(stderr, "bad_alloc occurred adding line to line infos\n");
		return false;
	}

	return true;
}


void
ParagraphLayout::_IncludeStyleInLine(LineInfo& line,
	const CharacterStyle& style)
{
	float ascent = style.Ascent();
	if (ascent > line.maxAscent)
		line.maxAscent = ascent;

	float descent = style.Descent();
	if (descent > line.maxDescent)
		line.maxDescent = descent;

	float height = ascent + descent;
	if (style.Font().Size() > height)
		height = style.Font().Size();

	if (height > line.height)
		line.height = height;
}


void
ParagraphLayout::_DrawLine(BView* view, const BPoint& offset,
	const LineInfo& line) const
{
	int textOffset = line.textOffset;
	int spanCount = static_cast<int>(line.layoutedSpans.size());
	for (int i = 0; i < spanCount; i++) {
		const TextSpan& span = line.layoutedSpans[i];
		_DrawSpan(view, offset, span, textOffset);
		textOffset += span.CountChars();
	}
}


void
ParagraphLayout::_DrawSpan(BView* view, BPoint offset,
	const TextSpan& span, int32 textOffset) const
{
	const BString& text = span.Text();
	if (text.Length() == 0)
		return;

	const GlyphInfo& glyph = fGlyphInfos[textOffset];
	const LineInfo& line = fLineInfos[glyph.lineIndex];

	offset.x += glyph.x;
	offset.y += line.y + line.maxAscent;

	const CharacterStyle& style = span.Style();

	view->SetFont(&style.Font());

	if (style.WhichForegroundColor() != B_NO_COLOR)
		view->SetHighUIColor(style.WhichForegroundColor());
	else
		view->SetHighColor(style.ForegroundColor());

	// TODO: Implement other style properties

	escapement_delta delta;
	delta.nonspace = line.extraGlyphSpacing;
	delta.space = line.extraWhiteSpacing;

	view->DrawString(span.Text(), offset, &delta);
}


void
ParagraphLayout::_GetEmptyLayoutBounds(float& x1, float& y1, float& x2,
	float& y2) const
{
	if (fLineInfos.empty()) {
		x1 = 0.0f;
		y1 = 0.0f;
		x2 = 0.0f;
		y2 = 0.0f;

		return;
	}

	// If the paragraph had at least a single empty TextSpan, the layout
	// can compute some meaningful bounds.
	const Bullet& bullet = fParagraphStyle.Bullet();
	x1 = fParagraphStyle.LineInset() + fParagraphStyle.FirstLineInset()
		+ bullet.Spacing();
	x2 = x1;
	const LineInfo& lineInfo = fLineInfos[0];
	y1 = lineInfo.y;
	y2 = lineInfo.y + lineInfo.height;
}


void
ParagraphLayout::_AppendTextSpans(const Paragraph& paragraph)
{
	int32 countTextSpans = paragraph.CountTextSpans();
	for (int32 i = 0; i< countTextSpans; i++)
		fTextSpans.push_back(paragraph.TextSpanAtIndex(i));
}
