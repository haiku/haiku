#ifndef _DECODER_PLUGIN_H
#define _DECODER_PLUGIN_H

#include <MediaTrack.h>
#include <MediaFormats.h>
#include "MediaPlugin.h"
#include "MediaExtractor.h"

namespace BPrivate { namespace media {

class Decoder
{
public:
						Decoder();
	virtual				~Decoder();
	
						// Setup get's called with the info data from Reader::GetStreamInfo
	virtual status_t	Setup(media_format *ioEncodedFormat, media_format *ioDecodedFormat,
							  const void *infoBuffer, int32 infoSize) = 0;
	
	virtual status_t	Seek(uint32 seekTo,
							 int64 seekFrame, int64 *frame,
							 bigtime_t seekTime, bigtime_t *time) = 0;
							 
	virtual status_t	Decode(void *buffer, int64 *frameCount,
							   media_header *mediaHeader, media_decode_info *info = 0) = 0;
							   
	status_t			GetNextChunk(void **chunkBuffer, int32 *chunkSize,
									 media_header *mediaHeader);

private:
	friend class MediaExtractor;
	
	void				Setup(MediaExtractor *extractor, int32 stream);
	MediaExtractor *	fExtractor;
	int32				fStream;
};


class DecoderPlugin : public MediaPlugin
{
public:
	DecoderPlugin();

	virtual Decoder *NewDecoder() = 0;
	
	status_t PublishDecoder(const char *meta_description, const char *short_name, const char *pretty_name, const char *default_mapping = 0);
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif
