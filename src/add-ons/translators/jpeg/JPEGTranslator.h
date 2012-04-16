/*

Copyright (c) 2002-2003, Marcin 'Shard' Konicki
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
#ifndef _JPEGTRANSLATOR_H_
#define _JPEGTRANSLATOR_H_


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

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jpeglib.h>

#include "BaseTranslator.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "JPEGTranslator"

// Settings
#define SETTINGS_FILE	"JPEGTranslator"

// View messages
#define VIEW_MSG_SET_QUALITY 'JSCQ'
#define VIEW_MSG_SET_SMOOTHING 'JSCS'
#define VIEW_MSG_SET_PROGRESSIVE 'JSCP'
#define VIEW_MSG_SET_OPTIMIZECOLORS 'JSBQ'
#define	VIEW_MSG_SET_SMALLERFILE 'JSSF'
#define	VIEW_MSG_SET_GRAY1ASRGB24 'JSGR'
#define	VIEW_MSG_SET_ALWAYSRGB32 'JSAC'
#define	VIEW_MSG_SET_PHOTOSHOPCMYK 'JSPC'
#define	VIEW_MSG_SET_SHOWREADERRORBOX 'JSEB'

// strings for use in TranslatorSettings
#define JPEG_SET_SMOOTHING "smoothing"
#define JPEG_SET_QUALITY "quality"
#define JPEG_SET_PROGRESSIVE "progressive"
#define JPEG_SET_OPT_COLORS "optimize"
#define JPEG_SET_SMALL_FILES "filesSmaller"
#define JPEG_SET_GRAY1_AS_RGB24 "gray"
#define JPEG_SET_ALWAYS_RGB32 "always"
#define JPEG_SET_PHOTOSHOP_CMYK "cmyk"
#define JPEG_SET_SHOWREADWARNING "readWarning"


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
	virtual const char* UpdateText() const;

	private:
		mutable char fStatusLabel[12];
};


class JPEGTranslator : public BaseTranslator {
	public:
		JPEGTranslator();

		virtual status_t DerivedIdentify(BPositionIO* inSource,
			const translation_format* inFormat, BMessage* ioExtension,
			translator_info* outInfo, uint32 outType);
			
		virtual status_t DerivedTranslate(BPositionIO* inSource,
			const translator_info* inInfo, BMessage* ioExtension,
			uint32 outType, BPositionIO* outDestination, int32 baseType);
			
		virtual BView* NewConfigView(TranslatorSettings* settings);

	private:
	
		status_t Copy(BPositionIO* in, BPositionIO* out);
		status_t Compress(BPositionIO* in, BPositionIO* out,
			const jmp_buf* longJumpBuffer);
		status_t Decompress(BPositionIO* in, BPositionIO* out,
			BMessage* ioExtension, const jmp_buf* longJumpBuffer);
		status_t Error(j_common_ptr cinfo, status_t error = B_ERROR);

		status_t PopulateInfoFromFormat(translator_info* info,
			uint32 formatType, translator_id id = 0);
		status_t PopulateInfoFromFormat(translator_info* info,
			uint32 formatType, const translation_format* formats,
			int32 formatCount);
};
		

class TranslatorReadView : public BView {
	public:
		TranslatorReadView(const char* name, TranslatorSettings* settings);
		virtual ~TranslatorReadView();

		virtual void	AttachedToWindow();
		virtual void	MessageReceived(BMessage* message);

	private:
		TranslatorSettings* fSettings;
		BCheckBox*		fAlwaysRGB32;
		BCheckBox*		fPhotoshopCMYK;
		BCheckBox*		fShowErrorBox;
};


class TranslatorWriteView : public BView {
	public:
		TranslatorWriteView(const char* name, TranslatorSettings* settings);
		virtual ~TranslatorWriteView();

		virtual void	AttachedToWindow();
		virtual void	MessageReceived(BMessage* message);

	private:
		TranslatorSettings* fSettings;
		SSlider*		fQualitySlider;
		SSlider*		fSmoothingSlider;
		BCheckBox*		fProgress;
		BCheckBox*		fOptimizeColors;
		BCheckBox*		fSmallerFile;
		BCheckBox*		fGrayAsRGB24;
};


class TranslatorAboutView : public BView {
	public:
		TranslatorAboutView(const char* name);
};


class TranslatorView : public BTabView {
	public:
		TranslatorView(const char* name, TranslatorSettings* settings);

	private:
		BView* fAboutView;
		BView* fReadView;
		BView* fWriteView;
};


//---------------------------------------------------
//	"Initializers" for jpeglib
//	based on default ones,
//	modified to work on BPositionIO instead of FILE
//---------------------------------------------------
EXTERN(void) be_jpeg_stdio_src(j_decompress_ptr cinfo, BPositionIO *infile);	// from "be_jdatasrc.cpp"
EXTERN(void) be_jpeg_stdio_dest(j_compress_ptr cinfo, BPositionIO *outfile);	// from "be_jdatadst.cpp"

//---------------------------------------------------
//	Error output functions
//	based on the one from jerror.c
//	modified to use settings
//	(so user can decide to show dialog-boxes or not)
//---------------------------------------------------
EXTERN(struct jpeg_error_mgr *) be_jpeg_std_error (struct jpeg_error_mgr * err,
	TranslatorSettings * settings, const jmp_buf* longJumpBuffer);
	// implemented in "be_jerror.cpp"

#endif // _JPEGTRANSLATOR_H_

