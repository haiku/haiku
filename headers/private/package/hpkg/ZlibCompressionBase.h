/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__ZLIB_COMPRESSION_BASE_H_
#define _PACKAGE__HPKG__PRIVATE__ZLIB_COMPRESSION_BASE_H_


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


class ZlibCompressionBase {
public:
	static	status_t			TranslateZlibError(int error);
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__ZLIB_COMPRESSION_BASE_H_
