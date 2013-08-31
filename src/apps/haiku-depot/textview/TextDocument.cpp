/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextDocument.h"


TextDocument::TextDocument()
	:
	fParagraphs(),
	fEmptyLastParagraph(),
	fDefaultTextStyle()
{
}


TextDocument::TextDocument(const TextStyle& textStyle,
	const ParagraphStyle& paragraphStyle)
	:
	fParagraphs(),
	fEmptyLastParagraph(paragraphStyle),
	fDefaultTextStyle(textStyle)
{
}


TextDocument::TextDocument(const TextDocument& other)
	:
	fParagraphs(other.fParagraphs),
	fEmptyLastParagraph(other.fEmptyLastParagraph),
	fDefaultTextStyle(other.fDefaultTextStyle)
{
}


TextDocument&
TextDocument::operator=(const TextDocument& other)
{
	fParagraphs = other.fParagraphs;
	fEmptyLastParagraph = other.fEmptyLastParagraph;
	fDefaultTextStyle = other.fDefaultTextStyle;

	return *this;
}


bool
TextDocument::operator==(const TextDocument& other) const
{
	if (this == &other)
		return true;

	return fEmptyLastParagraph == other.fEmptyLastParagraph
		&& fDefaultTextStyle == other.fDefaultTextStyle
		&& fParagraphs == other.fParagraphs;
}


bool
TextDocument::operator!=(const TextDocument& other) const
{
	return !(*this == other);
}


// #pragma mark -


status_t
TextDocument::Insert(int32 offset, const BString& text)
{
	return Insert(offset, text, TextStyleAt(offset));
}


status_t
TextDocument::Insert(int32 offset, const BString& text, const TextStyle& style)
{
	return Insert(offset, text, style, ParagraphStyleAt(offset));
}


status_t
TextDocument::Insert(int32 offset, const BString& text,
	const TextStyle& textStyle, const ParagraphStyle& paragraphStyle)
{
	return Replace(offset, 0, text, textStyle, paragraphStyle);
}


// #pragma mark -


status_t
TextDocument::Remove(int32 offset, int32 length)
{
	// TODO: Implement
	return B_ERROR;
}


// #pragma mark -


status_t
TextDocument::Replace(int32 offset, int32 length, const BString& text)
{
	return Replace(offset, length, text, TextStyleAt(offset));
}


status_t
TextDocument::Replace(int32 offset, int32 length, const BString& text,
	const TextStyle& style)
{
	return Replace(offset, length, text, style, ParagraphStyleAt(offset));
}


status_t
TextDocument::Replace(int32 offset, int32 length, const BString& text,
	const TextStyle& textStyle, const ParagraphStyle& paragraphStyle)
{
	// TODO: Implement
	return B_ERROR;
}


// #pragma mark -


const TextStyle&
TextDocument::TextStyleAt(int32 textOffset) const
{
	int32 paragraphOffset;
	const Paragraph& paragraph = ParagraphAt(textOffset, paragraphOffset);

	textOffset -= paragraphOffset;
	const TextSpanList& spans = paragraph.TextSpans();

	int32 index = 0;
	while (index < spans.CountItems()) {
		const TextSpan& span = spans.ItemAtFast(index);
		if (textOffset - span.CharCount() < 0)
			return span.Style();
		textOffset -= span.CharCount();
	}

	// TODO: Document should have a default TextStyle?	
	return fDefaultTextStyle;
}


const ParagraphStyle&
TextDocument::ParagraphStyleAt(int32 textOffset) const
{
	int32 paragraphOffset;
	return ParagraphAt(textOffset, paragraphOffset).Style();
}


// #pragma mark -


const Paragraph&
TextDocument::ParagraphAt(int32 textOffset, int32& paragraphOffset) const
{
	int32 textLength = 0;
	paragraphOffset = 0;
	int32 count = fParagraphs.CountItems();
	for (int32 i = 0; i < count; i++) {
		const Paragraph& paragraph = fParagraphs.ItemAtFast(i);
		paragraphOffset = textLength;
		if (textLength + paragraph.Length() > textOffset)
			return paragraph;
	}
	return fEmptyLastParagraph;
}


bool
TextDocument::Append(const Paragraph& paragraph)
{
	return fParagraphs.Add(paragraph);
}


