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
#include <CheckBox.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Slider.h>
#include <StringView.h>
#include <TranslationKit.h>
#include <TranslatorAddOn.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libjasper/jasper.h"


// Settings
#define SETTINGS_FILE	"JPEG2000Translator"

// View messages
#define VIEW_MSG_SET_QUALITY 'JSCQ'
#define	VIEW_MSG_SET_GRAY1ASRGB24 'JSGR'
#define	VIEW_MSG_SET_JPC 'JSJC'
#define	VIEW_MSG_SET_GRAYASRGB32 'JSAC'

// View labels
#define VIEW_LABEL_QUALITY "Output quality"
#define VIEW_LABEL_GRAY1ASRGB24 "Write Black&White images as RGB24"
#define VIEW_LABEL_JPC "Output only codestream (.jpc)"
#define	VIEW_LABEL_GRAYASRGB32 "Read Greyscale images as RGB32"


//!	Settings storage structure
struct jpeg_settings {
	// compression
	jpr_uchar_t	Quality;			// default: 25
	bool	JPC;				// default: false	// compress to JPC or JP2?
	bool	B_GRAY1_as_B_RGB24;	// default: false	// copress gray 1 as rgb24 or grayscale?
	// decompression
	bool	B_GRAY8_as_B_RGB32;	// default: true
};


/*!
	Slider used in TranslatorView
	With status showing actual value
*/
class SSlider : public BSlider {
	public:
				SSlider(BRect frame, const char *name, const char *label,
					BMessage *message, int32 minValue, int32 maxValue,
					orientation posture = B_HORIZONTAL,
					thumb_style thumbType = B_BLOCK_THUMB,
					uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
					uint32 flags = B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS);
		char*	UpdateText() const;
		void	ResizeToPreferred();

	private:
		mutable char fStatusLabel[12];
};

//!	Basic view class with resizing to needed size
class SView : public BView {
	public:
		SView(const char* name, float x = 0, float y = 0);

		virtual void	GetPreferredSize(float* _width, float* _height);
		virtual void	ResizeToPreferred();

		void			AddChild(BView* child, BView* before = NULL);

		float			GetPreferredWidth()
							{ return fPreferredWidth; }
		float			GetPreferredHeight()
							{ return fPreferredHeight; }
		void			ResizePreferredBy(float width, float height);

	private:
		float			fPreferredWidth;
		float			fPreferredHeight;
};

//!	Configuration view for reading settings
class TranslatorReadView : public SView {
	public:
		TranslatorReadView(const char* name, jpeg_settings* settings,
			float x = 0, float y = 0);

		virtual void	AttachedToWindow();
		virtual void	MessageReceived(BMessage* message);

	private:
		jpeg_settings*	fSettings;
		BCheckBox*		fGrayAsRGB32;
};

//! Configuration view for writing settings
class TranslatorWriteView : public SView {
	public:
		TranslatorWriteView(const char* name, jpeg_settings* settings,
			float x = 0, float y = 0);

		virtual void	AttachedToWindow();
		virtual void	MessageReceived(BMessage* message);

	private:
		jpeg_settings*	fSettings;
		SSlider*		fQualitySlider;
		BCheckBox*		fGrayAsRGB24;
		BCheckBox*		fCodeStreamOnly;
};

class TranslatorAboutView : public SView {
	public:
		TranslatorAboutView(const char* name, float x = 0, float y = 0);
};

//!	Configuration view
class TranslatorView : public SView {
	public:
		TranslatorView(const char *name);
		virtual ~TranslatorView();

		virtual void	AttachedToWindow();
		virtual void	Draw(BRect updateRect);
		virtual void	MouseDown(BPoint where);

	private:
		BRect			_TabFrame(int32 index) const;

		jpeg_settings	fSettings;
		BList			fTabs;
		int32			fTabWidth;
		int32			fTabHeight;
		int32			fActiveChild;
};

//!	Window used for configuration
class TranslatorWindow : public BWindow {
	public:
		TranslatorWindow(bool quitOnClose = true);
};


// Main functions of translator :)
status_t Copy(BPositionIO *in, BPositionIO *out);
status_t Compress(BPositionIO *in, BPositionIO *out);
status_t Decompress(BPositionIO *in, BPositionIO *out);
status_t Error(jas_stream_t *stream, jas_image_t *image, jas_matrix_t **pixels, int32 pixels_count, jpr_uchar_t *scanline, status_t error = B_ERROR);

#endif // _JP2TRANSLATOR_H_
