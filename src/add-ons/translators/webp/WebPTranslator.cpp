/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include "WebPTranslator.h"
#include "ConfigView.h"

#include "decode.h"

#include <BufferIO.h>
#include <Messenger.h>
#include <TranslatorRoster.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


class FreeAllocation {
	public:
		FreeAllocation(void* buffer)
			:
			fBuffer(buffer)
		{
		}

		~FreeAllocation()
		{
			free(fBuffer);
		}

	private:
		void*	fBuffer;
};



// The input formats that this translator supports.
translation_format sInputFormats[] = {
	{
		WEBP_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		WEBP_IN_QUALITY,
		WEBP_IN_CAPABILITY,
		"image/webp",
		"WebP image"
	},
};

// The output formats that this translator supports.
translation_format sOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_OUT_QUALITY,
		BITS_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (WebPTranslator)"
	},
};

// Default settings for the Translator
static TranSetting sDefaultSettings[] = {
	{B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false},
	{B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false}
};

const uint32 kNumInputFormats = sizeof(sInputFormats) / sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) / sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) / sizeof(TranSetting);


//	#pragma mark -


WebPTranslator::WebPTranslator()
	: BaseTranslator("WebP images", "WebP image translator",
		WEBP_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"WebPTranslator_Settings", sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, WEBP_IMAGE_FORMAT)
{
}


WebPTranslator::~WebPTranslator()
{
}


status_t
WebPTranslator::DerivedIdentify(BPositionIO* stream,
	const translation_format* format, BMessage* settings,
	translator_info* info, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	// Check RIFF and 'WEBPVP8 ' signatures...
	uint32 buf[4];
	ssize_t size = 16;
	if (stream->Read(buf, size) != size)
		return B_IO_ERROR;

	const uint32 kRIFFMagic = B_HOST_TO_BENDIAN_INT32('RIFF');
	const uint32 kWEBPMagic = B_HOST_TO_BENDIAN_INT32('WEBP');
	const uint32 kVP8Magic  = B_HOST_TO_BENDIAN_INT32('VP8 ');
	if (buf[0] != kRIFFMagic || buf[2] != kWEBPMagic || buf[3] != kVP8Magic)
		return B_ILLEGAL_DATA;

	info->type = WEBP_IMAGE_FORMAT;
	info->group = B_TRANSLATOR_BITMAP;
	info->quality = WEBP_IN_QUALITY;
	info->capability = WEBP_IN_CAPABILITY;
	snprintf(info->name, sizeof(info->name), "WebP image");
	strcpy(info->MIME, "image/webp");

	return B_OK;
}


status_t
WebPTranslator::DerivedTranslate(BPositionIO* stream,
		const translator_info* info, BMessage* settings,
		uint32 outType, BPositionIO* target, int32 baseType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP || baseType != 0)
		return B_NO_TRANSLATOR;

	off_t streamLength = 0;
	stream->GetSize(&streamLength);
	printf("stream GetSize(): %lld\n", streamLength);

	off_t streamSize = stream->Seek(0, SEEK_END);
	stream->Seek(0, SEEK_SET);

	void* streamData = malloc(streamSize);
	if (streamData == NULL)
		return B_NO_MEMORY;

	if (stream->Read(streamData, streamSize) != streamSize) {
		free(streamData);
		return B_IO_ERROR;
	}

	int width, height;
	uint8* out = WebPDecodeRGB((const uint8*)streamData, streamSize, &width, &height);
	free(streamData);

	if (out == NULL)
		return B_ILLEGAL_DATA;

	FreeAllocation _(out);

 	uint32 dataSize = width * 3 * height;

	TranslatorBitmap bitmapHeader;
	bitmapHeader.magic = B_TRANSLATOR_BITMAP;
	bitmapHeader.bounds.Set(0, 0, width - 1, height - 1);
	bitmapHeader.rowBytes = width * 3;
	bitmapHeader.colors = B_RGB24;
	bitmapHeader.dataSize = width * 3 * height;

	// write out Be's Bitmap header
	swap_data(B_UINT32_TYPE, &bitmapHeader, sizeof(TranslatorBitmap),
		B_SWAP_HOST_TO_BENDIAN);
	ssize_t bytesWritten = target->Write(&bitmapHeader, sizeof(TranslatorBitmap));
	if (bytesWritten < B_OK)
		return bytesWritten;

	if ((size_t)bytesWritten != sizeof(TranslatorBitmap))
		return B_IO_ERROR;

	bool headerOnly = false;
	if (settings != NULL)
		settings->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &headerOnly);

	if (headerOnly)
		return B_OK;

	uint32 dataLeft = dataSize;
	uint8* p = out;
	uint8 rgb[3];
	while (dataLeft) {
		rgb[0] = *(p+2);
		rgb[1] = *(p+1);
		rgb[2] = *(p);

		bytesWritten = target->Write(rgb, 3);
		if (bytesWritten < B_OK)
			return bytesWritten;

		if (bytesWritten != 3)
			return B_IO_ERROR;

		p += 3;
		dataLeft -= 3;
	}

	return B_OK;
}



BView*
WebPTranslator::NewConfigView(TranslatorSettings* settings)
{
	return new ConfigView();
}


//	#pragma mark -


BTranslator*
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (n != 0)
		return NULL;

	return new WebPTranslator();
}
