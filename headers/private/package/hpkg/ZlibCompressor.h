/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__ZLIB_COMPRESSOR_H_
#define _PACKAGE__HPKG__PRIVATE__ZLIB_COMPRESSOR_H_


#include <zlib.h>

#include <package/hpkg/ZlibCompressionBase.h>


namespace BPackageKit {

namespace BHPKG {


class BDataOutput;


namespace BPrivate {


class ZlibCompressor : public ZlibCompressionBase {
public:
								ZlibCompressor(BDataOutput* output);
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
			BDataOutput*		fOutput;
			bool				fStreamInitialized;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__ZLIB_COMPRESSOR_H_
