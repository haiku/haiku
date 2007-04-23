#ifndef __MEDIA_STREAM_DECODER_H
#define __MEDIA_STREAM_DECODER_H

#include <media/MediaDecoder.h>
#include "MediaStreamDecoder.h"

typedef status_t (*get_next_chunk_func)(const void **chunkData, size_t *chunkLen, media_header *mh, void *cookie);


class MediaStreamDecoder : private BMediaDecoder
{
public:
				MediaStreamDecoder(get_next_chunk_func next_chunk, void *cookie);

	status_t	SetInputFormat(const media_format &in_format);
	status_t	SetOutputFormat(media_format *output_format);

	status_t	Decode(void *out_buffer, int64 *out_frameCount,
		               media_header *out_mh, media_decode_info *info);

private:
	void *				fCookie;
	get_next_chunk_func	fGetNextChunk;

private:
	status_t	GetNextChunk(const void **chunkData, size_t *chunkLen, media_header *mh);
};

#endif
