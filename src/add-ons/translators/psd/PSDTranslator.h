/*
 * Copyright 2013, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef PSD_TRANSLATOR_H
#define PSD_TRANSLATOR_H

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

#define PSD_SETTING_COMPRESSION "psd /compression"
#define PSD_SETTING_VERSION 	"psd /psdversion"

#define PSD_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(1, 3, 0)
#define PSD_IMAGE_FORMAT	'PSD '

#define PSD_IN_QUALITY		0.7
#define PSD_IN_CAPABILITY	0.6
#define PSD_OUT_QUALITY		0.5
#define PSD_OUT_CAPABILITY	0.6

#define BITS_IN_QUALITY		0.7
#define BITS_IN_CAPABILITY	0.6
#define BITS_OUT_QUALITY	0.7
#define BITS_OUT_CAPABILITY	0.6

class PSDTranslator : public BaseTranslator {
	public:
		PSDTranslator();

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
		virtual ~PSDTranslator();
};

extern const char *kDocumentCount;
extern const char *kDocumentIndex;

#endif	/* PSD_TRANSLATOR_H */
