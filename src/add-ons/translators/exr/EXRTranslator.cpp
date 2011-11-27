/*
 * Copyright 2008, Jérôme Duval. All rights reserved.
 * Copyright (c) 2004, Industrial Light & Magic, a division of Lucas
 *   Digital Ltd. LLC
 * Distributed under the terms of the MIT License.
 */

#include <Catalog.h>

#include "ConfigView.h"
#include "EXRGamma.h"
#include "EXRTranslator.h"
#include "ImfArray.h"
#undef min
#undef max
#include "ImfRgbaFile.h"
#include "IStreamWrapper.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "EXRTranslator"


// The input formats that this translator supports.
static const translation_format sInputFormats[] = {
	{
		EXR_IMAGE_FORMAT,
		B_TRANSLATOR_BITMAP,
		EXR_IN_QUALITY,
		EXR_IN_CAPABILITY,
		"image/exr",
		"EXR"
	},
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		BITS_OUT_QUALITY,
		BITS_OUT_CAPABILITY,
		"image/x-be-bitmap",
		"Be Bitmap Format (EXRTranslator)"
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




//	#pragma mark -


EXRTranslator::EXRTranslator()
	: BaseTranslator(B_TRANSLATE("EXR Images"), 
		B_TRANSLATE("EXR image translator"),
		EXR_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		"EXRTranslator_Settings",
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, EXR_IMAGE_FORMAT)
{
}


EXRTranslator::~EXRTranslator()
{
}


status_t
EXRTranslator::DerivedIdentify(BPositionIO *stream,
	const translation_format *format, BMessage *settings,
	translator_info *outInfo, uint32 outType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP)
		return B_NO_TRANSLATOR;

	try {
		IStreamWrapper istream("filename", stream);
		RgbaInputFile inputFile(istream);

		if (outInfo) {
			outInfo->type = EXR_IMAGE_FORMAT;
			outInfo->group = B_TRANSLATOR_BITMAP;
			outInfo->quality = EXR_IN_QUALITY;
			outInfo->capability = EXR_IN_CAPABILITY;
			strcpy(outInfo->MIME, "image/exr");
			strlcpy(outInfo->name, B_TRANSLATE("EXR image"),
				sizeof(outInfo->name));
		}
	} catch (const std::exception &e) {
		return B_NO_TRANSLATOR;
	}

	return B_OK;
}


status_t
EXRTranslator::DerivedTranslate(BPositionIO* source,
	const translator_info* info, BMessage* settings,
	uint32 outType, BPositionIO* target, int32 baseType)
{
	if (!outType)
		outType = B_TRANSLATOR_BITMAP;
	if (outType != B_TRANSLATOR_BITMAP || baseType != 0)
		return B_NO_TRANSLATOR;

	status_t err = B_NO_TRANSLATOR;
	try {
		IStreamWrapper istream("filename", source);
		RgbaInputFile in(istream);

		//Imath::Box2i dw = in.dataWindow();
		const Imath::Box2i &displayWindow = in.displayWindow();
		const Imath::Box2i &dataWindow = in.dataWindow();
		//float a = in.pixelAspectRatio(); // TODO take into account the aspect ratio
		int dataWidth = dataWindow.max.x - dataWindow.min.x + 1;
		int dataHeight = dataWindow.max.y - dataWindow.min.y + 1;
		int displayWidth = displayWindow.max.x - displayWindow.min.x + 1;
		int displayHeight = displayWindow.max.y - displayWindow.min.y + 1;

		// Write out the data to outDestination
		// Construct and write Be bitmap header
		TranslatorBitmap bitsHeader;
		bitsHeader.magic = B_TRANSLATOR_BITMAP;
		bitsHeader.bounds.left = 0;
		bitsHeader.bounds.top = 0;
		bitsHeader.bounds.right = displayWidth - 1;
		bitsHeader.bounds.bottom = displayHeight - 1;
		bitsHeader.rowBytes = 4 * displayWidth;
		bitsHeader.colors = B_RGBA32;
		bitsHeader.dataSize = bitsHeader.rowBytes * displayHeight;
		if (swap_data(B_UINT32_TYPE, &bitsHeader,
			sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK) {
			return B_ERROR;
		}
		target->Write(&bitsHeader, sizeof(TranslatorBitmap));

		Array2D <Rgba> pixels(dataHeight, dataWidth);
		in.setFrameBuffer (&pixels[0][0] - dataWindow.min.y * dataWidth - dataWindow.min.x, 1, dataWidth);
		in.readPixels (dataWindow.min.y, dataWindow.max.y);

		float	_gamma = 0.4545f;
		float	_exposure = 0.0f;
		float	_defog = 0.0f;
		float	_kneeLow = 0.0f;
		float	_kneeHigh = 5.0f;

		float	_fogR = 0.0f;
		float	_fogG = 0.0f;
		float	_fogB = 0.0f;

		halfFunction<float>
		rGamma (Gamma (_gamma,
					_exposure,
					_defog * _fogR,
					_kneeLow,
					_kneeHigh),
			-HALF_MAX, HALF_MAX,
			0.f, 255.f, 0.f, 0.f);

		halfFunction<float>
		gGamma (Gamma (_gamma,
					_exposure,
					_defog * _fogG,
					_kneeLow,
					_kneeHigh),
			-HALF_MAX, HALF_MAX,
			0.f, 255.f, 0.f, 0.f);

		halfFunction<float>
		bGamma (Gamma (_gamma,
					_exposure,
					_defog * _fogB,
					_kneeLow,
					_kneeHigh),
			-HALF_MAX, HALF_MAX,
			0.f, 255.f, 0.f, 0.f);

		for (int y = displayWindow.min.y; y <= displayWindow.max.y; ++y) {
			if (y < dataWindow.min.y
				|| y > dataWindow.max.y) {
				unsigned char sp[4];
				sp[0] = 128;
				sp[1] = 128;
				sp[2] = 128;
				sp[3] = 255;
				for (int x = displayWindow.min.x; x <= displayWindow.max.x; ++x) {
					target->Write(sp, 4);
				}
				continue;
			}

			for (int x = displayWindow.min.x; x <= displayWindow.max.x; ++x) {
				unsigned char sp[4];
				if (x < dataWindow.min.x
					|| x > dataWindow.max.x) {
					sp[0] = 128;
					sp[1] = 128;
					sp[2] = 128;
					sp[3] = 255;
				} else {
					const Imf::Rgba &rp = pixels[y][x];

					sp[0] = (unsigned char)bGamma(rp.b);
					sp[1] = (unsigned char)gGamma(rp.g);
					sp[2] = (unsigned char)rGamma(rp.r);
					sp[3] = 255;
				}
				target->Write(sp, 4);
			}
		}

		err = B_OK;
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return err;
}


BView *
EXRTranslator::NewConfigView(TranslatorSettings *settings)
{
	return new ConfigView();
}


//	#pragma mark -


BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (n != 0)
		return NULL;

	return new EXRTranslator();
}
