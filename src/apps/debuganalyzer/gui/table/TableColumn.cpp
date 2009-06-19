/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "table/TableColumn.h"

#include <String.h>


TableColumn::TableColumn(int32 modelIndex, float width, float minWidth,
	float maxWidth, alignment align)
	:
	fModelIndex(modelIndex),
	fWidth(width),
	fMinWidth(minWidth),
	fMaxWidth(maxWidth),
	fAlignment(align)
{
}


TableColumn::~TableColumn()
{
}


void
TableColumn::GetColumnName(BString* into) const
{
	*into = "";
}


void
TableColumn::DrawTitle(BRect rect, BView* targetView)
{
}


void
TableColumn::DrawValue(const BVariant& value, BRect rect, BView* targetView)
{
}


int
TableColumn::CompareValues(const BVariant& a, const BVariant& b)
{
	return 0;
}


float
TableColumn::GetPreferredWidth(const BVariant& value, BView* parent) const
{
	return Width();
}
