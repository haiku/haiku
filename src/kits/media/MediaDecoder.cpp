/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaDecoder.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaDecoder.h>
#include "debug.h"

/*************************************************************
 * public BMediaDecoder
 *************************************************************/

BMediaDecoder::BMediaDecoder()
{
	UNIMPLEMENTED();
}

BMediaDecoder::BMediaDecoder(const media_format *in_format,
							 const void *info,
							 size_t info_size)
{
	UNIMPLEMENTED();
}

BMediaDecoder::BMediaDecoder(const media_codec_info *mci)
{
	UNIMPLEMENTED();
}

/* virtual */
BMediaDecoder::~BMediaDecoder()
{
	UNIMPLEMENTED();
}


status_t 
BMediaDecoder::InitCheck() const
{
	UNIMPLEMENTED();

	return B_OK;
}


status_t 
BMediaDecoder::SetTo(const media_format *in_format,
					 const void *info,
					 size_t info_size)
{
	UNIMPLEMENTED();
	return B_OK;
}

status_t 
BMediaDecoder::SetTo(const media_codec_info *mci)
{
	UNIMPLEMENTED();
	return B_OK;
}

status_t 
BMediaDecoder::SetInputFormat(const media_format *in_format,
							  const void *in_info,
							  size_t in_size)
{
	UNIMPLEMENTED();
	return B_OK;
}

status_t 
BMediaDecoder::SetOutputFormat(media_format *output_format)
{
	UNIMPLEMENTED();
	return B_OK;
}

// Set output format to closest acceptable format, returns the
// format used.
status_t 
BMediaDecoder::Decode(void *out_buffer, 
					  int64 *out_frameCount,
					  media_header *out_mh, 
					  media_decode_info *info)
{
	UNIMPLEMENTED();
	return B_OK;
}

status_t 
BMediaDecoder::GetDecoderInfo(media_codec_info *out_info) const
{
	UNIMPLEMENTED();
	return B_OK;
}


/*************************************************************
 * protected BMediaDecoder
 *************************************************************/


/*************************************************************
 * private BMediaDecoder
 *************************************************************/

/*
// unimplemented
BMediaDecoder::BMediaDecoder(const BMediaDecoder &);
BMediaDecoder::BMediaDecoder & operator=(const BMediaDecoder &);
*/

/* static */ status_t 
BMediaDecoder::next_chunk(void *classptr, void **chunkData, size_t *chunkLen, media_header *mh)
{
	UNIMPLEMENTED();
	return B_OK;
}

void
BMediaDecoder::ReleaseDecoder()
{
	UNIMPLEMENTED();
}


status_t BMediaDecoder::_Reserved_BMediaDecoder_0(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_1(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_2(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_3(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_4(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_5(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_6(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_7(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_8(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_9(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_10(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_11(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_12(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_13(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_14(int32 arg, ...) { return B_ERROR; }
status_t BMediaDecoder::_Reserved_BMediaDecoder_15(int32 arg, ...) { return B_ERROR; }

/*************************************************************
 * public BMediaBufferDecoder
 *************************************************************/

BMediaBufferDecoder::BMediaBufferDecoder()
{
	UNIMPLEMENTED();
}

BMediaBufferDecoder::BMediaBufferDecoder(const media_format *in_format,
										 const void *info,
										 size_t info_size)
{
	UNIMPLEMENTED();
}

BMediaBufferDecoder::BMediaBufferDecoder(const media_codec_info *mci)
{
	UNIMPLEMENTED();
}

status_t 
BMediaBufferDecoder::DecodeBuffer(const void *input_buffer, 
								  size_t input_size,
								  void *out_buffer, 
								  int64 *out_frameCount,
								  media_header *out_mh,
								  media_decode_info *info)
{
	UNIMPLEMENTED();
	return B_OK;
}

/*************************************************************
 * protected BMediaBufferDecoder
 *************************************************************/

/* virtual */
status_t BMediaBufferDecoder::GetNextChunk(const void **chunkData, 
										   size_t *chunkLen,
										   media_header *mh)
{
	UNIMPLEMENTED();
	return B_OK;
}

