/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef RAW_TRANSLATOR_H
#define RAW_TRANSLATOR_H


#include "BaseTranslator.h"

#include <ByteOrder.h>
#include <DataIO.h>
#include <File.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <TranslationDefs.h>
#include <Translator.h>
#include <TranslatorFormats.h>


#define RAW_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(0, 5, 0)
#define RAW_IMAGE_FORMAT	'RAWI'

#define RAW_IN_QUALITY		0.90
#define RAW_IN_CAPABILITY	0.90
#define BITS_IN_QUALITY		1
#define BITS_IN_CAPABILITY	1

#define RAW_OUT_QUALITY		0.8
#define RAW_OUT_CAPABILITY	0.8
#define BITS_OUT_QUALITY	1
#define BITS_OUT_CAPABILITY	0.9


class RAWTranslator : public BaseTranslator {
	public:
		RAWTranslator();
		virtual ~RAWTranslator();

		virtual status_t DerivedIdentify(BPositionIO *inSource,
			const translation_format *inFormat, BMessage *ioExtension,
			translator_info *outInfo, uint32 outType);

		virtual status_t DerivedTranslate(BPositionIO *inSource,
			const translator_info *inInfo, BMessage *ioExtension,
			uint32 outType, BPositionIO *outDestination, int32 baseType);

		virtual BView *NewConfigView(TranslatorSettings *settings);

	private:
		static void _ProgressMonitor(const char* message, float percentage,
			void* data);
};

#endif	// RAW_TRANSLATOR_H
