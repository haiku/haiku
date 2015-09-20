/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Paragraph.h"

#include <algorithm>
#include <stdio.h>


Paragraph::Paragraph()
	:
	fStyle()
{
}


Paragraph::Paragraph(const ParagraphStyle& style)
	:
	fStyle(style),
	fTextSpans(),
	fCachedLength(-1)
{
}


Paragraph::Paragraph(const Paragraph& other)
	:
	fStyle(other.fStyle),
	fTextSpans(other.fTextSpans),
	fCachedLength(other.fCachedLength)
{
}


Paragraph&
Paragraph::operator=(const Paragraph& other)
{
	fStyle = other.fStyle;
	fTextSpans = other.fTextSpans;
	fCachedLength = other.fCachedLength;

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
Paragraph::Prepend(const TextSpan& span)
{
	_InvalidateCachedLength();

	// Try to merge with first span if the TextStyles are equal
	if (fTextSpans.CountItems() > 0) {
		const TextSpan& firstSpan = fTextSpans.ItemAtFast(0);
		if (firstSpan.Style() == span.Style()) {
			BString text(span.Text());
			text.Append(firstSpan.Text());
			return fTextSpans.Replace(0, TextSpan(text, span.Style()));
		}
	}
	return fTextSpans.Add(span, 0);
}


bool
Paragraph::Append(const TextSpan& span)
{
	_InvalidateCachedLength();

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
	_InvalidateCachedLength();

	int32 index = 0;
	while (index < fTextSpans.CountItems()) {
		const TextSpan& span = fTextSpans.ItemAtFast(index);
		if (offset - span.CountChars() < 0)
			break;
		offset -= span.CountChars();
		index++;
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
				span.Insert(span.CountChars(), newSpan.Text());
				return fTextSpans.Replace(index - 1, span);
			}
		}
		// Just insert the new span before the one at index
		return fTextSpans.Add(newSpan, index);
	}

	// Split the span,
	TextSpan spanBefore = span.SubSpan(0, offset);
	TextSpan spanAfter = span.SubSpan(offset, span.CountChars() - offset);

	return fTextSpans.Replace(index, spanBefore)
		&& fTextSpans.Add(newSpan, index + 1)
		&& fTextSpans.Add(spanAfter, index + 2);
}


bool
Paragraph::Remove(int32 offset, int32 length)
{
	if (length == 0)
		return true;

	_InvalidateCachedLength();

	int32 index = 0;
	while (index < fTextSpans.CountItems()) {
		const TextSpan& span = fTextSpans.ItemAtFast(index);
		if (offset - span.CountChars() < 0)
			break;
		offset -= span.CountChars();
		index++;
	}

	if (index >= fTextSpans.CountItems())
		return false;

	TextSpan span(fTextSpans.ItemAtFast(index));
	int32 removeLength = std::min(span.CountChars() - offset, length);
	span.Remove(offset, removeLength);
	length -= removeLength;
	index += 1;

	// Remove more spans if necessary
	while (length > 0 && index < fTextSpans.CountItems()) {
		int32 spanLength = fTextSpans.ItemAtFast(index).CountChars();
		if (spanLength <= length) {
			fTextSpans.Remove(index);
			length -= spanLength;
		} else {
			// Reached last span
			removeLength = std::min(length, spanLength);
			TextSpan lastSpan = fTextSpans.ItemAtFast(index).SubSpan(
				removeLength, spanLength - removeLength);
			// Try to merge with first span, otherwise replace span at index
			if (lastSpan.Style() == span.Style()) {
				span.Insert(span.CountChars(), lastSpan.Text());
				fTextSpans.Remove(index);
			} else {
				fTextSpans.Replace(index, lastSpan);
			}

			break;
		}
	}

	// See if anything from the TextSpan at offset remained, keep it as empty
	// span if it is the last remaining span.
	index--;
	if (span.CountChars() > 0 || fTextSpans.CountItems() == 1) {
		fTextSpans.Replace(index, span);
	} else {
		fTextSpans.Remove(index);
		index--;
	}

	// See if spans can be merged after one has been removed.
	if (index >= 0 && index + 1 < fTextSpans.CountItems()) {
		const TextSpan& span1 = fTextSpans.ItemAtFast(index);
		const TextSpan& span2 = fTextSpans.ItemAtFast(index + 1);
		if (span1.Style() == span2.Style()) {
			span = span1;
			span.Append(span2.Text());
			fTextSpans.Replace(index, span);
			fTextSpans.Remove(index + 1);
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
	if (fCachedLength >= 0)
		return fCachedLength;

	int32 length = 0;
	for (int32 i = fTextSpans.CountItems() - 1; i >= 0; i--) {
		const TextSpan& span = fTextSpans.ItemAtFast(i);
		length += span.CountChars();
	}

	fCachedLength = length;
	return length;
}


bool
Paragraph::IsEmpty() const
{
	return fTextSpans.CountItems() == 0;
}


bool
Paragraph::EndsWith(BString string) const
{
	int length = Length();
	int endLength = string.CountChars();
	int start = length - endLength;
	BString end = Text(start, endLength);
	return end == string;
}


BString
Paragraph::Text() const
{
	BString result;

	int32 count = fTextSpans.CountItems();
	for (int32 i = 0; i < count; i++)
		result << fTextSpans.ItemAtFast(i).Text();

	return result;
}


BString
Paragraph::Text(int32 start, int32 length) const
{
	Paragraph subParagraph = SubParagraph(start, length);
	return subParagraph.Text();
}


Paragraph
Paragraph::SubParagraph(int32 start, int32 length) const
{
	if (start < 0)
		start = 0;

	if (start == 0 && length == Length())
		return *this;

	Paragraph result(fStyle);

	int32 count = fTextSpans.CountItems();
	for (int32 i = 0; i < count; i++) {
		const TextSpan& span = fTextSpans.ItemAtFast(i);
		int32 spanLength = span.CountChars();
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

		if (start == 0 && length == spanLength)
			result.Append(span);
		else
			result.Append(span.SubSpan(start, copyLength));

		length -= copyLength;
		if (length == 0)
			break;

		// Next span is copied from its beginning
		start = 0;
	}

	return result;
}


void
Paragraph::PrintToStream() const
{
	int32 spanCount = fTextSpans.CountItems();
	if (spanCount == 0) {
		printf("  <p/>\n");
		return;
	}
	printf("  <p>\n");
	for (int32 i = 0; i < spanCount; i++) {
		const TextSpan& span = fTextSpans.ItemAtFast(i);
		if (span.CountChars() == 0)
			printf("    <span/>\n");
		else {
			BString text = span.Text();
			text.ReplaceAll("\n", "\\n");
			printf("    <span>%s</span>\n", text.String());
		}
	}
	printf("  </p>\n");
}


// #pragma mark -


void
Paragraph::_InvalidateCachedLength()
{
	fCachedLength = -1;
}
