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
