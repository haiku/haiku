#ifndef _READER_PLUGIN_H
#define _READER_PLUGIN_H

#include <MediaTrack.h>
#include "MediaPlugin.h"

namespace BPrivate { namespace media {

enum {
	B_MEDIA_SEEK_TO_TIME	= 0x10000,
	B_MEDIA_SEEK_TO_FRAME	= 0x20000
};

class Reader
{
public:
						Reader();
	virtual				~Reader();
	
	virtual const char *Copyright() = 0;
	
	virtual status_t	Sniff(int32 *streamCount) = 0;

	virtual status_t	AllocateCookie(int32 streamNumber, void **cookie) = 0;
	virtual	status_t	FreeCookie(void *cookie) = 0;
	
	virtual status_t	GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
									  media_format *format, void **infoBuffer, int32 *infoSize) = 0;

	virtual status_t	Seek(void *cookie,
							 uint32 seekTo,
							 int64 *frame, bigtime_t *time) = 0;

	virtual status_t	GetNextChunk(void *cookie,
									 void **chunkBuffer, int32 *chunkSize,
									 media_header *mediaHeader) = 0;
	
	BDataIO *			Source();
	
private:
public: // XXX for test programs only
	void		Setup(BDataIO *source);

	BDataIO * fSource;
};


class ReaderPlugin : public MediaPlugin
{
public:
	virtual Reader *NewReader() = 0;
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif
