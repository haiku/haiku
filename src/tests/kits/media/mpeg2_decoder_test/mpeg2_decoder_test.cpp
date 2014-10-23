/*
 * Copyright 2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Colin Günther, coling@gmx.de
 */


/*! Tests video stream decoding functionality of the FFMPEG decoder plugin.

	This test is designed with testing the dvb media-addon video decoding
	capability in mind. Thus we are restricting this test to MPEG2-Video.

	The test requires a MPEG2 test file at the same directory you start the
	test from. Normally there is a test file included at the same location
	this source file is located if not have a look at the git history.

	Successful completion of this test results in a series of PNG image
	files created at the same location you start the test from.

	The originally included test file results in 85 PNG images,
	representing a movie sequence with the actress Anne Hathaway.
	This test file has the following properties:
		- The first frames cannot be decoded, due to missing I-Frames
		  This is by intention, and helps identifying possible bugs in the
		  FFMPEG decoder plugin regarding decoding of video streams, where
		  the stream may start in the middle of a group of pictures (GoP).
		- encoded_video.output.first_active = 0
		- encoded_video.output.last_active = 575
		- encoded_video.output.orientation = B_VIDEO_TOP_LEFT_RIGHT
		- encoded_video.output.display.format = B_YUV420
		- encoded_video.output.display.line_width = 720
		- encoded_video.output.display.line_count = 576

	In any way, there -MUST- be no need to properly initialize those video
	properties for this test to succeed. To put it in other terms: The
	FFMPEG decoder plugin should determine those properties by its own and
	decode the video accordingly.
*/


#include <stdlib.h>
#include <string.h>

#include <AppKit.h>
#include <InterfaceKit.h>
#include <MediaKit.h>
#include <SupportKit.h>
#include <TranslationKit.h>

#include <media/Buffer.h>
#include <media/BufferGroup.h>
#include <media/MediaDecoder.h>
#include <storage/File.h>
#include <support/Errors.h>


extern "C" {
	#include "avcodec.h"

#ifdef DEBUG
	// Needed to fix debug build, otherwise the linker complains about
	// "undefined reference to `ff_log2_tab'"
	const uint8_t ff_log2_tab[256] = {0};
#endif

}	// extern "C"


const char* kTestVideoFilename = "./AVCodecTestMpeg2VideoStreamRaw";


class FileDecoder : public BMediaDecoder {
private:
	BFile* sourceFile;

public:
	FileDecoder(BFile* file) : BMediaDecoder() {
		sourceFile = file;
	}

protected:
	virtual status_t	GetNextChunk(const void **chunkData, size_t* chunkLen,
							media_header* mh) {
		static const uint kReadSizeInBytes = 4096;

		memset(mh, 0, sizeof(media_header));

		void* fileData = malloc(kReadSizeInBytes);
		ssize_t readLength = this->sourceFile->Read(fileData,
			kReadSizeInBytes);
		if (readLength < 0)
			return B_ERROR;

		if (readLength == 0)
			return B_LAST_BUFFER_ERROR;

		*chunkData = fileData;
		*chunkLen = readLength;

		return B_OK;
	}
};


media_format* CreateMpeg2MediaFormat();
media_format CreateRawMediaFormat();
void WriteBufferToImageFileAndRecycleIt(BBuffer* buffer);
BBitmap* CreateBitmapUsingMediaFormat(media_format mediaFormat);
status_t SaveBitmapAtPathUsingFormat(BBitmap* bitmap, BString* filePath,
	uint32 bitmapStorageFormat);


int
main(int argc, char* argv[])
{
	BApplication app("application/x-vnd.mpeg2-decoder-test");

	BFile* mpeg2EncodedFile = new BFile(kTestVideoFilename, O_RDONLY);
	BMediaDecoder* mpeg2Decoder = new FileDecoder(mpeg2EncodedFile);

	media_format* mpeg2MediaFormat = CreateMpeg2MediaFormat();
	mpeg2Decoder->SetTo(mpeg2MediaFormat);
	status_t settingMpeg2DecoderStatus = mpeg2Decoder->InitCheck();
	if (settingMpeg2DecoderStatus < B_OK)
		exit(1);

	media_format rawMediaFormat = CreateRawMediaFormat();
	status_t settingMpeg2DecoderOutputStatus
		= mpeg2Decoder->SetOutputFormat(&rawMediaFormat);
	if (settingMpeg2DecoderOutputStatus < B_OK)
		exit(2);

	static const uint32 kVideoBufferRequestTimeout = 20000;
	uint32 videoBufferSizeInBytesMax = 720 * 576 * 4;
	BBufferGroup* rawVideoFramesGroup
		= new BBufferGroup(videoBufferSizeInBytesMax, 4);
	BBuffer* rawVideoFrame = NULL;

	int64 rawVideoFrameCount = 0;
	media_header mh;
	while (true) {
		rawVideoFrame = rawVideoFramesGroup->RequestBuffer(
			videoBufferSizeInBytesMax, kVideoBufferRequestTimeout);
		status_t decodingVideoFrameStatus = mpeg2Decoder->Decode(
			rawVideoFrame->Data(), &rawVideoFrameCount, &mh, NULL);
		if (decodingVideoFrameStatus < B_OK) {
			rawVideoFrame->Recycle();
			break;
		}

		WriteBufferToImageFileAndRecycleIt(rawVideoFrame);
	}

	// Cleaning up
	rawVideoFramesGroup->ReclaimAllBuffers();
}


/*!	The caller takes ownership of the returned media_format value.
	Thus the caller needs to free the returned value.
	The returned value may be NULL, when there was an error.
*/
media_format*
CreateMpeg2MediaFormat()
{
	// The following code is mainly copy 'n' paste from src/add-ons/media/
	// media-add-ons/dvb/MediaFormat.cpp:GetHeaderFormatMpegVideo()

	status_t status;
	media_format_description desc;
	desc.family = B_MISC_FORMAT_FAMILY;
	desc.u.misc.file_format = 'ffmp';
	desc.u.misc.codec = CODEC_ID_MPEG2VIDEO;
	static media_format* kFailedToCreateMpeg2MediaFormat = NULL;

	BMediaFormats formats;
	status = formats.InitCheck();
	if (status < B_OK) {
		printf("formats.InitCheck failed, error %lu\n", status);
		return kFailedToCreateMpeg2MediaFormat;
	}

	media_format* mpeg2MediaFormat
		= static_cast<media_format*>(malloc(sizeof(media_format)));
	memset(mpeg2MediaFormat, 0, sizeof(media_format));
	status = formats.GetFormatFor(desc, mpeg2MediaFormat);
	if (status) {
		printf("formats.GetFormatFor failed, error %lu\n", status);
		free(mpeg2MediaFormat);
		return kFailedToCreateMpeg2MediaFormat;
	}

	return mpeg2MediaFormat;
}


media_format
CreateRawMediaFormat()
{
	media_format rawMediaFormat;
	memset(&rawMediaFormat, 0, sizeof(media_format));

	rawMediaFormat.type = B_MEDIA_RAW_VIDEO;
	rawMediaFormat.u.raw_video.display.format = B_RGB32;
	rawMediaFormat.u.raw_video.display.line_width = 720;
	rawMediaFormat.u.raw_video.display.line_count = 576;
	rawMediaFormat.u.raw_video.last_active
		= rawMediaFormat.u.raw_video.display.line_count - 1;
	rawMediaFormat.u.raw_video.display.bytes_per_row
		= rawMediaFormat.u.raw_video.display.line_width * 4;
	rawMediaFormat.u.raw_video.field_rate = 0; // wildcard
	rawMediaFormat.u.raw_video.interlace = 1;
	rawMediaFormat.u.raw_video.first_active = 0;
	rawMediaFormat.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	rawMediaFormat.u.raw_video.pixel_width_aspect = 1;
	rawMediaFormat.u.raw_video.pixel_height_aspect = 1;
	rawMediaFormat.u.raw_video.display.pixel_offset = 0;
	rawMediaFormat.u.raw_video.display.line_offset = 0;
	rawMediaFormat.u.raw_video.display.flags = 0;

	return rawMediaFormat;
}


void
WriteBufferToImageFileAndRecycleIt(BBuffer* buffer)
{
	static BBitmap* image = NULL;

	if (image == NULL) {
		// Lazy initialization
		image = CreateBitmapUsingMediaFormat(CreateRawMediaFormat());
	}

	if (image == NULL) {
		// Failed to create the image, needed for converting the buffer
		buffer->Recycle();
		return;
	}

	memcpy(image->Bits(), buffer->Data(), image->BitsLength());
	buffer->Recycle();

	static int32 imageSavedCounter = 0;
	static const char* kImageFileNameTemplate = "./mpeg2TestImage%d.png";
	BString imageFileName;
	imageFileName.SetToFormat(kImageFileNameTemplate, imageSavedCounter);

	status_t savingBitmapStatus = SaveBitmapAtPathUsingFormat(image,
		&imageFileName, B_PNG_FORMAT);
	if (savingBitmapStatus >= B_OK)
		imageSavedCounter++;
}


BBitmap*
CreateBitmapUsingMediaFormat(media_format mediaFormat)
{
	const uint32 kNoFlags = 0;
	BBitmap* creatingBitmapFailed = NULL;

	float imageWidth = mediaFormat.u.raw_video.display.line_width;
	float imageHeight = mediaFormat.u.raw_video.display.line_count;
	BRect imageFrame(0, 0, imageWidth - 1, imageHeight - 1);
	color_space imageColorSpace = mediaFormat.u.raw_video.display.format;

	BBitmap* bitmap = NULL;

	bitmap = new BBitmap(imageFrame, kNoFlags, imageColorSpace);
	if (bitmap == NULL)
		return bitmap;

	if (bitmap->InitCheck() < B_OK) {
		delete bitmap;
		return creatingBitmapFailed;
	}

	if (bitmap->IsValid() == false) {
		delete bitmap;
		return creatingBitmapFailed;
	}

	return bitmap;
}


status_t
SaveBitmapAtPathUsingFormat(BBitmap* bitmap, BString* filePath,
	uint32 bitmapStorageFormat)
{
	BFile file(*filePath, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	status_t creatingFileStatus = file.InitCheck();
	if (creatingFileStatus < B_OK)
		return creatingFileStatus;

	BBitmapStream bitmapStream(bitmap);
	static BTranslatorRoster* translatorRoster = NULL;
	if (translatorRoster == NULL)
		translatorRoster = BTranslatorRoster::Default();

	status_t writingBitmapToFileStatus = translatorRoster->Translate(
		&bitmapStream, NULL, NULL, &file, bitmapStorageFormat,
		B_TRANSLATOR_BITMAP);
	bitmapStream.DetachBitmap(&bitmap);

	return writingBitmapToFileStatus;
}
