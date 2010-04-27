/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#include "PreviewColumn.h"
#include "ResFields.h"

#include <stdio.h>

PreviewColumn::PreviewColumn(const char *title, float width,
						 float minWidth, float maxWidth)
  :	BTitledColumn(title, width, minWidth, maxWidth)
{
}

void
PreviewColumn::DrawField(BField* field, BRect rect, BView* parent)
{
	PreviewField *pField = (PreviewField*)field;
	pField->DrawField(rect, parent);
}

bool
PreviewColumn::AcceptsField(const BField* field) const
{
	return dynamic_cast<const PreviewField*>(field);
}
