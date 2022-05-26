/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Emmanuel Gil Peyrot
 */
#ifndef AVIF_TRANSLATOR_H
#define AVIF_TRANSLATOR_H


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

#define AVIF_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(0,1,0)
#define AVIF_IMAGE_FORMAT	'AVIF'

#define AVIF_SETTING_LOSSLESS			"lossless"
#define AVIF_SETTING_PIXEL_FORMAT		"pixfmt"
#define AVIF_SETTING_QUALITY			"quality"
#define AVIF_SETTING_SPEED				"speed"
#define AVIF_SETTING_TILES_HORIZONTAL	"htiles"
#define AVIF_SETTING_TILES_VERTICAL		"vtiles"


#define AVIF_IN_QUALITY 0.90
#define AVIF_IN_CAPABILITY 0.90

#define AVIF_OUT_QUALITY 0.90
#define AVIF_OUT_CAPABILITY 0.5

#define BITS_IN_QUALITY 0.8
#define BITS_IN_CAPABILITY 0.6

#define BITS_OUT_QUALITY 0.5
#define BITS_OUT_CAPABILITY 0.4


struct AVIFPicture;


class AVIFTranslator : public BaseTranslator {
public:
							AVIFTranslator();

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
	virtual 				~AVIFTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user

private:
			status_t		_TranslateFromBits(BPositionIO* stream,
								BMessage* settings, uint32 outType,
								BPositionIO* target);

			status_t		_TranslateFromAVIF(BPositionIO* stream,
								BMessage* settings, uint32 outType,
								BPositionIO* target);
};


#endif // #ifndef AVIF_TRANSLATOR_H
