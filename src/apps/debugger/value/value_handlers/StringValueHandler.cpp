/*
 * Copyright 2010, Rene Gollent, rene@gollent.com
 * Distributed under the terms of the MIT License.
 */


#include "StringValueHandler.h"

#include <new>

#include <stdio.h>

#include "StringValue.h"
#include "TableCellStringRenderer.h"


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
	// TODO:...
	return B_UNSUPPORTED;
}


status_t
StringValueHandler::GetTableCellValueRenderer(Value* value,
	TableCellValueRenderer*& _renderer)
{
	if (dynamic_cast<StringValue*>(value) == NULL)
		return B_BAD_VALUE;

	// create the renderer
	TableCellValueRenderer* renderer = new(std::nothrow)
		TableCellStringRenderer;
	if (renderer == NULL)
		return B_NO_MEMORY;

	_renderer = renderer;
	return B_OK;
}
