/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef INT64_PROPERTY_H
#define INT64_PROPERTY_H

#include "Property.h"

class Int64Property : public Property {
 public:
								Int64Property(uint32 identifier,
											  int64 value = 0);
								Int64Property(const Int64Property& other);
	virtual						~Int64Property();

	// TODO: BArchivable
	virtual	Property*			Clone() const;

	virtual	type_code			Type() const
									{ return B_INT64_TYPE; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				InterpolateTo(const Property* other,
											  float scale);

	// IntProperty
			bool				SetValue(int64 value);

	inline	int64				Value() const
									{ return fValue; }

 private:
			int64				fValue;
};

#endif // PROPERTY_H
