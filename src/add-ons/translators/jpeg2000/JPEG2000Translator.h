/*

Copyright (c) 2003, Marcin 'Shard' Konicki
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation and/or
      other materials provided with the distribution.
    * Name "Marcin Konicki", "Shard" or any combination of them,
      must not be used to endorse or promote products derived from this
      software without specific prior written permission from Marcin Konicki.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef _JP2TRANSLATOR_H_
#define _JP2TRANSLATOR_H_


#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Slider.h>
#include <StringView.h>
#include <TabView.h>
#include <TranslationKit.h>
#include <TranslatorAddOn.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BaseTranslator.h"
#include "libjasper/jasper.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "JPEG2000Translator"

// Settings
#define JP2_SETTINGS_FILE	"JPEG2000Translator"

#define JP2_SET_QUALITY "quality"
#define JP2_SET_GRAY1_AS_B_RGB24 "24 from gray1"
#define JP2_SET_GRAY8_AS_B_RGB32 "32 from gray8"
#define JP2_SET_JPC "jpc"

// View messages
#define VIEW_MSG_SET_QUALITY 'JSCQ'
#define	VIEW_MSG_SET_GRAY1ASRGB24 'JSGR'
#define	VIEW_MSG_SET_JPC 'JSJC'
#define	VIEW_MSG_SET_GRAYASRGB32 'JSAC'


/*!
	Slider used in TranslatorView
	With status showing actual value
*/
class SSlider : public BSlider {
public:
	SSlider(const char* name, const char* label,
		BMessage* message, int32 minValue, int32 maxValue,
		orientation posture = B_HORIZONTAL,
		thumb_style thumbType = B_BLOCK_THUMB,
		uint32 flags = B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS);
	const char*	UpdateText() const;

private:
	mutable char fStatusLabel[12];
};

//!	Configuration view for reading settings
class TranslatorReadView : public BView {
public:
	TranslatorReadView(const char* name, TranslatorSettings* settings);
	~TranslatorReadView();

	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* message);

private:
	TranslatorSettings*	fSettings;
	BCheckBox*		fGrayAsRGB32;
};

//! Configuration view for writing settings
class TranslatorWriteView : public BView {
public:
	TranslatorWriteView(const char* name, TranslatorSettings* settings);
	~TranslatorWriteView();

	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* message);

private:
	TranslatorSettings*	fSettings;
	SSlider*		fQualitySlider;
	BCheckBox*		fGrayAsRGB24;
	BCheckBox*		fCodeStreamOnly;
};

class TranslatorAboutView : public BView {
public:
	TranslatorAboutView(const char* name);
};

//!	Configuration view
class TranslatorView : public BTabView {
public:
	TranslatorView(const char* name, TranslatorSettings* settings);
};

class JP2Translator : public BaseTranslator {
public:
	JP2Translator();
	virtual status_t DerivedIdentify(BPositionIO* inSource,
		const translation_format* inFormat, BMessage* ioExtension,
		translator_info* outInfo, uint32 outType);
		
	virtual status_t DerivedTranslate(BPositionIO* inSource,
		const translator_info* inInfo, BMessage* ioExtension,
		uint32 outType, BPositionIO* outDestination, int32 baseType);
		
	virtual BView* NewConfigView(TranslatorSettings* settings);


private:
	status_t Copy(BPositionIO* in, BPositionIO* out);
	status_t Compress(BPositionIO* in, BPositionIO* out);
	status_t Decompress(BPositionIO* in, BPositionIO* out);

	status_t PopulateInfoFromFormat(translator_info* info,
		uint32 formatType, translator_id id = 0);
	status_t PopulateInfoFromFormat(translator_info* info,
		uint32 formatType, const translation_format* formats,
		int32 formatCount);
};

status_t Error(jas_stream_t* stream, jas_image_t* image, jas_matrix_t** pixels,
	int32 pixels_count, jpr_uchar_t* scanline, status_t error = B_ERROR);

#endif // _JP2TRANSLATOR_H_
