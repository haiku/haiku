/*
 * Copyright 2014-2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_CELL_FORMATTED_VALUE_RENDERER_H
#define TABLE_CELL_FORMATTED_VALUE_RENDERER_H


#include "TableCellValueRenderer.h"


class ValueFormatter;


class TableCellFormattedValueRenderer : public TableCellValueRenderer {
public:
								TableCellFormattedValueRenderer(
									ValueFormatter* formatter);
	virtual						~TableCellFormattedValueRenderer();

	virtual	Settings*			GetSettings() const;

			ValueFormatter*		GetValueFormatter() const
									{ return fValueFormatter; }

	virtual	void				RenderValue(Value* value, bool valueChanged,
									BRect rect, BView* targetView);
	virtual	float				PreferredValueWidth(Value* value,
									BView* targetView);


private:
			ValueFormatter*		fValueFormatter;
};


#endif	// TABLE_CELL_FORMATTED_VALUE_RENDERER_H
