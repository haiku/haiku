#include <stdio.h>
#include <malloc.h>

#include "StreamIO.h"


static int
stream_read(struct FileCache *cc, ulonglong pos, void *buffer, int count)
{
	return reinterpret_cast<StreamIO *>(cc)->source->ReadAt(pos, buffer, count);
}


static longlong
stream_scan(struct FileCache *cc,ulonglong start,unsigned signature)
{
	return 0;
}


static void
stream_close(struct FileCache *cc)
{
}


static unsigned
stream_getsize(struct FileCache *cc)
{
	return reinterpret_cast<StreamIO *>(cc)->source->Seek(0, SEEK_END);
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

	io->source = posio;
	io->filecache.read = &stream_read;
	io->filecache.scan = &stream_scan;
	io->filecache.close = &stream_close;
	io->filecache.getsize = &stream_getsize;
	io->filecache.geterror = &stream_geterror;
	
	return reinterpret_cast<FileCache *>(io);
}

