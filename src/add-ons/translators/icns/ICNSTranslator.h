/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2012, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef ICNS_TRANSLATOR_H
#define ICNS_TRANSLATOR_H


#include "BaseTranslator.h"

#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <DataIO.h>
#include <File.h>
#include <ByteOrder.h>

#define DOCUMENT_COUNT "/documentCount"
#define DOCUMENT_INDEX "/documentIndex"

#define ICNS_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(1, 0, 0)
#define ICNS_IMAGE_FORMAT	'ICNS'

#define ICNS_IN_QUALITY		0.5
#define ICNS_IN_CAPABILITY	0.5
#define BITS_IN_QUALITY		1
#define BITS_IN_CAPABILITY	1

#define ICNS_OUT_QUALITY	0.8
#define ICNS_OUT_CAPABILITY	0.8
#define BITS_OUT_QUALITY	1
#define BITS_OUT_CAPABILITY	0.9


class ICNSTranslator : public BaseTranslator {
	public:
		ICNSTranslator();

		virtual status_t DerivedIdentify(BPositionIO *inSource,
			const translation_format *inFormat, BMessage *ioExtension,
			translator_info *outInfo, uint32 outType);

		virtual status_t DerivedTranslate(BPositionIO *inSource,
			const translator_info *inInfo, BMessage *ioExtension,
			uint32 outType, BPositionIO *outDestination, int32 baseType);

		virtual status_t DerivedCanHandleImageSize(float width,
			float height) const;

		virtual BView *NewConfigView(TranslatorSettings *settings);	

	protected:
		virtual ~ICNSTranslator();
};

// Extensions that ShowImage supports
extern const char *kDocumentCount;
extern const char *kDocumentIndex;

#endif	/* ICNS_TRANSLATOR_H */
