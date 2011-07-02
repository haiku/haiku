/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BIT_BUFFER_H
#define BIT_BUFFER_H


#include <SupportDefs.h>

#include <Array.h>


class BitBuffer {
public:
								BitBuffer();
								~BitBuffer();

			bool				AddBytes(const void* data, size_t size);
			bool				AddBits(const void* data, uint64 bitSize,
									uint32 bitOffset = 0);
			bool				AddZeroBits(uint64 bitSize);

			uint8*				Bytes() const	{ return fBytes.Elements(); }
			size_t				Size() const	{ return fBytes.Size(); }
			size_t				BitSize() const
									{ return Size() * 8 - fMissingBits; }

private:
			struct BitReader;

private:
			Array<uint8>		fBytes;
			uint8				fMissingBits;
};


#endif	// BIT_BUFFER_H
