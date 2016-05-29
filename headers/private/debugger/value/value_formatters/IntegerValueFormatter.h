/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef INTEGER_VALUE_FORMATTER_H
#define INTEGER_VALUE_FORMATTER_H


#include "util/IntegerFormatter.h"
#include "ValueFormatter.h"


class Settings;
class Value;


class IntegerValueFormatter : public ValueFormatter {
public:
	class Config;

public:
								IntegerValueFormatter(Config* config);
	virtual						~IntegerValueFormatter();

			Config*				GetConfig() const
									{ return fConfig; }

	virtual	Settings*			GetSettings() const;

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
			status_t			_ValidateSigned(const BString& input,
									type_code type,
									::Value*& _output,
									bool wantsValue = false) const;
			status_t			_ValidateUnsigned(const BString& input,
									type_code type,
									::Value*& _output,
									integer_format format,
									bool wantsValue = false) const;

			Config*				fConfig;
};


class IntegerValueFormatter::Config : public BReferenceable {
public:
	virtual						~Config();

	virtual	Settings*			GetSettings() const = 0;
	virtual	integer_format		IntegerFormat() const = 0;
};


#endif	// INTEGER_VALUE_FORMATTER_H
