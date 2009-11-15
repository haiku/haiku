/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ZlibDecompressor.h"

#include <errno.h>

#include <zlib.h>


ZlibDecompressor::ZlibDecompressor()
{
}


ZlibDecompressor::~ZlibDecompressor()
{
}


status_t
ZlibDecompressor::Decompress(const void* input, size_t inputSize, void* output,
	size_t outputSize, size_t& _uncompressedSize)
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


	// deflate
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
