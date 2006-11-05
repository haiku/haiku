/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef WONDERBRUSH_TRANSLATOR_H
#define WONDERBRUSH_TRANSLATOR_H

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

#define WBI_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(1,0,0)

#define WBI_IN_QUALITY 1.0
#define WBI_IN_CAPABILITY 1.0
#define WBI_OUT_QUALITY 1.0
#define WBI_OUT_CAPABILITY 1.0

#define BBT_IN_QUALITY 0.4
#define BBT_IN_CAPABILITY 0.6
#define BBT_OUT_QUALITY 0.4
#define BBT_OUT_CAPABILITY 0.6

enum {
	WBI_FORMAT	= 'WBI ',
};

class WonderBrushTranslator : public BaseTranslator {
 public:
								WonderBrushTranslator();
	
	virtual status_t			DerivedIdentify(BPositionIO* inSource,
									const translation_format* inFormat,
									BMessage* ioExtension,
									translator_info* outInfo, uint32 outType);
		
	virtual	status_t			DerivedTranslate(BPositionIO* inSource,
									const translator_info* inInfo,
									BMessage* ioExtension, uint32 outType,
									BPositionIO* outDestination,
									int32 baseType);
		
	virtual	BView*				NewConfigView(TranslatorSettings* settings);

 protected:
	virtual						~WonderBrushTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user
		
 private:
			status_t			_TranslateFromWBI(BPositionIO* inSource,
									uint32 outType, BPositionIO* outDestination);
};

#endif // WONDERBRUSH_TRANSLATOR_H
