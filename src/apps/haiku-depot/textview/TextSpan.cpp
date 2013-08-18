/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextSpan.h"


TextSpan::TextSpan()
	:
	fText(),
	fStyle()
{
}


TextSpan::TextSpan(const BString& text, const TextStyle& style)
	:
	fText(text),
	fStyle(style)
{
}


TextSpan::TextSpan(const TextSpan& other)
	:
	fText(other.fText),
	fStyle(other.fStyle)
{
}


TextSpan&
TextSpan::operator=(const TextSpan& other)
{
	fText = other.fText;
	fStyle = other.fStyle;

	return *this;
}


bool
TextSpan::operator==(const TextSpan& other) const
{
	return fText == other.fText
		&& fStyle == other.fStyle;
}


bool
TextSpan::operator!=(const TextSpan& other) const
{
	return !(*this == other);
}


void
TextSpan::SetText(const BString& text)
{
	fText = text;
}


void
TextSpan::SetStyle(const TextStyle& style)
{
	fStyle = style;
}


TextSpan
TextSpan::SubSpan(int32 start, int32 count) const
{
	if (start < 0) {
		count += start;
		start = 0;
	}

	BString subString;
	
	int32 charCount = fText.CountChars();
	if (start < charCount) {
		if (start + count > charCount)
			count = charCount - start;

		fText.CopyCharsInto(subString, start, count);
	}

	return TextSpan(subString, fStyle);
}

