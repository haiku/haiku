/*
 * Copyright (c) 2004, Marcus Overhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <malloc.h>

#include "StreamIO.h"

struct StreamIO {
	FileCache 	filecache;
	BPositionIO *source;
};

static int
stream_read(struct FileCache *cc, ulonglong pos, void *buffer, int count)
{
	return reinterpret_cast<StreamIO *>(cc)->source->ReadAt(pos, buffer, count);
}


static longlong
stream_scan(struct FileCache *cc, ulonglong start, unsigned signature)
{
	return -1;
}


static void
stream_close(struct FileCache *cc)
{
}


static unsigned
stream_getsize(struct FileCache *cc)
{
	return 524288;
}


static const char *
stream_geterror(struct FileCache *cc)
{
	return "";
}


FileCache *
CreateFileCache(BDataIO *dataio)
{
	BPositionIO *posio;
	posio = dynamic_cast<BPositionIO *>(dataio);
	if (!posio)
		return NULL;
	
	StreamIO *io;
	io = (StreamIO *)malloc(sizeof(StreamIO));
	if (!io)
		return NULL;

	io->filecache.read = &stream_read;
	io->filecache.scan = &stream_scan;
	io->filecache.close = &stream_close;
	io->filecache.getsize = &stream_getsize;
	io->filecache.geterror = &stream_geterror;
	io->source = posio;
	
	return reinterpret_cast<FileCache *>(io);
}

