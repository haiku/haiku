/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VALUE_H
#define VALUE_H


#include <String.h>

#include <Variant.h>


class Value : public BReferenceable {
public:
	virtual						~Value();

	virtual	bool				ToString(BString& _string) const = 0;
	virtual	bool				ToVariant(BVariant& _value) const = 0;

	virtual	bool				operator==(const Value& other) const = 0;
	inline	bool				operator!=(const Value& other) const
									{ return !(*this == other); }
};


#endif	// VALUE_H
