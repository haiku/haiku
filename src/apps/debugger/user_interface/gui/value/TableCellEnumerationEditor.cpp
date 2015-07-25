/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "TableCellEnumerationEditor.h"

#include "EnumerationValue.h"
#include "Type.h"


TableCellEnumerationEditor::TableCellEnumerationEditor(::Value* initialValue,
	ValueFormatter* formatter)
	:
	TableCellOptionPopUpEditor(initialValue, formatter)
{
}


TableCellEnumerationEditor::~TableCellEnumerationEditor()
{
}


status_t
TableCellEnumerationEditor::ConfigureOptions()
{
	EnumerationValue* initialValue = dynamic_cast<EnumerationValue*>(
		InitialValue());
	if (initialValue == NULL)
		return B_BAD_VALUE;

	EnumerationType* type = initialValue->GetType();
	for (int32 i = 0; i < type->CountValues(); i++) {
		EnumeratorValue* value = type->ValueAt(i);
		BString output;

		status_t error = AddOption(value->Name(), value->Value().ToInt32());
		if (error != B_OK)
			return error;
	}

	BVariant integerValue;
	if (!initialValue->ToVariant(integerValue))
		return B_NO_MEMORY;

	SelectOptionFor(integerValue.ToInt32());

	return B_OK;
}


status_t
TableCellEnumerationEditor::GetSelectedValue(::Value*& _value) const
{
	EnumerationValue* initialValue = dynamic_cast<EnumerationValue*>(
		InitialValue());
	EnumerationType* type = initialValue->GetType();
	const char* name = NULL;
	int32 selectedValue = 0;
	SelectedOption(&name, &selectedValue);

	EnumerationValue* value = new(std::nothrow) EnumerationValue(type,
		BVariant(selectedValue));
	if (value == NULL)
		return B_NO_MEMORY;

	_value = value;
	return B_OK;
}
