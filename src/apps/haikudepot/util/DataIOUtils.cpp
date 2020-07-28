/*
 * Copyright 2018-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "DataIOUtils.h"


#define BUFFER_SIZE 1024


/*static*/ status_t
DataIOUtils::CopyAll(BDataIO* target, BDataIO* source)
{
	status_t result = B_OK;
	uint8 buffer[BUFFER_SIZE];
	size_t sizeRead = 0;

	do {
		result = source->ReadExactly(buffer, BUFFER_SIZE, &sizeRead);

		switch (result)
		{
			case B_OK:
			case B_PARTIAL_READ:
				result = target->WriteExactly(buffer, sizeRead);
				break;
		}
	} while(result == B_OK && sizeRead > 0);

	return result;
}


ConstraintedDataIO::ConstraintedDataIO(BDataIO* delegate, size_t limit)
	:
	fDelegate(delegate),
	fLimit(limit)
{
}


ConstraintedDataIO::~ConstraintedDataIO()
{
}


ssize_t
ConstraintedDataIO::Read(void* buffer, size_t size)
{
	if (size > fLimit)
		size = fLimit;

	ssize_t actualRead = fDelegate->Read(buffer, size);

	if (actualRead > 0)
		fLimit -= actualRead;

	return actualRead;
}


ssize_t
ConstraintedDataIO::Write(const void* buffer, size_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
ConstraintedDataIO::Flush()
{
	return B_OK;
}
