/*
 * Copyright 2008, Jérôme Duval, korli@users.berlios.de. All rights reserved.
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PCX_TRANSLATOR_H
#define PCX_TRANSLATOR_H


#include "BaseTranslator.h"

#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <DataIO.h>
#include <File.h>
#include <ByteOrder.h>


#define PCX_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(0, 3, 0)
#define PCX_IMAGE_FORMAT	'PCX '

#define PCX_IN_QUALITY		0.5
#define PCX_IN_CAPABILITY	0.5
#define BITS_IN_QUALITY		1
#define BITS_IN_CAPABILITY	1

#define PCX_OUT_QUALITY		0.8
#define PCX_OUT_CAPABILITY	0.8
#define BITS_OUT_QUALITY	1
#define BITS_OUT_CAPABILITY	0.9


class PCXTranslator : public BaseTranslator {
	public:
		PCXTranslator();

		virtual status_t DerivedIdentify(BPositionIO *inSource,
			const translation_format *inFormat, BMessage *ioExtension,
			translator_info *outInfo, uint32 outType);

		virtual status_t DerivedTranslate(BPositionIO *inSource,
			const translator_info *inInfo, BMessage *ioExtension,
			uint32 outType, BPositionIO *outDestination, int32 baseType);

		virtual BView *NewConfigView(TranslatorSettings *settings);	

	protected:
		virtual ~PCXTranslator();
			// this is protected because the object is deleted by the
			// Release() function instead of being deleted directly by
			// the user

	private:
};

#endif	/* PCX_TRANSLATOR_H */
