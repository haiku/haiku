/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ZLIB_COMPRESSOR_H
#define ZLIB_COMPRESSOR_H


#include <zlib.h>

#include "ZlibCompressionBase.h"


class DataOutput;


class ZlibCompressor : public ZlibCompressionBase {
public:
								ZlibCompressor(DataOutput* output);
								~ZlibCompressor();

			status_t			Init();
			status_t			CompressNext(const void* input,
									size_t inputSize);
			status_t			Finish();

	static	status_t			CompressSingleBuffer(const void* input,
									size_t inputSize, void* output,
									size_t outputSize, size_t& _compressedSize);

private:
			z_stream			fStream;
			DataOutput*			fOutput;
			bool				fStreamInitialized;
};


#endif	// ZLIB_COMPRESSOR_H
