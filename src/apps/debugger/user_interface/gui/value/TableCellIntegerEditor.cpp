/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TableCellIntegerEditor.h"

#include <ctype.h>

#include <Variant.h>

#include "IntegerValue.h"
#include "IntegerValueFormatter.h"


TableCellIntegerEditor::TableCellIntegerEditor(::Value* initialValue,
	ValueFormatter* formatter)
	:
	TableCellTextControlEditor(initialValue, formatter)
{
}


TableCellIntegerEditor::~TableCellIntegerEditor()
{
}


bool
TableCellIntegerEditor::ValidateInput() const
{
	BVariant variantValue;
	if (!InitialValue()->ToVariant(variantValue))
		return false;

	return GetValueFormatter()->ValidateFormattedValue(Text(),
		variantValue.Type());
}


status_t
TableCellIntegerEditor::GetValueForInput(::Value*& _output) const
{
	BVariant variantValue;
	if (!InitialValue()->ToVariant(variantValue))
		return B_NO_MEMORY;

	return GetValueFormatter()->GetValueFromFormattedInput(Text(),
		variantValue.Type(), _output);
}


