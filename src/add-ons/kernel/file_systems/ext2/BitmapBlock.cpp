/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 *		Jérôme Duval
 */


#include "BitmapBlock.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("\33[34mext2:\33[0m " x)


BitmapBlock::BitmapBlock(Volume* volume, uint32 numBits)
	:
	CachedBlock(volume),
	fData(NULL),
	fReadOnlyData(NULL),
	fNumBits(numBits),
	fMaxIndex(fNumBits >> 5)
{
	TRACE("BitmapBlock::BitmapBlock(): num bits: %lu\n", fNumBits);
}


BitmapBlock::~BitmapBlock()
{
}


bool
BitmapBlock::SetTo(off_t block)
{
	fData = NULL;
	fReadOnlyData = (uint32*)CachedBlock::SetTo(block);

	return fReadOnlyData != NULL;
}


bool
BitmapBlock::SetToWritable(Transaction& transaction, off_t block, bool empty)
{
	fReadOnlyData = NULL;
	fData = (uint32*)CachedBlock::SetToWritable(transaction, block, empty);

	return fData != NULL;
}


bool
BitmapBlock::_Check(uint32 start, uint32 length, bool marked)
{
	const uint32* data = fData == NULL ? fReadOnlyData : fData;
	if (data == NULL)
		return false;

	if (start + length > fNumBits)
		return false;
	if (length == 0)
		return true;

	uint32 startIndex = start >> 5;
	uint32 startBit = start & 0x1F;
	uint32 remainingBits = (length + startBit) & 0x1F;

	uint32 iterations;
       
	if (length < 32) {
		if (startBit + length < 32) {
			uint32 bits = B_LENDIAN_TO_HOST_INT32(data[startIndex]);

			uint32 mask = (1 << (startBit + length)) - 1;
			mask &= ~((1 << startBit) - 1);
		       
			return (bits & mask) == (marked ? mask : 0);
		} else
			iterations = 0;
	} else
		iterations = (length - 32 + startBit) >> 5;

	uint32 index = startIndex;
	
	if (startBit != 0) {
		uint32 mask = ~((1 << startBit) - 1);
		uint32 bits = B_LENDIAN_TO_HOST_INT32(data[index]);

		if ((bits & mask) != (marked ? mask : 0)) {
			TRACE("BitmapBlock::_Check(): start %lx mask %lx\n", bits, mask);
			return false;
		}

		index += 1;
	} else
		iterations++;

	for (; iterations > 0; --iterations) {
		if (data[index++] != (marked ? 0xFFFFFFFF : 0)) {
			TRACE("BitmapBlock::_Check(): iterations %lu bits: %lX\n", iterations,
				data[index - 1]);
			return false;
		}
	}

	if (remainingBits != 0) {
		uint32 mask = (1 << remainingBits) - 1;

		uint32 bits = B_LENDIAN_TO_HOST_INT32(data[index]);
		if ((bits & mask) != (marked ? mask : 0)) {
			TRACE("BitmapBlock::_Check(): remainingBits %ld remaining %lX mask %lX\n",
				remainingBits, bits, mask);
			return false;
		}
	}

	return true;
}


bool
BitmapBlock::_Update(uint32 start, uint32 length, bool mark, bool force)
{
	TRACE("BitmapBlock::_Update(%lu, %lu, %c, %c)\n", start, length,
		unmark ? 't' : 'f', force ? 't' : 'f');

	if (fData == NULL || start + length > fNumBits)
		return false;

	uint32 startIndex = start >> 5;
	uint32 startBit = start & 0x1F;
	uint32 remainingBits = (length + startBit) & 0x1F;

	TRACE("BitmapBlock::_Update(): start index: %lu, start bit: %lu, remaining "
		"bits: %lu)\n", startIndex, startBit, remainingBits);
	uint32 iterations;
       
	if (length < 32) {
		if (startBit + length < 32) {
			uint32 bits = B_LENDIAN_TO_HOST_INT32(fData[startIndex]);
			TRACE("BitmapBlock::_Update(): bits: %lx\n", bits);

			uint32 mask = (1 << (startBit + length)) - 1;
			mask &= ~((1 << startBit) - 1);
		       
			TRACE("BitmapBlock::_Update(): mask: %lx\n", mask);

			if ((bits & mask) != (mark ? 0 : mask)) {
				ERROR("BitmapBlock::_Update() Marking failed bits %lx "
					"startBit %ld\n", bits, startBit);
				return false;
			}
		       
			if (mark)
			    bits |= mask;
			else
			    bits &= ~mask;
		       
			TRACE("BitmapBlock::_Update(): updated bits: %lx\n", bits);
			fData[startIndex] = B_HOST_TO_LENDIAN_INT32(bits);
		       
			return true;
		} else
			iterations = 0;
	} else
		iterations = (length - 32 + startBit) >> 5;

	TRACE("BitmapBlock::_Update(): iterations: %lu\n", iterations);
	uint32 index = startIndex;
	
	if (startBit != 0) {
		uint32 mask = ~((1 << startBit) - 1);
		uint32 bits = B_LENDIAN_TO_HOST_INT32(fData[index]);

		TRACE("BitmapBlock::_Update(): mask: %lx, bits: %lx\n", mask, bits);

		if (!force && (bits & mask) != (mark ? 0 : mask))
			return false;

		if (mark)
			bits |= mask;
		else
			bits &= ~mask;
		fData[index] = B_HOST_TO_LENDIAN_INT32(bits);

		TRACE("BitmapBlock::_Update(): updated bits: %lx\n", bits);
		index += 1;
	} else
		iterations++;

	for (; iterations > 0; --iterations) {
		if (!force && fData[index] != (mark ? 0 : 0xFFFFFFFF)) {
			ERROR("BitmapBlock::_Update() Marking failed "
				"index %ld, iterations %ld\n", index, iterations);
			return false;
		}
		fData[index++] = (mark ? 0xFFFFFFFF : 0);
	}

	TRACE("BitmapBlock::_Update(): Finished iterations\n");

	if (remainingBits != 0) {
		uint32 mask = (1 << remainingBits) - 1;
		uint32 bits = B_LENDIAN_TO_HOST_INT32(fData[index]);

		TRACE("BitmapBlock::_Update(): mask: %lx, bits: %lx\n", mask, bits);

		if (!force && (bits & mask) != (mark ? 0 : mask)) {
			ERROR("BitmapBlock::_Update() Marking failed remaining\n");
			return false;
		}

		if (mark)
			bits |= mask;
		else
			bits &= ~mask;
		fData[index] = B_HOST_TO_LENDIAN_INT32(bits);

		TRACE("BitmapBlock::_Update(): updated bits: %lx\n", bits);
	}

	return true;
}


void
BitmapBlock::_FindNext(uint32& pos, bool marked)
{
	TRACE("BitmapBlock::_FindNext(): pos: %lu\n", pos);

	const uint32* data = fData == NULL ? fReadOnlyData : fData;
	if (data == NULL)
		return;

	if (pos >= fNumBits) {
		pos = fNumBits;
		return;
	}

	uint32 index = pos >> 5;
	uint32 bit = pos & 0x1F;
	uint32 maxBit = 32;

	uint32 mask = ~((1 << bit) - 1);
	uint32 bits = B_LENDIAN_TO_HOST_INT32(data[index]);

	TRACE("BitmapBlock::_FindNext(): index: %lu, bit: %lu, mask: %lX, "
		"bits: %lX\n", index, bit, mask, bits);

	bits &= mask;
	if (bits == (marked ? 0 : mask) && index < fMaxIndex) {
		// Find a 32 bits block that has a marked bit
		do {
			index++;
		} while (index < fMaxIndex && data[index] == (marked ? 0 : 0xFFFFFFFF));
		bit = 0;
		mask = 0xffffffff;
	}

	if (index >= fMaxIndex) {
		maxBit = fNumBits & 0x1F;

		if (maxBit == 0) {
			// Not found
			TRACE("BitmapBlock::_FindNext(): reached end of block, "
				"num bits: %lu\n", fNumBits);
			pos = fNumBits;
			return;
		}
		bits = B_LENDIAN_TO_HOST_INT32(data[fMaxIndex]);
		mask &= (1 << maxBit) - 1;
		if ((bits & mask) == (marked ? 0 : mask)) {
			TRACE("BitmapBlock::_FindNext(): reached end of block, "
				"num bits: %lu\n", fNumBits);
			pos = fNumBits;
			return;
		}
		maxBit++;
	} else
		bits = B_LENDIAN_TO_HOST_INT32(data[index]);

	for (; bit < maxBit; ++bit) {
		// Find the marked bit
		if ((bits >> bit & 1) != (marked ? 0U : 1U)) {
			pos = index << 5 | bit;
			TRACE("BitmapBlock::_FindNext(): found bit: %lu\n", pos);
			return;
		}
	}

	panic("Couldn't find bit inside an uint32 (%lx)\n", bits);
}


void
BitmapBlock::FindPreviousMarked(uint32& pos)
{
	TRACE("BitmapBlock::FindPreviousMarked(%lu)\n", pos);
	const uint32* data = fData == NULL ? fReadOnlyData : fData;
	if (data == NULL)
		return;

	if (pos >= fNumBits)
		pos = fNumBits;

	if (pos == 0)
		return;

	uint32 index = pos >> 5;
	int32 bit = pos & 0x1F;

	uint32 mask = (1 << bit) - 1;
	uint32 bits = B_LENDIAN_TO_HOST_INT32(data[index]);
	bits = bits & mask;

	TRACE("BitmapBlock::FindPreviousMarked(): index: %lu bit: %lu bits: %lx\n",
		index, bit, bits);

	if (bits == 0) {
		// Find an block of 32 bits that has a marked bit
		do {
			index--;
		} while (data[index] == 0 && index > 0);

		bits = B_LENDIAN_TO_HOST_INT32(data[index]);
		if (bits == 0) {
			// Not found
			pos = 0;
			return;
		}

		bit = 31;
	}

	TRACE("BitmapBlock::FindPreviousMarked(): index: %lu bit: %lu bits: %lx\n",
		index, bit, bits);

	for (; bit >= 0; --bit) {
		// Find the marked bit
		if ((bits >> bit & 1) != 0) {
			pos = index << 5 | bit;
			return;
		}
	}

	panic("Couldn't find marked bit inside an int32 with value different than "
		"zero!?\n");
}


void
BitmapBlock::FindLargestUnmarkedRange(uint32& start, uint32& length)
{
	const uint32* data = fData == NULL ? fReadOnlyData : fData;
	if (data == NULL)
		return;

	uint32 wordSpan = length >> 5;
	uint32 startIndex = 0;
	uint32 index = 0;
	uint32 bits = B_LENDIAN_TO_HOST_INT32(data[0]);

	TRACE("BitmapBlock::FindLargestUnmarkedRange(): word span: %lu, last "
		"index: %lu, start index: %lu, index: %lu, bits: %lX, start: %lu, "
		"length: %lu\n", wordSpan, fMaxIndex, startIndex, index, bits, start,
		length);

	if (wordSpan == 0) {
		uint32 startPos = 0;
		uint32 endPos = 0;

		while (endPos < fNumBits) {
			FindNextUnmarked(startPos);
			endPos = startPos;

			if (startPos != fNumBits) {
				FindNextMarked(endPos);

				uint32 newLength = endPos - startPos;

				if (newLength > length) {
					start = startPos;
					length = newLength;
					TRACE("BitmapBlock::FindLargestUnmarkedRange(): Found "
						"larger length %lu starting at %lu\n", length, start);
				} 

				startPos = endPos;

				if (newLength >= 32)
					break;
			}
		}
		
		if (endPos >= fNumBits)
			return;

		wordSpan = length >> 5;
		startIndex = startPos >> 5;
		index = (endPos >> 5) + 1;
		bits = B_LENDIAN_TO_HOST_INT32(data[index]);
	}

	for (; index < fMaxIndex; ++index) {
		bits = B_LENDIAN_TO_HOST_INT32(data[index]);

		if (bits != 0) {
			// Contains marked bits
			if (index - startIndex >= wordSpan) {
				uint32 newLength = (index - startIndex - 1) << 5;
				uint32 newStart = (startIndex + 1) << 5;

				uint32 startBits =
					B_LENDIAN_TO_HOST_INT32(data[startIndex]);

				for (int32 bit = 31; bit >= 0; --bit) {
					if ((startBits >> bit & 1) != 0)
						break;

					++newLength;
					--newStart;
				}

				for (int32 bit = 0; bit < 32; ++bit) {
					if ((bits >> bit & 1) != 0)
						break;

					++newLength;
				}

				if (newLength > length) {
					start = newStart;
					length = newLength;
					wordSpan = length >> 5;
					
					TRACE("BitmapBlock::FindLargestUnmarkedRange(): Found "
						"larger length %lu starting at %lu; word span: "
						"%lu\n", length, start, wordSpan);
				}
			}

			startIndex = index;
		}
	}
	
	--index;

	if (index - startIndex >= wordSpan) {
		uint32 newLength = (index - startIndex) << 5;
		uint32 newStart = (startIndex + 1) << 5;
		
		TRACE("BitmapBlock::FindLargestUnmarkedRange(): Possibly found a "
			"larger range. index: %lu, start index: %lu, word span: %lu, "
			"new length: %lu, new start: %lu\n", index, startIndex, wordSpan,
			newLength, newStart);

		if (newStart != 0) {
			uint32 startBits = B_LENDIAN_TO_HOST_INT32(data[startIndex]);
			
			TRACE("BitmapBlock::FindLargestUnmarkedRange(): start bits: %lu\n",
				startBits);

			for (int32 bit = 31; bit >= 0; --bit) {
				if ((startBits >> bit & 1) != 0)
					break;

				++newLength;
				--newStart;
			}
			
			TRACE("BitmapBlock::FindLargestUnmarkedRange(): updated new start "
				"to %lu and new length to %lu\n", newStart, newLength);
		}

		for (int32 bit = 0; bit < 32; ++bit) {
			if ((bits >> bit & 1) == 0)
				break;

			++newLength;
		}
		
		TRACE("BitmapBlock::FindLargestUnmarkedRange(): updated new length to "
			"%lu\n", newLength);

		if (newLength > length) {
			start = newStart;
			length = newLength;
			TRACE("BitmapBlock::FindLargestUnmarkedRange(): Found "
				"largest length %lu starting at %lu\n", length, start);
		}
	}
}

