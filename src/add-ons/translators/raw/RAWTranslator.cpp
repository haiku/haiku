/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "RAWTranslator.h"
#include "ConfigView.h"
#include "RAW.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// Extensions that ShowImage supports
const char *kDocumentCount = "/documentCount";
const char *kDocumentIndex = "/documentIndex";

// The input formats that this translator supports.
translation_format sInputFormats[] = {
	{
		RAW_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		RAW_IN_QUALITY,
		RAW_IN_CAPABILITY,
		"image/x-vnd.adobe-dng",
		"Adobe Digital Negative"
	},
	{
		RAW_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		RAW_IN_QUALITY,
		RAW_IN_CAPABILITY,
		"image/x-vnd.photo-raw",
		"Digital Photo RAW image"
	},
};

// The output formats that this translator supports.
translation_format sOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_OUT_QUALITY,
		BITS_OUT_CAPABILITY,
		"x-be-bitmap",
		"Be Bitmap image"
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


RAWTranslator::RAWTranslator()
	: BaseTranslator("Raw Images", "Raw Image Translator",
		RAW_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"RawTranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, RAW_IMAGE_FORMAT)
{
}


RAWTranslator::~RAWTranslator()
{
}


status_t
RAWTranslator::DerivedIdentify(BPositionIO *stream,
	const translation_format *format, BMessage *settings,
	translator_info *info, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	DCRaw raw(*stream);
	status_t status;

	try {
		status = raw.Identify();
	} catch (status_t error) {
		status = error;
	}

	if (status < B_OK)
		return B_NO_TRANSLATOR;

	image_meta_info meta;
	raw.GetMetaInfo(meta);

	if (settings) {
		int32 count = raw.CountImages();

		// Add page count to ioExtension
		settings->RemoveName(kDocumentCount);
		settings->AddInt32(kDocumentCount, count);

		// Check if a document index has been specified
		int32 index;
		if (settings->FindInt32(kDocumentIndex, &index) == B_OK)
			index--;
		else
			index = 0;

		if (index < 0 || index >= count)
			return B_NO_TRANSLATOR;
	}

	info->type = RAW_IMAGE_FORMAT;
	info->group = B_TRANSLATOR_BITMAP;
	info->quality = RAW_IN_QUALITY;
	info->capability = RAW_IN_CAPABILITY;
	snprintf(info->name, sizeof(info->name), "%s RAW image", meta.manufacturer);
	strcpy(info->MIME, "image/x-vnd.photo-raw");

	return B_OK;
}


status_t
RAWTranslator::DerivedTranslate(BPositionIO* source,
	const translator_info* info, BMessage* settings,
	uint32 outType, BPositionIO* target, int32 baseType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP || baseType != 0)
		return B_NO_TRANSLATOR;

	DCRaw raw(*source);

	int32 imageIndex = 0;
	uint8* buffer = NULL;
	size_t bufferSize;
	status_t status;

	try {
		status = raw.Identify();

		if (status == B_OK && settings) {
			// Check if a document index has been specified	
			if (settings->FindInt32(kDocumentIndex, &imageIndex) == B_OK)
				imageIndex--;
			else
				imageIndex = 0;
	
			if (imageIndex < 0 || imageIndex >= (int32)raw.CountImages())
				status = B_BAD_VALUE;
		}
		if (status == B_OK)
			status = raw.ReadImageAt(imageIndex, buffer, bufferSize);
	} catch (status_t error) {
		status = error;
	}

	if (status < B_OK)
		return B_NO_TRANSLATOR;

	image_meta_info meta;
	raw.GetMetaInfo(meta);

	image_data_info data;
	raw.ImageAt(imageIndex, data);

	if (!data.is_raw)
		return B_NO_TRANSLATOR;

	uint32 dataSize = data.output_width * 4 * data.output_height;

	TranslatorBitmap header;
	header.magic = B_TRANSLATOR_BITMAP;
	header.bounds.Set(0, 0, data.output_width - 1, data.output_height - 1);
	header.rowBytes = data.output_width * 4;
	header.colors = B_RGB32;
	header.dataSize = dataSize;

	// write out Be's Bitmap header
	swap_data(B_UINT32_TYPE, &header, sizeof(TranslatorBitmap),
		B_SWAP_HOST_TO_BENDIAN);
	target->Write(&header, sizeof(TranslatorBitmap));

	ssize_t bytesWritten = target->Write(buffer, dataSize);
	if (bytesWritten < B_OK)
		return bytesWritten;

	if ((size_t)bytesWritten != dataSize)
		return B_IO_ERROR;

	return B_OK;
}


BView *
RAWTranslator::NewConfigView(TranslatorSettings *settings)
{
	return new ConfigView(BRect(0, 0, 225, 175));
}


//	#pragma mark -


BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (n != 0)
		return NULL;

	return new RAWTranslator();
}

