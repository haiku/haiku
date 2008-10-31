/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "SHA256.h"

#include <stdio.h>
#include <string.h>

#include <ByteOrder.h>


static const uint32 kChunkSize = 64;	// 64 bytes == 512 bits

static const uint32 kRounds[64] = {
   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
   0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
   0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
   0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
   0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
   0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
   0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
   0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};
static const uint32 kHash[8] = {
	0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
	0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};


static inline uint32
rotate_right(uint32 value, int bits)
{
	return (value >> bits) | (value << (32 - bits));
}


//	#pragma mark -


SHA256::SHA256()
{
	Init();
}


SHA256::~SHA256()
{
}


void
SHA256::Init()
{
	memcpy(fHash, kHash, sizeof(kHash));

	fBytesInBuffer = 0;
	fMessageSize = 0;
	fDigested = false;
}


void
SHA256::Update(const void* _buffer, size_t size)
{
	const uint8* buffer = (const uint8*)_buffer;
	fMessageSize += size;

	while (fBytesInBuffer + size >= kChunkSize) {
		size_t toCopy = kChunkSize - fBytesInBuffer;
		memcpy((uint8*)fBuffer + fBytesInBuffer, buffer, toCopy);
		buffer += toCopy;
		size -= toCopy;

		_ProcessChunk();
		fBytesInBuffer = 0;
	}

	if (size > 0) {
		memcpy((uint8*)fBuffer + fBytesInBuffer, buffer, size);
		fBytesInBuffer += size;
	}
}


const uint8*
SHA256::Digest()
{
	if (!fDigested) {
		// We need to append a 1 bit, append padding with 0 bits, and append
		// the message size in bits (64 bit big-endian int), so that the whole
		// is chunk-aligned. So we either have to process one last chunk or two
		// chunks.

		// append the 1 bit
		((uint8*)fBuffer)[fBytesInBuffer] = 0x80;
		fBytesInBuffer++;

		// if the message size doesn't fit anymore, we pad the chunk and
		// process it
		if (fBytesInBuffer > kChunkSize - 8) {
			memset((uint8*)fBuffer + fBytesInBuffer, 0,
				kChunkSize - fBytesInBuffer);
			_ProcessChunk();
			fBytesInBuffer = 0;
		}

		// pad the buffer
		if (fBytesInBuffer < kChunkSize - 8) {
			memset((uint8*)fBuffer + fBytesInBuffer, 0,
				kChunkSize - 8 - fBytesInBuffer);
		}

		// write the (big-endian) message size in bits
		*(uint64*)((uint8*)fBuffer + kChunkSize - 8)
			= B_HOST_TO_BENDIAN_INT64((uint64)fMessageSize * 8);

		_ProcessChunk();

		// set digest
		for (int i = 0; i < 8; i++)
			fDigest[i] = B_HOST_TO_BENDIAN_INT32(fHash[i]);

		fDigested = true;
	}

	return (uint8*)fDigest;
}


void
SHA256::_ProcessChunk()
{
	// convert endianess -- the data are supposed to be a stream of
	// 32 bit big-endian integers
	#if B_HOST_IS_LENDIAN
		for (int i = 0; i < (int)kChunkSize / 4; i++)
			fBuffer[i] = B_SWAP_INT32(fBuffer[i]);
	#endif

	// pre-process buffer (extend to 64 elements)
	for (int i = 16; i < 64; i++) {
		uint32 v0 = fBuffer[i - 15];
		uint32 v1 = fBuffer[i - 2];
		uint32 s0 = rotate_right(v0, 7) ^ rotate_right(v0, 18) ^ (v0 >> 3);
		uint32 s1 = rotate_right(v1, 17) ^ rotate_right(v1, 19) ^ (v1 >> 10);
		fBuffer[i] = fBuffer[i - 16] + s0 + fBuffer[i - 7] + s1;
	}

	uint32 a = fHash[0];
	uint32 b = fHash[1];
	uint32 c = fHash[2];
	uint32 d = fHash[3];
	uint32 e = fHash[4];
	uint32 f = fHash[5];
	uint32 g = fHash[6];
	uint32 h = fHash[7];

	// process the buffer
	for (int i = 0; i < 64; i++) {
		uint32 s0 = rotate_right(a, 2) ^ rotate_right(a, 13)
			^ rotate_right(a, 22);
		uint32 maj = (a & b) ^ (a & c) ^ (b & c);
		uint32 t2 = s0 + maj;
		uint32 s1 = rotate_right(e, 6) ^ rotate_right(e, 11)
			^ rotate_right(e, 25);
		uint32 ch = (e & f) ^ (~e & g);
		uint32 t1 = h + s1 + ch + kRounds[i] + fBuffer[i];

		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	fHash[0] += a;
	fHash[1] += b;
	fHash[2] += c;
	fHash[3] += d;
	fHash[4] += e;
	fHash[5] += f;
	fHash[6] += g;
	fHash[7] += h;
}
