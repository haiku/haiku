/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ZLIB_DECOMPRESSOR_H
#define ZLIB_DECOMPRESSOR_H


#include <zlib.h>

#include <package/hpkg/ZlibCompressionBase.h>


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


class DataOutput;


class ZlibDecompressor : public ZlibCompressionBase {
public:
								ZlibDecompressor(DataOutput* output);
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
			DataOutput*			fOutput;
			bool				fStreamInitialized;
			bool				fFinished;
};


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit


#endif	// ZLIB_DECOMPRESSOR_H
