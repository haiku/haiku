/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ZLIB_COMPRESSOR_H
#define ZLIB_COMPRESSOR_H


#include "ZlibCompressionBase.h"


class ZlibCompressor : public ZlibCompressionBase {
public:
								ZlibCompressor();
								~ZlibCompressor();

			status_t			Compress(const void* input, size_t inputSize,
									void* output, size_t outputSize,
									size_t& _compressedSize);
};


#endif	// ZLIB_COMPRESSOR_H
