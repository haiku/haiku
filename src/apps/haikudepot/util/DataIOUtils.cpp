/*
 * Copyright 2018-2023, Andrew Lindesay <apl@lindesay.co.nz>.
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


// #pragma mark - ConstraintedDataIO

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


// #pragma mark - Base64DecodingDataIO

Base64DecodingDataIO::Base64DecodingDataIO(BDataIO* delegate, char char62, char char63)
	:
	fDelegate(delegate),
	fChar62(char62),
	fChar63(char63),
	fNextByteAssembly(0),
    fNextByteAssemblyBits(0)
{
}


Base64DecodingDataIO::~Base64DecodingDataIO()
{
}


status_t
Base64DecodingDataIO::_CharToInt(uint8 ch, uint8* value)
{
	if (ch >= 0x41 && ch <= 0x5A) {
		*value = (ch - 0x41);
		return B_OK;
	}

	if (ch >= 0x61 && ch <= 0x7a) {
		*value = (ch - 0x61) + 26;
		return B_OK;
	}

	if (ch >= 0x30 && ch <= 0x39) {
		*value = (ch - 0x30) + 52;
		return B_OK;
	}

	if (ch == fChar62) {
		*value = 62;
		return B_OK;
	}

	if (ch == fChar63) {
		*value = 63;
		return B_OK;
	}

	if (ch == '=') {
		*value = 0;
		return B_OK;
	}

	return B_BAD_DATA;
}


status_t
Base64DecodingDataIO::_ReadSingleByte(void* buffer)
{
	uint8 delegateRead;
	uint8 delegateReadInt;
	status_t result = B_OK;

	if (result == B_OK)
		result = fDelegate->ReadExactly(&delegateRead, 1);

	if (result == B_OK)
		result = _CharToInt(delegateRead, &delegateReadInt);

	if (result == B_OK && 0 == fNextByteAssemblyBits) {
		fNextByteAssembly = delegateReadInt;
		fNextByteAssemblyBits = 6;
		return _ReadSingleByte(buffer);
	}

	if (result == B_OK) {
		uint8 followingNextByteAssemblyBits = (6 - (8 - fNextByteAssemblyBits));
		*((uint8 *) buffer) = fNextByteAssembly << (8 - fNextByteAssemblyBits)
			| (delegateReadInt >> followingNextByteAssemblyBits);
		fNextByteAssembly = delegateReadInt & (0x3f >> (6 - followingNextByteAssemblyBits));
		fNextByteAssemblyBits = followingNextByteAssemblyBits;
	}

	return result;
}


ssize_t
Base64DecodingDataIO::Read(void* buffer, size_t size)
{
	size_t readSize = 0;
	status_t result = B_OK;

	while (result == B_OK && readSize < size) {
		result = _ReadSingleByte(&((uint8_t*) buffer)[readSize]);

		if (result == B_OK)
			readSize++;
	}

	return readSize++;
}


ssize_t
Base64DecodingDataIO::Write(const void* buffer, size_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
Base64DecodingDataIO::Flush()
{
	return B_OK;
}