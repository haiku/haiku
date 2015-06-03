/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_CELL_FORMATTED_VALUE_EDITOR_H
#define TABLE_CELL_FORMATTED_VALUE_EDITOR_H

#include "TableCellValueEditor.h"


class ValueFormatter;


class TableCellFormattedValueEditor : public TableCellValueEditor {
public:
								TableCellFormattedValueEditor(
									Value* initialValue,
									ValueFormatter* formatter);
	virtual						~TableCellFormattedValueEditor();

protected:
			ValueFormatter*		GetValueFormatter() const
									{ return fValueFormatter; }

private:
			ValueFormatter*		fValueFormatter;
};


#endif	// TABLE_CELL_FORMATTED_VALUE_EDITOR_H
