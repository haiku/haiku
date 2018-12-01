/* Copyright 2005-2009 SHINTA
 * Distributed under the terms of the MIT license
 */


#include <InterfaceDefs.h>
#include <MediaIO.h>

#include "APEReader.h"
#include "MACLib.h"


B_DECLARE_CODEC_KIT_PLUGIN(
	TAPEReaderPlugin,
	"ape_reader",
	B_CODEC_KIT_PLUGIN_VERSION
);


static const char*	kCopyrightString
	= "Copyright " B_UTF8_COPYRIGHT " 2005-2009 by SHINTA";


TAPEReader::TAPEReader()
	: SUPER()
{
	mDecodedData = NULL;
	mDecomp = NULL;
	Unset();
}


TAPEReader::~TAPEReader()
{
}


status_t
TAPEReader::AllocateCookie(int32 oStreamNumber, void** oCookie)
{
	*oCookie = NULL;
	return B_OK;
}


const char*
TAPEReader::Copyright()
{
	return kCopyrightString;
}


bigtime_t
TAPEReader::CurrentTime() const
{
	return mDecomp->GetInfo(APE_DECOMPRESS_CURRENT_MS)
		* static_cast<bigtime_t>(1000);
}


status_t
TAPEReader::FreeCookie(void* oCookie)
{
	return B_OK;
}


void
TAPEReader::GetFileFormatInfo(media_file_format* oMFF)
{
	oMFF->capabilities = media_file_format::B_READABLE
			| media_file_format::B_PERFECTLY_SEEKABLE
//			| media_file_format::B_IMPERFECTLY_SEEKABLE
			| media_file_format::B_KNOWS_RAW_AUDIO
			| media_file_format::B_KNOWS_ENCODED_AUDIO;
	oMFF->family = B_ANY_FORMAT_FAMILY;
	oMFF->version = MEDIA_FILE_FORMAT_VERSION;
	strcpy(oMFF->mime_type, MIME_TYPE_APE);
	strcpy(oMFF->pretty_name, MIME_TYPE_APE_LONG_DESCRIPTION);
	strcpy(oMFF->short_name, MIME_TYPE_APE_SHORT_DESCRIPTION);
	strcpy(oMFF->file_extension, MIME_TYPE_APE_EXTENSION);
}


status_t
TAPEReader::GetNextChunk(void* oCookie, const void** oChunkBuffer,
	size_t* oChunkSize, media_header* oMediaHeader)
{
	int64		aOutSize;

	// check whether song is finished or not
	if (mReadPosTotal - mReadPos + mPlayPos >= mDataSize)
		return B_ERROR;

	// reading data
	if (mPlayPos >= mReadPos )
		ReadBlocks();

	// passing data
	if (mReadPos-mPlayPos >= BUFFER_SIZE)
		aOutSize = BUFFER_SIZE;
	else
		aOutSize = mReadPos-mPlayPos;

	*oChunkBuffer = &mDecodedData[mPlayPos];
	mPlayPos += aOutSize;

	// passing info
	*oChunkSize = aOutSize;
	oMediaHeader->start_time = CurrentTime();
	oMediaHeader->file_pos = mPlayPos;
	return B_OK;
}


status_t
TAPEReader::GetStreamInfo(void* oCookie, int64* oFrameCount,
	bigtime_t* oDuration, media_format* oFormat, const void** oInfoBuffer,
	size_t* oInfoSize)
{
	if (LoadAPECheck() != B_OK)
		return LoadAPECheck();

	*oFrameCount = mDataSize / (mDecomp->GetInfo(APE_INFO_BITS_PER_SAMPLE) / 8
			* mDecomp->GetInfo(APE_INFO_CHANNELS));
	*oDuration = mDecomp->GetInfo(APE_INFO_LENGTH_MS)
		* static_cast<bigtime_t>(1000);
	// media_format
	oFormat->type = B_MEDIA_RAW_AUDIO;
	oFormat->u.raw_audio.frame_rate = mDecomp->GetInfo(APE_INFO_SAMPLE_RATE);
	oFormat->u.raw_audio.channel_count = mDecomp->GetInfo(APE_INFO_CHANNELS);
	if ( mDecomp->GetInfo(APE_INFO_BITS_PER_SAMPLE) == 16 )
		oFormat->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	else
		oFormat->u.raw_audio.format = media_raw_audio_format::B_AUDIO_UCHAR;

	oFormat->u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	oFormat->u.raw_audio.buffer_size = BUFFER_SIZE;
	oInfoBuffer = NULL;
	oInfoSize = NULL;
	return B_OK;
}


status_t
TAPEReader::LoadAPECheck() const
{
	return mLoadAPECheck;
}


status_t
TAPEReader::ReadBlocks()
{
	int aBlocksRead;
	int aRetVal = 0;

	aRetVal = mDecomp->GetData(reinterpret_cast<char*>(mDecodedData),
		BLOCK_COUNT, &aBlocksRead);
	if (aRetVal != ERROR_SUCCESS)
		return B_ERROR;

	mPlayPos = 0;
	mReadPos = aBlocksRead*mDecomp->GetInfo(APE_INFO_BLOCK_ALIGN);
	mReadPosTotal += mReadPos;
	return B_OK;
}


status_t
TAPEReader::FindKeyFrame(void* cookie, uint32 flags, int64* frame,
	bigtime_t* time)
{
	if (flags & B_MEDIA_SEEK_TO_FRAME) {
		*time = *frame * 1000 /  mDecomp->GetInfo(APE_DECOMPRESS_TOTAL_BLOCKS)
			* mDecomp->GetInfo(APE_DECOMPRESS_LENGTH_MS);
		printf("FindKeyFrame for frame %Ld: %Ld\n", *frame, *time);
	} else if (flags & B_MEDIA_SEEK_TO_TIME) {
		*frame = (*time) / 1000 * mDecomp->GetInfo(APE_DECOMPRESS_TOTAL_BLOCKS)
			/ mDecomp->GetInfo(APE_DECOMPRESS_LENGTH_MS);
		printf("FindKeyFrame for time %Ld: %Ld\n", *time, *frame);
	} else
		return B_ERROR;

	return B_OK;
}


status_t
TAPEReader::Seek(void *cookie, uint32 flags, int64 *frame, bigtime_t *time)
{
	int32 aNewBlock;

	if (flags & B_MEDIA_SEEK_TO_FRAME) {
		printf("Seek to frame %Ld\n", *frame);
		aNewBlock = *frame;
	} else if (flags & B_MEDIA_SEEK_TO_TIME) {
		printf("Seek for time %Ld\n", *time);
		aNewBlock = (*time) / 1000 * mDecomp->GetInfo(APE_DECOMPRESS_TOTAL_BLOCKS)
			/ mDecomp->GetInfo(APE_DECOMPRESS_LENGTH_MS);
	} else
		return B_ERROR;

	int64 aNewTime = aNewBlock * mDecomp->GetInfo(APE_INFO_BLOCK_ALIGN);
	if (mReadPosTotal - mReadPos < aNewTime && mReadPosTotal > aNewTime) {
		// Requested seek frame is already in the current buffer, no need to
		// actually seek, just set the play position
		mPlayPos = aNewTime - mReadPosTotal + mReadPos;
	} else {
		mReadPosTotal = aNewBlock * mDecomp->GetInfo(APE_INFO_BLOCK_ALIGN);
		mDecomp->Seek(aNewBlock);
		ReadBlocks();
	}
	return B_OK;
}


status_t
TAPEReader::Sniff(int32* oStreamCount)
{
	Unset();
	// prepare about file
	mSrcPIO = dynamic_cast<BPositionIO*>(Source());
	if (mSrcPIO == NULL)
		return B_ERROR;

	BMediaIO* mediaIO = dynamic_cast<BMediaIO*>(Source());
	if (mediaIO != NULL) {
		int32 flags = 0;
		mediaIO->GetFlags(&flags);
		// This plugin doesn't support streamed data.
		// The APEHeader::FindDescriptor function always
		// analyze the whole file to find the APE_DESCRIPTOR.
		if ((flags & B_MEDIA_STREAMING) == true)
			return B_ERROR;
	}

	int nFunctionRetVal = ERROR_SUCCESS;
	mPositionBridgeIO.SetPositionIO(mSrcPIO);

	mDecomp = CreateIAPEDecompressEx(&mPositionBridgeIO, &nFunctionRetVal);
	if (mDecomp == NULL || nFunctionRetVal != ERROR_SUCCESS)
		return B_ERROR;

	// prepare about data
	mDataSize = static_cast<int64>(mDecomp->GetInfo(APE_DECOMPRESS_TOTAL_BLOCKS))
			*mDecomp->GetInfo(APE_INFO_BLOCK_ALIGN);
	mDecodedData = new char [max_c(BUFFER_SIZE*mDecomp->GetInfo(APE_INFO_CHANNELS),
			BLOCK_COUNT*mDecomp->GetInfo(APE_INFO_BLOCK_ALIGN))];
	mLoadAPECheck = B_OK;
	*oStreamCount = 1;
	return B_OK;
}


void
TAPEReader::Unset()
{
	mLoadAPECheck = B_NO_INIT;
	// about file
	mPositionBridgeIO.SetPositionIO(NULL);
	mSrcPIO = NULL;
	delete mDecomp;
	// about data
	mDataSize = 0;
	mReadPos = 0;
	mReadPosTotal = 0;
	mPlayPos = 0;
	delete [] mDecodedData;
}


TAPEReaderPlugin::TAPEReaderPlugin()
{
}


TAPEReaderPlugin::~TAPEReaderPlugin()
{
}


BReader*
TAPEReaderPlugin::NewReader()
{
	return new TAPEReader();
}
