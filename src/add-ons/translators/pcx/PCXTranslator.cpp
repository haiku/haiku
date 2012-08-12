/*
 * Copyright 2008, Jérôme Duval, korli@users.berlios.de. All rights reserved.
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "PCXTranslator.h"
#include "ConfigView.h"
#include "PCX.h"

#include <Catalog.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PCXTranslator"

#define kPCXMimeType "image/x-pcx"


// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
	{
		PCX_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		PCX_IN_QUALITY,
		PCX_IN_CAPABILITY,
		kPCXMimeType,
		"PCX image"
	}/*,
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_IN_QUALITY,
		BITS_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (PCXTranslator)"
	},*/
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
	/*{
		PCX_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		PCX_OUT_QUALITY,
		PCX_OUT_CAPABILITY,
		kPCXMimeType,
		"PCX image"
	},*/
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_OUT_QUALITY,
		BITS_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (PCXTranslator)"
	},
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
	{B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false},
	{B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false}
};

const uint32 kNumInputFormats = sizeof(sInputFormats) / sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) / sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) / sizeof(TranSetting);


PCXTranslator::PCXTranslator()
	: BaseTranslator(B_TRANSLATE("PCX images"),
		B_TRANSLATE("PCX image translator"),
		PCX_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"PCXTranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, PCX_IMAGE_FORMAT)
{
}


PCXTranslator::~PCXTranslator()
{
}


status_t
PCXTranslator::DerivedIdentify(BPositionIO *stream,
	const translation_format *format, BMessage *ioExtension,
	translator_info *info, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != PCX_IMAGE_FORMAT)
		return B_NO_TRANSLATOR;

	int32 bitsPerPixel;
	uint8 type;
	if (PCX::identify(ioExtension, *stream, type, bitsPerPixel) != B_OK)
		return B_NO_TRANSLATOR;

	info->type = PCX_IMAGE_FORMAT;
	info->group = B_TRANSLATOR_BITMAP;
	info->quality = PCX_IN_QUALITY;
	info->capability = PCX_IN_CAPABILITY;
	snprintf(info->name, sizeof(info->name), B_TRANSLATE("PCX %lu bit image"),
		bitsPerPixel);
	strcpy(info->MIME, kPCXMimeType);

	return B_OK;
}


status_t
PCXTranslator::DerivedTranslate(BPositionIO *source,
	const translator_info *info, BMessage *ioExtension,
	uint32 outType, BPositionIO *target, int32 baseType)
{
	/*if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != PCX_IMAGE_FORMAT)
		return B_NO_TRANSLATOR;*/

	switch (baseType) {
		/*case 1:
		{
			if (outType != PCX_IMAGE_FORMAT)
				return B_NO_TRANSLATOR;

			// Source is in bits format - this has to be done here, because
			// identify_bits_header() is a member of the BaseTranslator class...
			TranslatorBitmap bitsHeader;
			status_t status = identify_bits_header(source, NULL, &bitsHeader);
			if (status != B_OK)
				return status;

			return PCX::convert_bits_to_pcx(ioExtension, *source, bitsHeader, *target);
		}*/

		case 0:
		{
			// source is NOT in bits format
			if (outType != B_TRANSLATOR_BITMAP)
				return B_NO_TRANSLATOR;

			return PCX::convert_pcx_to_bits(ioExtension, *source, *target);
		}

		default:
			return B_NO_TRANSLATOR;
	}
}


BView *
PCXTranslator::NewConfigView(TranslatorSettings *settings)
{
	return new ConfigView(BRect(0, 0, 225, 175));
}


//	#pragma mark -


BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (n != 0)
		return NULL;

	return new PCXTranslator();
}

