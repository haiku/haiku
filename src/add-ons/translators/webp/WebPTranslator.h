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

#define WEBP_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(0,1,0)
#define WEBP_IMAGE_FORMAT	'WebP'

#define WEBP_IN_QUALITY 0.90
#define WEBP_IN_CAPABILITY 0.90

#define BITS_OUT_QUALITY 1
#define BITS_OUT_CAPABILITY 0.9


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
};


#endif // #ifndef WEBP_TRANSLATOR_H
