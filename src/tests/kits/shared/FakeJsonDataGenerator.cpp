/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "FakeJsonDataGenerator.h"

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>


static const char* kTextData = "abcdefghijklmnopqrstuvwxyz!@#$"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ)(*&"
	"0123456789{}[]|:;'<>,.?/~`_+-="
	"abcdefghijklmnopqrstuvwxyz!@#$"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ)(*&"
	"0123456789{}[]|:;'<>,.?/~`_+-=";
static const int kTextDataLength = 180;


FakeJsonStreamDataIO::FakeJsonStreamDataIO(int count, uint32 checksumLimit)
	:
	fFeedOutState(OPEN_ARRAY),
	fItemCount(count),
	fItemUpto(0),
	fChecksum(0),
	fChecksumLimit(checksumLimit)
{
}


FakeJsonStreamDataIO::~FakeJsonStreamDataIO()
{
}


ssize_t
FakeJsonStreamDataIO::Read(void* buffer, size_t size)
{
	char* buffer_c = static_cast<char*>(buffer);
	status_t result = B_OK;
	size_t i = 0;

	while (i < size && result == B_OK) {
		result = NextChar(&buffer_c[i]);
		if (result == B_OK)
			i++;
	}

	if(0 != i)
		return i;

	return result;
}


ssize_t
FakeJsonStreamDataIO::Write(const void* buffer, size_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
FakeJsonStreamDataIO::Flush()
{
	return B_OK;
}


uint32
FakeJsonStreamDataIO::Checksum() const
{
	return fChecksum;
}


void
FakeJsonStreamDataIO::_ChecksumProcessCharacter(const char c)
{
	fChecksum = (fChecksum + static_cast<int32>(c)) % fChecksumLimit;
}


// #pragma mark - FakeJsonStringStreamDataIO


FakeJsonStringStreamDataIO::FakeJsonStringStreamDataIO(int count, uint32 checksumLimit)
	:
	FakeJsonStreamDataIO(count, checksumLimit),
	fItemBufferSize(0),
	fItemBufferUpto(0)
{
	FillBuffer();
}


FakeJsonStringStreamDataIO::~FakeJsonStringStreamDataIO()
{
}


status_t
FakeJsonStringStreamDataIO::NextChar(char* c)
{
	switch (fFeedOutState) {
		case OPEN_ARRAY:
			c[0] = '[';
			fFeedOutState = OPEN_QUOTE;
			return B_OK;
		case OPEN_QUOTE:
			c[0] = '"';
			fFeedOutState = ITEM;
			return B_OK;
		case ITEM:
			c[0] = kTextData[fItemBufferUpto];
			_ChecksumProcessCharacter(kTextData[fItemBufferUpto]);
			fItemBufferUpto++;
			if (fItemBufferUpto >= fItemBufferSize) {
				fFeedOutState = CLOSE_QUOTE;
				FillBuffer();
			}
			return B_OK;
		case CLOSE_QUOTE:
			c[0] = '"';
			fItemUpto++;
			if (fItemUpto >= fItemCount)
				fFeedOutState = CLOSE_ARRAY;
			else
				fFeedOutState = SEPARATOR;
			return B_OK;
		case SEPARATOR:
			c[0] = ',';
			fFeedOutState = OPEN_QUOTE;
			return B_OK;
		case CLOSE_ARRAY:
			c[0] = ']';
			fFeedOutState = END;
			return B_OK;
		default:
			return -1; // end of file
	}
}


void
FakeJsonStringStreamDataIO::FillBuffer()
{
	fItemBufferSize = random() % kTextDataLength;
	fItemBufferUpto = 0;
}


// #pragma mark - FakeJsonStringStreamDataIO


FakeJsonNumberStreamDataIO::FakeJsonNumberStreamDataIO(int count, uint32 checksumLimit)
	:
	FakeJsonStreamDataIO(count, checksumLimit),
	fItemBufferSize(0),
	fItemBufferUpto(0)
{
	bzero(fBuffer, 32);
	FillBuffer();
}


FakeJsonNumberStreamDataIO::~FakeJsonNumberStreamDataIO()
{
}


status_t
FakeJsonNumberStreamDataIO::NextChar(char* c)
{
	switch (fFeedOutState) {
		case OPEN_ARRAY:
			c[0] = '[';
			fFeedOutState = ITEM;
			return B_OK;
		case ITEM:
			c[0] = fBuffer[fItemBufferUpto];
			_ChecksumProcessCharacter(fBuffer[fItemBufferUpto]);
			fItemBufferUpto++;
			if (fItemBufferUpto >= fItemBufferSize) {
				fItemUpto++;
				if (fItemUpto >= fItemCount)
					fFeedOutState = CLOSE_ARRAY;
				else
					fFeedOutState = SEPARATOR;
				FillBuffer();
			}
			return B_OK;
		case SEPARATOR:
			c[0] = ',';
			fFeedOutState = ITEM;
			return B_OK;
		case CLOSE_ARRAY:
			c[0] = ']';
			fFeedOutState = END;
			return B_OK;
		default:
			return -1; // end of file
	}
}


void
FakeJsonNumberStreamDataIO::FillBuffer()
{
	int32 value = static_cast<int32>(random());
	fItemBufferSize = snprintf(fBuffer, 32, "%" B_PRIu32, value);
	fItemBufferUpto = 0;
}