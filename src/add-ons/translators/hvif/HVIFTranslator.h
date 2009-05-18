/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */
#ifndef HVIF_TRANSLATOR_H
#define HVIF_TRANSLATOR_H

#include "BaseTranslator.h"

#define HVIF_TRANSLATOR_VERSION		B_TRANSLATION_MAKE_VERSION(1, 0, 0)
#define HVIF_SETTING_RENDER_SIZE	"hvif /renderSize"

class HVIFTranslator : public BaseTranslator {
public:
							HVIFTranslator();

virtual	status_t			DerivedIdentify(BPositionIO *inSource,
								const translation_format *inFormat,
								BMessage *ioExtension, translator_info *outInfo,
								uint32 outType);

virtual	status_t			DerivedTranslate(BPositionIO *inSource,
								const translator_info *inInfo,
								BMessage *ioExtension, uint32 outType,
								BPositionIO *outDestination, int32 baseType);

virtual	BView *				NewConfigView(TranslatorSettings *settings);

protected:
virtual						~HVIFTranslator();
			// this is protected because the object is deleted by the
			// Release() function instead of being deleted directly by
			// the user
};

#endif // HVIF_TRANSLATOR_H
