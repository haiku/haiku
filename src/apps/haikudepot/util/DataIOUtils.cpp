/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "DataIOUtils.h"


#define BUFFER_SIZE 1024


status_t
DataIOUtils::Copy(BDataIO* target, BDataIO* source, size_t size)
{
	status_t result = B_OK;
	uint8 buffer[BUFFER_SIZE];

	while (size > 0 && result == B_OK) {
		size_t sizeToRead = size;
		size_t sizeRead = 0;

		if (sizeToRead > BUFFER_SIZE)
			sizeToRead = BUFFER_SIZE;

		result = source->ReadExactly(buffer, sizeToRead, &sizeRead);

		if (result == B_OK)
			result = target->WriteExactly(buffer, sizeRead);

		size -= sizeRead;
	}

	return result;
}