/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef INTEGER_VALUE_H
#define INTEGER_VALUE_H


#include "Value.h"


class IntegerValue : public Value {
public:
								IntegerValue(const BVariant& value);
	virtual						~IntegerValue();

			bool				IsSigned() const;

			int64				ToInt64() const
									{ return fValue.ToInt64(); }
			uint64				ToUInt64() const
									{ return fValue.ToUInt64(); }
			const BVariant&		GetValue() const
									{ return fValue; }

	virtual	bool				ToString(BString& _string) const;
	virtual	bool				ToVariant(BVariant& _value) const;

	virtual	bool				operator==(const Value& other) const;

protected:
			BVariant			fValue;
};


#endif	// INTEGER_VALUE_H
