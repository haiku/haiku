#ifndef _MEDIA_EXTRACTOR_H
#define _MEDIA_EXTRACTOR_H

#include "ReaderPlugin.h"
#include "DecoderPlugin.h"

namespace BPrivate {
namespace media {

struct stream_info
{
	status_t		status;
	void *			cookie;
	void *			infoBuffer;
	int32			infoBufferSize;
	media_format	encodedFormat;
};

class MediaExtractor
{
public:
					MediaExtractor(BDataIO * source, int32 flags);
					~MediaExtractor();

	status_t		InitCheck();
			
	void			GetFileFormatInfo(media_file_format *mfi) const;

	int32			StreamCount();
	
	const media_format * EncodedFormat(int32 stream);
	int64			CountFrames(int32 stream) const;
	bigtime_t		Duration(int32 stream) const;

	status_t		Seek(int32 stream, uint32 seekTo,
						 int64 *frame, bigtime_t *time);
	
	status_t		GetNextChunk(int32 stream,
								 void **chunkBuffer, int32 *chunkSize,
								 media_header *mediaHeader);

	status_t		CreateDecoder(int32 stream, Decoder **decoder, media_codec_info *mci);


private:
	status_t		fErr;

	BDataIO			*fSource;
	Reader			*fReader;
	
	stream_info *	fStreamInfo;
	int32			fStreamCount;

	media_file_format fMff;
};

}; // namespace media
}; // namespace BPrivate

using namespace BPrivate::media;

#endif
