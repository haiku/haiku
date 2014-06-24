/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Uuid.h>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


static const char* const kHexChars = "0123456789abcdef";

static const size_t kVersionByteIndex = 6;
static const size_t kVariantByteIndex = 8;


static bool
init_random_seed()
{
	// set a time-based seed
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	uint32 seed = (uint32)time.tv_sec ^ (uint32)time.tv_nsec;

	// factor in a stack address -- with address space layout randomization
	// that adds a bit of additional randomness
	seed ^= (uint32)(addr_t)&time;

	srandom(seed);

	return true;
}


namespace BPrivate {

BUuid::BUuid()
{
	memset(fValue, 0, sizeof(fValue));
}


BUuid::BUuid(const BUuid& other)
{
	memcpy(fValue, other.fValue, sizeof(fValue));
}


BUuid::~BUuid()
{
}


bool
BUuid::IsNil() const
{
	for (size_t i = 0; i < sizeof(fValue); i++) {
		if (fValue[i] != 0)
			return false;
	}

	return true;
}


BUuid&
BUuid::SetToRandom()
{
	if (!BUuid::_SetToDevRandom())
		BUuid::_SetToRandomFallback();

	// set variant and version
	fValue[kVariantByteIndex] &= 0x3f;
	fValue[kVariantByteIndex] |= 0x80;
	fValue[kVersionByteIndex] &= 0x0f;
	fValue[kVersionByteIndex] |= 4 << 4;
		// version 4

	return *this;
}


BString
BUuid::ToString() const
{
	char buffer[32];
	for (size_t i = 0; i < 16; i++) {
		buffer[2 * i] = kHexChars[fValue[i] >> 4];
		buffer[2 * i + 1] = kHexChars[fValue[i] & 0xf];
	}

	return BString().SetToFormat("%.8s-%.4s-%.4s-%.4s-%.12s",
		buffer, buffer + 8, buffer + 12, buffer + 16, buffer + 20);
}


int
BUuid::Compare(const BUuid& other) const
{
	return memcmp(fValue, other.fValue, sizeof(fValue));
}


BUuid&
BUuid::operator=(const BUuid& other)
{
	memcpy(fValue, other.fValue, sizeof(fValue));

	return *this;
}


bool
BUuid::_SetToDevRandom()
{
	// open device
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		fd = open("/dev/random", O_RDONLY);
		if (fd < 0)
			return false;
	}

	// read bytes
	ssize_t bytesRead = read(fd, fValue, sizeof(fValue));
	close(fd);

	return bytesRead == (ssize_t)sizeof(fValue);
}


void
BUuid::_SetToRandomFallback()
{
	static bool sSeedInitialized = init_random_seed();
	(void)sSeedInitialized;

	for (int32 i = 0; i < 4; i++) {
		uint32 value = random();
		fValue[4 * i + 0] = uint8(value >> 24);
		fValue[4 * i + 1] = uint8(value >> 16);
		fValue[4 * i + 2] = uint8(value >> 8);
		fValue[4 * i + 3] = uint8(value);
	}

	// random() returns 31 bit numbers only, so we move a few bits from where
	// we overwrite them with the version anyway.
	uint8 bitsToMove = fValue[kVersionByteIndex];
	for (int32 i = 0; i < 4; i++)
		fValue[4 * i] |= (bitsToMove << i) & 0x80;
}

}	// namespace BPrivate


using BPrivate::BUuid;
