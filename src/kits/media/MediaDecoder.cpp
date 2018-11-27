/***********************************************************************
 * AUTHOR: Andrew Bachmann, Marcus Overhagen
 *   FILE: MediaDecoder.cpp
 *  DESCR: 
 ***********************************************************************/
#include <CodecRoster.h>
#include <Decoder.h>
#include <MediaDecoder.h>

#include <new>

#include "MediaDebug.h"

using namespace BCodecKit;


/*************************************************************
 * public BMediaDecoder
 *************************************************************/

BMediaDecoder::BMediaDecoder()
 :	fDecoder(NULL),
 	fInitStatus(B_NO_INIT)
{
}


BMediaDecoder::BMediaDecoder(const media_format *in_format,
							 const void *info,
							 size_t info_size)
 :	fDecoder(NULL),
 	fInitStatus(B_NO_INIT)
{
	SetTo(in_format, info, info_size);
}


BMediaDecoder::BMediaDecoder(const media_codec_info *mci)
 :	fDecoder(NULL),
 	fInitStatus(B_NO_INIT)
{
	SetTo(mci);
}


/* virtual */
BMediaDecoder::~BMediaDecoder()
{
	BCodecRoster::ReleaseDecoder(fDecoder);
}


status_t 
BMediaDecoder::InitCheck() const
{
	return fInitStatus;
}


status_t 
BMediaDecoder::SetTo(const media_format *in_format,
					 const void *info,
					 size_t info_size)
{
	BCodecRoster::ReleaseDecoder(fDecoder);
	fDecoder = NULL;

	status_t err = BCodecRoster::InstantiateDecoder(&fDecoder, *in_format);
	if (err < B_OK)
		goto fail;

	err = AttachToDecoder();
	if (err < B_OK)
		goto fail;

	err = SetInputFormat(in_format, info, info_size);
	if (err < B_OK)
		goto fail;

	fInitStatus = B_OK;
	return B_OK;

fail:
	BCodecRoster::ReleaseDecoder(fDecoder);
	fDecoder = NULL;
	fInitStatus = B_NO_INIT;
	return err;
}


status_t 
BMediaDecoder::SetTo(const media_codec_info *mci)
{
	BCodecRoster::ReleaseDecoder(fDecoder);
	fDecoder = NULL;

	status_t err = BCodecRoster::InstantiateDecoder(&fDecoder, *mci);
	if (err < B_OK)
		goto fail;

	err = AttachToDecoder();
	if (err < B_OK)
		goto fail;

	fInitStatus = B_OK;
	return B_OK;

fail:
	BCodecRoster::ReleaseDecoder(fDecoder);
	fDecoder = NULL;
	fInitStatus = B_NO_INIT;
	return err;
}


/**	SetInputFormat() sets the input data format to in_format.
 *	Unlike SetTo(), the SetInputFormat() function does not
 *	select a codec, so the currently-selected codec will
 *	continue to be used.  You should only use SetInputFormat()
 *	to refine the format settings if it will not require the
 *	use of a different decoder.
 */

status_t 
BMediaDecoder::SetInputFormat(const media_format *in_format,
							  const void *in_info,
							  size_t in_size)
{
	if (!fDecoder)
		return B_NO_INIT;

	media_format format = *in_format;
	return fDecoder->Setup(&format, in_info, in_size);
}


/**	SetOutputFormat() sets the format the decoder should output.
 *	On return, the output_format is changed to match the actual
 *	format that will be output; this can be different if you
 *	specified any wildcards.
 */

status_t 
BMediaDecoder::SetOutputFormat(media_format *output_format)
{
	if (!fDecoder)
		return B_NO_INIT;

	return fDecoder->NegotiateOutputFormat(output_format);
}


/**	Decodes a chunk of media data into the output buffer specified
 *	by out_buffer.  On return, out_frameCount is set to indicate how
 *	many frames of data were decoded, and out_mh is the header for
 *	the decoded buffer.  The media_decode_info structure info is used
 *	on input to specify decoding parameters.
 *
 *	The amount of data decoded is part of the format determined by
 *	SetTo() or SetInputFormat().  For audio, it's the buffer_size.
 *	For video, it's one frame, which is height*row_bytes.  The data
 *	to be decoded will be fetched from the source by the decoder
 *	add-on calling the derived class' GetNextChunk() function.
 */

status_t 
BMediaDecoder::Decode(void *out_buffer, 
					  int64 *out_frameCount,
					  media_header *out_mh, 
					  media_decode_info *info)
{
	if (!fDecoder)
		return B_NO_INIT;

	return fDecoder->Decode(out_buffer, out_frameCount, out_mh, info);
}


status_t 
BMediaDecoder::GetDecoderInfo(media_codec_info *out_info) const
{
	if (!fDecoder)
		return B_NO_INIT;

	return BCodecRoster::GetDecoderInfo(fDecoder, out_info);
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

status_t
BMediaDecoder::AttachToDecoder()
{
	class MediaDecoderChunkProvider : public BChunkProvider {
	private:
		BMediaDecoder * fDecoder;
	public:
		MediaDecoderChunkProvider(BMediaDecoder * decoder) {
			fDecoder = decoder;
		}
		virtual status_t GetNextChunk(const void **chunkBuffer, size_t *chunkSize,
		                              media_header *mediaHeader) {
			return fDecoder->GetNextChunk(chunkBuffer, chunkSize, mediaHeader);
		}
	} * provider = new(std::nothrow) MediaDecoderChunkProvider(this);
	
	if (!provider)
		return B_NO_MEMORY;
	
	fDecoder->SetChunkProvider(provider);
	return B_OK;
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
 :	BMediaDecoder()
 ,	fBufferSize(0)
{
}


BMediaBufferDecoder::BMediaBufferDecoder(const media_format *in_format,
										 const void *info,
										 size_t info_size)
 :	BMediaDecoder(in_format, info, info_size)
 ,	fBufferSize(0)
{
}


BMediaBufferDecoder::BMediaBufferDecoder(const media_codec_info *mci)
 :	BMediaDecoder(mci)
 ,	fBufferSize(0)
{
}


status_t 
BMediaBufferDecoder::DecodeBuffer(const void *input_buffer, 
								  size_t input_size,
								  void *out_buffer, 
								  int64 *out_frameCount,
								  media_header *out_mh,
								  media_decode_info *info)
{
	fBuffer = input_buffer;
	fBufferSize = input_size;
	return Decode(out_buffer, out_frameCount, out_mh,info);
}


/*************************************************************
 * protected BMediaBufferDecoder
 *************************************************************/

/* virtual */
status_t
BMediaBufferDecoder::GetNextChunk(const void **chunkData,
								  size_t *chunkLen,
                                  media_header *mh)
{
	if (!fBufferSize)
		return B_LAST_BUFFER_ERROR;

	*chunkData = fBuffer;
	*chunkLen = fBufferSize;
	fBufferSize = 0;
	return B_OK;
}
