/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef INTEGER_FORMATTER_H
#define INTEGER_FORMATTER_H


#include <Variant.h>


enum integer_format {
	INTEGER_FORMAT_DEFAULT		= 0,
	INTEGER_FORMAT_SIGNED		= 1,
	INTEGER_FORMAT_UNSIGNED		= 2,
	INTEGER_FORMAT_HEX_DEFAULT	= 3,
	INTEGER_FORMAT_HEX_8		= 8,
	INTEGER_FORMAT_HEX_16		= 16,
	INTEGER_FORMAT_HEX_32	 	= 32,
	INTEGER_FORMAT_HEX_64		= 64
};


class IntegerFormatter {
public:
	static	bool				FormatValue(const BVariant& value,
									integer_format format, char* buffer,
									size_t bufferSize);
};


#endif	// INTEGER_FORMATTER_H
