/*
 * Copyright 2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */
#ifndef WEBP_TRANSLATOR_H
#define WEBP_TRANSLATOR_H


#include <ByteOrder.h>
#include <DataIO.h>
#include <File.h>
#include <fs_attr.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <TranslationDefs.h>
#include <Translator.h>
#include <TranslatorFormats.h>

#include "BaseTranslator.h"

#define WEBP_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(0,2,0)
#define WEBP_IMAGE_FORMAT	'WebP'

#define WEBP_SETTING_QUALITY		"quality"
#define WEBP_SETTING_PRESET			"preset"
#define WEBP_SETTING_SHARPNESS		"sharpness"
#define WEBP_SETTING_METHOD			"method"
#define WEBP_SETTING_PREPROCESSING	"preprocessing"


#define WEBP_IN_QUALITY 0.90
#define WEBP_IN_CAPABILITY 0.90

#define WEBP_OUT_QUALITY 0.90
#define WEBP_OUT_CAPABILITY 0.5

#define BITS_IN_QUALITY 0.8
#define BITS_IN_CAPABILITY 0.6

#define BITS_OUT_QUALITY 0.5
#define BITS_OUT_CAPABILITY 0.4


struct WebPPicture;


class WebPTranslator : public BaseTranslator {
public:
							WebPTranslator();

	virtual status_t 		DerivedIdentify(BPositionIO* stream,
								const translation_format* format,
								BMessage* settings, translator_info* info,
								uint32 outType);

	virtual status_t 		DerivedTranslate(BPositionIO* stream,
								const translator_info* info,
								BMessage* settings, uint32 outType,
								BPositionIO* target, int32 baseType);

	virtual BView*			NewConfigView(TranslatorSettings* settings);

protected:
	virtual 				~WebPTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user

private:
			status_t		_TranslateFromBits(BPositionIO* stream,
								BMessage* settings, uint32 outType,
								BPositionIO* target);

			status_t		_TranslateFromWebP(BPositionIO* stream,
								BMessage* settings, uint32 outType,
								BPositionIO* target);

	static	int 			_EncodedWriter(const uint8_t* data,
								size_t dataSize,
								const WebPPicture* const picture);
};


#endif // #ifndef WEBP_TRANSLATOR_H
