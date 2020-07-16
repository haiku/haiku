/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Emmanuel Gil Peyrot
 */


#include "AVIFTranslator.h"

#include <BufferIO.h>
#include <Catalog.h>
#include <Messenger.h>
#include <TranslatorRoster.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avif/avif.h"

#include "ConfigView.h"
#include "TranslatorSettings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AVIFTranslator"


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
		AVIF_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		AVIF_IN_QUALITY,
		AVIF_IN_CAPABILITY,
		"image/avif",
		"AV1 Image File Format"
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_IN_QUALITY,
		BITS_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (AVIFTranslator)"
	},
};


// The output formats that this translator knows how to write
static const translation_format sOutputFormats[] = {
	{
		AVIF_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		AVIF_OUT_QUALITY,
		AVIF_OUT_CAPABILITY,
		"image/avif",
		"AV1 Image File Format"
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_OUT_QUALITY,
		BITS_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (AVIFTranslator)"
	},
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
	{ B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false },
	{ B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false },
	{ AVIF_SETTING_LOSSLESS, TRAN_SETTING_BOOL, false },
	{ AVIF_SETTING_PIXEL_FORMAT, TRAN_SETTING_INT32,
		AVIF_PIXEL_FORMAT_YUV444 },
	{ AVIF_SETTING_QUALITY, TRAN_SETTING_INT32, 60 },
	{ AVIF_SETTING_SPEED, TRAN_SETTING_INT32, -1 },
	{ AVIF_SETTING_TILES_HORIZONTAL, TRAN_SETTING_INT32, 1 },
	{ AVIF_SETTING_TILES_VERTICAL, TRAN_SETTING_INT32, 1 },
};

const uint32 kNumInputFormats = sizeof(sInputFormats) /
	sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) /
	sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) /
	sizeof(TranSetting);


//	#pragma mark -


AVIFTranslator::AVIFTranslator()
	:
	BaseTranslator(B_TRANSLATE("AVIF images"),
		B_TRANSLATE("AVIF image translator"),
		AVIF_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"AVIFTranslator_Settings", sDefaultSettings,
		kNumDefaultSettings, B_TRANSLATOR_BITMAP, AVIF_IMAGE_FORMAT)
{
}


AVIFTranslator::~AVIFTranslator()
{
}


status_t
AVIFTranslator::DerivedIdentify(BPositionIO* stream,
	const translation_format* format, BMessage* settings,
	translator_info* info, uint32 outType)
{
	(void)format;
	(void)settings;
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	// Read header and first chunck bytes...
	uint32 buf[64];
	ssize_t size = sizeof(buf);
	if (stream->Read(buf, size) != size)
		return B_IO_ERROR;

	// Check it's a valid AVIF format
	avifROData data;
	data.data = reinterpret_cast<const uint8_t*>(buf);
	data.size = static_cast<size_t>(size);
	if (!avifPeekCompatibleFileType(&data))
		return B_ILLEGAL_DATA;

	info->type = AVIF_IMAGE_FORMAT;
	info->group = B_TRANSLATOR_BITMAP;
	info->quality = AVIF_IN_QUALITY;
	info->capability = AVIF_IN_CAPABILITY;
	snprintf(info->name, sizeof(info->name), B_TRANSLATE("AVIF image"));
	strcpy(info->MIME, "image/avif");

	return B_OK;
}


status_t
AVIFTranslator::DerivedTranslate(BPositionIO* stream,
	const translator_info* info, BMessage* ioExtension, uint32 outType,
	BPositionIO* target, int32 baseType)
{
	(void)info;
	if (baseType == 1)
		// if stream is in bits format
		return _TranslateFromBits(stream, ioExtension, outType, target);
	else if (baseType == 0)
		// if stream is NOT in bits format
		return _TranslateFromAVIF(stream, ioExtension, outType, target);
	else
		// if BaseTranslator dit not properly identify the data as
		// bits or not bits
		return B_NO_TRANSLATOR;
}


BView*
AVIFTranslator::NewConfigView(TranslatorSettings* settings)
{
	return new ConfigView(settings);
}


status_t
AVIFTranslator::_TranslateFromBits(BPositionIO* stream, BMessage* ioExtension,
	uint32 outType, BPositionIO* target)
{
	// FIXME: This codepath is completely untested for now, due to libavif
	// being built without any encoder in haikuports.

	(void)ioExtension;
	if (!outType)
		outType = AVIF_IMAGE_FORMAT;
	if (outType != AVIF_IMAGE_FORMAT)
		return B_NO_TRANSLATOR;

	TranslatorBitmap bitsHeader;
	status_t status;

	status = identify_bits_header(stream, NULL, &bitsHeader);
	if (status != B_OK)
		return status;

	avifPixelFormat format = static_cast<avifPixelFormat>(
		fSettings->SetGetInt32(AVIF_SETTING_PIXEL_FORMAT));
	int32 bytesPerPixel;
	avifRGBFormat rgbFormat;
	bool isRGB = true;
	bool ignoreAlpha = false;
	switch (bitsHeader.colors) {
		case B_RGB32:
			rgbFormat = AVIF_RGB_FORMAT_BGRA;
			ignoreAlpha = true;
			bytesPerPixel = 4;
			break;

		case B_RGB32_BIG:
			rgbFormat = AVIF_RGB_FORMAT_ARGB;
			ignoreAlpha = true;
			bytesPerPixel = 4;
			break;

		case B_RGBA32:
			rgbFormat = AVIF_RGB_FORMAT_BGRA;
			bytesPerPixel = 4;
			break;

		case B_RGBA32_BIG:
			rgbFormat = AVIF_RGB_FORMAT_ARGB;
			bytesPerPixel = 4;
			break;

		case B_RGB24:
			rgbFormat = AVIF_RGB_FORMAT_BGR;
			bytesPerPixel = 3;
			break;

		case B_RGB24_BIG:
			rgbFormat = AVIF_RGB_FORMAT_RGB;
			bytesPerPixel = 3;
			break;

		case B_YCbCr444:
			bytesPerPixel = 3;
			isRGB = false;
			break;

		case B_GRAY8:
			bytesPerPixel = 1;
			isRGB = false;
			break;

		default:
			printf("ERROR: Colorspace not supported: %d\n",
				bitsHeader.colors);
			return B_NO_TRANSLATOR;
	}

	int width = bitsHeader.bounds.IntegerWidth() + 1;
	int height = bitsHeader.bounds.IntegerHeight() + 1;
	int depth = 8;

	avifImage* image = avifImageCreate(width, height, depth, format);
	image->colorPrimaries = AVIF_COLOR_PRIMARIES_BT709;
	image->transferCharacteristics = AVIF_TRANSFER_CHARACTERISTICS_SRGB;
	image->matrixCoefficients = AVIF_MATRIX_COEFFICIENTS_IDENTITY;

	if (isRGB) {
		image->yuvRange = AVIF_RANGE_FULL;

		avifRGBImage rgb;
		avifRGBImageSetDefaults(&rgb, image);
		rgb.depth = depth;
		rgb.format = rgbFormat;
		rgb.ignoreAlpha = ignoreAlpha;
		int bitsSize = height * bitsHeader.rowBytes;
		rgb.pixels = static_cast<uint8_t*>(malloc(bitsSize));
		if (rgb.pixels == NULL)
			return B_NO_MEMORY;
		rgb.rowBytes = bitsHeader.rowBytes;

		if (stream->Read(rgb.pixels, bitsSize) != bitsSize) {
			free(rgb.pixels);
			return B_IO_ERROR;
		}

		avifResult conversionResult = avifImageRGBToYUV(image, &rgb);
		free(rgb.pixels);
		if (conversionResult != AVIF_RESULT_OK)
			return B_ERROR;
	} else if (bytesPerPixel == 3) {
		// TODO: Investigate moving that to libavif instead, and do so
		// for other Y'CbCr formats too.
		//
		// See also https://github.com/AOMediaCodec/libavif/pull/235
		assert(bitsHeader.colors == B_YCbCr444);
		int bitsSize = height * bitsHeader.rowBytes;
		uint8_t* pixels = static_cast<uint8_t*>(malloc(bitsSize));
		if (stream->Read(pixels, bitsSize) != bitsSize)
			return B_IO_ERROR;

		uint8_t* luma = static_cast<uint8_t*>(malloc(bitsSize / 3));
		uint8_t* cb = static_cast<uint8_t*>(malloc(bitsSize / 3));
		uint8_t* cr = static_cast<uint8_t*>(malloc(bitsSize / 3));

		for (int i = 0; i < bitsSize / 3; ++i) {
			luma[i] = pixels[3 * i + 0];
			cb[i] = pixels[3 * i + 1];
			cr[i] = pixels[3 * i + 2];
		}

		image->yuvPlanes[0] = luma;
		image->yuvPlanes[1] = cb;
		image->yuvPlanes[2] = cr;

		image->yuvRowBytes[0] = bitsHeader.rowBytes / 3;
		image->yuvRowBytes[1] = bitsHeader.rowBytes / 3;
		image->yuvRowBytes[2] = bitsHeader.rowBytes / 3;

		image->yuvRange = AVIF_RANGE_LIMITED;
	} else {
		assert(bitsHeader.colors == B_GRAY8);
		int bitsSize = height * bitsHeader.rowBytes;
		uint8_t* luma = static_cast<uint8_t*>(malloc(bitsSize));
		if (stream->Read(luma, bitsSize) != bitsSize)
			return B_IO_ERROR;

		image->yuvPlanes[0] = luma;
		image->yuvPlanes[1] = nullptr;
		image->yuvPlanes[2] = nullptr;

		image->yuvRowBytes[0] = bitsHeader.rowBytes;
		image->yuvRowBytes[1] = 0;
		image->yuvRowBytes[2] = 0;
	}

	avifRWData output = AVIF_DATA_EMPTY;
	avifEncoder* encoder = avifEncoderCreate();

	system_info info;
	encoder->maxThreads = (get_system_info(&info) == B_OK) ?
		info.cpu_count : 1;

	if (fSettings->SetGetBool(AVIF_SETTING_LOSSLESS)) {
		encoder->minQuantizer = AVIF_QUANTIZER_LOSSLESS;
		encoder->maxQuantizer = AVIF_QUANTIZER_LOSSLESS;
	} else {
		encoder->minQuantizer = encoder->maxQuantizer
			= fSettings->SetGetInt32(AVIF_SETTING_QUALITY);
	}
	encoder->speed = fSettings->SetGetInt32(AVIF_SETTING_SPEED);
	encoder->tileColsLog2
		= fSettings->SetGetInt32(AVIF_SETTING_TILES_HORIZONTAL);
	encoder->tileRowsLog2
		= fSettings->SetGetInt32(AVIF_SETTING_TILES_VERTICAL);

	avifResult encodeResult = avifEncoderWrite(encoder, image, &output);
	avifImageDestroy(image);
	avifEncoderDestroy(encoder);

	if (encodeResult != AVIF_RESULT_OK) {
		printf("ERROR: Failed to encode: %s\n",
			avifResultToString(encodeResult));
		avifRWDataFree(&output);
		return B_ERROR;
	}

	// output contains a valid .avif file's contents
	target->Write(output.data, output.size);
	avifRWDataFree(&output);
	return B_OK;
}


status_t
AVIFTranslator::_TranslateFromAVIF(BPositionIO* stream, BMessage* ioExtension,
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

	avifDecoder* decoder = avifDecoderCreate();
	if (decoder == NULL) {
		free(streamData);
		return B_NO_MEMORY;
	}

	avifResult setIOMemoryResult = avifDecoderSetIOMemory(decoder,
		reinterpret_cast<const uint8_t *>(streamData), streamSize);
	if (setIOMemoryResult != AVIF_RESULT_OK) {
		free(streamData);
		return B_NO_MEMORY;
	}

	avifResult decodeResult = avifDecoderParse(decoder);
	if (decodeResult != AVIF_RESULT_OK) {
		free(streamData);
		return B_ILLEGAL_DATA;
	}

	// We don’t support animations yet.
	if (decoder->imageCount != 1) {
		free(streamData);
		return B_ILLEGAL_DATA;
	}

	avifResult nextImageResult = avifDecoderNextImage(decoder);
	free(streamData);
	if (nextImageResult != AVIF_RESULT_OK)
		return B_ILLEGAL_DATA;

	avifImage* image = decoder->image;
	int width = image->width;
	int height = image->height;
	avifRGBFormat format;
	uint8_t* pixels;
	uint32_t rowBytes;
	color_space colors;

	bool convertToRGB = true;
	if (image->alphaPlane) {
		format = AVIF_RGB_FORMAT_BGRA;
		colors = B_RGBA32;
	} else if (image->yuvFormat == AVIF_PIXEL_FORMAT_YUV400) {
		colors = B_GRAY8;
		convertToRGB = false;
	} else {
		format = AVIF_RGB_FORMAT_BGR;
		colors = B_RGB24;
	}

	if (convertToRGB) {
		avifRGBImage rgb;
		avifRGBImageSetDefaults(&rgb, image);
		rgb.depth = 8;
		rgb.format = format;

		avifRGBImageAllocatePixels(&rgb);
		avifResult conversionResult = avifImageYUVToRGB(image, &rgb);
		if (conversionResult != AVIF_RESULT_OK)
			return B_ILLEGAL_DATA;

		pixels = rgb.pixels;
		rowBytes = rgb.rowBytes;
	} else {
		// TODO: Add a downsampling (with dithering?) path here, or
		// alternatively add support for higher bit depth to Haiku
		// bitmaps, possibly with HDR too.
		if (image->depth > 8)
			return B_ILLEGAL_DATA;

		// TODO: Add support for more than just the luma plane.
		pixels = image->yuvPlanes[0];
		rowBytes = image->yuvRowBytes[0];
	}

	uint32 dataSize = rowBytes * height;

	TranslatorBitmap bitmapHeader;
	bitmapHeader.magic = B_TRANSLATOR_BITMAP;
	bitmapHeader.bounds.Set(0, 0, width - 1, height - 1);
	bitmapHeader.rowBytes = rowBytes;
	bitmapHeader.colors = colors;
	bitmapHeader.dataSize = dataSize;

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
		ioExtension->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY,
			&headerOnly);

	if (headerOnly)
		return B_OK;

	bytesWritten = target->Write(pixels, dataSize);
	if (bytesWritten < B_OK)
		return bytesWritten;
	return B_OK;
}


//	#pragma mark -


BTranslator*
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	(void)you;
	(void)flags;
	if (n != 0)
		return NULL;

	return new AVIFTranslator();
}
