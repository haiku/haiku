/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU L-GPL license.
 */

#include "AVFormatWriter.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <new>

#include <AutoDeleter.h>
#include <Autolock.h>
#include <ByteOrder.h>
#include <DataIO.h>
#include <MediaDefs.h>
#include <MediaFormats.h>

extern "C" {
	#include "avformat.h"
}

#include "DemuxerTable.h"
#include "gfx_util.h"


#define TRACE_AVFORMAT_WRITER
#ifdef TRACE_AVFORMAT_WRITER
#	define TRACE printf
#	define TRACE_IO(a...)
#	define TRACE_PACKET(a...)
#else
#	define TRACE(a...)
#	define TRACE_IO(a...)
#	define TRACE_PACKET(a...)
#endif

#define ERROR(a...) fprintf(stderr, a)


// #pragma mark - AVFormatWriter::StreamCookie


class AVFormatWriter::StreamCookie {
public:
								StreamCookie(BPositionIO* target,
									BLocker* streamLock);
	virtual						~StreamCookie();

private:
			BPositionIO*		fTarget;
			off_t				fPosition;
			// Since different threads may read from the source,
			// we need to protect the file position and I/O by a lock.
			BLocker*			fStreamLock;
};



AVFormatWriter::StreamCookie::StreamCookie(BPositionIO* target,
		BLocker* streamLock)
	:
	fTarget(target),
	fPosition(0),
	fStreamLock(streamLock)
{
}


AVFormatWriter::StreamCookie::~StreamCookie()
{
}


// #pragma mark - AVFormatWriter


AVFormatWriter::AVFormatWriter()
	:
	fStreams(NULL),
	fStreamLock("stream lock")
{
	TRACE("AVFormatWriter::AVFormatWriter\n");
}


AVFormatWriter::~AVFormatWriter()
{
	TRACE("AVFormatWriter::~AVFormatWriter\n");
	if (fStreams != NULL) {
		delete fStreams[0];
		delete[] fStreams;
	}
}


// #pragma mark -


status_t
AVFormatWriter::SetCopyright(const char* copyright)
{
	TRACE("AVFormatWriter::SetCopyright(%s)\n", copyright);

	return B_NOT_SUPPORTED;
}


status_t
AVFormatWriter::CommitHeader()
{
	TRACE("AVFormatWriter::CommitHeader\n");

	return B_NOT_SUPPORTED;
}


status_t
AVFormatWriter::Flush()
{
	TRACE("AVFormatWriter::Flush\n");

	return B_NOT_SUPPORTED;
}


status_t
AVFormatWriter::Close()
{
	TRACE("AVFormatWriter::Close\n");

	return B_NOT_SUPPORTED;
}


status_t
AVFormatWriter::AllocateCookie(void** _cookie)
{
	TRACE("AVFormatWriter::AllocateCookie()\n");

	BAutolock _(fStreamLock);

	if (fStreams == NULL)
		return B_NO_INIT;

	if (_cookie == NULL)
		return B_BAD_VALUE;

	return B_NOT_SUPPORTED;
}


status_t
AVFormatWriter::FreeCookie(void* _cookie)
{
	BAutolock _(fStreamLock);

	StreamCookie* cookie = reinterpret_cast<StreamCookie*>(_cookie);

	if (cookie != NULL) {
//		if (fStreams != NULL)
//			fStreams[cookie->VirtualIndex()] = NULL;
		delete cookie;
	}

	return B_OK;
}


// #pragma mark -


status_t
AVFormatWriter::AddTrackInfo(void* cookie, uint32 code,
	const void* data, size_t size, uint32 flags)
{
	TRACE("AVFormatWriter::AddTrackInfo(%lu, %p, %ld, %lu)\n",
		code, data, size, flags);

	return B_NOT_SUPPORTED;
}


status_t
AVFormatWriter::WriteChunk(void* cookie, const void* chunkBuffer,
	size_t chunkSize, uint32 flags)
{
	TRACE("AVFormatWriter::WriteChunk(%p, %ld, %lu)\n", chunkBuffer, chunkSize,
		flags);

	return B_NOT_SUPPORTED;
}


