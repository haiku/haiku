/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ZlibCompressionBase.h>

#include <errno.h>
#include <string.h>

#include <new>

#include "ZlibCompressionBasePrivate.h"


BZlibCompressionBase::BZlibCompressionBase(BDataIO* output)
	:
	fOutput(output),
	fStream(NULL)
{
}


BZlibCompressionBase::~BZlibCompressionBase()
{
	DeleteStream();
}


/*static*/ status_t
BZlibCompressionBase::TranslateZlibError(int error)
{
	switch (error) {
		case Z_OK:
			return B_OK;
		case Z_STREAM_END:
		case Z_NEED_DICT:
			// a special event (no error), but the caller doesn't seem to handle
			// it
			return B_ERROR;
		case Z_ERRNO:
			return errno;
		case Z_STREAM_ERROR:
			return B_BAD_VALUE;
		case Z_DATA_ERROR:
			return B_BAD_DATA;
		case Z_MEM_ERROR:
			return B_NO_MEMORY;
		case Z_BUF_ERROR:
			return B_BUFFER_OVERFLOW;
		case Z_VERSION_ERROR:
			return B_BAD_VALUE;
		default:
			return B_ERROR;
	}
}


status_t
BZlibCompressionBase::CreateStream()
{
	if (fStream != NULL)
		return B_BAD_VALUE;

	fStream = new(std::nothrow) ZStream;
	if (fStream == NULL)
		return B_NO_MEMORY;

	memset(fStream, 0, sizeof(*fStream));
	return B_OK;
}


void
BZlibCompressionBase::DeleteStream()
{
	delete fStream;
	fStream = NULL;
}
