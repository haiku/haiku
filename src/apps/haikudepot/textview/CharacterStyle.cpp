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

	if (fStyleData.IsSet() && other.fStyleData.IsSet())
		return *fStyleData == *other.fStyleData;

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


const BFont&
CharacterStyle::Font() const
{
	return fStyleData->Font();
}


bool
CharacterStyle::SetFontSize(float size)
{
	BFont font(Font());
	font.SetSize(size);
	return SetFont(font);
}


float
CharacterStyle::FontSize() const
{
	return Font().Size();
}


bool
CharacterStyle::SetBold(bool bold)
{
	uint16 face = Font().Face();
	if ((bold && (face & B_BOLD_FACE) != 0)
		|| (!bold && (face & B_BOLD_FACE) == 0)) {
		return true;
	}

	uint16 neededFace = face;
	if (bold) {
		neededFace |= B_BOLD_FACE;
		neededFace &= ~B_REGULAR_FACE;
	} else {
		neededFace &= ~B_BOLD_FACE;
		if (neededFace == 0)
			neededFace |= B_REGULAR_FACE;
	}

	return SetFont(_FindFontForFace(neededFace));
}


bool
CharacterStyle::IsBold() const
{
	return (Font().Face() & B_BOLD_FACE) != 0;
}


bool
CharacterStyle::SetItalic(bool italic)
{
	uint16 face = Font().Face();
	if ((italic && (face & B_ITALIC_FACE) != 0)
		|| (!italic && (face & B_ITALIC_FACE) == 0)) {
		return true;
	}

	uint16 neededFace = face;
	if (italic) {
		neededFace |= B_ITALIC_FACE;
		neededFace &= ~B_REGULAR_FACE;
	} else {
		neededFace &= ~B_ITALIC_FACE;
		if (neededFace == 0)
			neededFace |= B_REGULAR_FACE;
	}

	return SetFont(_FindFontForFace(neededFace));
}


bool
CharacterStyle::IsItalic() const
{
	return (Font().Face() & B_ITALIC_FACE) != 0;
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


float
CharacterStyle::Ascent() const
{
	return fStyleData->Ascent();
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


float
CharacterStyle::Descent() const
{
	return fStyleData->Descent();
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


float
CharacterStyle::Width() const
{
	return fStyleData->Width();
}


bool
CharacterStyle::SetGlyphSpacing(float spacing)
{
	CharacterStyleDataRef data = fStyleData->SetGlyphSpacing(spacing);
	if (data == fStyleData)
		return data->GlyphSpacing() == spacing;

	fStyleData = data;
	return true;
}


float
CharacterStyle::GlyphSpacing() const
{
	return fStyleData->GlyphSpacing();
}


bool
CharacterStyle::SetForegroundColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	return SetForegroundColor((rgb_color){ r, g, b, a });
}


bool
CharacterStyle::SetForegroundColor(color_which which)
{
	CharacterStyleDataRef data = fStyleData->SetForegroundColor(which);
	if (data == fStyleData)
		return data->WhichForegroundColor() == which;

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


rgb_color
CharacterStyle::ForegroundColor() const
{
	return fStyleData->ForegroundColor();
}


color_which
CharacterStyle::WhichForegroundColor() const
{
	return fStyleData->WhichForegroundColor();
}


bool
CharacterStyle::SetBackgroundColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	return SetBackgroundColor((rgb_color){ r, g, b, a });
}


bool
CharacterStyle::SetBackgroundColor(color_which which)
{
	CharacterStyleDataRef data = fStyleData->SetBackgroundColor(which);
	if (data == fStyleData)
		return data->WhichBackgroundColor() == which;

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


rgb_color
CharacterStyle::BackgroundColor() const
{
	return fStyleData->BackgroundColor();
}


color_which
CharacterStyle::WhichBackgroundColor() const
{
	return fStyleData->WhichBackgroundColor();
}


bool
CharacterStyle::SetStrikeOutColor(color_which which)
{
	CharacterStyleDataRef data = fStyleData->SetStrikeOutColor(which);
	if (data == fStyleData)
		return data->WhichStrikeOutColor() == which;

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


rgb_color
CharacterStyle::StrikeOutColor() const
{
	return fStyleData->StrikeOutColor();
}


color_which
CharacterStyle::WhichStrikeOutColor() const
{
	return fStyleData->WhichStrikeOutColor();
}


bool
CharacterStyle::SetUnderlineColor(color_which which)
{
	CharacterStyleDataRef data = fStyleData->SetUnderlineColor(which);
	if (data == fStyleData)
		return data->WhichUnderlineColor() == which;

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


rgb_color
CharacterStyle::UnderlineColor() const
{
	return fStyleData->UnderlineColor();
}


color_which
CharacterStyle::WhichUnderlineColor() const
{
	return fStyleData->WhichUnderlineColor();
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


uint8
CharacterStyle::StrikeOut() const
{
	return fStyleData->StrikeOut();
}


bool
CharacterStyle::SetUnderline(uint8 underline)
{
#if 1
	uint16 face = Font().Face();
	if ((underline && (face & B_UNDERSCORE_FACE) != 0)
		|| (!underline && (face & B_UNDERSCORE_FACE) == 0)) {
		return true;
	}

	uint16 neededFace = face;
	if (underline) {
		neededFace |= B_UNDERSCORE_FACE;
		neededFace &= ~B_REGULAR_FACE;
	} else {
		neededFace &= ~B_UNDERSCORE_FACE;
		if (neededFace == 0)
			neededFace |= B_REGULAR_FACE;
	}

	return SetFont(_FindFontForFace(neededFace));
#else
	// TODO: re-enable this instead of using B_UNDERSCORE_FACE when TextDocumentView actually
	// implements drawing the fancy underline styles. Until then, we can at least get simple
	// underlines.
	CharacterStyleDataRef data = fStyleData->SetUnderline(underline);
	if (data == fStyleData)
		return data->Underline() == underline;

	fStyleData = data;
	return true;
#endif
}


uint8
CharacterStyle::Underline() const
{
	return fStyleData->Underline();
}


// #pragma mark - private


BFont
CharacterStyle::_FindFontForFace(uint16 face) const
{
	BFont font(Font());
	font.SetFamilyAndFace(NULL, face);

	return font;
}

