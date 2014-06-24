/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ZLIB_COMPRESSOR_H_
#define _ZLIB_COMPRESSOR_H_


#include <ZlibCompressionBase.h>


class BDataIO;


// compression level
enum {
	B_ZLIB_COMPRESSION_NONE		= 0,
	B_ZLIB_COMPRESSION_FASTEST	= 1,
	B_ZLIB_COMPRESSION_BEST		= 9,
	B_ZLIB_COMPRESSION_DEFAULT	= -1,
};


class BZlibCompressor : public BZlibCompressionBase {
public:
								BZlibCompressor(BDataIO* output);
								~BZlibCompressor();

			status_t			Init(int compressionLevel
									= B_ZLIB_COMPRESSION_BEST);
			status_t			CompressNext(const void* input,
									size_t inputSize);
			status_t			Finish();

	static	status_t			CompressSingleBuffer(const void* input,
									size_t inputSize, void* output,
									size_t outputSize, size_t& _compressedSize,
									int compressionLevel
										= B_ZLIB_COMPRESSION_BEST);
};


#endif	// _ZLIB_COMPRESSOR_H_
