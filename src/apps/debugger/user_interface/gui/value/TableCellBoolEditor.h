/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_CELL_BOOL_EDITOR_H
#define TABLE_CELL_BOOL_EDITOR_H

#include <TextControl.h>

#include "TableCellOptionPopUpEditor.h"


class TableCellBoolEditor : public TableCellOptionPopUpEditor {
public:
								TableCellBoolEditor(::Value* initialValue,
									ValueFormatter* formatter);
	virtual						~TableCellBoolEditor();

	virtual	status_t			ConfigureOptions();

};

#endif	// TABLE_CELL_BOOL_EDITOR_H
