/*
 * Copyright 2013-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 *		Augustin Cavalier <waddlesplash>
 */
#include <util/Bitmap.h>

#include <stdlib.h>
#include <string.h>

#include <utils/BitUtils.h>


namespace BKernel {


Bitmap::Bitmap(size_t bitCount)
	:
	fElementsCount(0),
	fSize(0),
	fBits(NULL)
{
	Resize(bitCount);
}


Bitmap::~Bitmap()
{
	free(fBits);
}


status_t
Bitmap::InitCheck()
{
	return (fBits != NULL) ? B_OK : B_NO_MEMORY;
}


status_t
Bitmap::Resize(size_t bitCount)
{
	const size_t count = (bitCount + kBitsPerElement - 1) / kBitsPerElement;
	if (count == fElementsCount) {
		fSize = bitCount;
		return B_OK;
	}

	void* bits = realloc(fBits, sizeof(addr_t) * count);
	if (bits == NULL)
		return B_NO_MEMORY;
	fBits = (addr_t*)bits;

	if (fElementsCount < count)
		memset(&fBits[fElementsCount], 0, sizeof(addr_t) * (count - fElementsCount));

	fSize = bitCount;
	fElementsCount = count;
	return B_OK;
}


void
Bitmap::Shift(ssize_t bitCount)
{
	return bitmap_shift<addr_t>(fBits, fSize, bitCount);
}


void
Bitmap::SetRange(size_t index, size_t count)
{
	// TODO: optimize
	for (; count > 0; count--)
		Set(index++);
}


void
Bitmap::ClearRange(size_t index, size_t count)
{
	// TODO: optimize
	for (; count > 0; count--)
		Clear(index++);
}


ssize_t
Bitmap::GetLowestClear(size_t fromIndex) const
{
	// TODO: optimize

	for (size_t i = fromIndex; i < fSize; i++) {
		if (!Get(i))
			return i;
	}
	return -1;
}


ssize_t
Bitmap::GetLowestContiguousClear(size_t count, size_t fromIndex) const
{
	// TODO: optimize

	// nothing to find
	if (count == 0)
		return fromIndex;

	for (;;) {
		ssize_t index = GetLowestClear(fromIndex);
		if (index < 0)
			return index;

		// overflow check
		if ((size_t)index + count - 1 < (size_t)index)
			return -1;

		size_t curCount = 1;
		while (curCount < count && Get(index + curCount))
			curCount++;

		if (curCount == count)
			return index;

		fromIndex = index + curCount;
	}
}


ssize_t
Bitmap::GetHighestSet() const
{
	size_t i = fElementsCount - 1;
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


} // namespace BKernel
