////////////////////////////////////////////////////////////////////////////////
//
//	File: GIFTranslator.h
//
//	Date: December 1999
//
//	Author: Daniel Switkin
//
//	Copyright 2003 (c) by Daniel Switkin. This file is made publically available
//	under the BSD license, with the stipulations that this complete header must
//	remain at the top of the file indefinitely, and credit must be given to the
//	original author in any about box using this software.
//
////////////////////////////////////////////////////////////////////////////////

// Additional authors:	John Scipione, <jscipione@gmail.com>

#ifndef GIF_TRANSLATOR_H
#define GIF_TRANSLATOR_H


#include "BaseTranslator.h"


#define GIF_IN_QUALITY		0.8
#define GIF_IN_CAPABILITY	0.8
#define BBM_IN_QUALITY		0.3
#define BBM_IN_CAPABILITY	0.3

#define GIF_OUT_QUALITY		0.8
#define GIF_OUT_CAPABILITY	0.8
#define BBM_OUT_QUALITY		0.3
#define BBM_OUT_CAPABILITY	0.3

#define GIF_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(1, 5, 0)

// settings

#define GIF_SETTING_INTERLACED				"interlaced"
#define GIF_SETTING_USE_TRANSPARENT			"use transparent"
#define GIF_SETTING_USE_TRANSPARENT_AUTO	"use transparent auto"
#define GIF_SETTING_USE_DITHERING			"use dithering"
#define GIF_SETTING_PALETTE_MODE			"palette mode"
#define GIF_SETTING_PALETTE_SIZE			"palette size"
#define GIF_SETTING_TRANSPARENT_RED			"transparent red"
#define GIF_SETTING_TRANSPARENT_GREEN		"transparent green"
#define GIF_SETTING_TRANSPARENT_BLUE		"transparent blue"


class GIFTranslator : public BaseTranslator {
public:
								GIFTranslator();
	virtual	status_t			DerivedIdentify(BPositionIO* inSource,
									const translation_format* inFormat,
									BMessage* ioExtension,
									translator_info* outInfo, uint32 outType);

	virtual	status_t			DerivedTranslate(BPositionIO* inSource,
									const translator_info* inInfo,
									BMessage* ioExtension,
									uint32 outType, BPositionIO* outDestination,
									int32 baseType);

	virtual	BView*				NewConfigView(TranslatorSettings* settings);

protected:
	virtual						~GIFTranslator();
};


#endif	// GIF_TRANSLATOR_H
