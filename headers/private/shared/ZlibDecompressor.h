/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PRIVATE__ZLIB_DECOMPRESSOR_H_
#define _PRIVATE__ZLIB_DECOMPRESSOR_H_


#include <zlib.h>

#include <ZlibCompressionBase.h>


class BDataIO;


namespace BPrivate {


class ZlibDecompressor : public ZlibCompressionBase {
public:
								ZlibDecompressor(BDataIO* output);
								~ZlibDecompressor();

			status_t			Init();
			status_t			DecompressNext(const void* input,
									size_t inputSize);
			status_t			Finish();

	static	status_t			DecompressSingleBuffer(const void* input,
									size_t inputSize, void* output,
									size_t outputSize,
									size_t& _uncompressedSize);

private:
			z_stream			fStream;
			BDataIO*			fOutput;
			bool				fStreamInitialized;
			bool				fFinished;
};


}	// namespace BPrivate

#endif	// _PACKAGE__HPKG__PRIVATE__ZLIB_DECOMPRESSOR_H_
