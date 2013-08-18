/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ParagraphStyle.h"


ParagraphStyle::ParagraphStyle()
	:
	fStyleData(new ParagraphStyleData(), true)
{
}


ParagraphStyle::ParagraphStyle(const ParagraphStyle& other)
	:
	fStyleData(other.fStyleData)
{
}


ParagraphStyle&
ParagraphStyle::operator=(const ParagraphStyle& other)
{
	if (this == &other)
		return *this;
	
	fStyleData = other.fStyleData;
	return *this;
}


bool
ParagraphStyle::operator==(const ParagraphStyle& other) const
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
ParagraphStyle::operator!=(const ParagraphStyle& other) const
{
	return !(*this == other);
}


bool
ParagraphStyle::SetAlignment(::Alignment alignment)
{
	fStyleData.SetTo(fStyleData->SetAlignment(alignment));
	return fStyleData->Alignment() == alignment;
}


bool
ParagraphStyle::SetJustify(bool justify)
{
	fStyleData.SetTo(fStyleData->SetJustify(justify));
	return fStyleData->Justify() == justify;
}


bool
ParagraphStyle::SetFirstLineInset(float inset)
{
	fStyleData.SetTo(fStyleData->SetFirstLineInset(inset));
	return fStyleData->FirstLineInset() == inset;
}


bool
ParagraphStyle::SetLineInset(float inset)
{
	fStyleData.SetTo(fStyleData->SetLineInset(inset));
	return fStyleData->LineInset() == inset;
}


bool
ParagraphStyle::SetSpacingTop(float spacing)
{
	fStyleData.SetTo(fStyleData->SetSpacingTop(spacing));
	return fStyleData->SpacingTop() == spacing;
}


bool
ParagraphStyle::SetSpacingBottom(float spacing)
{
	fStyleData.SetTo(fStyleData->SetSpacingBottom(spacing));
	return fStyleData->SpacingBottom() == spacing;
}


