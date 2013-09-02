/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "CharacterStyle.h"


CharacterStyle::CharacterStyle()
	:
	fStyleData(new CharacterStyleData(), true)
{
}


CharacterStyle::CharacterStyle(const CharacterStyle& other)
	:
	fStyleData(other.fStyleData)
{
}


CharacterStyle&
CharacterStyle::operator=(const CharacterStyle& other)
{
	if (this == &other)
		return *this;
	
	fStyleData = other.fStyleData;
	return *this;
}


bool
CharacterStyle::operator==(const CharacterStyle& other) const
{
	if (this == &other)
		return true;

	if (fStyleData == other.fStyleData)
		return true;

	if (fStyleData.Get() != NULL && other.fStyleData.Get() != NULL)
		return *fStyleData.Get() == *other.fStyleData.Get();

	return false;
}


bool
CharacterStyle::operator!=(const CharacterStyle& other) const
{
	return !(*this == other);
}


bool
CharacterStyle::SetFont(const BFont& font)
{
	CharacterStyleDataRef data = fStyleData->SetFont(font);
	if (data == fStyleData)
		return data->Font() == font;

	fStyleData = data;
	return true;
}


bool
CharacterStyle::SetAscent(float ascent)
{
	CharacterStyleDataRef data = fStyleData->SetAscent(ascent);
	if (data == fStyleData)
		return data->Ascent() == ascent;

	fStyleData = data;
	return true;
}


bool
CharacterStyle::SetDescent(float descent)
{
	CharacterStyleDataRef data = fStyleData->SetDescent(descent);
	if (data == fStyleData)
		return data->Descent() == descent;

	fStyleData = data;
	return true;
}


bool
CharacterStyle::SetWidth(float width)
{
	CharacterStyleDataRef data = fStyleData->SetWidth(width);
	if (data == fStyleData)
		return data->Width() == width;

	fStyleData = data;
	return true;
}


bool
CharacterStyle::SetForegroundColor(rgb_color color)
{
	CharacterStyleDataRef data = fStyleData->SetForegroundColor(color);
	if (data == fStyleData)
		return data->ForegroundColor() == color;

	fStyleData = data;
	return true;
}


bool
CharacterStyle::SetBackgroundColor(rgb_color color)
{
	CharacterStyleDataRef data = fStyleData->SetBackgroundColor(color);
	if (data == fStyleData)
		return data->BackgroundColor() == color;

	fStyleData = data;
	return true;
}


bool
CharacterStyle::SetStrikeOutColor(rgb_color color)
{
	CharacterStyleDataRef data = fStyleData->SetStrikeOutColor(color);
	if (data == fStyleData)
		return data->StrikeOutColor() == color;

	fStyleData = data;
	return true;
}


bool
CharacterStyle::SetUnderlineColor(rgb_color color)
{
	CharacterStyleDataRef data = fStyleData->SetUnderlineColor(color);
	if (data == fStyleData)
		return data->UnderlineColor() == color;

	fStyleData = data;
	return true;
}


bool
CharacterStyle::SetStrikeOut(uint8 strikeOut)
{
	CharacterStyleDataRef data = fStyleData->SetStrikeOut(strikeOut);
	if (data == fStyleData)
		return data->StrikeOut() == strikeOut;

	fStyleData = data;
	return true;
}


bool
CharacterStyle::SetUnderline(uint8 underline)
{
	CharacterStyleDataRef data = fStyleData->SetUnderline(underline);
	if (data == fStyleData)
		return data->Underline() == underline;

	fStyleData = data;
	return true;
}


