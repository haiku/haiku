/*
 * Copyright 2010-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include "WebPTranslator.h"

#include <BufferIO.h>
#include <Catalog.h>
#include <Messenger.h>
#include <TranslatorRoster.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "webp/encode.h"
#include "webp/decode.h"

#include "ConfigView.h"
#include "TranslatorSettings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WebPTranslator"


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



// The input formats that this translator knows how to read
static const translation_format sInputFormats[] = {
	{
		WEBP_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		WEBP_IN_QUALITY,
		WEBP_IN_CAPABILITY,
		"image/webp",
		"WebP image"
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_IN_QUALITY,
		BITS_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (WebPTranslator)"
	},
};

// The output formats that this translator knows how to write
static const translation_format sOutputFormats[] = {
	{
		WEBP_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		WEBP_OUT_QUALITY,
		WEBP_OUT_CAPABILITY,
		"image/webp",
		"WebP image"
	},
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
static const TranSetting sDefaultSettings[] = {
	{ B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false },
	{ B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false },
	{ WEBP_SETTING_QUALITY, TRAN_SETTING_INT32, 60 },
	{ WEBP_SETTING_PRESET, TRAN_SETTING_INT32, 0 },
	{ WEBP_SETTING_METHOD, TRAN_SETTING_INT32, 2 },
	{ WEBP_SETTING_PREPROCESSING, TRAN_SETTING_BOOL, false },
};

const uint32 kNumInputFormats = sizeof(sInputFormats) /
	sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) /
	sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) /
	sizeof(TranSetting);


//	#pragma mark -


WebPTranslator::WebPTranslator()
	: BaseTranslator(B_TRANSLATE("WebP images"),
		B_TRANSLATE("WebP image translator"),
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
	snprintf(info->name, sizeof(info->name), B_TRANSLATE("WebP image"));
	strcpy(info->MIME, "image/webp");

	return B_OK;
}


status_t
WebPTranslator::DerivedTranslate(BPositionIO* stream,
		const translator_info* info, BMessage* ioExtension,
		uint32 outType, BPositionIO* target, int32 baseType)
{
	if (baseType == 1)
		// if stream is in bits format
		return _TranslateFromBits(stream, ioExtension, outType, target);
	else if (baseType == 0)
		// if stream is NOT in bits format
		return _TranslateFromWebP(stream, ioExtension, outType, target);
	else
		// if BaseTranslator dit not properly identify the data as
		// bits or not bits
		return B_NO_TRANSLATOR;
}


BView*
WebPTranslator::NewConfigView(TranslatorSettings* settings)
{
	return new ConfigView(settings);
}


status_t
WebPTranslator::_TranslateFromBits(BPositionIO* stream, BMessage* ioExtension,
		uint32 outType, BPositionIO* target)
{
	if (!outType)
		outType = WEBP_IMAGE_FORMAT;
	if (outType != WEBP_IMAGE_FORMAT)
		return B_NO_TRANSLATOR;

	TranslatorBitmap bitsHeader;
	status_t status;

	status = identify_bits_header(stream, NULL, &bitsHeader);
	if (status != B_OK)
		return status;

	if (bitsHeader.colors == B_CMAP8) {
		// TODO: support whatever colospace by intermediate colorspace conversion
		printf("Error! Colorspace not supported\n");
		return B_NO_TRANSLATOR;
	}

	int32 bitsBytesPerPixel = 0;
	switch (bitsHeader.colors) {
		case B_RGB32:
		case B_RGB32_BIG:
		case B_RGBA32:
		case B_RGBA32_BIG:
		case B_CMY32:
		case B_CMYA32:
		case B_CMYK32:
			bitsBytesPerPixel = 4;
			break;

		case B_RGB24:
		case B_RGB24_BIG:
		case B_CMY24:
			bitsBytesPerPixel = 3;
			break;

		case B_RGB16:
		case B_RGB16_BIG:
		case B_RGBA15:
		case B_RGBA15_BIG:
		case B_RGB15:
		case B_RGB15_BIG:
			bitsBytesPerPixel = 2;
			break;

		case B_CMAP8:
		case B_GRAY8:
			bitsBytesPerPixel = 1;
			break;

		default:
			return B_ERROR;
	}

	if (bitsBytesPerPixel < 3) {
		// TODO support
		return B_NO_TRANSLATOR;
	}

	WebPPicture picture;
	WebPConfig config;

	if (!WebPPictureInit(&picture) || !WebPConfigInit(&config)) {
		printf("Error! Version mismatch!\n");
  		return B_ERROR;
	}

	WebPPreset preset = (WebPPreset)fSettings->SetGetInt32(WEBP_SETTING_PRESET);
	config.quality = (float)fSettings->SetGetInt32(WEBP_SETTING_QUALITY);

	if (!WebPConfigPreset(&config, (WebPPreset)preset, config.quality)) {
		printf("Error! Could initialize configuration with preset.");
		return B_ERROR;
	}

	config.method = fSettings->SetGetInt32(WEBP_SETTING_METHOD);
	config.preprocessing = fSettings->SetGetBool(WEBP_SETTING_PREPROCESSING);

	if (!WebPValidateConfig(&config)) {
		printf("Error! Invalid configuration.\n");
 		return B_ERROR;
	}

	picture.width = bitsHeader.bounds.IntegerWidth() + 1;
	picture.height = bitsHeader.bounds.IntegerHeight() + 1;

	int stride = bitsHeader.rowBytes;
	int bitsSize = picture.height * stride;
	uint8* bits = (uint8*)malloc(bitsSize);
	if (bits == NULL)
		return B_NO_MEMORY;

	if (stream->Read(bits, bitsSize) != bitsSize) {
		free(bits);
		return B_IO_ERROR;
	}

	if (!WebPPictureImportBGRA(&picture, bits, stride)) {
		printf("Error! WebPEncode() failed!\n");
		free(bits);
		return B_ERROR;
	}
	free(bits);

	picture.writer = _EncodedWriter;
	picture.custom_ptr = target;
	picture.stats = NULL;

	if (!WebPEncode(&config, &picture)) {
		printf("Error! WebPEncode() failed!\n");
		status = B_NO_TRANSLATOR;
	} else
		status = B_OK;

	WebPPictureFree(&picture);
	return status;
}


status_t
WebPTranslator::_TranslateFromWebP(BPositionIO* stream, BMessage* ioExtension,
		uint32 outType, BPositionIO* target)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	off_t streamLength = 0;
	stream->GetSize(&streamLength);

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
	uint8* out = WebPDecodeBGRA((const uint8*)streamData, streamSize, &width,
		&height);
	free(streamData);

	if (out == NULL)
		return B_ILLEGAL_DATA;

	FreeAllocation _(out);

 	uint32 dataSize = width * 4 * height;

	TranslatorBitmap bitmapHeader;
	bitmapHeader.magic = B_TRANSLATOR_BITMAP;
	bitmapHeader.bounds.Set(0, 0, width - 1, height - 1);
	bitmapHeader.rowBytes = width * 4;
	bitmapHeader.colors = B_RGBA32;
	bitmapHeader.dataSize = width * 4 * height;

	// write out Be's Bitmap header
	swap_data(B_UINT32_TYPE, &bitmapHeader, sizeof(TranslatorBitmap),
		B_SWAP_HOST_TO_BENDIAN);
	ssize_t bytesWritten = target->Write(&bitmapHeader,
		sizeof(TranslatorBitmap));
	if (bytesWritten < B_OK)
		return bytesWritten;

	if ((size_t)bytesWritten != sizeof(TranslatorBitmap))
		return B_IO_ERROR;

	bool headerOnly = false;
	if (ioExtension != NULL)
		ioExtension->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &headerOnly);

	if (headerOnly)
		return B_OK;

	uint32 dataLeft = dataSize;
	uint8* p = out;
	while (dataLeft) {
		bytesWritten = target->Write(p, 4);
		if (bytesWritten < B_OK)
			return bytesWritten;

		if (bytesWritten != 4)
			return B_IO_ERROR;

		p += 4;
		dataLeft -= 4;
	}

	return B_OK;
}


/* static */ int
WebPTranslator::_EncodedWriter(const uint8_t* data, size_t dataSize,
	const WebPPicture* const picture)
{
	BPositionIO* target = (BPositionIO*)picture->custom_ptr;
	return dataSize ? (target->Write(data, dataSize) == (ssize_t)dataSize) : 1;
}


//	#pragma mark -


BTranslator*
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (n != 0)
		return NULL;

	return new WebPTranslator();
}
