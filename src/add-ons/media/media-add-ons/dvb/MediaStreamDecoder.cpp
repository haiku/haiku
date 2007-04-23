#include "MediaStreamDecoder.h"

MediaStreamDecoder::MediaStreamDecoder(get_next_chunk_func next_chunk, void *cookie)
 :	BMediaDecoder()
 ,	fCookie(cookie)
 ,	fGetNextChunk(next_chunk)
{
}


status_t
MediaStreamDecoder::SetInputFormat(const media_format &in_format)
{
	status_t err = BMediaDecoder::InitCheck();
	if (err)
		return err;

	return BMediaDecoder::SetTo(&in_format);
}


status_t
MediaStreamDecoder::SetOutputFormat(media_format *output_format)
{
	status_t err = BMediaDecoder::InitCheck();
	if (err)
		return err;

	return BMediaDecoder::SetOutputFormat(output_format);
}


status_t
MediaStreamDecoder::Decode(void *out_buffer, int64 *out_frameCount,
						 media_header *out_mh, media_decode_info *info)
{
	return BMediaDecoder::Decode(out_buffer, out_frameCount, out_mh, info);
}


status_t
MediaStreamDecoder::GetNextChunk(const void **chunkData, size_t *chunkLen, media_header *mh)
{
	return (*fGetNextChunk)(chunkData, chunkLen, mh, fCookie);
}
