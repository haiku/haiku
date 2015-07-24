/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "FloatValueHandler.h"

#include <new>

#include "FloatValue.h"
#include "FloatValueFormatter.h"
#include "TableCellFloatEditor.h"
#include "TableCellFormattedValueRenderer.h"


FloatValueHandler::FloatValueHandler()
{
}


FloatValueHandler::~FloatValueHandler()
{
}


status_t
FloatValueHandler::Init()
{
	return B_OK;
}


float
FloatValueHandler::SupportsValue(Value* value)
{
	return dynamic_cast<FloatValue*>(value) != NULL ? 0.5f : 0;
}


status_t
FloatValueHandler::GetValueFormatter(Value* value,
	ValueFormatter*& _formatter)
{
	if (dynamic_cast<FloatValue*>(value) == NULL)
		return B_BAD_VALUE;

	FloatValueFormatter* formatter = new(std::nothrow) FloatValueFormatter;
	if (formatter == NULL)
		return B_NO_MEMORY;

	_formatter = formatter;
	return B_OK;
}


status_t
FloatValueHandler::GetTableCellValueRenderer(Value* value,
	TableCellValueRenderer*& _renderer)
{
	if (dynamic_cast<FloatValue*>(value) == NULL)
		return B_BAD_VALUE;

	ValueFormatter* formatter = NULL;
	status_t error = GetValueFormatter(value, formatter);
	if (error != B_OK)
		return error;
	BReference<ValueFormatter> formatterReference(formatter, true);

	// create the renderer
	TableCellValueRenderer* renderer
		= new(std::nothrow) TableCellFormattedValueRenderer(formatter);
	if (renderer == NULL)
		return B_NO_MEMORY;

	_renderer = renderer;
	return B_OK;
}


status_t
FloatValueHandler::GetTableCellValueEditor(Value* _value, Settings* settings,
	TableCellValueEditor*& _editor)
{
	FloatValue* value = dynamic_cast<FloatValue*>(_value);
	if (value == NULL)
		return B_BAD_VALUE;

	ValueFormatter* formatter;
	status_t error = GetValueFormatter(value, formatter);
	if (error != B_OK)
		return error;
	BReference<ValueFormatter> formatterReference(formatter, true);

	TableCellFloatEditor* editor = new(std::nothrow)
		TableCellFloatEditor(value, formatter);
	if (editor == NULL)
		return B_NO_MEMORY;

	BReference<TableCellFloatEditor> editorReference(editor, true);
	error = editor->Init();
	if (error != B_OK)
		return error;

	editorReference.Detach();
	_editor = editor;
	return B_OK;
}
