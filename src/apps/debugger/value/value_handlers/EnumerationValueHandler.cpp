/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "EnumerationValueHandler.h"

#include <new>

#include "EnumerationValue.h"
#include "TableCellEnumerationRenderer.h"
#include "Type.h"


EnumerationValueHandler::EnumerationValueHandler()
{
}


EnumerationValueHandler::~EnumerationValueHandler()
{
}


status_t
EnumerationValueHandler::Init()
{
	return B_OK;
}


float
EnumerationValueHandler::SupportsValue(Value* value)
{
	return dynamic_cast<EnumerationValue*>(value) != NULL ? 0.7f : 0;
}



integer_format
EnumerationValueHandler::DefaultIntegerFormat(IntegerValue* _value)
{
	EnumerationValue* value = dynamic_cast<EnumerationValue*>(_value);
	if (value != NULL && value->GetType()->ValueFor(value->GetValue()) != NULL)
		return INTEGER_FORMAT_DEFAULT;

	return IntegerValueHandler::DefaultIntegerFormat(_value);
}


status_t
EnumerationValueHandler::AddIntegerFormatSettingOptions(IntegerValue* _value,
	OptionsSettingImpl* setting)
{
	EnumerationValue* value = dynamic_cast<EnumerationValue*>(_value);
	if (value != NULL
		&& value->GetType()->ValueFor(value->GetValue()) != NULL) {
		status_t error = AddIntegerFormatOption(setting, "name", "Enum Name",
			INTEGER_FORMAT_DEFAULT);
		if (error != B_OK)
			return error;
	}

	return IntegerValueHandler::AddIntegerFormatSettingOptions(_value, setting);
}


status_t
EnumerationValueHandler::CreateTableCellValueRenderer(IntegerValue* _value,
	TableCellIntegerRenderer::Config* config,
	TableCellValueRenderer*& _renderer)
{
	EnumerationValue* value = dynamic_cast<EnumerationValue*>(_value);
	if (value != NULL
		&& value->GetType()->ValueFor(value->GetValue()) != NULL) {
		TableCellValueRenderer* renderer
			= new(std::nothrow) TableCellEnumerationRenderer(config);
		if (renderer == NULL)
			return B_NO_MEMORY;

		_renderer = renderer;
		return B_OK;
	}

	return IntegerValueHandler::CreateTableCellValueRenderer(_value, config,
		_renderer);
}
