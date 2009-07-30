/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU L-GPL license.
 */
#ifndef AV_FORMAT_WRITER_H
#define AV_FORMAT_WRITER_H


#include <Locker.h>

#include "WriterPlugin.h"


class AVFormatWriter : public Writer {
public:
								AVFormatWriter();
								~AVFormatWriter();
	
	virtual	status_t			SetCopyright(const char* copyright);
	virtual	status_t			CommitHeader();
	virtual	status_t			Flush();
	virtual	status_t			Close();

	virtual	status_t			AllocateCookie(void** cookie);
	virtual	status_t			FreeCookie(void* cookie);
	
	virtual	status_t			AddTrackInfo(void* cookie, uint32 code,
									const void* data, size_t size,
									uint32 flags = 0);

	virtual	status_t			WriteNextChunk(void* cookie,
									const void* chunkBuffer, size_t chunkSize,
									uint32 flags);

private:
	class StreamCookie;

			StreamCookie**		fStreams;
			BLocker				fStreamLock;
};


#endif // AV_FORMAT_WRITER_H
