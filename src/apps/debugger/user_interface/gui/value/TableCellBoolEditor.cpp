/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TableCellBoolEditor.h"

#include "BoolValue.h"


TableCellBoolEditor::TableCellBoolEditor(::Value* initialValue,
	ValueFormatter* formatter)
	:
	TableCellOptionPopUpEditor(initialValue, formatter)
{
}


TableCellBoolEditor::~TableCellBoolEditor()
{
}


status_t
TableCellBoolEditor::ConfigureOptions()
{
	BoolValue* initialValue = dynamic_cast<BoolValue*>(InitialValue());
	if (initialValue == NULL)
		return B_BAD_VALUE;

	status_t error = AddOption("true", true);
	if (error != B_OK)
		return error;

	error = AddOption("false", false);
	if (error != B_OK)
		return error;

	return SelectOptionFor(initialValue->GetValue());
}


status_t
TableCellBoolEditor::GetSelectedValue(::Value*& _value) const
{
	const char* name = NULL;
	int32 selectedValue = 0;
	SelectedOption(&name, &selectedValue);
	BoolValue* value = new(std::nothrow) BoolValue((bool)selectedValue);
	if (value == NULL)
		return B_NO_MEMORY;

	_value = value;
	return B_OK;
}
