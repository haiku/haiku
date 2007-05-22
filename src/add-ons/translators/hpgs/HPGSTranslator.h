/*
 * Copyright 2007, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef HPGS_TRANSLATOR_H
#define HPGS_TRANSLATOR_H


#include "BaseTranslator.h"

#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <DataIO.h>
#include <File.h>
#include <ByteOrder.h>


#define HPGS_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(0, 1, 0)
#define HPGS_IMAGE_FORMAT	'HPGI'

#define HPGS_IN_QUALITY		0.90
#define HPGS_IN_CAPABILITY	0.90
#define BITS_IN_QUALITY		1
#define BITS_IN_CAPABILITY	1

#define HPGS_OUT_QUALITY		0.8
#define HPGS_OUT_CAPABILITY	0.8
#define BITS_OUT_QUALITY	1
#define BITS_OUT_CAPABILITY	0.9


class HPGSTranslator : public BaseTranslator {
	public:
		HPGSTranslator();
		virtual ~HPGSTranslator();

		virtual status_t DerivedIdentify(BPositionIO *inSource,
			const translation_format *inFormat, BMessage *ioExtension,
			translator_info *outInfo, uint32 outType);

		virtual status_t DerivedTranslate(BPositionIO *inSource,
			const translator_info *inInfo, BMessage *ioExtension,
			uint32 outType, BPositionIO *outDestination, int32 baseType);

		virtual BView *NewConfigView(TranslatorSettings *settings);

	private:
};

#endif	// HPGS_TRANSLATOR_H
