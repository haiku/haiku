/*
 * Copyright 2010, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_VALUE_HANDLER_H
#define STRING_VALUE_HANDLER_H


#include "ValueHandler.h"


class StringValueHandler : public ValueHandler {
public:
								StringValueHandler();
								~StringValueHandler();

			status_t			Init();

	virtual	float				SupportsValue(Value* value);
	virtual	status_t			GetValueFormatter(Value* value,
									ValueFormatter*& _formatter);
	virtual	status_t			GetTableCellValueRenderer(Value* value,
									TableCellValueRenderer*& _renderer);
};


#endif	// STRING_VALUE_HANDLER_H
