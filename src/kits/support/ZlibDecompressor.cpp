/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ZlibDecompressor.h>

#include <string.h>

#include <DataIO.h>

#include "ZlibCompressionBasePrivate.h"


// TODO: For the kernel the buffer shouldn't be allocated on the stack.
static const size_t kOutputBufferSize = 1024;


BZlibDecompressor::BZlibDecompressor(BDataIO* output)
	:
	BZlibCompressionBase(output),
	fFinished(false)
{
}


BZlibDecompressor::~BZlibDecompressor()
{
	if (fStream != NULL)
		inflateEnd(fStream);
}


status_t
BZlibDecompressor::Init()
{
	status_t error = CreateStream();
	if (error != B_OK)
		return error;

	int zlibError = inflateInit(fStream);
	if (zlibError != Z_OK) {
		DeleteStream();
		return TranslateZlibError(zlibError);
	}

	return B_OK;
}


status_t
BZlibDecompressor::DecompressNext(const void* input, size_t inputSize)
{
	fStream->next_in = (Bytef*)input;
	fStream->avail_in = inputSize;

	while (fStream->avail_in > 0) {
		if (fFinished)
			return B_BAD_DATA;

		uint8 outputBuffer[kOutputBufferSize];
		fStream->next_out = (Bytef*)outputBuffer;
		fStream->avail_out = sizeof(outputBuffer);

		int zlibError = inflate(fStream, 0);
		if (zlibError == Z_STREAM_END)
			fFinished = true;
		else if (zlibError != Z_OK)
			return TranslateZlibError(zlibError);

		if (fStream->avail_out < sizeof(outputBuffer)) {
			status_t error = fOutput->WriteExactly(outputBuffer,
				sizeof(outputBuffer) - fStream->avail_out);
			if (error != B_OK)
				return error;
		}
	}

	return B_OK;
}


status_t
BZlibDecompressor::Finish()
{
	fStream->next_in = (Bytef*)NULL;
	fStream->avail_in = 0;

	while (!fFinished) {
		uint8 outputBuffer[kOutputBufferSize];
		fStream->next_out = (Bytef*)outputBuffer;
		fStream->avail_out = sizeof(outputBuffer);

		int zlibError = inflate(fStream, Z_FINISH);
		if (zlibError == Z_STREAM_END)
			fFinished = true;
		else if (zlibError != Z_OK)
			return TranslateZlibError(zlibError);

		if (fStream->avail_out < sizeof(outputBuffer)) {
			status_t error = fOutput->WriteExactly(outputBuffer,
				sizeof(outputBuffer) - fStream->avail_out);
			if (error != B_OK)
				return error;
		}
	}

	inflateEnd(fStream);
	DeleteStream();

	return B_OK;
}


/*static*/ status_t
BZlibDecompressor::DecompressSingleBuffer(const void* input, size_t inputSize,
	void* output, size_t outputSize, size_t& _uncompressedSize)
{
	if (inputSize == 0 || outputSize == 0)
		return B_BAD_VALUE;

	// prepare stream
	z_stream zStream;
	memset(&zStream, 0, sizeof(zStream));
	zStream.next_in = (Bytef*)input;
	zStream.avail_in = uInt(inputSize);
	zStream.next_out = (Bytef*)output;
	zStream.avail_out = uInt(outputSize);

	int zlibError = inflateInit(&zStream);
	if (zlibError != Z_OK)
		return TranslateZlibError(zlibError);

	// inflate
	status_t error = B_OK;
	zlibError = inflate(&zStream, Z_FINISH);
	if (zlibError != Z_STREAM_END) {
		if (zlibError == Z_OK)
			error = B_BUFFER_OVERFLOW;
		else
			error = TranslateZlibError(zlibError);
	}

	// clean up
	zlibError = inflateEnd(&zStream);
	if (zlibError != Z_OK && error == B_OK)
		error = TranslateZlibError(zlibError);

	if (error != B_OK)
		return error;

	_uncompressedSize = zStream.total_out;
	return B_OK;
}
