/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "EnumerationValueHandler.h"

#include <new>

#include "EnumerationValue.h"
#include "EnumerationValueFormatter.h"
#include "TableCellEnumerationEditor.h"
#include "TableCellFormattedValueRenderer.h"
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


status_t
EnumerationValueHandler::GetValueFormatter(Value* _value,
	ValueFormatter*& _formatter)
{
	EnumerationValue* value = dynamic_cast<EnumerationValue*>(_value);
	if (value == NULL)
		return B_BAD_VALUE;

	IntegerValueFormatter::Config* config = NULL;
	status_t error = CreateIntegerFormatterConfig(value, config);
	if (error != B_OK)
		return error;
	BReference<IntegerValueFormatter::Config> configReference(config, true);

	ValueFormatter* formatter = NULL;
	error = CreateValueFormatter(config, formatter);
	if (error != B_OK)
		return error;

	_formatter = formatter;

	return B_OK;
}


status_t
EnumerationValueHandler::GetTableCellValueEditor(Value* _value,
	Settings* settings, TableCellValueEditor*& _editor)
{
	EnumerationValue* value = dynamic_cast<EnumerationValue*>(_value);
	if (value == NULL)
		return B_BAD_VALUE;

	IntegerValueFormatter::Config* config = NULL;
	status_t error = CreateIntegerFormatterConfig(value, config);
	if (error != B_OK)
		return error;
	BReference<IntegerValueFormatter::Config> configReference(config, true);

	ValueFormatter* formatter;
	error = CreateValueFormatter(config, formatter);
	if (error != B_OK)
		return error;
	BReference<ValueFormatter> formatterReference(formatter, true);

	TableCellEnumerationEditor* editor = new(std::nothrow)
		TableCellEnumerationEditor(value, formatter);
	if (editor == NULL)
		return B_NO_MEMORY;

	BReference<TableCellEnumerationEditor> editorReference(editor, true);
	error = editor->Init();
	if (error != B_OK)
		return error;

	editorReference.Detach();
	_editor = editor;
	return B_OK;
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
EnumerationValueHandler::CreateValueFormatter(
	IntegerValueFormatter::Config* config, ValueFormatter*& _formatter)
{
	ValueFormatter* formatter = new(std::nothrow) EnumerationValueFormatter(
		config);
	if (formatter == NULL)
		return B_NO_MEMORY;

	_formatter = formatter;
	return B_OK;
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
	IntegerValueFormatter::Config* config,
	TableCellValueRenderer*& _renderer)
{
	EnumerationValue* value = dynamic_cast<EnumerationValue*>(_value);
	if (value != NULL
		&& value->GetType()->ValueFor(value->GetValue()) != NULL) {
		ValueFormatter* formatter = NULL;
		status_t error = GetValueFormatter(value, formatter);
		if (error != B_OK)
			return error;
		BReference<ValueFormatter> formatterReference(formatter,
			true);

		TableCellValueRenderer* renderer
			= new(std::nothrow) TableCellFormattedValueRenderer(formatter);
		if (renderer == NULL)
			return B_NO_MEMORY;

		_renderer = renderer;
		return B_OK;
	}

	return IntegerValueHandler::CreateTableCellValueRenderer(_value, config,
		_renderer);
}
