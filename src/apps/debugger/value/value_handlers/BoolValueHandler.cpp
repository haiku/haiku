/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "BoolValueHandler.h"

#include <new>

#include "BoolValue.h"
#include "BoolValueFormatter.h"
#include "TableCellFormattedValueRenderer.h"


BoolValueHandler::BoolValueHandler()
{
}


BoolValueHandler::~BoolValueHandler()
{
}


status_t
BoolValueHandler::Init()
{
	return B_OK;
}


float
BoolValueHandler::SupportsValue(Value* value)
{
	return dynamic_cast<BoolValue*>(value) != NULL ? 0.5f : 0;
}


status_t
BoolValueHandler::GetValueFormatter(Value* value,
	ValueFormatter*& _formatter)
{
	if (dynamic_cast<BoolValue*>(value) == NULL)
		return B_BAD_VALUE;

	BoolValueFormatter* formatter = new(std::nothrow) BoolValueFormatter;
	if (formatter == NULL)
		return B_NO_MEMORY;

	_formatter = formatter;
	return B_OK;
}


status_t
BoolValueHandler::GetTableCellValueRenderer(Value* value,
	TableCellValueRenderer*& _renderer)
{
	if (dynamic_cast<BoolValue*>(value) == NULL)
		return B_BAD_VALUE;

	ValueFormatter* formatter = NULL;
	if (GetValueFormatter(value, formatter))
		return B_NO_MEMORY;
	BReference<ValueFormatter> formatterReference(formatter, true);

	// create the renderer
	TableCellValueRenderer* renderer
		= new(std::nothrow) TableCellFormattedValueRenderer(formatter);
	if (renderer == NULL)
		return B_NO_MEMORY;

	_renderer = renderer;
	return B_OK;
}
