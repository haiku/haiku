/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU L-GPL license.
 */
#ifndef AV_FORMAT_WRITER_H
#define AV_FORMAT_WRITER_H


#include <Writer.h>
#include <Locker.h>

using namespace BCodecKit;


extern "C" {
	#include "avformat.h"
}


class AVFormatWriter : public BWriter {
public:
								AVFormatWriter();
								~AVFormatWriter();

	virtual	status_t			Init(const media_file_format* fileFormat);

	virtual	status_t			SetMetaData(BMetaData* data);
	virtual	status_t			CommitHeader();
	virtual	status_t			Flush();
	virtual	status_t			Close();

	virtual	status_t			AllocateCookie(void** cookie,
									media_format* format,
									const media_codec_info* codecInfo);
	virtual	status_t			FreeCookie(void* cookie);

	virtual	status_t			SetMetaData(void* cookie,
									BMetaData* data);

	virtual	status_t			AddTrackInfo(void* cookie, uint32 code,
									const void* data, size_t size,
									uint32 flags = 0);

	virtual	status_t			WriteChunk(void* cookie,
									const void* chunkBuffer, size_t chunkSize,
									media_encode_info* encodeInfo);

private:
	static	int					_Write(void* cookie, uint8* buffer,
									int bufferSize);
	static	off_t				_Seek(void* cookie, off_t offset, int whence);

private:
			class StreamCookie;

			AVFormatContext*	fFormatContext;
			bool				fCodecOpened;
			int					fHeaderError;

			AVIOContext*		fIOContext;

			StreamCookie**		fStreams;
			BLocker				fStreamLock;
};


#endif // AV_FORMAT_WRITER_H
