/*
 * Copyright 2010-2015, Rene Gollent, rene@gollent.com
 * Distributed under the terms of the MIT License.
 */


#include "StringValueHandler.h"

#include <new>

#include "StringValue.h"
#include "StringValueFormatter.h"
#include "TableCellFormattedValueRenderer.h"


StringValueHandler::StringValueHandler()
{
}


StringValueHandler::~StringValueHandler()
{
}


status_t
StringValueHandler::Init()
{
	return B_OK;
}


float
StringValueHandler::SupportsValue(Value* value)
{
	return dynamic_cast<StringValue*>(value) != NULL ? 0.8f : 0;
}


status_t
StringValueHandler::GetValueFormatter(Value* value,
	ValueFormatter*& _formatter)
{
	if (dynamic_cast<StringValue*>(value) == NULL)
		return B_BAD_VALUE;

	ValueFormatter* formatter = new(std::nothrow) StringValueFormatter;
	if (formatter == NULL)
		return B_NO_MEMORY;

	_formatter = formatter;
	return B_OK;
}


status_t
StringValueHandler::GetTableCellValueRenderer(Value* value,
	TableCellValueRenderer*& _renderer)
{
	if (dynamic_cast<StringValue*>(value) == NULL)
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
