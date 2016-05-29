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

	virtual	bool				SupportsValidation() const;

	virtual	bool				ValidateFormattedValue(
									const BString& input,
									type_code type) const;

	virtual	status_t			GetValueFromFormattedInput(
									const BString& input, type_code type,
									Value*& _output) const;
private:

			status_t			_PerformValidation(const BString& input,
									type_code type,
									::Value*& _output,
									bool wantsValue) const;
};


#endif	// FLOAT_VALUE_FORMATTER_H
