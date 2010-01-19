/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ICO_TRANSLATOR_H
#define ICO_TRANSLATOR_H


#include "BaseTranslator.h"

#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <DataIO.h>
#include <File.h>
#include <ByteOrder.h>


#define ICO_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(0, 3, 0)
#define ICO_IMAGE_FORMAT	'ICO '

#define ICO_IN_QUALITY		0.5
#define ICO_IN_CAPABILITY	0.5
#define BITS_IN_QUALITY		1
#define BITS_IN_CAPABILITY	1

#define ICO_OUT_QUALITY		0.8
#define ICO_OUT_CAPABILITY	0.8
#define BITS_OUT_QUALITY	1
#define BITS_OUT_CAPABILITY	0.9


class ICOTranslator : public BaseTranslator {
	public:
		ICOTranslator();

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
		virtual ~ICOTranslator();
			// this is protected because the object is deleted by the
			// Release() function instead of being deleted directly by
			// the user

	private:
};

// Extensions that ShowImage supports
extern const char *kDocumentCount;
extern const char *kDocumentIndex;

#endif	/* ICO_TRANSLATOR_H */
