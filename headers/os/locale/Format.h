/*
 * Copyright 2003-2010, Haiku, Inc.
 * Distributed under the terms of the MIT Licence.
*/
#ifndef _B_FORMAT_H_
#define _B_FORMAT_H_

#include <FormattingConventions.h>
#include <Locker.h>
#include <Language.h>
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
protected:
								BFormat(const BLocale* locale = NULL);
								BFormat(const BLanguage& language,
									const BFormattingConventions& conventions);

								BFormat(const BFormat& other);
	virtual 					~BFormat();

private:
			BFormat&			operator=(const BFormat& other);

			status_t			_Initialize(const BLocale& locale);
			status_t			_Initialize(const BLanguage& language,
									const BFormattingConventions& conventions);

protected:
			BFormattingConventions	fConventions;
			BLanguage			fLanguage;
			status_t			fInitStatus;
};


#endif	// _B_FORMAT_H_
