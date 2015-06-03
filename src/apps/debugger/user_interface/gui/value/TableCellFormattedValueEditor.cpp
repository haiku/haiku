/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TableCellFormattedValueEditor.h"

#include "ValueFormatter.h"


TableCellFormattedValueEditor::TableCellFormattedValueEditor(
	Value* initialValue, ValueFormatter* formatter)
	:
	TableCellValueEditor(),
	fValueFormatter(formatter)
{
	SetInitialValue(initialValue);
	fValueFormatter->AcquireReference();
}


TableCellFormattedValueEditor::~TableCellFormattedValueEditor()
{
	fValueFormatter->ReleaseReference();
}
