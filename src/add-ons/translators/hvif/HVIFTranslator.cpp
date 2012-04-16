/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */

#include "HVIFTranslator.h"
#include "HVIFView.h"

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Catalog.h>
#include <IconUtils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HVIF_FORMAT_CODE				'HVIF'
#define HVIF_TRANSLATION_QUALITY		1.0
#define HVIF_TRANSLATION_CAPABILITY		1.0

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HVIFTranslator"


static const translation_format sInputFormats[] = {
	{
		HVIF_FORMAT_CODE,
		B_TRANSLATOR_BITMAP,
		HVIF_TRANSLATION_QUALITY,
		HVIF_TRANSLATION_CAPABILITY,
		"application/x-vnd.Haiku-icon",
		"Native Haiku vector icon"
	}
};


static const translation_format sOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		0.4,
		0.4,
		"image/x-be-bitmap",
		"Be Bitmap Format (HVIFTranslator)"
	}
};


static const TranSetting sDefaultSettings[] = {
	{ HVIF_SETTING_RENDER_SIZE, TRAN_SETTING_INT32, 64 }
};

const uint32 kNumInputFormats = sizeof(sInputFormats) / sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) / sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) / sizeof(TranSetting);



BTranslator *
make_nth_translator(int32 n, image_id image, uint32 flags, ...)
{
	if (n == 0)
		return new HVIFTranslator();
	return NULL;
}


HVIFTranslator::HVIFTranslator()
	: BaseTranslator(B_TRANSLATE("HVIF icons"), 
		B_TRANSLATE("Native Haiku vector icon translator"),
		HVIF_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"HVIFTranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, HVIF_FORMAT_CODE)
{
}


HVIFTranslator::~HVIFTranslator()
{
}


status_t
HVIFTranslator::DerivedIdentify(BPositionIO *inSource,
	const translation_format *inFormat, BMessage *ioExtension,
	translator_info *outInfo, uint32 outType)
{
	// TODO: we do the fully work twice!
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	// filter out invalid sizes
	off_t size = inSource->Seek(0, SEEK_END);
	if (size <= 0 || size > 256 * 1024 || inSource->Seek(0, SEEK_SET) != 0)
		return B_NO_TRANSLATOR;

	uint8 *buffer = (uint8 *)malloc(size);
	if (buffer == NULL)
		return B_NO_MEMORY;

	if (inSource->Read(buffer, size) != size) {
		free(buffer);
		return B_NO_TRANSLATOR;
	}

	BBitmap dummy(BRect(0, 0, 1, 1), B_BITMAP_NO_SERVER_LINK, B_RGBA32);
	if (BIconUtils::GetVectorIcon(buffer, size, &dummy) != B_OK) {
		free(buffer);
		return B_NO_TRANSLATOR;
	}

	if (outInfo) {
		outInfo->type = sInputFormats[0].type;
		outInfo->group = sInputFormats[0].group;
		outInfo->quality = sInputFormats[0].quality;
		outInfo->capability = sInputFormats[0].capability;
		strcpy(outInfo->MIME, sInputFormats[0].MIME);
		strcpy(outInfo->name, sInputFormats[0].name);
	}

	free(buffer);
	return B_OK;
}


status_t
HVIFTranslator::DerivedTranslate(BPositionIO *inSource,
	const translator_info *inInfo, BMessage *ioExtension, uint32 outType,
	BPositionIO *outDestination, int32 baseType)
{
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	// filter out invalid sizes
	off_t size = inSource->Seek(0, SEEK_END);
	if (size <= 0 || size > 256 * 1024 || inSource->Seek(0, SEEK_SET) != 0)
		return B_NO_TRANSLATOR;

	uint8 *buffer = (uint8 *)malloc(size);
	if (buffer == NULL)
		return B_NO_MEMORY;

	if (inSource->Read(buffer, size) != size) {
		free(buffer);
		return B_NO_TRANSLATOR;
	}

	int32 renderSize = fSettings->SetGetInt32(HVIF_SETTING_RENDER_SIZE);
	if (renderSize <= 0 || renderSize > 1024)
		renderSize = 64;

	BBitmap rendered(BRect(0, 0, renderSize - 1, renderSize - 1),
		B_BITMAP_NO_SERVER_LINK, B_RGBA32);
	if (BIconUtils::GetVectorIcon(buffer, size, &rendered) != B_OK) {
		free(buffer);
		return B_NO_TRANSLATOR;
	}

	BBitmapStream stream(&rendered);
	stream.Seek(0, SEEK_SET);
	ssize_t read = 0;

	while ((read = stream.Read(buffer, size)) > 0)
		outDestination->Write(buffer, read);

	BBitmap *dummy = NULL;
	stream.DetachBitmap(&dummy);
	free(buffer);
	return B_OK;
}


BView *
HVIFTranslator::NewConfigView(TranslatorSettings *settings)
{
	return new HVIFView(B_TRANSLATE("HVIFTranslator Settings"), 
		B_WILL_DRAW, settings);
}
