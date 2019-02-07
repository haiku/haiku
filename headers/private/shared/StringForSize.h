/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_FOR_SIZE_H
#define STRING_FOR_SIZE_H

#include <SupportDefs.h>


namespace BPrivate {


const char* string_for_size(double size, char* string, size_t stringSize);
int64 parse_size(const char* sizeString);


}	// namespace BPrivate


using BPrivate::string_for_size;
using BPrivate::parse_size;


#endif // STRING_FOR_SIZE_H
