/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_VALUE_FORMATTER_H
#define STRING_VALUE_FORMATTER_H


#include "ValueFormatter.h"


class Settings;
class Value;


class StringValueFormatter : public ValueFormatter {
public:
								StringValueFormatter();
	virtual						~StringValueFormatter();

	virtual	Settings*			GetSettings() const
									{ return NULL; }

	virtual	status_t			FormatValue(Value* value, BString& _output);
};


#endif	// STRING_VALUE_FORMATTER_H
