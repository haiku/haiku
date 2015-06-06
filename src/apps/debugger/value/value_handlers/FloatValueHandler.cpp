/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "FloatValueHandler.h"

#include <new>

#include "FloatValue.h"
#include "FloatValueFormatter.h"
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
