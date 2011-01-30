/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/ZlibDecompressor.h>

#include <errno.h>
#include <stdio.h>

#include <package/hpkg/DataOutput.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


// TODO: For the kernel the buffer shouldn't be allocated on the stack.
static const size_t kOutputBufferSize = 1024;


ZlibDecompressor::ZlibDecompressor(BDataOutput* output)
	:
	fOutput(output),
	fStreamInitialized(false),
	fFinished(false)
{
}


ZlibDecompressor::~ZlibDecompressor()
{
	if (fStreamInitialized)
		inflateEnd(&fStream);
}


status_t
ZlibDecompressor::Init()
{
	// initialize the stream
	fStream.next_in = NULL;
	fStream.avail_in = 0;
	fStream.total_in = 0;
	fStream.next_out = NULL;
	fStream.avail_out = 0;
	fStream.total_out = 0;
	fStream.msg = 0;
	fStream.state = 0;
	fStream.zalloc = Z_NULL;
	fStream.zfree = Z_NULL;
	fStream.opaque = Z_NULL;
	fStream.data_type = 0;
	fStream.adler = 0;
	fStream.reserved = 0;

	int zlibError = inflateInit(&fStream);
	if (zlibError != Z_OK)
		return TranslateZlibError(zlibError);

	fStreamInitialized = true;

	return B_OK;
}


status_t
ZlibDecompressor::DecompressNext(const void* input, size_t inputSize)
{
	fStream.next_in = (Bytef*)input;
	fStream.avail_in = inputSize;

	while (fStream.avail_in > 0) {
		if (fFinished)
			return B_BAD_DATA;

		uint8 outputBuffer[kOutputBufferSize];
		fStream.next_out = (Bytef*)outputBuffer;
		fStream.avail_out = sizeof(outputBuffer);

		int zlibError = inflate(&fStream, 0);
		if (zlibError == Z_STREAM_END)
			fFinished = true;
		else if (zlibError != Z_OK)
			return TranslateZlibError(zlibError);

		if (fStream.avail_out < sizeof(outputBuffer)) {
			status_t error = fOutput->WriteData(outputBuffer,
				sizeof(outputBuffer) - fStream.avail_out);
			if (error != B_OK)
				return error;
		}
	}

	return B_OK;
}


status_t
ZlibDecompressor::Finish()
{
	fStream.next_in = (Bytef*)NULL;
	fStream.avail_in = 0;

	while (!fFinished) {
		uint8 outputBuffer[kOutputBufferSize];
		fStream.next_out = (Bytef*)outputBuffer;
		fStream.avail_out = sizeof(outputBuffer);

		int zlibError = inflate(&fStream, Z_FINISH);
		if (zlibError == Z_STREAM_END)
			fFinished = true;
		else if (zlibError != Z_OK)
			return TranslateZlibError(zlibError);

		if (fStream.avail_out < sizeof(outputBuffer)) {
			status_t error = fOutput->WriteData(outputBuffer,
				sizeof(outputBuffer) - fStream.avail_out);
			if (error != B_OK)
				return error;
		}
	}

	inflateEnd(&fStream);
	fStreamInitialized = false;

	return B_OK;
}


/*static*/ status_t
ZlibDecompressor::DecompressSingleBuffer(const void* input, size_t inputSize,
	void* output, size_t outputSize, size_t& _uncompressedSize)
{
	if (inputSize == 0 || outputSize == 0)
		return B_BAD_VALUE;

	// prepare stream
	z_stream zStream = {
		(Bytef*)input,				// next_in
		inputSize,					// avail_in
		0,							// total_in
		(Bytef*)output,				// next_out
		outputSize,					// avail_out
		0,							// total_out
		0,							// msg
		0,							// state;
		Z_NULL,						// zalloc
		Z_NULL,						// zfree
		Z_NULL,						// opaque
		0,							// data_type
		0,							// adler
		0							// reserved
	};

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


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
