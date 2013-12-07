/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextDocument.h"

#include <algorithm>


TextDocument::TextDocument()
	:
	fParagraphs(),
	fEmptyLastParagraph(),
	fDefaultCharacterStyle()
{
}


TextDocument::TextDocument(const CharacterStyle& CharacterStyle,
	const ParagraphStyle& paragraphStyle)
	:
	fParagraphs(),
	fEmptyLastParagraph(paragraphStyle),
	fDefaultCharacterStyle(CharacterStyle)
{
}


TextDocument::TextDocument(const TextDocument& other)
	:
	fParagraphs(other.fParagraphs),
	fEmptyLastParagraph(other.fEmptyLastParagraph),
	fDefaultCharacterStyle(other.fDefaultCharacterStyle)
{
}


TextDocument&
TextDocument::operator=(const TextDocument& other)
{
	fParagraphs = other.fParagraphs;
	fEmptyLastParagraph = other.fEmptyLastParagraph;
	fDefaultCharacterStyle = other.fDefaultCharacterStyle;

	return *this;
}


bool
TextDocument::operator==(const TextDocument& other) const
{
	if (this == &other)
		return true;

	return fEmptyLastParagraph == other.fEmptyLastParagraph
		&& fDefaultCharacterStyle == other.fDefaultCharacterStyle
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
	return Insert(offset, text, CharacterStyleAt(offset));
}


status_t
TextDocument::Insert(int32 offset, const BString& text, const CharacterStyle& style)
{
	return Insert(offset, text, style, ParagraphStyleAt(offset));
}


status_t
TextDocument::Insert(int32 offset, const BString& text,
	const CharacterStyle& CharacterStyle, const ParagraphStyle& paragraphStyle)
{
	return Replace(offset, 0, text, CharacterStyle, paragraphStyle);
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
	return Replace(offset, length, text, CharacterStyleAt(offset));
}


status_t
TextDocument::Replace(int32 offset, int32 length, const BString& text,
	const CharacterStyle& style)
{
	return Replace(offset, length, text, style, ParagraphStyleAt(offset));
}


status_t
TextDocument::Replace(int32 offset, int32 length, const BString& text,
	const CharacterStyle& CharacterStyle, const ParagraphStyle& paragraphStyle)
{
	// TODO: Implement
	return B_ERROR;
}


// #pragma mark -


const CharacterStyle&
TextDocument::CharacterStyleAt(int32 textOffset) const
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

	return fDefaultCharacterStyle;
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
	// TODO: Could binary search the Paragraphs if they were wrapped in classes
	// that knew there text offset in the document.
	int32 textLength = 0;
	paragraphOffset = 0;
	int32 count = fParagraphs.CountItems();
	for (int32 i = 0; i < count; i++) {
		const Paragraph& paragraph = fParagraphs.ItemAtFast(i);
		paragraphOffset = textOffset - textLength;
		int32 paragraphLength = paragraph.Length();
		if (textLength + paragraphLength > textOffset)
			return paragraph;
		textLength += paragraphLength;
	}
	return fEmptyLastParagraph;
}


const Paragraph&
TextDocument::ParagraphAt(int32 index) const
{
	if (index >= 0 && index < fParagraphs.CountItems())
		return fParagraphs.ItemAtFast(index);
	return fEmptyLastParagraph;
}


bool
TextDocument::Append(const Paragraph& paragraph)
{
	return fParagraphs.Add(paragraph);
}


int32
TextDocument::Length() const
{
	// TODO: Could be O(1) if the Paragraphs were wrapped in classes that
	// knew there text offset in the document.
	int32 textLength = 0;
	int32 count = fParagraphs.CountItems();
	for (int32 i = 0; i < count; i++) {
		const Paragraph& paragraph = fParagraphs.ItemAtFast(i);
		textLength += paragraph.Length();
	}
	return textLength;
}


BString
TextDocument::GetText(int32 start, int32 length) const
{
	if (start < 0)
		start = 0;
	
	BString text;
	
	int32 count = fParagraphs.CountItems();
	for (int32 i = 0; i < count; i++) {
		const Paragraph& paragraph = fParagraphs.ItemAtFast(i);
		int32 paragraphLength = paragraph.Length();
		if (paragraphLength == 0)
			continue;
		if (start > paragraphLength) {
			// Skip paragraph if its before start
			start -= paragraphLength;
			continue;
		}

		// Remaining paragraph length after start
		paragraphLength -= start;
		int32 copyLength = std::min(paragraphLength, length);
		
		text << paragraph.GetText(start, copyLength);
		
		length -= copyLength;
		if (length == 0)
			break;
		else if (i < count - 1)
			text << '\n';

		// Next paragraph is copied from its beginning
		start = 0;
	}

	return text;
}
