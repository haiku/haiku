/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaDecoder.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaDecoder.h>
#include <DecoderPlugin.h>
#include "debug.h"

/*************************************************************
 * public BMediaDecoder
 *************************************************************/

BMediaDecoder::BMediaDecoder()
{
	fDecoder = NULL;
	fDecoderPlugin = NULL;
	fInitStatus = B_NO_INIT;
}

BMediaDecoder::BMediaDecoder(const media_format *in_format,
							 const void *info,
							 size_t info_size)
{
	fDecoder = NULL;
	fDecoderPlugin = NULL;
	SetTo(in_format,info,info_size);
}

BMediaDecoder::BMediaDecoder(const media_codec_info *mci)
{
	fDecoder = NULL;
	fDecoderPlugin = NULL;
	SetTo(mci);
}

/* virtual */
BMediaDecoder::~BMediaDecoder()
{
	delete fDecoder;
}


status_t 
BMediaDecoder::InitCheck() const
{
	return fInitStatus;
}

DecoderPlugin * GetDecoderPlugin(int32 id)
{
	UNIMPLEMENTED();
	// if our decoder plugin area id is not valid,
	// ask the server for a new one.
	// retrieve the id'th plugin here.
	return NULL;
}

// ask the server for a good decoder for the arguments
// GETS THE CODEC for in_format->type + in_format->u.x.encoding
status_t 
BMediaDecoder::SetTo(const media_format *in_format,
					 const void *info,
					 size_t info_size)
{
	UNIMPLEMENTED();
	return fInitStatus = B_NO_INIT;
}

// ask the server for a good decoder for the arguments
// GETS THE CODEC for mci->id, mci->sub_id
// fails if the mci->id = 0
status_t 
BMediaDecoder::SetTo(const media_codec_info *mci)
{
	class MediaDecoderChunkProvider : public ChunkProvider {
	private:
		BMediaDecoder * fDecoder;
	public:
		MediaDecoderChunkProvider(BMediaDecoder * decoder) {
			fDecoder = decoder;
		}
		virtual status_t GetNextChunk(void **chunkBuffer, int32 *chunkSize,
	                                  media_header *mediaHeader) {
			const void ** buffer = const_cast<const void**>(chunkBuffer);
			size_t * size = reinterpret_cast<size_t*>(chunkSize);
			return fDecoder->GetNextChunk(buffer,size,mediaHeader);
		}
	} * provider = new MediaDecoderChunkProvider(this);
	if (provider == NULL) {
		return fInitStatus = B_NO_MEMORY;
	}
	DecoderPlugin * plugin = GetDecoderPlugin(mci->id);
	if (plugin == NULL) {
		return fInitStatus = B_BAD_VALUE;
	}
	Decoder * decoder = plugin->NewDecoder();
	if (decoder == NULL) {
		return fInitStatus = B_ERROR;
	}
	decoder->Setup(provider);
	delete fDecoder;
	fDecoderPluginID = mci->id;
	fDecoderPlugin = plugin;
	fDecoderID = mci->sub_id;
	fDecoder = decoder;
	return fInitStatus = B_OK;
}

/* SetInputFormat() sets the input data format to in_format.
   Unlike SetTo(), the SetInputFormat() function does not
   select a codec, so the currently-selected codec will
   continue to be used.  You should only use SetInputFormat()
   to refine the format settings if it will not require the
   use of a different decoder. */
status_t 
BMediaDecoder::SetInputFormat(const media_format *in_format,
							  const void *in_info,
							  size_t in_size)
{
	if (fInitStatus != B_OK) {
		return fInitStatus;
	}
	media_format format = *in_format;
	return fDecoder->Setup(&format,in_info,in_size);
}

/* SetOutputFormat() sets the format the decoder should output.
   On return, the output_format is changed to match the actual
   format that will be output; this can be different if you
   specified any wildcards. */
status_t 
BMediaDecoder::SetOutputFormat(media_format *output_format)
{
	if (fInitStatus != B_OK) {
		return fInitStatus;
	}
	return fDecoder->NegotiateOutputFormat(output_format);
}

/* Decodes a chunk of media data into the output buffer specified
   by out_buffer.  On return, out_frameCount is set to indicate how
   many frames of data were decoded, and out_mh is the header for
   the decoded buffer.  The media_decode_info structure info is used
   on input to specify decoding parameters.

   The amount of data decoded is part of the format determined by
   SetTo() or SetInputFormat().  For audio, it's the buffer_size.
   For video, it's one frame, which is height*row_bytes.  The data
   to be decoded will be fetched from the source by the decoder
   add-on calling the derived class' GetNextChunk() function. */
status_t 
BMediaDecoder::Decode(void *out_buffer, 
					  int64 *out_frameCount,
					  media_header *out_mh, 
					  media_decode_info *info)
{
	if (fInitStatus != B_OK) {
		return fInitStatus;
	}
	return fDecoder->Decode(out_buffer,out_frameCount,out_mh,info);
}

status_t 
BMediaDecoder::GetDecoderInfo(media_codec_info *out_info) const
{
	if (fInitStatus != B_OK) {
		return fInitStatus;
	}
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
 : BMediaDecoder()
{
	buffer_size = 0;
}

BMediaBufferDecoder::BMediaBufferDecoder(const media_format *in_format,
										 const void *info,
										 size_t info_size)
 : BMediaDecoder(in_format,info,info_size)
{
	buffer_size = 0;
}

BMediaBufferDecoder::BMediaBufferDecoder(const media_codec_info *mci)
 : BMediaDecoder(mci)
{
	buffer_size = 0;
}

status_t 
BMediaBufferDecoder::DecodeBuffer(const void *input_buffer, 
								  size_t input_size,
								  void *out_buffer, 
								  int64 *out_frameCount,
								  media_header *out_mh,
								  media_decode_info *info)
{
	buffer = input_buffer;
	buffer_size = input_size;
	return Decode(out_buffer,out_frameCount,out_mh,info);
}

/*************************************************************
 * protected BMediaBufferDecoder
 *************************************************************/

/* virtual */
status_t BMediaBufferDecoder::GetNextChunk(const void **chunkData, 
										   size_t *chunkLen,
										   media_header *mh)
{
	if (!buffer_size) {
		return B_LAST_BUFFER_ERROR;
	}
	*chunkData = buffer;
	*chunkLen = buffer_size;
	buffer_size = 0;
	return B_OK;
}
