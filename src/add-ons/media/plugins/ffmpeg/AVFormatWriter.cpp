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


static const size_t kIOBufferSize = 64 * 1024;
	// TODO: This could depend on the BMediaFile creation flags, IIRC,
	// they allow to specify a buffering mode.


// #pragma mark - URLProtocol
// TODO: Do we need to write an URLProtocol?


// #pragma mark - AVFormatWriter::StreamCookie


class AVFormatWriter::StreamCookie {
public:
								StreamCookie(AVFormatContext* context,
									BLocker* streamLock);
	virtual						~StreamCookie();

			status_t			Init(const media_format* format);

			status_t			WriteChunk(const void* chunkBuffer,
									size_t chunkSize,
									media_encode_info* encodeInfo);

			status_t			AddTrackInfo(uint32 code, const void* data,
									size_t size, uint32 flags);

private:
			AVFormatContext*	fContext;
			AVStream*			fStream;
			// Since different threads may read from the source,
			// we need to protect the file position and I/O by a lock.
			BLocker*			fStreamLock;
};



AVFormatWriter::StreamCookie::StreamCookie(AVFormatContext* context,
		BLocker* streamLock)
	:
	fContext(context),
	fStream(NULL),
	fStreamLock(streamLock)
{
}


AVFormatWriter::StreamCookie::~StreamCookie()
{
}


status_t
AVFormatWriter::StreamCookie::Init(const media_format* format)
{
	TRACE("AVFormatWriter::StreamCookie::Init()\n");

	BAutolock _(fStreamLock);

	fStream = av_new_stream(fContext, fContext->nb_streams);

	if (fStream == NULL) {
		TRACE("  failed to add new stream\n");
		return B_ERROR;
	}

	// TODO: Setup the stream according to the media format...

	return B_OK;
}


status_t
AVFormatWriter::StreamCookie::WriteChunk(const void* chunkBuffer,
	size_t chunkSize, media_encode_info* encodeInfo)
{
	TRACE("AVFormatWriter::StreamCookie::WriteChunk(%p, %ld)\n",
		chunkBuffer, chunkSize);

	BAutolock _(fStreamLock);

	return B_ERROR;
}


status_t
AVFormatWriter::StreamCookie::AddTrackInfo(uint32 code,
	const void* data, size_t size, uint32 flags)
{
	TRACE("AVFormatWriter::StreamCookie::AddTrackInfo(%lu, %p, %ld, %lu)\n",
		code, data, size, flags);

	BAutolock _(fStreamLock);

	return B_NOT_SUPPORTED;
}


// #pragma mark - AVFormatWriter


AVFormatWriter::AVFormatWriter()
	:
	fContext(avformat_alloc_context()),
	fHeaderWritten(false),
	fIOBuffer(NULL),
	fStreamLock("stream lock")
{
	TRACE("AVFormatWriter::AVFormatWriter\n");
}


AVFormatWriter::~AVFormatWriter()
{
	TRACE("AVFormatWriter::~AVFormatWriter\n");

	av_free(fContext);

	delete[] fIOBuffer;
}


// #pragma mark -


status_t
AVFormatWriter::Init(const media_file_format* fileFormat)
{
	TRACE("AVFormatWriter::Init()\n");

	delete[] fIOBuffer;
	fIOBuffer = new(std::nothrow) uint8[kIOBufferSize];
	if (fIOBuffer == NULL)
		return B_NO_MEMORY;

	// Init I/O context with buffer and hook functions, pass ourself as
	// cookie.
	if (init_put_byte(&fIOContext, fIOBuffer, kIOBufferSize, 0, this,
			0, _Write, _Seek) != 0) {
		TRACE("  init_put_byte() failed!\n");
		return B_ERROR;
	}

	// TODO: Is this how it works?
	// TODO: Is the cookie stored in ByteIOContext? Or does it need to be
	// stored in fContext->priv_data?
	fContext->pb = &fIOContext;

	// TODO: Set the AVOutputFormat according to fileFormat...
	fContext->oformat = guess_format(fileFormat->short_name,
		fileFormat->file_extension, fileFormat->mime_type);
	if (fContext->oformat == NULL) {
		TRACE("  failed to find AVOuputFormat for %s\n",
			fileFormat->short_name);
		return B_NOT_SUPPORTED;
	}

	TRACE("  found AVOuputFormat for %s: %s\n", fileFormat->short_name,
		fContext->oformat->name);

	return B_OK;
}


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

	if (fContext == NULL)
		return B_NO_INIT;

	if (fHeaderWritten)
		return B_NOT_ALLOWED;

	int result = av_write_header(fContext);
	if (result < 0)
		TRACE("  av_write_header(): %d\n", result);
	else
		fHeaderWritten = true;

	return result == 0 ? B_OK : B_ERROR;
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

	if (fContext == NULL)
		return B_NO_INIT;

	if (!fHeaderWritten)
		return B_NOT_ALLOWED;

	int result = av_write_trailer(fContext);
	if (result < 0)
		TRACE("  av_write_trailer(): %d\n", result);

	return result == 0 ? B_OK : B_ERROR;
}


status_t
AVFormatWriter::AllocateCookie(void** _cookie, const media_format* format)
{
	TRACE("AVFormatWriter::AllocateCookie()\n");

	BAutolock _(fStreamLock);

	if (_cookie == NULL)
		return B_BAD_VALUE;

	StreamCookie* cookie = new(std::nothrow) StreamCookie(fContext,
		&fStreamLock);

	return cookie->Init(format);
}


status_t
AVFormatWriter::FreeCookie(void* _cookie)
{
	BAutolock _(fStreamLock);

	StreamCookie* cookie = reinterpret_cast<StreamCookie*>(_cookie);
	delete cookie;

	return B_OK;
}


// #pragma mark -


status_t
AVFormatWriter::SetCopyright(void* cookie, const char* copyright)
{
	TRACE("AVFormatWriter::SetCopyright(%p, %s)\n", cookie, copyright);

	return B_NOT_SUPPORTED;
}


status_t
AVFormatWriter::AddTrackInfo(void* _cookie, uint32 code,
	const void* data, size_t size, uint32 flags)
{
	TRACE("AVFormatWriter::AddTrackInfo(%lu, %p, %ld, %lu)\n",
		code, data, size, flags);

	StreamCookie* cookie = reinterpret_cast<StreamCookie*>(_cookie);
	return cookie->AddTrackInfo(code, data, size, flags);
}


status_t
AVFormatWriter::WriteChunk(void* _cookie, const void* chunkBuffer,
	size_t chunkSize, media_encode_info* encodeInfo)
{
	TRACE("AVFormatWriter::WriteChunk(%p, %ld, %p)\n", chunkBuffer, chunkSize,
		encodeInfo);

	StreamCookie* cookie = reinterpret_cast<StreamCookie*>(_cookie);
	return cookie->WriteChunk(chunkBuffer, chunkSize, encodeInfo);
}


// #pragma mark -


/*static*/ int
AVFormatWriter::_Write(void* cookie, const uint8* buffer, int bufferSize)
{
	TRACE_IO("AVFormatWriter::_Write(%p, %p, %d)\n",
		cookie, buffer, bufferSize);

	AVFormatWriter* writer = reinterpret_cast<AVFormatWriter*>(cookie);

	ssize_t written = writer->fTarget->Write(buffer, bufferSize);

	TRACE_IO("  written: %ld\n", written);
	return (int)written;

}


/*static*/ off_t
AVFormatWriter::_Seek(void* cookie, off_t offset, int whence)
{
	TRACE_IO("AVFormatWriter::_Seek(%p, %lld, %d)\n",
		cookie, offset, whence);

	AVFormatWriter* writer = reinterpret_cast<AVFormatWriter*>(cookie);

	BPositionIO* positionIO = dynamic_cast<BPositionIO*>(writer->fTarget);
	if (positionIO == NULL)
		return -1;

	// Support for special file size retrieval API without seeking anywhere:
	if (whence == AVSEEK_SIZE) {
		off_t size;
		if (positionIO->GetSize(&size) == B_OK)
			return size;
		return -1;
	}

	off_t position = positionIO->Seek(offset, whence);
	TRACE_IO("  position: %lld\n", position);
	if (position < 0)
		return -1;

	return position;
}


