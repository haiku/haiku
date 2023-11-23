/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
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


int32
Paragraph::CountTextSpans() const
{
	return static_cast<int32>(fTextSpans.size());
}


const TextSpan&
Paragraph::TextSpanAtIndex(int32 index) const
{
	return fTextSpans[index];
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
	if (!fTextSpans.empty()) {
		const TextSpan& firstSpan = fTextSpans[0];
		if (firstSpan.Style() == span.Style()) {
			BString text(span.Text());
			text.Append(firstSpan.Text());
			fTextSpans[0] = TextSpan(text, span.Style());
			return true;
		}
	}
	fTextSpans.push_back(span);
	return true;
}


bool
Paragraph::Append(const TextSpan& span)
{
	_InvalidateCachedLength();

	// Try to merge with last span if the TextStyles are equal
	if (!fTextSpans.empty()) {
		const TextSpan& lastSpan = fTextSpans[fTextSpans.size() - 1];
		if (lastSpan.Style() == span.Style()) {
			BString text(lastSpan.Text());
			text.Append(span.Text());
			fTextSpans.pop_back();
			fTextSpans.push_back(TextSpan(text, span.Style()));
			return true;
		}
	}
	fTextSpans.push_back(span);
	return true;
}


bool
Paragraph::Insert(int32 offset, const TextSpan& newSpan)
{
	_InvalidateCachedLength();

	int32 index = 0;

	{
		int32 countTextSpans = static_cast<int32>(fTextSpans.size());
		while (index < countTextSpans) {
			const TextSpan& span = fTextSpans[index];
			if (offset - span.CountChars() < 0)
				break;
			offset -= span.CountChars();
			index++;
		}

		if (countTextSpans == index)
			return Append(newSpan);
	}

	// Try to merge with span at index if the TextStyles are equal
	TextSpan span = fTextSpans[index];
	if (span.Style() == newSpan.Style()) {
		span.Insert(offset, newSpan.Text());
		fTextSpans[index] = span;
		return true;
	}

	if (offset == 0) {
		if (index > 0) {
			// Try to merge with TextSpan before if offset == 0 && index > 0
			TextSpan span = fTextSpans[index - 1];
			if (span.Style() == newSpan.Style()) {
				span.Insert(span.CountChars(), newSpan.Text());
				fTextSpans[index - 1] = span;
				return true;
			}
		}
		// Just insert the new span before the one at index
		fTextSpans.insert(fTextSpans.begin() + index, newSpan);
		return true;
	}

	// Split the span,
	TextSpan spanBefore = span.SubSpan(0, offset);
	TextSpan spanAfter = span.SubSpan(offset, span.CountChars() - offset);

	fTextSpans[index] = spanBefore;
	fTextSpans.insert(fTextSpans.begin() + (index + 1), newSpan);
	fTextSpans.insert(fTextSpans.begin() + (index + 2), spanAfter);
	return true;
}


bool
Paragraph::Remove(int32 offset, int32 length)
{
	if (length == 0)
		return true;

	_InvalidateCachedLength();

	int32 index = 0;

	{
		int32 countTextSpans = static_cast<int32>(fTextSpans.size());
		while (index < countTextSpans) {
			const TextSpan& span = fTextSpans[index];
			if (offset - span.CountChars() < 0)
				break;
			offset -= span.CountChars();
			index++;
		}

		if (index >= countTextSpans)
			return false;
	}

	TextSpan span(fTextSpans[index]);
	int32 removeLength = std::min(span.CountChars() - offset, length);
	span.Remove(offset, removeLength);
	length -= removeLength;
	index += 1;

	// Remove more spans if necessary
	while (length > 0 && index < static_cast<int32>(fTextSpans.size())) {
		int32 spanLength = fTextSpans[index].CountChars();
		if (spanLength <= length) {
			fTextSpans.erase(fTextSpans.begin() + index);
			length -= spanLength;
		} else {
			// Reached last span
			removeLength = std::min(length, spanLength);
			TextSpan lastSpan = fTextSpans[index].SubSpan(
				removeLength, spanLength - removeLength);
			// Try to merge with first span, otherwise replace span at index
			if (lastSpan.Style() == span.Style()) {
				span.Insert(span.CountChars(), lastSpan.Text());
				fTextSpans.erase(fTextSpans.begin() + index);
			} else {
				fTextSpans[index] = lastSpan;
			}

			break;
		}
	}

	// See if anything from the TextSpan at offset remained, keep it as empty
	// span if it is the last remaining span.
	index--;
	if (span.CountChars() > 0 || static_cast<int32>(fTextSpans.size()) == 1) {
		fTextSpans[index] = span;
	} else {
		fTextSpans.erase(fTextSpans.begin() + index);
		index--;
	}

	// See if spans can be merged after one has been removed.
	if (index >= 0 && index + 1 < static_cast<int32>(fTextSpans.size())) {
		const TextSpan& span1 = fTextSpans[index];
		const TextSpan& span2 = fTextSpans[index + 1];
		if (span1.Style() == span2.Style()) {
			span = span1;
			span.Append(span2.Text());
			fTextSpans[index] = span;
			fTextSpans.erase(fTextSpans.begin() + (index + 1));
		}
	}

	return true;
}


void
Paragraph::Clear()
{
	fTextSpans.clear();
}


int32
Paragraph::Length() const
{
	if (fCachedLength >= 0)
		return fCachedLength;

	int32 length = 0;
	std::vector<TextSpan>::const_iterator it;
	for (it = fTextSpans.begin(); it != fTextSpans.end(); it++) {
		const TextSpan& span = *it;
		length += span.CountChars();
	}

	fCachedLength = length;
	return length;
}


bool
Paragraph::IsEmpty() const
{
	return fTextSpans.empty();
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
	std::vector<TextSpan>::const_iterator it;
	for (it = fTextSpans.begin(); it != fTextSpans.end(); it++) {
		const TextSpan& span = *it;
		result << span.Text();
	}
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

	std::vector<TextSpan>::const_iterator it;
	for (it = fTextSpans.begin(); it != fTextSpans.end(); it++) {
		const TextSpan& span = *it;
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
	int32 spanCount = static_cast<int32>(fTextSpans.size());
	if (spanCount == 0) {
		printf("  <p/>\n");
		return;
	}
	printf("  <p>\n");
	for (int32 i = 0; i < spanCount; i++) {
		const TextSpan& span = fTextSpans[i];
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
