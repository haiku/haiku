/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaDecoder.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaDecoder.h>
#include <DecoderPlugin.h>
#include "PluginManager.h"
#include "DataExchange.h"
#include "debug.h"

extern PluginManager _plugin_manager;

/*************************************************************
 * public BMediaDecoder
 *************************************************************/

BMediaDecoder::BMediaDecoder()
 :	fDecoder(0),
 	fDecoderID(0),
 	fDecoderPlugin(0),
 	fDecoderPluginID(0),
 	fInitStatus(B_NO_INIT),
 	fNeedsInit(false),
 	fInitFormat(0),
 	fInitInfo(0),
 	fInitInfoSize(0),
 	fInitMCI(0)
{
}

BMediaDecoder::BMediaDecoder(const media_format *in_format,
							 const void *info,
							 size_t info_size)
 :	fDecoder(0),
 	fDecoderID(0),
 	fDecoderPlugin(0),
 	fDecoderPluginID(0),
 	fInitStatus(B_NO_INIT),
 	fNeedsInit(true),
 	fInitFormat(new media_format(*in_format)),
 	fInitInfo(0),
 	fInitInfoSize(0),
 	fInitMCI(0)
{
	if (info_size) {
		fInitInfoSize = info_size;
		fInitInfo = new char[info_size];
		memcpy(fInitInfo, info, info_size);
	}
}

BMediaDecoder::BMediaDecoder(const media_codec_info *mci)
 :	fDecoder(0),
 	fDecoderID(0),
 	fDecoderPlugin(0),
 	fDecoderPluginID(0),
 	fInitStatus(B_NO_INIT),
 	fNeedsInit(true),
 	fInitFormat(0),
 	fInitInfo(0),
 	fInitInfoSize(0),
 	fInitMCI(new media_codec_info(*mci))
{
}

/* virtual */
BMediaDecoder::~BMediaDecoder()
{
	delete fDecoder;
	delete fInitFormat;
	delete fInitInfo;
	delete fInitMCI;
}


status_t 
BMediaDecoder::InitCheck() const
{
	if (fNeedsInit)
		const_cast<BMediaDecoder *>(this)->DoLateInit();
	return fInitStatus;
}


static DecoderPlugin *
GetDecoderPlugin(const media_format * format)
{
	server_get_decoder_for_format_request request;
	server_get_decoder_for_format_reply reply;
	request.format = *format;
	status_t result = QueryServer(SERVER_GET_DECODER_FOR_FORMAT, &request, sizeof(request), &reply, sizeof(reply));
	if (result != B_OK) {
		printf("BMediaDecoder::SetTo: can't get decoder for format\n");
		return NULL;
	}
	MediaPlugin * media_plugin = _plugin_manager.GetPlugin(reply.ref);
	if (!media_plugin) {
		printf("BMediaDecoder::SetTo: GetPlugin failed\n");
		return NULL;
	}
	DecoderPlugin * plugin = dynamic_cast<DecoderPlugin *>(media_plugin);
	if (!plugin) {
		printf("BMediaDecoder::SetTo: dynamic_cast failed\n");
		return NULL;
	}
	return plugin;
}

// ask the server for a good decoder for the arguments
// GETS THE CODEC for in_format->type + in_format->u.x.encoding
status_t 
BMediaDecoder::SetTo(const media_format *in_format,
					 const void *info,
					 size_t info_size)
{
	// ToDo: should be moved into the PluginManager, or better yet, use
	//		the existing function _CreateDecoder()

	status_t result;
	fNeedsInit = false;
	fInitStatus = B_NO_INIT;
	delete fDecoder;
	DecoderPlugin * plugin = GetDecoderPlugin(in_format);
	if (plugin == NULL) {
		return fInitStatus = B_ERROR;
	}
	Decoder * decoder = plugin->NewDecoder();
	if (decoder == NULL) {
		return fInitStatus = B_ERROR;
	}
	fDecoder = decoder;
//	fDecoderID = mci->sub_id;
	result = SetInputFormat(in_format,info,info_size);
	if (result != B_OK) {
		return fInitStatus = result;
	}
	return fInitStatus = B_OK;
}

// ask the server for the id'th plugin
static DecoderPlugin *
GetDecoderPlugin(int32 id)
{
	// ToDo: should be moved into the PluginManager
	if (id == 0) {
		return NULL;
	}
	UNIMPLEMENTED();
	return NULL;
}

// ask the server for a good decoder for the arguments
// GETS THE CODEC for mci->id, mci->sub_id
// fails if the mci->id = 0
status_t 
BMediaDecoder::SetTo(const media_codec_info *mci)
{
	fNeedsInit = false;
	fInitStatus = B_NO_INIT;
	delete fDecoder;
	DecoderPlugin * plugin = GetDecoderPlugin(mci->id);
	if (plugin == NULL) {
		return fInitStatus = B_ERROR;
	}
	Decoder * decoder = plugin->NewDecoder();
	if (decoder == NULL) {
		return fInitStatus = B_ERROR;
	}
	// ToDo: what's the sub_id used for? - asks Axel.
	fDecoder = decoder;
	fDecoderID = mci->sub_id;
	return fInitStatus = B_OK;
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
	if (fNeedsInit)
		DoLateInit();
	if (fInitStatus != B_OK)
		return fInitStatus;

	printf("DISCARDING FORMAT %s\n",__PRETTY_FUNCTION__);
	media_format format = *in_format;
	return fDecoder->Setup(&format,in_info,in_size);
}


/**	SetOutputFormat() sets the format the decoder should output.
 *	On return, the output_format is changed to match the actual
 *	format that will be output; this can be different if you
 *	specified any wildcards.
 */

status_t 
BMediaDecoder::SetOutputFormat(media_format *output_format)
{
	if (fNeedsInit)
		DoLateInit();
	if (fInitStatus != B_OK)
		return fInitStatus;

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
	if (fNeedsInit)
		DoLateInit();
	if (fInitStatus != B_OK)
		return fInitStatus;

	return fDecoder->Decode(out_buffer,out_frameCount,out_mh,info);
}


status_t 
BMediaDecoder::GetDecoderInfo(media_codec_info *outInfo) const
{
	if (fNeedsInit)
		const_cast<BMediaDecoder *>(this)->DoLateInit();
	if (fInitStatus != B_OK)
		return fInitStatus;

	if (fDecoder != NULL)
		fDecoder->GetCodecInfo(outInfo);
	else {
		strcpy(outInfo->short_name, "unknown");
		strcpy(outInfo->pretty_name, "unknown");
	}

	outInfo->id = fDecoderPluginID;
	outInfo->sub_id = fDecoderID;
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

void
BMediaDecoder::DoLateInit()
{
	if (fInitFormat) {
		SetTo(fInitFormat, fInitInfo, fInitInfoSize);
		delete fInitFormat;
		delete fInitInfo;
		fInitFormat = 0;
		fInitInfo = 0;
	} else if (fInitMCI) {
		SetTo(fInitMCI);
		delete fInitMCI;
		fInitMCI = 0;
	}
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
	if (!buffer_size)
		return B_LAST_BUFFER_ERROR;

	*chunkData = buffer;
	*chunkLen = buffer_size;
	buffer_size = 0;
	return B_OK;
}
