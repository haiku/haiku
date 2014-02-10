/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PRIVATE__ZLIB_COMPRESSION_BASE_H_
#define _PRIVATE__ZLIB_COMPRESSION_BASE_H_


#include <SupportDefs.h>


namespace BPrivate {


class ZlibCompressionBase {
public:
	static	status_t			TranslateZlibError(int error);
};


}	// namespace BPrivate

#endif	// _PRIVATE__ZLIB_COMPRESSION_BASE_H_
