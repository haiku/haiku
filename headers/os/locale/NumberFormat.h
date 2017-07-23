/*
 * Copyright 2003-2017, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_NUMBER_FORMAT_H_
#define _B_NUMBER_FORMAT_H_


#include <Format.h>


enum BNumberElement {
	B_DECIMAL_SEPARATOR = 10, // Values 0-9 are reserved for digit symbols
	B_GROUPING_SEPARATOR,
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

			status_t			Parse(const BString& string, double& value);

			BString			GetSeparator(BNumberElement element);

private:
								BNumberFormat(const BNumberFormat &other);

private:
			BNumberFormatImpl*	fPrivateData;
};


#endif	// _B_NUMBER_FORMAT_H_
