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

	return SelectOptionFor(integerValue.ToInt32());
}
