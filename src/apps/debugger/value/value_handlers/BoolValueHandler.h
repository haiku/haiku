/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOL_VALUE_HANDLER_H
#define BOOL_VALUE_HANDLER_H


#include "ValueHandler.h"


class BoolValueHandler : public ValueHandler {
public:
								BoolValueHandler();
								~BoolValueHandler();

			status_t			Init();

	virtual	float				SupportsValue(Value* value);
	virtual	status_t			GetValueFormatter(Value* value,
									ValueFormatter*& _formatter);
	virtual	status_t			GetTableCellValueRenderer(Value* value,
									TableCellValueRenderer*& _renderer);
};


#endif	// BOOL_VALUE_HANDLER_H
