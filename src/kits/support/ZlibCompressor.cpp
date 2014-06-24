/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ZlibCompressor.h>

#include <string.h>

#include <DataIO.h>

#include "ZlibCompressionBasePrivate.h"


static const size_t kOutputBufferSize = 1024;


BZlibCompressor::BZlibCompressor(BDataIO* output)
	:
	BZlibCompressionBase(output)
{
}


BZlibCompressor::~BZlibCompressor()
{
	if (fStream != NULL)
		deflateEnd(fStream);
}


status_t
BZlibCompressor::Init(int compressionLevel)
{
	status_t error = CreateStream();
	if (error != B_OK)
		return error;

	int zlibError = deflateInit(fStream, compressionLevel);
	if (zlibError != Z_OK) {
		DeleteStream();
		return TranslateZlibError(zlibError);
	}

	return B_OK;
}


status_t
BZlibCompressor::CompressNext(const void* input, size_t inputSize)
{
	fStream->next_in = (Bytef*)input;
	fStream->avail_in = inputSize;

	while (fStream->avail_in > 0) {
		uint8 outputBuffer[kOutputBufferSize];
		fStream->next_out = (Bytef*)outputBuffer;
		fStream->avail_out = sizeof(outputBuffer);

		int zlibError = deflate(fStream, 0);
		if (zlibError != Z_OK)
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
BZlibCompressor::Finish()
{
	fStream->next_in = (Bytef*)NULL;
	fStream->avail_in = 0;

	while (true) {
		uint8 outputBuffer[kOutputBufferSize];
		fStream->next_out = (Bytef*)outputBuffer;
		fStream->avail_out = sizeof(outputBuffer);

		int zlibError = deflate(fStream, Z_FINISH);
		if (zlibError != Z_OK && zlibError != Z_STREAM_END)
			return TranslateZlibError(zlibError);

		if (fStream->avail_out < sizeof(outputBuffer)) {
			status_t error = fOutput->WriteExactly(outputBuffer,
				sizeof(outputBuffer) - fStream->avail_out);
			if (error != B_OK)
				return error;
		}

		if (zlibError == Z_STREAM_END)
			break;
	}

	deflateEnd(fStream);
	DeleteStream();

	return B_OK;
}


/*static*/ status_t
BZlibCompressor::CompressSingleBuffer(const void* input, size_t inputSize,
	void* output, size_t outputSize, size_t& _compressedSize,
	int compressionLevel)
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

	int zlibError = deflateInit(&zStream, compressionLevel);
	if (zlibError != Z_OK)
		return TranslateZlibError(zlibError);

	// deflate
	status_t error = B_OK;
	zlibError = deflate(&zStream, Z_FINISH);
	if (zlibError != Z_STREAM_END) {
		if (zlibError == Z_OK)
			error = B_BUFFER_OVERFLOW;
		else
			error = TranslateZlibError(zlibError);
	}

	// clean up
	zlibError = deflateEnd(&zStream);
	if (zlibError != Z_OK && error == B_OK)
		error = TranslateZlibError(zlibError);

	if (error != B_OK)
		return error;

	_compressedSize = zStream.total_out;
	return B_OK;
}
