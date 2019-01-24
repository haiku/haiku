/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2012, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ICNSTranslator.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ConfigView.h"
#include "ICNSLoader.h"

#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ICNSTranslator"

extern "C" {
#include "icns.h"
}

const char *kDocumentCount = "/documentCount";
const char *kDocumentIndex = "/documentIndex";

#define kICNSMimeType "image/icns"
#define kICNSName "Apple Icon"

// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
	{
		ICNS_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		ICNS_IN_QUALITY,
		ICNS_IN_CAPABILITY,
		kICNSMimeType,
		kICNSName
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_IN_QUALITY,
		BITS_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (ICNSTranslator)"
	},
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
	{
		ICNS_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		ICNS_OUT_QUALITY,
		ICNS_OUT_CAPABILITY,
		kICNSMimeType,
		kICNSName
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_OUT_QUALITY,
		BITS_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (ICNSTranslator)"
	},
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
	{B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false},
	{B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false}
};

const uint32 kNumInputFormats = sizeof(sInputFormats)
	/ sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats)
	/ sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings)
	/ sizeof(TranSetting);


ICNSTranslator::ICNSTranslator()
	: BaseTranslator(B_TRANSLATE("Apple icons"), 
		B_TRANSLATE("Apple icon translator"),
		ICNS_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"ICNSTranslator",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, ICNS_IMAGE_FORMAT)
{
}


ICNSTranslator::~ICNSTranslator()
{
}


status_t
ICNSTranslator::DerivedIdentify(BPositionIO *stream,
	const translation_format *format, BMessage *ioExtension,
	translator_info *info, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != ICNS_IMAGE_FORMAT)
		return B_NO_TRANSLATOR;

	uint32 signatureData;
	ssize_t signatureSize = 4;
	if (stream->Read(&signatureData, signatureSize) != signatureSize)
		return B_IO_ERROR;
			
	const uint32 kICNSMagic = B_HOST_TO_BENDIAN_INT32('icns');
	
	if (signatureData != kICNSMagic)
		return B_ILLEGAL_DATA;	
		

	ICNSLoader icnsFile(stream);
	if(!icnsFile.IsLoaded() || icnsFile.IconsCount() <= 0)
		return B_ILLEGAL_DATA;

	int32 documentCount = icnsFile.IconsCount();
	int32 documentIndex = 1;
	
	if (ioExtension) {
		if (ioExtension->FindInt32(DOCUMENT_INDEX, &documentIndex) != B_OK)
			documentIndex = 1;
		if (documentIndex < 1 || documentIndex > documentCount)
			return B_NO_TRANSLATOR;

		ioExtension->RemoveName(DOCUMENT_COUNT);
		ioExtension->AddInt32(DOCUMENT_COUNT, documentCount);
	}	
		
	info->type = ICNS_IMAGE_FORMAT;
	info->group = B_TRANSLATOR_BITMAP;
	info->quality = ICNS_IN_QUALITY;
	info->capability = ICNS_IN_CAPABILITY;
	BString iconName("Apple icon");
	if (documentCount > 1)
		iconName << " #" << documentIndex;
	snprintf(info->name, sizeof(info->name), iconName.String());
	strcpy(info->MIME, kICNSMimeType);

	return B_OK;
}


status_t
ICNSTranslator::DerivedTranslate(BPositionIO *source,
	const translator_info *info, BMessage *ioExtension,
	uint32 outType, BPositionIO *target, int32 baseType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP && outType != ICNS_IMAGE_FORMAT)
		return B_NO_TRANSLATOR;

	switch (baseType) {
		case 1:
		{
			TranslatorBitmap bitsHeader;		
			status_t result;		
			result = identify_bits_header(source, NULL, &bitsHeader);
			if (result != B_OK)
				return B_NO_TRANSLATOR;
							
			icns_type_t type = ICNSFormat(bitsHeader.bounds.Width() + 1,
				bitsHeader.bounds.Height() + 1, bitsHeader.colors);				
			if (type == ICNS_NULL_TYPE)
				return B_NO_TRANSLATOR;
			
			if (outType == ICNS_IMAGE_FORMAT) {
				ICNSSaver icnsFile(source, bitsHeader.rowBytes, type);
				if (icnsFile.IsCreated())
					return icnsFile.SaveData(target);
			}
			
			return B_NO_TRANSLATOR;
		}

		case 0:
		{								
			if (outType != B_TRANSLATOR_BITMAP)
				return B_NO_TRANSLATOR;			
							
			ICNSLoader icnsFile(source);
			if (!icnsFile.IsLoaded() || icnsFile.IconsCount() <= 0)
				return B_NO_TRANSLATOR;

			int32 documentIndex = 1;
			
			if (ioExtension) {
				if (ioExtension->FindInt32(DOCUMENT_INDEX, &documentIndex) != B_OK)
					documentIndex = 1;
			}
								
			return icnsFile.GetIcon(target, documentIndex);
		}

		default:
			return B_NO_TRANSLATOR;
	}
}


status_t
ICNSTranslator::DerivedCanHandleImageSize(float width, float height) const
{
	if (ICNSFormat(width, height, B_RGBA32) == ICNS_NULL_TYPE)
		return B_NO_TRANSLATOR;
	return B_OK;
}


BView *
ICNSTranslator::NewConfigView(TranslatorSettings *settings)
{
	return new ConfigView(settings);
}


//	#pragma mark -


BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (n != 0)
		return NULL;

	return new ICNSTranslator();
}

