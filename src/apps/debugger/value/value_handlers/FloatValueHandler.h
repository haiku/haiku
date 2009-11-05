/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FLOAT_VALUE_HANDLER_H
#define FLOAT_VALUE_HANDLER_H


#include "ValueHandler.h"


class FloatValueHandler : public ValueHandler {
public:
								FloatValueHandler();
								~FloatValueHandler();

			status_t			Init();

	virtual	float				SupportsValue(Value* value);
	virtual	status_t			GetValueFormatter(Value* value,
									ValueFormatter*& _formatter);
	virtual	status_t			GetTableCellValueRenderer(Value* value,
									TableCellValueRenderer*& _renderer);
};


#endif	// FLOAT_VALUE_HANDLER_H
