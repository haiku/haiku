/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ParagraphStyleData.h"

#include <new>


ParagraphStyleData::ParagraphStyleData()
	:
	fAlignment(ALIGN_LEFT),
	fJustify(false),

	fFirstLineInset(0.0f),
	fLineInset(0.0f),

	fSpacingTop(0.0f),
	fSpacingBottom(0.0f)
{
}


ParagraphStyleData::ParagraphStyleData(const ParagraphStyleData& other)
	:
	fAlignment(other.fAlignment),
	fJustify(other.fJustify),

	fFirstLineInset(other.fFirstLineInset),
	fLineInset(other.fLineInset),

	fSpacingTop(other.fSpacingTop),
	fSpacingBottom(other.fSpacingBottom)
{
}


bool
ParagraphStyleData::operator==(const ParagraphStyleData& other) const
{
	if (this == &other)
		return true;

	return fAlignment == other.fAlignment
		&& fJustify == other.fJustify
		&& fFirstLineInset == other.fFirstLineInset
		&& fLineInset == other.fLineInset
		&& fSpacingTop == other.fSpacingTop
		&& fSpacingBottom == other.fSpacingBottom;
}


bool
ParagraphStyleData::operator!=(const ParagraphStyleData& other) const
{
	return !(*this == other);
}


ParagraphStyleData*
ParagraphStyleData::SetAlignment(::Alignment alignment)
{
	if (fAlignment == alignment)
		return this;

	ParagraphStyleData* ret = new(std::nothrow) ParagraphStyleData(*this);
	if (ret == NULL)
		return this;

	ret->fAlignment = alignment;
	return ret;
}


ParagraphStyleData*
ParagraphStyleData::SetJustify(bool justify)
{
	if (fJustify == justify)
		return this;

	ParagraphStyleData* ret = new(std::nothrow) ParagraphStyleData(*this);
	if (ret == NULL)
		return this;

	ret->fJustify = justify;
	return ret;
}


ParagraphStyleData*
ParagraphStyleData::SetFirstLineInset(float inset)
{
	if (fFirstLineInset == inset)
		return this;

	ParagraphStyleData* ret = new(std::nothrow) ParagraphStyleData(*this);
	if (ret == NULL)
		return this;

	ret->fFirstLineInset = inset;
	return ret;
}


ParagraphStyleData*
ParagraphStyleData::SetLineInset(float inset)
{
	if (fLineInset == inset)
		return this;

	ParagraphStyleData* ret = new(std::nothrow) ParagraphStyleData(*this);
	if (ret == NULL)
		return this;

	ret->fLineInset = inset;
	return ret;
}


ParagraphStyleData*
ParagraphStyleData::SetSpacingTop(float spacing)
{
	if (fSpacingTop == spacing)
		return this;

	ParagraphStyleData* ret = new(std::nothrow) ParagraphStyleData(*this);
	if (ret == NULL)
		return this;

	ret->fSpacingTop = spacing;
	return ret;
}


ParagraphStyleData*
ParagraphStyleData::SetSpacingBottom(float spacing)
{
	if (fSpacingBottom == spacing)
		return this;

	ParagraphStyleData* ret = new(std::nothrow) ParagraphStyleData(*this);
	if (ret == NULL)
		return this;

	ret->fSpacingBottom = spacing;
	return ret;
}


// #pragma mark - private


ParagraphStyleData&
ParagraphStyleData::operator=(const ParagraphStyleData& other)
{
	return *this;
}
