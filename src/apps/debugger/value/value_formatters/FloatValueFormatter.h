/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef FLOAT_VALUE_FORMATTER_H
#define FLOAT_VALUE_FORMATTER_H


#include "ValueFormatter.h"


class Settings;
class Value;


class FloatValueFormatter : public ValueFormatter {
public:
								FloatValueFormatter();
	virtual						~FloatValueFormatter();

	virtual	Settings*			GetSettings() const
									{ return NULL; }

	virtual	status_t			FormatValue(Value* value, BString& _output);
};


#endif	// FLOAT_VALUE_FORMATTER_H
