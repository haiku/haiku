/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_FOR_RATE_H
#define STRING_FOR_RATE_H

#include <SupportDefs.h>


namespace BPrivate {


const char* string_for_rate(double rate, char* string, size_t stringSize,
	double base = 1000.0f);


}	// namespace BPrivate


using BPrivate::string_for_rate;


#endif // COLOR_QUANTIZER_H
