/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOL_VALUE_FORMATTER_H
#define BOOL_VALUE_FORMATTER_H


#include "ValueFormatter.h"


class Settings;
class Value;


class BoolValueFormatter : public ValueFormatter {
public:
								BoolValueFormatter();
	virtual						~BoolValueFormatter();

	virtual	Settings*			GetSettings() const
									{ return NULL; }

	virtual	status_t			FormatValue(Value* value, BString& _output);
};


#endif	// BOOL_VALUE_FORMATTER_H
