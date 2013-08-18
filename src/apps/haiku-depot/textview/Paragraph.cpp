/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Paragraph.h"


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
		return fTextSpans.Replace(span, index);
	}

	if (offset == 0) {
		if (index > 0) {
			// Try to merge with TextSpan before if offset == 0 && index > 0
			TextSpan span = fTextSpans.ItemAtFast(index - 1);
			if (span.Style() == newSpan.Style()) {
				span.Insert(span.CharCount(), newSpan.Text());
				return fTextSpans.Replace(span, index - 1);
			}
		}
		// Just insert the new span before the one at index
		return fTextSpans.Add(newSpan, index);
	}

	// Split the span, 
	TextSpan spanBefore = span.SubSpan(0, offset);
	TextSpan spanAfter = span.SubSpan(offset, span.CharCount() - offset);

	return fTextSpans.Replace(spanBefore, index)
		&& fTextSpans.Add(newSpan, index + 1)
		&& fTextSpans.Add(spanAfter, index + 2);
}
