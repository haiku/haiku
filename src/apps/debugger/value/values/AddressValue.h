/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ADDRESS_VALUE_H
#define ADDRESS_VALUE_H


#include "IntegerValue.h"


class AddressValue : public IntegerValue {
public:
								AddressValue(const BVariant& value);
	virtual						~AddressValue();

	virtual	bool				ToString(BString& _string) const;
};


#endif	// ADDRESS_VALUE_H
