/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#ifndef PREVIEW_COLUMN_H
#define PREVIEW_COLUMN_H

#include <ColumnTypes.h>

class PreviewColumn : public BTitledColumn
{
public:
				PreviewColumn(const char *title, float width,
						 float minWidth, float maxWidth);
	void		DrawField(BField* field, BRect rect, BView* parent);
	bool		AcceptsField(const BField* field) const;
};

#endif
