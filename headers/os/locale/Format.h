/*
 * Copyright 2003-2010, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
*/
#ifndef _B_FORMAT_H_
#define _B_FORMAT_H_

#include <FormatParameters.h>
#include <SupportDefs.h>


// types of fields contained in formatted strings
enum {
	// number format fields
	B_CURRENCY_FIELD,
	B_DECIMAL_SEPARATOR_FIELD,
	B_EXPONENT_FIELD,
	B_EXPONENT_SIGN_FIELD,
	B_EXPONENT_SYMBOL_FIELD,
	B_FRACTION_FIELD,
	B_GROUPING_SEPARATOR_FIELD,
	B_INTEGER_FIELD,
	B_PERCENT_FIELD,
	B_PERMILLE_FIELD,
	B_SIGN_FIELD,

	// date format fields
	// TODO: ...
};

// structure filled in while formatting
struct format_field_position {
	uint32	field_type;
	int32	start;
	int32	length;
};


class BLocale;

class BFormat {
public:
			status_t			InitCheck() const;

	virtual	status_t			SetLocale(const BLocale* locale);
protected:
								BFormat();
								BFormat(const BFormat& other);
	virtual 					~BFormat();

			BFormat&			operator=(const BFormat& other);

			status_t			fInitStatus;
			BLocale*			fLocale;
};


#endif	// _B_FORMAT_H_
