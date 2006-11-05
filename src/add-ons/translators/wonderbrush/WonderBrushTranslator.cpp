/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "WonderBrushTranslator.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <OS.h>

#include "blending.h"

#include "WonderBrushImage.h"
#include "WonderBrushView.h"

using std::nothrow;

// The input formats that this translator supports.
translation_format gInputFormats[] = {
	/*{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_IN_QUALITY,
		BBT_IN_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (WonderBrushTranslator)"
	},*/
	{
		WBI_FORMAT,
		B_TRANSLATOR_BITMAP,
		WBI_IN_QUALITY,
		WBI_IN_CAPABILITY,
		"image/x-wonderbrush",
		"WonderBrush image"
	}
};

// The output formats that this translator supports.
translation_format gOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BBT_OUT_QUALITY,
		BBT_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (WonderBrushTranslator)"
	}/*,
	{
		WBI_FORMAT,
		B_TRANSLATOR_BITMAP,
		WBI_OUT_QUALITY,
		WBI_OUT_CAPABILITY,
		"image/x-wonderbrush",
		"WonderBrush image"
	}*/
};


// Default settings for the Translator
TranSetting gDefaultSettings[] = {
	{ B_TRANSLATOR_EXT_HEADER_ONLY, TRAN_SETTING_BOOL, false },
	{ B_TRANSLATOR_EXT_DATA_ONLY, TRAN_SETTING_BOOL, false }
};


BTranslator*
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (!n)
		return new WonderBrushTranslator();
	else
		return NULL;
}


WonderBrushTranslator::WonderBrushTranslator()
	: BaseTranslator("WonderBrush Images", "WonderBrush image translator",
		WBI_TRANSLATOR_VERSION,
		gInputFormats, sizeof(gInputFormats) / sizeof(translation_format),
		gOutputFormats, sizeof(gOutputFormats) / sizeof(translation_format),
		"WBITranslator_Settings",
		gDefaultSettings, sizeof(gDefaultSettings) / sizeof(TranSetting),
		B_TRANSLATOR_BITMAP, WBI_FORMAT)
{
#if GAMMA_BLEND
	init_gamma_blending();
#endif
}


WonderBrushTranslator::~WonderBrushTranslator()
{
#if GAMMA_BLEND
	uninit_gamma_blending();
#endif
}


status_t
identify_wbi_header(BPositionIO* inSource, translator_info* outInfo,
	uint32 outType, WonderBrushImage** _wbImage = NULL)
{
	status_t status = B_NO_MEMORY;
	// construct new SGIImage object and set it to the provided BPositionIO
	WonderBrushImage* wbImage = new(nothrow) WonderBrushImage();
	if (wbImage)
		status = wbImage->SetTo(inSource);
	
	if (status >= B_OK) {
		if (outInfo) {
			outInfo->type = WBI_FORMAT;
			outInfo->group = B_TRANSLATOR_BITMAP;
			outInfo->quality = WBI_IN_QUALITY;
			outInfo->capability = WBI_IN_CAPABILITY;
			strcpy(outInfo->MIME, "image/x-wonderbrush");
			strcpy(outInfo->name, "WonderBrush image");
		}
	} else {
		delete wbImage;
		wbImage = NULL;
	}
	if (!_wbImage) {
		// close WonderBrushImage if caller is not interested in handle
		delete wbImage;
	} else {
		// leave WonderBrushImage open (if it is) and return handle if
		// caller needs it
		*_wbImage = wbImage;
	}

	return status;
}
	

status_t
WonderBrushTranslator::DerivedIdentify(BPositionIO* inSource,
	const translation_format* inFormat, BMessage* ioExtension,
	translator_info* outInfo, uint32 outType)
{
	return identify_wbi_header(inSource, outInfo, outType);
}


status_t
WonderBrushTranslator::DerivedTranslate(BPositionIO* inSource,
	const translator_info* inInfo, BMessage* ioExtension,
	uint32 outType, BPositionIO* outDestination, int32 baseType)
{
	if (baseType == 0)
		// if inSource is NOT in bits format
		return _TranslateFromWBI(inSource, outType, outDestination);
	else
		// if BaseTranslator did not properly identify the data as
		// bits or not bits
		return B_NO_TRANSLATOR;		
}


BView*
WonderBrushTranslator::NewConfigView(TranslatorSettings* settings)
{
	return new WonderBrushView(BRect(0, 0, 225, 175), "WBI Settings",
		B_FOLLOW_ALL, B_WILL_DRAW, settings);
}


// #pragma mark -


status_t
WonderBrushTranslator::_TranslateFromWBI(BPositionIO* inSource, uint32 outType,
	BPositionIO* outDestination)
{
	// if copying WBI_FORMAT to WBI_FORMAT
	if (outType == WBI_FORMAT) {
		translate_direct_copy(inSource, outDestination);
		return B_OK;
	}

	WonderBrushImage* wbImage = NULL;
	ssize_t ret = identify_wbi_header(inSource, NULL, outType, &wbImage);
	if (ret < B_OK)
		return ret;

	bool headerOnly = false;
	bool dataOnly = false;

	BBitmap* bitmap = wbImage->Bitmap();
	if (!bitmap)
		return B_ERROR;

	uint32 width = bitmap->Bounds().IntegerWidth() + 1;
	uint32 height = bitmap->Bounds().IntegerHeight() + 1;
	color_space format = bitmap->ColorSpace();
	uint32 bytesPerRow = bitmap->BytesPerRow();

	if (!dataOnly) {
		// Construct and write Be bitmap header
		TranslatorBitmap bitsHeader;
		bitsHeader.magic = B_TRANSLATOR_BITMAP;
		bitsHeader.bounds.left = 0;
		bitsHeader.bounds.top = 0;
		bitsHeader.bounds.right = width - 1;
		bitsHeader.bounds.bottom = height - 1;
		bitsHeader.rowBytes = bytesPerRow;
		bitsHeader.colors = format;
		bitsHeader.dataSize = bitsHeader.rowBytes * height;
		if ((ret = swap_data(B_UINT32_TYPE, &bitsHeader,
			sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN)) < B_OK) {
			return ret;
		} else
			ret = outDestination->Write(&bitsHeader, sizeof(TranslatorBitmap));
	}

	if (ret >= B_OK && !headerOnly) {
		// read one row at a time and write out the results
		uint8* row = (uint8*)bitmap->Bits();
		for (uint32 y = 0; y < height && ret >= B_OK; y++) {
			ret = outDestination->Write(row, bytesPerRow);
			row += bytesPerRow;
		}

	}

	delete bitmap;
	delete wbImage;

	if (ret >= B_OK)
		ret = B_OK;

	return (status_t)ret;
}


