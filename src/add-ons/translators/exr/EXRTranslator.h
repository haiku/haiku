/*
 * Copyright 2008, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXR_TRANSLATOR_H
#define EXR_TRANSLATOR_H


#include "BaseTranslator.h"


#define EXR_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(0, 1, 0)
#define EXR_IMAGE_FORMAT	'EXRI'

#define EXR_IN_QUALITY		0.90
#define EXR_IN_CAPABILITY	0.90
#define BITS_IN_QUALITY		1
#define BITS_IN_CAPABILITY	1

#define EXR_OUT_QUALITY		0.8
#define EXR_OUT_CAPABILITY	0.8
#define BITS_OUT_QUALITY	1
#define BITS_OUT_CAPABILITY	0.9


class EXRTranslator : public BaseTranslator {
	public:
		EXRTranslator();
		virtual ~EXRTranslator();

		virtual status_t DerivedIdentify(BPositionIO *inSource,
			const translation_format *inFormat, BMessage *ioExtension,
			translator_info *outInfo, uint32 outType);

		virtual status_t DerivedTranslate(BPositionIO *inSource,
			const translator_info *inInfo, BMessage *ioExtension,
			uint32 outType, BPositionIO *outDestination, int32 baseType);

		virtual BView *NewConfigView(TranslatorSettings *settings);

	private:
};

#endif	// EXR_TRANSLATOR_H

