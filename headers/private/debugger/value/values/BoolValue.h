/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOL_VALUE_H
#define BOOL_VALUE_H


#include "Value.h"


class BoolValue : public Value {
public:
								BoolValue(bool value);
	virtual						~BoolValue();

			bool				GetValue() const
									{ return fValue; }

	virtual	bool				ToString(BString& _string) const;
	virtual	bool				ToVariant(BVariant& _value) const;

	virtual	bool				operator==(const Value& other) const;

private:
			bool				fValue;
};


#endif	// BOOL_VALUE_H
