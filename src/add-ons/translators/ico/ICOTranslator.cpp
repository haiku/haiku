/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ICOTranslator.h"

#include <Catalog.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Catalog.h>

#include "ConfigView.h"
#include "ICO.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ICOTranslator"


const char *kDocumentCount = "/documentCount";
const char *kDocumentIndex = "/documentIndex";

#define kICOMimeType "image/vnd.microsoft.icon"
#define kICOName "Windows icon"
	// I'm lazy - structure initializers don't like const variables...


// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
	{
		ICO_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		ICO_IN_QUALITY,
		ICO_IN_CAPABILITY,
		kICOMimeType,
		kICOName
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_IN_QUALITY,
		BITS_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (ICOTranslator)"
	},
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
	{
		ICO_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		ICO_OUT_QUALITY,
		ICO_OUT_CAPABILITY,
		kICOMimeType,
		kICOName
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_OUT_QUALITY,
		BITS_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (ICOTranslator)"
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


ICOTranslator::ICOTranslator()
	: BaseTranslator(B_TRANSLATE("Windows icon images"), 
		B_TRANSLATE("Windows icon translator"),
		ICO_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"ICOTranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, ICO_IMAGE_FORMAT)
{
}


ICOTranslator::~ICOTranslator()
{
}


status_t
ICOTranslator::DerivedIdentify(BPositionIO *stream,
	const translation_format *format, BMessage *ioExtension,
	translator_info *info, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != ICO_IMAGE_FORMAT)
		return B_NO_TRANSLATOR;

	int32 bitsPerPixel;
	uint8 type;
	if (ICO::identify(ioExtension, *stream, type, bitsPerPixel) != B_OK)
		return B_NO_TRANSLATOR;

	info->type = ICO_IMAGE_FORMAT;
	info->group = B_TRANSLATOR_BITMAP;
	info->quality = ICO_IN_QUALITY;
	info->capability = ICO_IN_CAPABILITY;
	snprintf(info->name, sizeof(info->name), 
		B_TRANSLATE("Windows %s %ld bit image"),
		type == ICO::kTypeIcon ? B_TRANSLATE("Icon") : B_TRANSLATE("Cursor"), 
		bitsPerPixel);
	strcpy(info->MIME, kICOMimeType);

	return B_OK;
}


status_t
ICOTranslator::DerivedTranslate(BPositionIO *source,
	const translator_info *info, BMessage *ioExtension,
	uint32 outType, BPositionIO *target, int32 baseType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != ICO_IMAGE_FORMAT)
		return B_NO_TRANSLATOR;

	switch (baseType) {
		case 1:
		{
			if (outType != ICO_IMAGE_FORMAT)
				return B_NO_TRANSLATOR;

			// Source is in bits format - this has to be done here, because
			// identify_bits_header() is a member of the BaseTranslator class...
			TranslatorBitmap bitsHeader;
			status_t status = identify_bits_header(source, NULL, &bitsHeader);
			if (status != B_OK)
				return status;

			return ICO::convert_bits_to_ico(ioExtension, *source, bitsHeader, *target);
		}

		case 0:
		{
			// source is NOT in bits format
			if (outType != B_TRANSLATOR_BITMAP)
				return B_NO_TRANSLATOR;

			return ICO::convert_ico_to_bits(ioExtension, *source, *target);
		}

		default:
			return B_NO_TRANSLATOR;
	}
}


status_t
ICOTranslator::DerivedCanHandleImageSize(float width, float height) const
{
	if (!ICO::is_valid_size((int)width) || !ICO::is_valid_size((int)height))
		return B_NO_TRANSLATOR;
	return B_OK;
}


BView *
ICOTranslator::NewConfigView(TranslatorSettings *settings)
{
	return new ConfigView();
}


//	#pragma mark -


BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (n != 0)
		return NULL;

	return new ICOTranslator();
}

