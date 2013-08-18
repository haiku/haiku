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
	fStyle(style)
{
}


Paragraph::Paragraph(const Paragraph& other)
	:
	fStyle(other.fStyle)
{
}


Paragraph&
Paragraph::operator=(const Paragraph& other)
{
	fStyle = other.fStyle;

	return *this;
}


bool
Paragraph::operator==(const Paragraph& other) const
{
	return fStyle == other.fStyle;
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

