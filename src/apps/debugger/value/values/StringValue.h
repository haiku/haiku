/*
 * Copyright 2010, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_VALUE_H
#define STRING_VALUE_H


#include "Value.h"


class StringValue : public Value {
public:
								StringValue(const char* value);
	virtual						~StringValue();

			BString				GetValue() const
									{ return fValue; }

	virtual	bool				ToString(BString& _string) const;
	virtual	bool				ToVariant(BVariant& _value) const;

	virtual	bool				operator==(const Value& other) const;

private:
			BString				fValue;
};


#endif	// STRING_VALUE_H
