/*
 * Copyright 2016, Haiku, inc.
 * Distributed under terms of the MIT license.
 */


#include "TextEncoding.h"

#include <unicode/ucsdet.h>


TextEncoding::TextEncoding(const char* data, size_t length)
{
	UErrorCode error = U_ZERO_ERROR;

	UCharsetDetector* detector = ucsdet_open(&error);
	ucsdet_setText(detector, data, length, &error);
	const UCharsetMatch* encoding = ucsdet_detect(detector, &error);

	fName = ucsdet_getName(encoding, &error);
	ucsdet_close(detector);
}


BString
TextEncoding::GetName()
{
	return fName;
}
