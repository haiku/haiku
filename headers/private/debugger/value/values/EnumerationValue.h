/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ENUMERATION_VALUE_H
#define ENUMERATION_VALUE_H


#include "IntegerValue.h"


class EnumerationType;


class EnumerationValue : public IntegerValue {
public:
								EnumerationValue(EnumerationType* type,
									const BVariant& value);
	virtual						~EnumerationValue();

			EnumerationType*	GetType() const
									{ return fType; }

	virtual	bool				ToString(BString& _string) const;

private:
			EnumerationType*	fType;
};


#endif	// ENUMERATION_VALUE_H
