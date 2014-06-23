/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__ZLIB_COMPRESSOR_H_
#define _PACKAGE__HPKG__PRIVATE__ZLIB_COMPRESSOR_H_


#include <zlib.h>

#include <package/hpkg/ZlibCompressionBase.h>


class BDataIO;


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


class ZlibCompressor : public ZlibCompressionBase {
public:
								ZlibCompressor(BDataIO* output);
								~ZlibCompressor();

			status_t			Init(int compressionLevel = Z_BEST_COMPRESSION);
			status_t			CompressNext(const void* input,
									size_t inputSize);
			status_t			Finish();

	static	status_t			CompressSingleBuffer(const void* input,
									size_t inputSize, void* output,
									size_t outputSize, size_t& _compressedSize,
									int compressionLevel = Z_BEST_COMPRESSION);

private:
			z_stream			fStream;
			BDataIO*			fOutput;
			bool				fStreamInitialized;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__ZLIB_COMPRESSOR_H_
