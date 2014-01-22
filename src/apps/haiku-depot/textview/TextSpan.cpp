/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextSpan.h"


TextSpan::TextSpan()
	:
	fText(),
	fCharCount(0),
	fStyle()
{
}


TextSpan::TextSpan(const BString& text, const CharacterStyle& style)
	:
	fText(text),
	fCharCount(text.CountChars()),
	fStyle(style)
{
}


TextSpan::TextSpan(const TextSpan& other)
	:
	fText(other.fText),
	fCharCount(other.fCharCount),
	fStyle(other.fStyle)
{
}


TextSpan&
TextSpan::operator=(const TextSpan& other)
{
	fText = other.fText;
	fCharCount = other.fCharCount;
	fStyle = other.fStyle;

	return *this;
}


bool
TextSpan::operator==(const TextSpan& other) const
{
	return fCharCount == other.fCharCount
		&& fStyle == other.fStyle
		&& fText == other.fText;
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
	fCharCount = fText.CountChars();
}


void
TextSpan::SetStyle(const CharacterStyle& style)
{
	fStyle = style;
}


bool
TextSpan::Append(const BString& text)
{
	return Insert(fCharCount, text);
}


bool
TextSpan::Insert(int32 offset, const BString& text)
{
	_TruncateInsert(offset);

	fText.InsertChars(text, offset);

	int32 charCount = fText.CountChars();
	bool success = charCount > fCharCount;
	fCharCount = charCount;

	return success;
}


bool
TextSpan::Remove(int32 start, int32 count)
{
	_TruncateRemove(start, count);

	if (count > 0) {
		fText.RemoveChars(start, count);

		int32 charCount = fText.CountChars();
		bool success = charCount < fCharCount;
		fCharCount = charCount;

		return success;
	}
	return true;
}


TextSpan
TextSpan::SubSpan(int32 start, int32 count) const
{
	_TruncateRemove(start, count);

	BString subString;
	if (count > 0)
		fText.CopyCharsInto(subString, start, count);

	return TextSpan(subString, fStyle);
}


// #pragma mark - private


void
TextSpan::_TruncateInsert(int32& start) const
{
	if (start < 0)
		start = 0;

	if (start >= fCharCount)
		start = fCharCount;
}


void
TextSpan::_TruncateRemove(int32& start, int32& count) const
{
	if (count < 0) {
		count = 0;
		return;
	}

	if (start < 0) {
		count += start;
		start = 0;
	}

	if (start < fCharCount) {
		if (start + count > fCharCount)
			count = fCharCount - start;
	} else {
		count = 0;
	}
}
