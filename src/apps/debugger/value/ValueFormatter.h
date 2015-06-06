/*
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef VALUE_FORMATTER_H
#define VALUE_FORMATTER_H


#include <Referenceable.h>

class BString;
class Settings;
class Value;


class ValueFormatter : public BReferenceable {
public:
	virtual						~ValueFormatter();

	virtual	Settings*			GetSettings() const = 0;
									// returns NULL, if no settings

	virtual	status_t			FormatValue(Value* value, BString& _output)
									= 0;

	virtual	bool				SupportsValidation() const;
	virtual	bool				ValidateFormattedValue(
									const BString& input,
									type_code type) const;
									// checks if the passed in string
									// would be considered a valid value
									// according to the current format
									// configuration and the size constraints
									// imposed by the passed in type.
	virtual	status_t			GetValueFromFormattedInput(
									const BString& input, type_code type,
									Value*& _output) const;
									// returns reference
};


#endif	// VALUE_FORMATTER_H
