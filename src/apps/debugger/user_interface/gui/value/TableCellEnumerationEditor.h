/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_CELL_ENUMERATION_EDITOR_H
#define TABLE_CELL_ENUMERATION_EDITOR_H

#include <TextControl.h>

#include "TableCellOptionPopUpEditor.h"


class TableCellEnumerationEditor : public TableCellOptionPopUpEditor {
public:
								TableCellEnumerationEditor(
									::Value* initialValue,
									ValueFormatter* formatter);
	virtual						~TableCellEnumerationEditor();

	virtual	status_t			ConfigureOptions();

protected:
	virtual	status_t			GetSelectedValue(::Value*& _value) const;
};

#endif	// TABLE_CELL_ENUMERATION_EDITOR_H
