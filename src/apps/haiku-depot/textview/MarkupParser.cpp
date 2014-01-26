/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "MarkupParser.h"

#include <new>

#include <math.h>

#include <utf8_functions.h>


MarkupParser::MarkupParser()
	:
	fNormalStyle(),
	fBoldStyle(),
	fItalicStyle(),
	fBoldItalicStyle(),
	fHeadingStyle(),

	fParagraphStyle(),
	fHeadingParagraphStyle(),
	fBulletStyle(),

	fCurrentCharacterStyle(&fNormalStyle),
	fCurrentParagraphStyle(&fParagraphStyle),
	fSpanStartOffset(0)
{
	_InitStyles();
}


MarkupParser::MarkupParser(const CharacterStyle& characterStyle,
	const ParagraphStyle& paragraphStyle)
	:
	fNormalStyle(characterStyle),
	fBoldStyle(),
	fItalicStyle(),
	fBoldItalicStyle(),
	fHeadingStyle(),

	fParagraphStyle(paragraphStyle),
	fHeadingParagraphStyle(),
	fBulletStyle(),

	fCurrentCharacterStyle(&fNormalStyle),
	fCurrentParagraphStyle(&fParagraphStyle),
	fSpanStartOffset(0)
{
	_InitStyles();
}


void
MarkupParser::SetStyles(const CharacterStyle& characterStyle,
	const ParagraphStyle& paragraphStyle)
{
	fNormalStyle = characterStyle;
	fParagraphStyle = paragraphStyle;
	_InitStyles();
}


TextDocumentRef
MarkupParser::CreateDocumentFromMarkup(const BString& text)
{
	TextDocumentRef document(new(std::nothrow) TextDocument(), true);
	if (document.Get() == NULL)
		return document;

	AppendMarkup(document, text);

	return document;
}


void
MarkupParser::AppendMarkup(const TextDocumentRef& document, const BString& text)
{
	fTextDocument.SetTo(document);

	fCurrentCharacterStyle = &fNormalStyle;
	fCurrentParagraphStyle = &fParagraphStyle;

	fCurrentParagraph = Paragraph(*fCurrentParagraphStyle);
	fSpanStartOffset = 0;

	_ParseText(text);

	fTextDocument.Unset();
}


// #pragma mark - private


void
MarkupParser::_InitStyles()
{
	fBoldStyle = fNormalStyle;
	fBoldStyle.SetBold(true);

	fItalicStyle = fNormalStyle;
	fItalicStyle.SetItalic(true);

	fBoldItalicStyle = fNormalStyle;
	fBoldItalicStyle.SetBold(true);
	fBoldItalicStyle.SetItalic(true);

	float fontSize = fNormalStyle.Font().Size();

	fHeadingStyle = fNormalStyle;
	fHeadingStyle.SetFontSize(ceilf(fontSize * 1.15f));
	fHeadingStyle.SetBold(true);

	fHeadingParagraphStyle = fParagraphStyle;
	fHeadingParagraphStyle.SetSpacingTop(ceilf(fontSize * 0.8f));
	fHeadingParagraphStyle.SetSpacingBottom(ceilf(fontSize * 0.5f));
	fHeadingParagraphStyle.SetJustify(false);

	fBulletStyle = fParagraphStyle;
	fBulletStyle.SetBullet(Bullet("•", fontSize));
	fBulletStyle.SetLineInset(ceilf(fontSize * 0.8f));
}


void
MarkupParser::_ParseText(const BString& text)
{
	int32 start = 0;
	int32 offset = 0;

	int32 charCount = text.CountChars();
	const char* c = text.String();

	while (offset <= charCount) {
		uint32 nextChar = UTF8ToCharCode(&c);

		switch (nextChar) {
// Requires two line-breaks to start a new paragraph, unles the current
// paragraph is already considered a bullet list item. Doesn't work well
// with current set of packages.
//			case '\n':
//				_CopySpan(text, start, offset);
//				if (offset + 1 < charCount && c[0] == '\n') {
//					_FinishParagraph();
//					offset += 1;
//					c += 1;
//				} else if (fCurrentParagraph.Style() == fBulletStyle) {
//					_FinishParagraph();
//				}
//				start = offset + 1;
//				break;

			case '\n':
				_CopySpan(text, start, offset);
				if (offset > 0 && c[-1] != ' ')
					_FinishParagraph(offset >= charCount);
				start = offset + 1;
				break;

			case '\0':
				_CopySpan(text, start, offset);
				_FinishParagraph(true);
				start = offset + 1;
				break;

			case '\'':
				if (offset + 2 < charCount && c[0] == '\'') {
					int32 tickCount = 2;
					if (c[1] == '\'')
						tickCount = 3;

					// Copy previous span using current style, excluding the
					// ticks.
					_CopySpan(text, start, offset);

					if (tickCount == 2)
						_ToggleStyle(fItalicStyle);
					else if (tickCount == 3)
						_ToggleStyle(fBoldStyle);

					// Don't include the ticks in the next span.
					offset += tickCount - 1;
					start = offset + 1;
					c += tickCount - 1;
				}
				break;

			case '=':
				// Detect headings
				if (offset == start
					&& fCurrentParagraph.IsEmpty()
					&& offset + 2 < charCount
					&& c[0] == '=' && c[1] == ' ') {

					fCurrentParagraph.SetStyle(fHeadingParagraphStyle);
					fCurrentCharacterStyle = &fHeadingStyle;

					offset += 2;
					c += 2;

					start = offset + 1;
				} else if (offset > start
					&& offset + 2 < charCount
					&& c[0] == '=' && c[1] == '\n') {

					_CopySpan(text, start, offset - 1);

					offset += 2;
					c += 2;

					_FinishParagraph(offset >= charCount);

					start = offset + 1;
				}
				break;

			case ' ':
				// Detect bullets at line starts (preceeding space)
				if (offset == start
					&& fCurrentParagraph.IsEmpty()
					&& offset + 2 < charCount
					&& (c[0] == '*' || c[0] == '-') && c[1] == ' ') {

					fCurrentParagraph.SetStyle(fBulletStyle);

					offset += 2;
					c += 2;

					start = offset + 1;
				}
				break;

			case '*':
			case '-':
				// Detect bullets at line starts (no preceeding space)
				if (offset == start
					&& fCurrentParagraph.IsEmpty()
					&& offset + 1 < charCount
					&& c[0] == ' ') {

					fCurrentParagraph.SetStyle(fBulletStyle);

					offset += 1;
					c += 1;

					start = offset + 1;
				}
				break;

			default:
				break;

		}
		offset++;
	}
}


void
MarkupParser::_CopySpan(const BString& text, int32& start, int32 end)
{
	if (start >= end)
		return;

	BString subString;
	text.CopyCharsInto(subString, start, end - start);
	fCurrentParagraph.Append(TextSpan(subString, *fCurrentCharacterStyle));

	start = end;
}


void
MarkupParser::_ToggleStyle(const CharacterStyle& style)
{
	if (fCurrentCharacterStyle == &style)
		fCurrentCharacterStyle = &fNormalStyle;
	else
		fCurrentCharacterStyle = &style;
}


void
MarkupParser::_FinishParagraph(bool isLast)
{
	if (!isLast)
		fCurrentParagraph.Append(TextSpan("\n", *fCurrentCharacterStyle));

	if (fCurrentParagraph.IsEmpty()) {
		// Append empty span
		fCurrentParagraph.Append(TextSpan("", fNormalStyle));
	}

	fTextDocument->Append(fCurrentParagraph);
	fCurrentParagraph.Clear();
	fCurrentParagraph.SetStyle(fParagraphStyle);
	fCurrentCharacterStyle = &fNormalStyle;
}
