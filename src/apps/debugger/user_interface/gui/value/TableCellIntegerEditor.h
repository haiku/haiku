/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_CELL_INTEGER_EDITOR_H
#define TABLE_CELL_INTEGER_EDITOR_H

#include <TextControl.h>

#include "IntegerFormatter.h"
#include "TableCellTextControlEditor.h"


class TableCellIntegerEditor : public TableCellTextControlEditor {
public:
								TableCellIntegerEditor(::Value* initialValue,
									ValueFormatter* formatter);
	virtual						~TableCellIntegerEditor();

	virtual	bool				ValidateInput() const;

	virtual status_t			GetValueForInput(::Value*& _output) const;
									// returns reference
};

#endif	// TABLE_CELL_INTEGER_EDITOR_H
