/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaEncoder.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaEncoder.h>
#include "debug.h"


/*************************************************************
 * public BMediaEncoder
 *************************************************************/

BMediaEncoder::BMediaEncoder()
{
	UNIMPLEMENTED();
}


BMediaEncoder::BMediaEncoder(const media_format *output_format)
{
	UNIMPLEMENTED();
}


BMediaEncoder::BMediaEncoder(const media_codec_info *mci)
{
	UNIMPLEMENTED();
}


/* virtual */
BMediaEncoder::~BMediaEncoder()
{
	UNIMPLEMENTED();
}


status_t 
BMediaEncoder::InitCheck() const
{
	UNIMPLEMENTED();

	return B_OK;
}

status_t 
BMediaEncoder::SetTo(const media_format *output_format)
{
	UNIMPLEMENTED();
	return B_OK;
}

status_t 
BMediaEncoder::SetTo(const media_codec_info *mci)
{
	UNIMPLEMENTED();
	return B_OK;
}


status_t 
BMediaEncoder::SetFormat(media_format *input_format,
						 media_format *output_format,
						 media_file_format *mfi)
{
	UNIMPLEMENTED();
	return B_OK;
}

status_t 
BMediaEncoder::Encode(const void *buffer, 
					  int64 frame_count,
					  media_encode_info *info)
{
	UNIMPLEMENTED();
	return B_OK;
}


status_t 
BMediaEncoder::GetEncodeParameters(encode_parameters *parameters) const
{
	UNIMPLEMENTED();
	return B_OK;
}


status_t 
BMediaEncoder::SetEncodeParameters(encode_parameters *parameters)
{
	UNIMPLEMENTED();
	return B_OK;
}


/*************************************************************
 * protected BMediaEncoder
 *************************************************************/

/* virtual */ status_t 
BMediaEncoder::AddTrackInfo(uint32 code, const char *data, size_t size)
{
	UNIMPLEMENTED();
	return B_OK;
}


/*************************************************************
 * private BMediaEncoder
 *************************************************************/

/*
//	unimplemented
BMediaEncoder::BMediaEncoder(const BMediaEncoder &);
BMediaEncoder::BMediaEncoder & operator=(const BMediaEncoder &);
*/

/* static */ status_t 
BMediaEncoder::write_chunk(void *classptr, 
						   const void *chunk_data,
						   size_t chunk_len, 
						   media_encode_info *info)
{
	UNIMPLEMENTED();
	return B_OK;
}


void
BMediaEncoder::Init()
{
	UNIMPLEMENTED();
}


void 
BMediaEncoder::ReleaseEncoder()
{
	UNIMPLEMENTED();
}

status_t BMediaEncoder::_Reserved_BMediaEncoder_0(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_1(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_2(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_3(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_4(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_5(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_6(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_7(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_8(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_9(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_10(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_11(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_12(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_13(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_14(int32 arg, ...) { return B_ERROR; }
status_t BMediaEncoder::_Reserved_BMediaEncoder_15(int32 arg, ...) { return B_ERROR; }

/*************************************************************
 * public BMediaBufferEncoder
 *************************************************************/

BMediaBufferEncoder::BMediaBufferEncoder()
{
	UNIMPLEMENTED();
}


BMediaBufferEncoder::BMediaBufferEncoder(const media_format *output_format)
{
	UNIMPLEMENTED();
}


BMediaBufferEncoder::BMediaBufferEncoder(const media_codec_info *mci)
{
	UNIMPLEMENTED();
}


status_t
BMediaBufferEncoder::EncodeToBuffer(void *output_buffer,
									size_t *output_size,
									const void *input_buffer,
									int64 frame_count,
									media_encode_info *info)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


/*************************************************************
 * public BMediaBufferEncoder
 *************************************************************/

status_t
BMediaBufferEncoder::WriteChunk(const void *chunk_data,
								size_t chunk_len,
								media_encode_info *info)
{
	UNIMPLEMENTED();

	return B_ERROR;
}


