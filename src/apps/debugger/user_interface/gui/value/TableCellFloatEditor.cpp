/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TableCellFloatEditor.h"

#include <ctype.h>

#include <Variant.h>

#include "IntegerValue.h"
#include "IntegerValueFormatter.h"


TableCellFloatEditor::TableCellFloatEditor(::Value* initialValue,
	ValueFormatter* formatter)
	:
	TableCellTextControlEditor(initialValue, formatter)
{
}


TableCellFloatEditor::~TableCellFloatEditor()
{
}


bool
TableCellFloatEditor::ValidateInput() const
{
	BVariant variantValue;
	if (!InitialValue()->ToVariant(variantValue))
		return false;

	return GetValueFormatter()->ValidateFormattedValue(Text(),
		variantValue.Type());
}


status_t
TableCellFloatEditor::GetValueForInput(::Value*& _output) const
{
	BVariant variantValue;
	if (!InitialValue()->ToVariant(variantValue))
		return B_NO_MEMORY;

	return GetValueFormatter()->GetValueFromFormattedInput(Text(),
		variantValue.Type(), _output);
}


