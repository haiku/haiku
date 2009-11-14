/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ZLIB_DECOMPRESSOR_H
#define ZLIB_DECOMPRESSOR_H


#include "ZlibCompressionBase.h"


class ZlibDecompressor : public ZlibCompressionBase {
public:
								ZlibDecompressor();
								~ZlibDecompressor();

			status_t			Decompress(const void* input, size_t inputSize,
									void* output, size_t outputSize,
									size_t& _uncompressedSize);
};


#endif	// ZLIB_DECOMPRESSOR_H
