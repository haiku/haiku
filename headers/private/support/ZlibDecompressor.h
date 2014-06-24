/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ZLIB_DECOMPRESSOR_H_
#define _ZLIB_DECOMPRESSOR_H_


#include <ZlibCompressionBase.h>


class BZlibDecompressor : public BZlibCompressionBase {
public:
								BZlibDecompressor(BDataIO* output);
								~BZlibDecompressor();

			status_t			Init();
			status_t			DecompressNext(const void* input,
									size_t inputSize);
			status_t			Finish();

	static	status_t			DecompressSingleBuffer(const void* input,
									size_t inputSize, void* output,
									size_t outputSize,
									size_t& _uncompressedSize);

private:
			bool				fFinished;
};


#endif	// _ZLIB_DECOMPRESSOR_H_
