/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ZLIB_COMPRESSION_BASE_H
#define ZLIB_COMPRESSION_BASE_H


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


class ZlibCompressionBase {
public:
	static	status_t			TranslateZlibError(int error);
};


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit


#endif	// ZLIB_COMPRESSION_BASE_H
