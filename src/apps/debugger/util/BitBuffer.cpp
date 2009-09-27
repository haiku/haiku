/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "BitBuffer.h"


// #pragma mark - BitReader


struct BitBuffer::BitReader {
	const uint8*	data;
	uint64			bitSize;
	uint32			bitOffset;

	BitReader(const uint8* data, uint64 bitSize, uint32 bitOffset)
		:
		data(data),
		bitSize(bitSize),
		bitOffset(bitOffset)
	{
	}

	uint8 ReadByte()
	{
		uint8 byte = *data;
		data++;
		bitSize -= 8;

		if (bitOffset == 0)
			return byte;

		return (byte << bitOffset) | (*data >> (8 - bitOffset));
	}

	uint8 ReadBits(uint32 count)
	{
		uint8 byte = *data;
		bitSize -= count;
		bitOffset += count;

		if (bitOffset <= 8) {
			if (bitOffset == 8) {
				bitOffset = 0;
				data++;
				return byte & ((1 << count) - 1);
			}

			return (byte >> (8 - bitOffset)) & ((1 << count) - 1);
		}

		data++;
		bitOffset -= 8;
		return ((byte << bitOffset) | (*data >> (8 - bitOffset)))
			& ((1 << count) - 1);
	}
};


// #pragma mark - BitBuffer


BitBuffer::BitBuffer()
	:
	fMissingBits(0)
{
}


BitBuffer::~BitBuffer()
{
}


bool
BitBuffer::AddBytes(const void* data, size_t size)
{
	if (size == 0)
		return true;

	if (fMissingBits == 0) {
		size_t oldSize = fBytes.Size();
		if (!fBytes.AddUninitialized(size))
			return false;

		memcpy(fBytes.Elements() + oldSize, data, size);
		return true;
	}

	return AddBits(data, (uint64)size * 8, 0);
}


bool
BitBuffer::AddBits(const void* _data, uint64 bitSize, uint32 bitOffset)
{
	if (bitSize == 0)
		return true;

	const uint8* data = (const uint8*)_data + bitOffset / 8;
	bitOffset %= 8;

	BitReader reader(data, bitSize, bitOffset);

	// handle special case first: no more bits than missing
	size_t oldSize = fBytes.Size();
	if (fMissingBits > 0 && bitSize <= fMissingBits) {
		fMissingBits -= bitSize;
		uint8 bits = reader.ReadBits(bitSize) << fMissingBits;
		fBytes[oldSize - 1] |= bits;
		return true;
	}

	// resize the buffer
	if (!fBytes.AddUninitialized((reader.bitSize - fMissingBits + 7) / 8))
		return false;

	// fill in missing bits
	if (fMissingBits > 0) {
		fBytes[oldSize - 1] |= reader.ReadBits(fMissingBits);
		fMissingBits = 0;
	}

	// read full bytes as long as we can
	uint8* buffer = fBytes.Elements() + oldSize;
	while (reader.bitSize >= 8) {
		*buffer = reader.ReadByte();
		buffer++;
	}

	// If we have left-over bits, write a partial byte.
	if (reader.bitSize > 0) {
		fMissingBits = 8 - reader.bitSize;
		*buffer = reader.ReadBits(reader.bitSize) << fMissingBits;
	}

	return true;
}


bool
BitBuffer::AddZeroBits(uint64 bitSize)
{
	if (bitSize == 0)
		return true;

	// handle special case first: no more bits than missing
	size_t oldSize = fBytes.Size();
	if (fMissingBits > 0 && bitSize <= fMissingBits) {
		fMissingBits -= bitSize;
		return true;
	}

	// resize the buffer
	if (!fBytes.AddUninitialized((bitSize - fMissingBits + 7) / 8))
		return false;

	// fill in missing bits
	if (fMissingBits > 0) {
		bitSize -= fMissingBits;
		fMissingBits = 0;
	}

	// zero the remaining bytes, including a potentially partial last byte
	uint8* buffer = fBytes.Elements() + oldSize;
	memset(buffer, 0, (bitSize + 7) / 8);
	bitSize %= 8;

	if (bitSize > 0)
		fMissingBits = 8 - bitSize;

	return true;
}
