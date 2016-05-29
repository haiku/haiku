/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef ENUMERATION_VALUE_FORMATTER_H
#define ENUMERATION_VALUE_FORMATTER_H


#include "IntegerValueFormatter.h"


class EnumerationValueFormatter : public IntegerValueFormatter {
public:
								EnumerationValueFormatter(Config* config);
	virtual						~EnumerationValueFormatter();

	virtual	status_t			FormatValue(Value* value, BString& _output);
};


#endif	// ENUMERATION_VALUE_FORMATTER_H
