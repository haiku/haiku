/*
 * Copyright 2015, Dario Casalinuovo
 * Copyright 2010, Oleg Krysenkov, beos344@mail.ru.
 * Copyright 2012, Fredrik Modéen, [firstname]@[lastname].se.
 * Copyright 2004-2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <MediaEncoder.h>

#include <EncoderPlugin.h>
#include <PluginManager.h>

#include <new>

#include "debug.h"

//TODO: SetTo, SetFormat, Enode Class

/*************************************************************
 * public BMediaEncoder
 *************************************************************/

BMediaEncoder::BMediaEncoder()
	:
	fEncoder(NULL),
	fInitStatus(B_NO_INIT)
{
	CALLED();
}


BMediaEncoder::BMediaEncoder(const media_format *output_format)
	:
	fEncoder(NULL),
	fInitStatus(B_NO_INIT)
{
	CALLED();
	SetTo(output_format);
}


BMediaEncoder::BMediaEncoder(const media_codec_info *mci)
	:
	fEncoder(NULL),
	fInitStatus(B_NO_INIT)
{
	CALLED();
	SetTo(mci);
}


/* virtual */
BMediaEncoder::~BMediaEncoder()
{
	CALLED();
	ReleaseEncoder();
}


status_t 
BMediaEncoder::InitCheck() const
{
	return fInitStatus;
}


status_t 
BMediaEncoder::SetTo(const media_format *output_format)
{
	CALLED();

	status_t err = B_ERROR;
	ReleaseEncoder();

	if (output_format == NULL)
		return fInitStatus;

	media_format format = *output_format;
	err = gPluginManager.CreateEncoder(&fEncoder, format);
	if (fEncoder != NULL && err == B_OK) {
		err = _AttachToEncoder();
		if (err == B_OK) {
			err = SetFormat(NULL, &format);
			if (err == B_OK) {
				fInitStatus = B_OK;
				return B_OK;
			}
		}
	}
	ReleaseEncoder();
	fInitStatus = err;
	return err;
}


status_t 
BMediaEncoder::SetTo(const media_codec_info *mci)
{
	CALLED();

	ReleaseEncoder();
	status_t err = gPluginManager.CreateEncoder(&fEncoder, mci, 0);
	if (fEncoder != NULL && err == B_OK) {
		err = _AttachToEncoder();
		if (err == B_OK) {
			fInitStatus = B_OK;
			return B_OK;
		}
	}

	ReleaseEncoder();
	fInitStatus = err;
	return err;
}


status_t 
BMediaEncoder::SetFormat(media_format *input_format,
	media_format *output_format, media_file_format *mfi)
{
	CALLED();
	TRACE("BMediaEncoder::SetFormat. Input = %d, Output = %d\n",
		input_format->type, output_format->type);

	if (!fEncoder)
		return B_NO_INIT;

	//TODO: How we support output_format and mfi?
	return fEncoder->SetUp(input_format);
}


status_t 
BMediaEncoder::Encode(const void *buffer, 
	int64 frame_count, media_encode_info *info)
{
	CALLED();

	if (!fEncoder)
		return B_NO_INIT;

	return fEncoder->Encode(buffer, frame_count, info);
}


status_t 
BMediaEncoder::GetEncodeParameters(encode_parameters *parameters) const
{
	CALLED();

	if (fEncoder == NULL)
		return B_NO_INIT;
	else
		return fEncoder->GetEncodeParameters(parameters);
}


status_t 
BMediaEncoder::SetEncodeParameters(encode_parameters *parameters)
{
	CALLED();

	if (fEncoder == NULL)
		return B_NO_INIT;
	else
		return fEncoder->SetEncodeParameters(parameters);
}


/*************************************************************
 * protected BMediaEncoder
 *************************************************************/

/* virtual */ status_t 
BMediaEncoder::AddTrackInfo(uint32 code, const char *data, size_t size)
{
	CALLED();

	if (fEncoder == NULL)
		return B_NO_INIT;
	else
		return fEncoder->AddTrackInfo(code, data, size);
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
BMediaEncoder::write_chunk(void* classptr, const void* chunk_data,
	size_t chunk_len, media_encode_info* info)
{
	CALLED();

	BMediaEncoder* encoder = static_cast<BMediaEncoder*>(classptr);
	if (encoder == NULL)
		return B_BAD_VALUE;
	return encoder->WriteChunk(chunk_data, chunk_len, info);
}


void
BMediaEncoder::Init()
{
	UNIMPLEMENTED();
}


void 
BMediaEncoder::ReleaseEncoder()
{
	CALLED();
	if (fEncoder != NULL) {
		gPluginManager.DestroyEncoder(fEncoder);
		fEncoder = NULL;
	}
	fInitStatus = B_NO_INIT;
}


status_t
BMediaEncoder::_AttachToEncoder()
{
	class MediaEncoderChunkWriter : public ChunkWriter {
		public:
			MediaEncoderChunkWriter(BMediaEncoder* encoder)
			{
				fEncoder = encoder;
			}
			virtual status_t WriteChunk(const void* chunkBuffer,
				size_t chunkSize, media_encode_info* encodeInfo)
			{
				return fEncoder->WriteChunk(chunkBuffer, chunkSize, encodeInfo);
			}
		private:
			BMediaEncoder* fEncoder;
	} *writer = new(std::nothrow) MediaEncoderChunkWriter(this);

	if (!writer)
		return B_NO_MEMORY;

	fEncoder->SetChunkWriter(writer);
	return B_OK;
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
	:
	BMediaEncoder(),
	fBuffer(NULL)
{
	CALLED();
}


BMediaBufferEncoder::BMediaBufferEncoder(const media_format *output_format)
	:
	BMediaEncoder(output_format),
	fBuffer(NULL)
{
	CALLED();
}


BMediaBufferEncoder::BMediaBufferEncoder(const media_codec_info *mci)
	:
	BMediaEncoder(mci),
	fBuffer(NULL)
{
	CALLED();
}


status_t
BMediaBufferEncoder::EncodeToBuffer(void *output_buffer,
	size_t *output_size, const void *input_buffer,
	int64 frame_count, media_encode_info *info)
{
	CALLED();

	status_t error;
	fBuffer = output_buffer;
	fBufferSize = *output_size;
	error = Encode(input_buffer, frame_count, info);
	if (fBuffer) {
		fBuffer = NULL;
		*output_size = 0;
	} else {
		*output_size = fBufferSize;
	}
	return error;
}


/*************************************************************
 * public BMediaBufferEncoder
 *************************************************************/

status_t
BMediaBufferEncoder::WriteChunk(const void *chunk_data,
	size_t chunk_len, media_encode_info *info)
{
	CALLED();

	if (fBuffer == NULL)
		return B_ENTRY_NOT_FOUND;

	if (chunk_len < 0)
		return B_ERROR;

	if (chunk_len > (size_t)fBufferSize) {
		memcpy(fBuffer, chunk_data, fBufferSize);
		fBuffer = NULL;
		return B_DEVICE_FULL;
	}

	memcpy(fBuffer, chunk_data, chunk_len);
	fBufferSize = chunk_len;
	fBuffer = NULL;
	return B_NO_ERROR;
}
