/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "FloatValueHandler.h"

#include <new>

#include "FloatValue.h"
#include "TableCellFloatRenderer.h"


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
	// TODO:...
	return B_UNSUPPORTED;
}


status_t
FloatValueHandler::GetTableCellValueRenderer(Value* value,
	TableCellValueRenderer*& _renderer)
{
	if (dynamic_cast<FloatValue*>(value) == NULL)
		return B_BAD_VALUE;

	// create the renderer
	TableCellValueRenderer* renderer = new(std::nothrow) TableCellFloatRenderer;
	if (renderer == NULL)
		return B_NO_MEMORY;

	_renderer = renderer;
	return B_OK;
}
