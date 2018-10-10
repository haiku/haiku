/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 *		Jérôme Duval
 */


#include "BitmapBlock.h"

#include "CRCTable.h"


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
	fVolume(volume),
	fData(NULL),
	fReadOnlyData(NULL),
	fNumBits(numBits),
	fMaxIndex(fNumBits >> 5)
{
	TRACE("BitmapBlock::BitmapBlock(): num bits: %" B_PRIu32 "\n", fNumBits);
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
			TRACE("BitmapBlock::_Check(): start %" B_PRIx32 " mask %" B_PRIx32
				"\n", bits, mask);
			return false;
		}

		index += 1;
	} else
		iterations++;

	for (; iterations > 0; --iterations) {
		if (data[index++] != (marked ? 0xFFFFFFFF : 0)) {
			TRACE("BitmapBlock::_Check(): iterations %" B_PRIu32 " bits: %"
				B_PRIx32 "\n", iterations, data[index - 1]);
			return false;
		}
	}

	if (remainingBits != 0) {
		uint32 mask = (1 << remainingBits) - 1;

		uint32 bits = B_LENDIAN_TO_HOST_INT32(data[index]);
		if ((bits & mask) != (marked ? mask : 0)) {
			TRACE("BitmapBlock::_Check(): remainingBits %" B_PRIu32
				" remaining %" B_PRIx32 " mask %" B_PRIx32 "\n", remainingBits,
				bits, mask);
			return false;
		}
	}

	return true;
}


bool
BitmapBlock::_Update(uint32 start, uint32 length, bool mark, bool force)
{
	TRACE("BitmapBlock::_Update(%" B_PRIu32 ", %" B_PRIu32 ", %c, %c)\n",
		start, length, mark ? 't' : 'f', force ? 't' : 'f');

	if (fData == NULL || start + length > fNumBits)
		return false;

	uint32 startIndex = start >> 5;
	uint32 startBit = start & 0x1F;
	uint32 remainingBits = (length + startBit) & 0x1F;

	TRACE("BitmapBlock::_Update(): start index: %" B_PRIu32 ", start bit: %"
		B_PRIu32 ", remaining bits: %" B_PRIu32 ")\n", startIndex, startBit,
		remainingBits);
	uint32 iterations;
       
	if (length < 32) {
		if (startBit + length < 32) {
			uint32 bits = B_LENDIAN_TO_HOST_INT32(fData[startIndex]);
			TRACE("BitmapBlock::_Update(): bits: %" B_PRIx32 "\n", bits);

			uint32 mask = (1 << (startBit + length)) - 1;
			mask &= ~((1 << startBit) - 1);
		       
			TRACE("BitmapBlock::_Update(): mask: %" B_PRIx32 "\n", mask);

			if ((bits & mask) != (mark ? 0 : mask)) {
				ERROR("BitmapBlock::_Update() Marking failed bits %" B_PRIx32
					" startBit %" B_PRIu32 "\n", bits, startBit);
				return false;
			}
		       
			if (mark)
			    bits |= mask;
			else
			    bits &= ~mask;
		       
			TRACE("BitmapBlock::_Update(): updated bits: %" B_PRIx32 "\n",
				bits);
			fData[startIndex] = B_HOST_TO_LENDIAN_INT32(bits);
		       
			return true;
		} else
			iterations = 0;
	} else
		iterations = (length - 32 + startBit) >> 5;

	TRACE("BitmapBlock::_Update(): iterations: %" B_PRIu32 "\n", iterations);
	uint32 index = startIndex;
	
	if (startBit != 0) {
		uint32 mask = ~((1 << startBit) - 1);
		uint32 bits = B_LENDIAN_TO_HOST_INT32(fData[index]);

		TRACE("BitmapBlock::_Update(): mask: %" B_PRIx32 ", bits: %" B_PRIx32
			"\n", mask, bits);

		if (!force && (bits & mask) != (mark ? 0 : mask))
			return false;

		if (mark)
			bits |= mask;
		else
			bits &= ~mask;
		fData[index] = B_HOST_TO_LENDIAN_INT32(bits);

		TRACE("BitmapBlock::_Update(): updated bits: %" B_PRIx32 "\n", bits);
		index += 1;
	} else
		iterations++;

	for (; iterations > 0; --iterations) {
		if (!force && fData[index] != (mark ? 0 : 0xFFFFFFFF)) {
			ERROR("BitmapBlock::_Update() Marking failed "
				"index %" B_PRIu32 ", iterations %" B_PRId32 "\n", index,
				iterations);
			return false;
		}
		fData[index++] = (mark ? 0xFFFFFFFF : 0);
	}

	TRACE("BitmapBlock::_Update(): Finished iterations\n");

	if (remainingBits != 0) {
		uint32 mask = (1 << remainingBits) - 1;
		uint32 bits = B_LENDIAN_TO_HOST_INT32(fData[index]);

		TRACE("BitmapBlock::_Update(): mask: %" B_PRIx32 ", bits: %" B_PRIx32
			"\n", mask, bits);

		if (!force && (bits & mask) != (mark ? 0 : mask)) {
			ERROR("BitmapBlock::_Update() Marking failed remaining\n");
			return false;
		}

		if (mark)
			bits |= mask;
		else
			bits &= ~mask;
		fData[index] = B_HOST_TO_LENDIAN_INT32(bits);

		TRACE("BitmapBlock::_Update(): updated bits: %" B_PRIx32 "\n", bits);
	}

	return true;
}


void
BitmapBlock::_FindNext(uint32& pos, bool marked)
{
	TRACE("BitmapBlock::_FindNext(): pos: %" B_PRIu32 "\n", pos);

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

	TRACE("BitmapBlock::_FindNext(): index: %" B_PRIu32 ", bit: %" B_PRIu32
		", mask: %" B_PRIx32 ", bits: %" B_PRIx32 "\n", index, bit, mask,
		bits);

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
				"num bits: %" B_PRIu32 "\n", fNumBits);
			pos = fNumBits;
			return;
		}
		bits = B_LENDIAN_TO_HOST_INT32(data[fMaxIndex]);
		mask &= (1 << maxBit) - 1;
		if ((bits & mask) == (marked ? 0 : mask)) {
			TRACE("BitmapBlock::_FindNext(): reached end of block, "
				"num bits: %" B_PRIu32 "\n", fNumBits);
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
			TRACE("BitmapBlock::_FindNext(): found bit: %" B_PRIu32 "\n", pos);
			return;
		}
	}

	panic("Couldn't find bit inside an uint32 (%" B_PRIx32 ")\n", bits);
}


void
BitmapBlock::FindPreviousMarked(uint32& pos)
{
	TRACE("BitmapBlock::FindPreviousMarked(%" B_PRIu32 ")\n", pos);
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

	TRACE("BitmapBlock::FindPreviousMarked(): index: %" B_PRIu32 " bit: %"
		B_PRIu32 " bits: %" B_PRIx32 "\n", index, bit, bits);

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

	TRACE("BitmapBlock::FindPreviousMarked(): index: %" B_PRIu32 " bit: %"
		B_PRIu32 " bits: %" B_PRIx32 "\n", index, bit, bits);

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

	TRACE("BitmapBlock::FindLargestUnmarkedRange(): word span: %" B_PRIu32
		", last index: %" B_PRIu32 ", start index: %" B_PRIu32 ", index: %"
		B_PRIu32 ", bits: %" B_PRIx32 ", start: %" B_PRIu32 ", length: %"
		B_PRIu32 "\n", wordSpan, fMaxIndex, startIndex, index, bits, start,
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
						"larger length %" B_PRIu32 " starting at %" B_PRIu32
						"\n", length, start);
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
						"larger length %" B_PRIu32 " starting at %" B_PRIu32
						"; word span: %" B_PRIu32 "\n", length, start,
						wordSpan);
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
			"larger range. index: %" B_PRIu32 ", start index: %" B_PRIu32
			", word span: %" B_PRIu32 ", new length: %" B_PRIu32
			", new start: %" B_PRIu32 "\n", index, startIndex, wordSpan,
			newLength, newStart);

		if (newStart != 0) {
			uint32 startBits = B_LENDIAN_TO_HOST_INT32(data[startIndex]);
			
			TRACE("BitmapBlock::FindLargestUnmarkedRange(): start bits: %"
				B_PRIu32 "\n", startBits);

			for (int32 bit = 31; bit >= 0; --bit) {
				if ((startBits >> bit & 1) != 0)
					break;

				++newLength;
				--newStart;
			}
			
			TRACE("BitmapBlock::FindLargestUnmarkedRange(): updated new start "
				"to %" B_PRIu32 " and new length to %" B_PRIu32 "\n", newStart,
				newLength);
		}

		for (int32 bit = 0; bit < 32; ++bit) {
			if ((bits >> bit & 1) == 0)
				break;

			++newLength;
		}
		
		TRACE("BitmapBlock::FindLargestUnmarkedRange(): updated new length to "
			"%" B_PRIu32 "\n", newLength);

		if (newLength > length) {
			start = newStart;
			length = newLength;
			TRACE("BitmapBlock::FindLargestUnmarkedRange(): Found "
				"largest length %" B_PRIu32 " starting at %" B_PRIu32 "\n",
				length, start);
		}
	}
}


uint32
BitmapBlock::Checksum(uint32 unitsPerGroup) const
{
	const uint32* data = fData == NULL ? fReadOnlyData : fData;
	if (data == NULL)
		panic("BitmapBlock::Checksum() data is NULL\n");
	return calculate_crc32c(fVolume->ChecksumSeed(),
		(uint8*)data, unitsPerGroup / 8);
}
