/*
 * Copyright 2003-2017, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_NUMBER_FORMAT_H_
#define _B_NUMBER_FORMAT_H_


#include <Format.h>


enum BNumberElement {
	B_NUMBER_ELEMENT_INVALID = B_BAD_DATA,
	B_NUMBER_ELEMENT_INTEGER = 0,
	B_NUMBER_ELEMENT_FRACTIONAL,
	B_NUMBER_ELEMENT_CURRENCY
};

class BNumberFormatImpl;


class BNumberFormat : public BFormat {
public:
								BNumberFormat();
								~BNumberFormat();

			ssize_t				Format(char* string, size_t maxSize,
									const double value);
			status_t			Format(BString& string, const double value);
			ssize_t				Format(char* string, size_t maxSize,
									const int32 value);
			status_t			Format(BString& string, const int32 value);

			ssize_t				FormatMonetary(char* string, size_t maxSize,
									const double value);
			status_t			FormatMonetary(BString& string,
									const double value);

private:
								BNumberFormat(const BNumberFormat &other);

private:
			BNumberFormatImpl*	fPrivateData;
};


#endif	// _B_NUMBER_FORMAT_H_
