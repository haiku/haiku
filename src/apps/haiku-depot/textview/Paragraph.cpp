/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Paragraph.h"

#include <algorithm>


Paragraph::Paragraph()
	:
	fStyle()
{
}


Paragraph::Paragraph(const ParagraphStyle& style)
	:
	fStyle(style),
	fTextSpans()
{
}


Paragraph::Paragraph(const Paragraph& other)
	:
	fStyle(other.fStyle),
	fTextSpans(other.fTextSpans)
{
}


Paragraph&
Paragraph::operator=(const Paragraph& other)
{
	fStyle = other.fStyle;
	fTextSpans = other.fTextSpans;

	return *this;
}


bool
Paragraph::operator==(const Paragraph& other) const
{
	if (this == &other)
		return true;

	return fStyle == other.fStyle
		&& fTextSpans == other.fTextSpans;
}


bool
Paragraph::operator!=(const Paragraph& other) const
{
	return !(*this == other);
}


void
Paragraph::SetStyle(const ParagraphStyle& style)
{
	fStyle = style;
}


bool
Paragraph::Append(const TextSpan& span)
{
	// Try to merge with last span if the TextStyles are equal
	if (fTextSpans.CountItems() > 0) {
		const TextSpan& lastSpan = fTextSpans.LastItem();
		if (lastSpan.Style() == span.Style()) {
			BString text(lastSpan.Text());
			text.Append(span.Text());
			fTextSpans.Remove();
			return fTextSpans.Add(TextSpan(text, span.Style()));
		}
	}
	return fTextSpans.Add(span);
}


bool
Paragraph::Insert(int32 offset, const TextSpan& newSpan)
{
	int32 index = 0;
	while (index < fTextSpans.CountItems()) {
		const TextSpan& span = fTextSpans.ItemAtFast(index);
		if (offset - span.CharCount() < 0)
			break;
		offset -= span.CharCount();
	}

	if (fTextSpans.CountItems() == index)
		return Append(newSpan);

	// Try to merge with span at index if the TextStyles are equal
	TextSpan span = fTextSpans.ItemAtFast(index);
	if (span.Style() == newSpan.Style()) {
		span.Insert(offset, newSpan.Text());
		return fTextSpans.Replace(index, span);
	}

	if (offset == 0) {
		if (index > 0) {
			// Try to merge with TextSpan before if offset == 0 && index > 0
			TextSpan span = fTextSpans.ItemAtFast(index - 1);
			if (span.Style() == newSpan.Style()) {
				span.Insert(span.CharCount(), newSpan.Text());
				return fTextSpans.Replace(index - 1, span);
			}
		}
		// Just insert the new span before the one at index
		return fTextSpans.Add(newSpan, index);
	}

	// Split the span,
	TextSpan spanBefore = span.SubSpan(0, offset);
	TextSpan spanAfter = span.SubSpan(offset, span.CharCount() - offset);

	return fTextSpans.Replace(index, spanBefore)
		&& fTextSpans.Add(newSpan, index + 1)
		&& fTextSpans.Add(spanAfter, index + 2);
}


bool
Paragraph::Remove(int32 offset, int32 length)
{
	if (length == 0)
		return true;

	int32 index = 0;
	while (index < fTextSpans.CountItems()) {
		const TextSpan& span = fTextSpans.ItemAtFast(index);
		if (offset - span.CharCount() < 0)
			break;
		offset -= span.CharCount();
	}

	if (index >= fTextSpans.CountItems())
		return false;
	
	TextSpan span = fTextSpans.ItemAtFast(index).SubSpan(0, offset);
	fTextSpans.Replace(index, span);
	index += 1;
	if (index >= fTextSpans.CountItems())
		return true;
	
	while (length > 0) {
		int32 spanLength = fTextSpans.ItemAtFast(index).CharCount();
		if (spanLength <= length) {
			fTextSpans.Remove(index);
			length -= spanLength;
		} else {
			// Reached last span
			int32 removeLength = std::min(length, spanLength);
			TextSpan lastSpan = fTextSpans.ItemAtFast(index).SubSpan(
				removeLength, spanLength - removeLength);
			// Try to merge with first span, otherwise replace span at index
			if (lastSpan.Style() == span.Style()) {
				span.Insert(span.CharCount(), lastSpan.Text());
				fTextSpans.Replace(index - 1, span);
			} else {
				fTextSpans.Replace(index, lastSpan);
			}

			break;
		}
	}

	return true;
}


void
Paragraph::Clear()
{
	fTextSpans.Clear();
}


int32
Paragraph::Length() const
{
	int32 length = 0;
	for (int32 i = fTextSpans.CountItems() - 1; i >= 0; i--) {
		const TextSpan& span = fTextSpans.ItemAtFast(i);
		length += span.CharCount();
	}
	return length;
}


bool
Paragraph::IsEmpty() const
{
	return fTextSpans.CountItems() == 0;
}


BString
Paragraph::GetText(int32 start, int32 length) const
{
	if (start < 0)
		start = 0;
	
	BString text;
	
	int32 count = fTextSpans.CountItems();
	for (int32 i = 0; i < count; i++) {
		const TextSpan& span = fTextSpans.ItemAtFast(i);
		int32 spanLength = span.CharCount();
		if (spanLength == 0)
			continue;
		if (start > spanLength) {
			// Skip span if its before start
			start -= spanLength;
			continue;
		}

		// Remaining span length after start
		spanLength -= start;
		int32 copyLength = std::min(spanLength, length);

		if (start == 0 && length == spanLength) {
			text << span.Text();
		} else {
			BString subString;
			span.Text().CopyCharsInto(subString, start, copyLength);
			text << subString;
		}
		
		length -= copyLength;
		if (length == 0)
			break;

		// Next span is copied from its beginning
		start = 0;
	}

	return text;
}
