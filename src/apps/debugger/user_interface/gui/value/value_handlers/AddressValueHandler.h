/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ADDRESS_VALUE_HANDLER_H
#define ADDRESS_VALUE_HANDLER_H


#include "IntegerValueHandler.h"


class AddressValueHandler : public IntegerValueHandler {
public:
	virtual	float				SupportsValue(Value* value);

protected:
	virtual	integer_format		DefaultIntegerFormat(IntegerValue* value);
};


#endif	// ADDRESS_VALUE_HANDLER_H
