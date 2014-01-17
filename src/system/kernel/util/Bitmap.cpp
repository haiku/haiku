/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include <util/Bitmap.h>

#include <new>

#include <string.h>

#include <util/BitUtils.h>


const int Bitmap::kBitsPerElement = sizeof(addr_t) * 8;


Bitmap::Bitmap(int bitCount)
	:
	fInitStatus(B_OK),
	fElementsCount(0),
	fSize(bitCount)
{
	int count = fSize + kBitsPerElement - 1;
	count /= kBitsPerElement;

	fBits = new(std::nothrow) addr_t[count];
	if (fBits == NULL) {
		fSize = 0;
		fInitStatus = B_NO_MEMORY;
	}

	fElementsCount = count;
	memset(fBits, 0, sizeof(addr_t) * count);
}


Bitmap::~Bitmap()
{
	delete[] fBits;
}


int
Bitmap::GetHighestSet() const
{
	int i = fElementsCount - 1;
	while (i >= 0 && fBits[i] == 0)
		i--;

	if (i < 0)
		return -1;

	STATIC_ASSERT(sizeof(addr_t) == sizeof(uint64)
		|| sizeof(addr_t) == sizeof(uint32));
	if (sizeof(addr_t) == sizeof(uint32))
		return log2(fBits[i]) + i * kBitsPerElement;

	uint32 v = (uint64)fBits[i] >> 32;
	if (v != 0)
		return log2(v) + sizeof(uint32) * 8 + i * kBitsPerElement;
	return log2(fBits[i]) + i * kBitsPerElement;
}

