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


#include "JPEGTranslator.h"
#include "TranslatorWindow.h"
#include "exif_parser.h"

#include <syslog.h>

#include <Alignment.h>
#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <TabView.h>
#include <TextView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "JPEGTranslator"

#define MARKER_EXIF	0xe1

// Set these accordingly
#define JPEG_ACRONYM "JPEG"
#define JPEG_FORMAT 'JPEG'
#define JPEG_MIME_STRING "image/jpeg"
#define JPEG_DESCRIPTION "JPEG image"

// The translation kit's native file type
#define B_TRANSLATOR_BITMAP_MIME_STRING "image/x-be-bitmap"
#define B_TRANSLATOR_BITMAP_DESCRIPTION "Be Bitmap Format (JPEGTranslator)"


static const int32 sTranslatorVersion = B_TRANSLATION_MAKE_VERSION(1, 2, 0);

static const char* sTranslatorName = B_TRANSLATE("JPEG images");
static const char* sTranslatorInfo = B_TRANSLATE("©2002-2003, Marcin Konicki\n"
	"©2005-2007, Haiku\n"
	"\n"
	"Based on IJG library ©  1994-2009, Thomas G. Lane, Guido Vollbeding.\n"
	"\thttp://www.ijg.org/files/\n"
	"\n"
	"with \"lossless\" encoding support patch by Ken Murchison\n"
	"\thttp://www.oceana.com/ftp/ljpeg/\n"
	"\n"
	"With some colorspace conversion routines by Magnus Hellman\n"
	"\thttp://www.bebits.com/app/802\n");

// Define the formats we know how to read
static const translation_format sInputFormats[] = {
	{ JPEG_FORMAT, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		JPEG_MIME_STRING, JPEG_DESCRIPTION },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		B_TRANSLATOR_BITMAP_MIME_STRING, B_TRANSLATOR_BITMAP_DESCRIPTION }
};

// Define the formats we know how to write
static const translation_format sOutputFormats[] = {
	{ JPEG_FORMAT, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		JPEG_MIME_STRING, JPEG_DESCRIPTION },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		B_TRANSLATOR_BITMAP_MIME_STRING, B_TRANSLATOR_BITMAP_DESCRIPTION }
};


static const TranSetting sDefaultSettings[] = {
	{JPEG_SET_SMOOTHING, TRAN_SETTING_INT32, 0},
	{JPEG_SET_QUALITY, TRAN_SETTING_INT32, 95},
	{JPEG_SET_PROGRESSIVE, TRAN_SETTING_BOOL, true},
	{JPEG_SET_OPT_COLORS, TRAN_SETTING_BOOL, true},
	{JPEG_SET_SMALL_FILES, TRAN_SETTING_BOOL, false},
	{JPEG_SET_GRAY1_AS_RGB24, TRAN_SETTING_BOOL, false},
	{JPEG_SET_ALWAYS_RGB32, TRAN_SETTING_BOOL, true},
	{JPEG_SET_PHOTOSHOP_CMYK, TRAN_SETTING_BOOL, true},
	{JPEG_SET_SHOWREADWARNING, TRAN_SETTING_BOOL, true}
};

const uint32 kNumInputFormats = sizeof(sInputFormats) / sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats) / sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings) / sizeof(TranSetting);


namespace conversion {


static bool
x_flipped(int32 orientation)
{
	return orientation == 2 || orientation == 3
		|| orientation == 6 || orientation == 7;
}


static bool
y_flipped(int32 orientation)
{
	return orientation == 3 || orientation == 4
		|| orientation == 7 || orientation == 8;
}


static int32
dest_index(uint32 width, uint32 height, uint32 x, uint32 y, int32 orientation)
{
	if (orientation > 4) {
		uint32 temp = x;
		x = y;
		y = temp;
	}
	if (y_flipped(orientation))
		y = height - 1 - y;
	if (x_flipped(orientation))
		x = width - 1 - x;

	return y * width + x;
}


//	#pragma mark - conversion for compression


inline void
convert_from_gray1_to_gray8(uint8* in, uint8* out, int32 inRowBytes)
{
	int32 index = 0;
	int32 index2 = 0;
	while (index < inRowBytes) {
		unsigned char c = in[index++];
		for (int b = 128; b; b = b>>1) {
			unsigned char color;
			if (c & b)
				color = 0;
			else
				color = 255;
			out[index2++] = color;
		}
	}
}


inline void
convert_from_gray1_to_24(uint8* in, uint8* out, int32 inRowBytes)
{
	int32 index = 0;
	int32 index2 = 0;
	while (index < inRowBytes) {
		unsigned char c = in[index++];
		for (int b = 128; b; b = b>>1) {
			unsigned char color;
			if (c & b)
				color = 0;
			else
				color = 255;
			out[index2++] = color;
			out[index2++] = color;
			out[index2++] = color;
		}
	}
}


inline void
convert_from_cmap8_to_24(uint8* in, uint8* out, int32 inRowBytes)
{
	const color_map * map = system_colors();
	int32 index = 0;
	int32 index2 = 0;
	while (index < inRowBytes) {
		rgb_color color = map->color_list[in[index++]];

		out[index2++] = color.red;
		out[index2++] = color.green;
		out[index2++] = color.blue;
	}
}


inline void
convert_from_15_to_24(uint8* in, uint8* out, int32 inRowBytes)
{
	int32 index = 0;
	int32 index2 = 0;
	int16 in_pixel;
	while (index < inRowBytes) {
		in_pixel = in[index] | (in[index + 1] << 8);
		index += 2;

		out[index2++] = (((in_pixel & 0x7c00)) >> 7) | (((in_pixel & 0x7c00)) >> 12);
		out[index2++] = (((in_pixel & 0x3e0)) >> 2) | (((in_pixel & 0x3e0)) >> 7);
		out[index2++] = (((in_pixel & 0x1f)) << 3) | (((in_pixel & 0x1f)) >> 2);
	}
}


inline void
convert_from_15b_to_24(uint8* in, uint8* out, int32 inRowBytes)
{
	int32 index = 0;
	int32 index2 = 0;
	int16 in_pixel;
	while (index < inRowBytes) {
		in_pixel = in[index + 1] | (in[index] << 8);
		index += 2;

		out[index2++] = (((in_pixel & 0x7c00)) >> 7) | (((in_pixel & 0x7c00)) >> 12);
		out[index2++] = (((in_pixel & 0x3e0)) >> 2) | (((in_pixel & 0x3e0)) >> 7);
		out[index2++] = (((in_pixel & 0x1f)) << 3) | (((in_pixel & 0x1f)) >> 2);
	}
}


inline void
convert_from_16_to_24(uint8* in, uint8* out, int32 inRowBytes)
{
	int32 index = 0;
	int32 index2 = 0;
	int16 in_pixel;
	while (index < inRowBytes) {
		in_pixel = in[index] | (in[index + 1] << 8);
		index += 2;

		out[index2++] = (((in_pixel & 0xf800)) >> 8) | (((in_pixel & 0xf800)) >> 13);
		out[index2++] = (((in_pixel & 0x7e0)) >> 3) | (((in_pixel & 0x7e0)) >> 9);
		out[index2++] = (((in_pixel & 0x1f)) << 3) | (((in_pixel & 0x1f)) >> 2);
	}
}


inline void
convert_from_16b_to_24(uint8* in, uint8* out, int32 inRowBytes)
{
	int32 index = 0;
	int32 index2 = 0;
	int16 in_pixel;
	while (index < inRowBytes) {
		in_pixel = in[index + 1] | (in[index] << 8);
		index += 2;

		out[index2++] = (((in_pixel & 0xf800)) >> 8) | (((in_pixel & 0xf800)) >> 13);
		out[index2++] = (((in_pixel & 0x7e0)) >> 3) | (((in_pixel & 0x7e0)) >> 9);
		out[index2++] = (((in_pixel & 0x1f)) << 3) | (((in_pixel & 0x1f)) >> 2);
	}
}


inline void
convert_from_24_to_24(uint8* in, uint8* out, int32 inRowBytes)
{
	int32 index = 0;
	int32 index2 = 0;
	while (index < inRowBytes) {
		out[index2++] = in[index + 2];
		out[index2++] = in[index + 1];
		out[index2++] = in[index];
		index+=3;
	}
}


inline void
convert_from_32_to_24(uint8* in, uint8* out, int32 inRowBytes)
{
	inRowBytes /= 4;

	for (int32 i = 0; i < inRowBytes; i++) {
		out[0] = in[2];
		out[1] = in[1];
		out[2] = in[0];

		in += 4;
		out += 3;
	}
}


inline void
convert_from_32b_to_24(uint8* in, uint8* out, int32 inRowBytes)
{
	inRowBytes /= 4;

	for (int32 i = 0; i < inRowBytes; i++) {
		out[0] = in[1];
		out[1] = in[2];
		out[2] = in[3];

		in += 4;
		out += 3;
	}
}


//	#pragma mark - conversion for decompression


inline void
convert_from_CMYK_to_32_photoshop(uint8* in, uint8* out, int32 inRowBytes, int32 xStep)
{
	for (int32 i = 0; i < inRowBytes; i += 4) {
		int32 black = in[3];
		out[0] = in[2] * black / 255;
		out[1] = in[1] * black / 255;
		out[2] = in[0] * black / 255;
		out[3] = 255;

		in += 4;
		out += xStep;
	}
}


//!	!!! UNTESTED !!!
inline void
convert_from_CMYK_to_32(uint8* in, uint8* out, int32 inRowBytes, int32 xStep)
{
	for (int32 i = 0; i < inRowBytes; i += 4) {
		int32 black = 255 - in[3];
		out[0] = ((255 - in[2]) * black) / 255;
		out[1] = ((255 - in[1]) * black) / 255;
		out[2] = ((255 - in[0]) * black) / 255;
		out[3] = 255;

		in += 4;
		out += xStep;
	}
}


//!	RGB24 8:8:8 to xRGB 8:8:8:8
inline void
convert_from_24_to_32(uint8* in, uint8* out, int32 inRowBytes, int32 xStep)
{
	for (int32 i = 0; i < inRowBytes; i += 3) {
		out[0] = in[2];
		out[1] = in[1];
		out[2] = in[0];
		out[3] = 255;

		in += 3;
		out += xStep;
	}
}


//! 8-bit to 8-bit, only need when rotating the image
void
translate_8(uint8* in, uint8* out, int32 inRowBytes, int32 xStep)
{
	for (int32 i = 0; i < inRowBytes; i++) {
		out[0] = in[0];

		in++;
		out += xStep;
	}
}


} // namespace conversion


//	#pragma mark -


SSlider::SSlider(const char* name, const char* label,
		BMessage* message, int32 minValue, int32 maxValue, orientation posture,
		thumb_style thumbType, uint32 flags)
	: BSlider(name, label, message, minValue, maxValue,
		posture, thumbType, flags)
{
	rgb_color barColor = { 0, 0, 229, 255 };
	UseFillColor(true, &barColor);
}


//!	Update status string - show actual value
const char*
SSlider::UpdateText() const
{
	snprintf(fStatusLabel, sizeof(fStatusLabel), "%ld", Value());
	return fStatusLabel;
}


//	#pragma mark -


TranslatorReadView::TranslatorReadView(const char* name,
	TranslatorSettings* settings)
	:
	BView(name, 0, new BGroupLayout(B_HORIZONTAL)),
	fSettings(settings)
		// settings should already be Acquired()
{
	fAlwaysRGB32 = new BCheckBox("alwaysrgb32",
		B_TRANSLATE("Read greyscale images as RGB32"),
		new BMessage(VIEW_MSG_SET_ALWAYSRGB32));
	if (fSettings->SetGetBool(JPEG_SET_ALWAYS_RGB32, NULL))
		fAlwaysRGB32->SetValue(B_CONTROL_ON);

	fPhotoshopCMYK = new BCheckBox("photoshopCMYK",
		B_TRANSLATE("Use CMYK code with 0 for 100% ink coverage"),
		new BMessage(VIEW_MSG_SET_PHOTOSHOPCMYK));
	if (fSettings->SetGetBool(JPEG_SET_PHOTOSHOP_CMYK, NULL))
		fPhotoshopCMYK->SetValue(B_CONTROL_ON);

	fShowErrorBox = new BCheckBox("error",
		B_TRANSLATE("Show warning messages"),
		new BMessage(VIEW_MSG_SET_SHOWREADERRORBOX));
	if (fSettings->SetGetBool(JPEG_SET_SHOWREADWARNING, NULL))
		fShowErrorBox->SetValue(B_CONTROL_ON);

	float padding = 5.0f;
	AddChild(BGroupLayoutBuilder(B_VERTICAL, padding)
		.Add(fAlwaysRGB32)
		.Add(fPhotoshopCMYK)
		.Add(fShowErrorBox)
		.AddGlue()
		.SetInsets(padding, padding, padding, padding)
	);

}


TranslatorReadView::~TranslatorReadView()
{
	fSettings->Release();
}


void
TranslatorReadView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fAlwaysRGB32->SetTarget(this);
	fPhotoshopCMYK->SetTarget(this);
	fShowErrorBox->SetTarget(this);
}


void
TranslatorReadView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case VIEW_MSG_SET_ALWAYSRGB32:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				bool boolValue = value;
				fSettings->SetGetBool(JPEG_SET_ALWAYS_RGB32, &boolValue);
				fSettings->SaveSettings();
			}
			break;
		}
		case VIEW_MSG_SET_PHOTOSHOPCMYK:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				bool boolValue = value;
				fSettings->SetGetBool(JPEG_SET_PHOTOSHOP_CMYK, &boolValue);
				fSettings->SaveSettings();
			}
			break;
		}
		case VIEW_MSG_SET_SHOWREADERRORBOX:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				bool boolValue = value;
				fSettings->SetGetBool(JPEG_SET_SHOWREADWARNING, &boolValue);
				fSettings->SaveSettings();
			}
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}


//	#pragma mark - TranslatorWriteView


TranslatorWriteView::TranslatorWriteView(const char* name,
	TranslatorSettings* settings)
	:
	BView(name, 0, new BGroupLayout(B_VERTICAL)),
	fSettings(settings)
		// settings should already be Acquired()
{
	fQualitySlider = new SSlider("quality", B_TRANSLATE("Output quality"),
		new BMessage(VIEW_MSG_SET_QUALITY), 0, 100);
	fQualitySlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fQualitySlider->SetHashMarkCount(10);
	fQualitySlider->SetLimitLabels(B_TRANSLATE("Low"), B_TRANSLATE("High"));
	fQualitySlider->SetValue(fSettings->SetGetInt32(JPEG_SET_QUALITY, NULL));

	fSmoothingSlider = new SSlider("smoothing",
		B_TRANSLATE("Output smoothing strength"),
		new BMessage(VIEW_MSG_SET_SMOOTHING), 0, 100);
	fSmoothingSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fSmoothingSlider->SetHashMarkCount(10);
	fSmoothingSlider->SetLimitLabels(B_TRANSLATE("None"), B_TRANSLATE("High"));
	fSmoothingSlider->SetValue(
		fSettings->SetGetInt32(JPEG_SET_SMOOTHING, NULL));

	fProgress = new BCheckBox("progress",
		B_TRANSLATE("Use progressive compression"),
		new BMessage(VIEW_MSG_SET_PROGRESSIVE));
	if (fSettings->SetGetBool(JPEG_SET_PROGRESSIVE, NULL))
		fProgress->SetValue(B_CONTROL_ON);

	fSmallerFile = new BCheckBox("smallerfile",
		B_TRANSLATE("Make file smaller (sligthtly worse quality)"),
		new BMessage(VIEW_MSG_SET_SMALLERFILE));
	if (fSettings->SetGetBool(JPEG_SET_SMALL_FILES))
		fSmallerFile->SetValue(B_CONTROL_ON);

	fOptimizeColors = new BCheckBox("optimizecolors",
		B_TRANSLATE("Prevent colors 'washing out'"),
		new BMessage(VIEW_MSG_SET_OPTIMIZECOLORS));
	if (fSettings->SetGetBool(JPEG_SET_OPT_COLORS, NULL))
		fOptimizeColors->SetValue(B_CONTROL_ON);
	else
		fSmallerFile->SetEnabled(false);

	fGrayAsRGB24 = new BCheckBox("gray1asrgb24",
		B_TRANSLATE("Write black-and-white images as RGB24"),
		new BMessage(VIEW_MSG_SET_GRAY1ASRGB24));
	if (fSettings->SetGetBool(JPEG_SET_GRAY1_AS_RGB24))
		fGrayAsRGB24->SetValue(B_CONTROL_ON);

	float padding = 5.0f;
	AddChild(BGroupLayoutBuilder(B_VERTICAL, padding)
		.Add(fQualitySlider)
		.Add(fSmoothingSlider)
		.Add(fProgress)
		.Add(fOptimizeColors)
		.Add(fSmallerFile)
		.Add(fGrayAsRGB24)
		.AddGlue()
		.SetInsets(padding, padding, padding, padding)
	);
}


TranslatorWriteView::~TranslatorWriteView()
{
	fSettings->Release();
}


void
TranslatorWriteView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fQualitySlider->SetTarget(this);
	fSmoothingSlider->SetTarget(this);
	fProgress->SetTarget(this);
	fOptimizeColors->SetTarget(this);
	fSmallerFile->SetTarget(this);
	fGrayAsRGB24->SetTarget(this);
}


void
TranslatorWriteView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case VIEW_MSG_SET_QUALITY:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				fSettings->SetGetInt32(JPEG_SET_QUALITY, &value);
				fSettings->SaveSettings();
			}
			break;
		}
		case VIEW_MSG_SET_SMOOTHING:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				fSettings->SetGetInt32(JPEG_SET_SMOOTHING, &value);
				fSettings->SaveSettings();
			}
			break;
		}
		case VIEW_MSG_SET_PROGRESSIVE:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				bool boolValue = value;
				fSettings->SetGetBool(JPEG_SET_PROGRESSIVE, &boolValue);
				fSettings->SaveSettings();
			}
			break;
		}
		case VIEW_MSG_SET_OPTIMIZECOLORS:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				bool boolValue = value;
				fSettings->SetGetBool(JPEG_SET_OPT_COLORS, &boolValue);
				fSmallerFile->SetEnabled(value);
				fSettings->SaveSettings();
			}
			break;
		}
		case VIEW_MSG_SET_SMALLERFILE:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				bool boolValue = value;
				fSettings->SetGetBool(JPEG_SET_SMALL_FILES, &boolValue);
				fSettings->SaveSettings();
			}
			break;
		}
		case VIEW_MSG_SET_GRAY1ASRGB24:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				bool boolValue = value;
				fSettings->SetGetBool(JPEG_SET_GRAY1_AS_RGB24, &boolValue);
				fSettings->SaveSettings();
			}
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}


//	#pragma mark -


TranslatorAboutView::TranslatorAboutView(const char* name)
	:
	BView(name, 0, new BGroupLayout(B_VERTICAL))
{
	BAlignment labelAlignment = BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP);
	BStringView* title = new BStringView("Title", sTranslatorName);
	title->SetFont(be_bold_font);
	title->SetExplicitAlignment(labelAlignment);

	char versionString[16];
	sprintf(versionString, "v%d.%d.%d", (int)(sTranslatorVersion >> 8),
		(int)((sTranslatorVersion >> 4) & 0xf), (int)(sTranslatorVersion & 0xf));

	BStringView* version = new BStringView("Version", versionString);
	version->SetExplicitAlignment(labelAlignment);

	BTextView* infoView = new BTextView("info");
	infoView->SetText(sTranslatorInfo);
	infoView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	infoView->MakeEditable(false);

	float padding = 5.0f;
	AddChild(BGroupLayoutBuilder(B_VERTICAL, padding)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, padding)
			.Add(title)
			.Add(version)
			.AddGlue()
		)
		.Add(infoView)
		.SetInsets(padding, padding, padding, padding)
	);
}


TranslatorView::TranslatorView(const char* name, TranslatorSettings* settings)
	:
	BTabView(name)
{
	AddTab(new TranslatorWriteView(B_TRANSLATE("Write"), settings->Acquire()));
	AddTab(new TranslatorReadView(B_TRANSLATE("Read"), settings->Acquire()));
	AddTab(new TranslatorAboutView(B_TRANSLATE("About")));

	settings->Release();

 	BFont font;
 	GetFont(&font);
 	SetExplicitPreferredSize(
		BSize((font.Size() * 380) / 12, (font.Size() * 250) / 12));
}


//	#pragma mark - Translator Add-On


BView*
JPEGTranslator::NewConfigView(TranslatorSettings* settings)
{
	BView* configView = new TranslatorView("TranslatorView", settings);
	return configView;
}


/*! Determine whether or not we can handle this data */
status_t
JPEGTranslator::DerivedIdentify(BPositionIO* inSource,
	const translation_format* inFormat, BMessage* ioExtension,
	translator_info* outInfo, uint32 outType)
{
	if (outType != 0 && outType != B_TRANSLATOR_BITMAP && outType != JPEG_FORMAT)
		return B_NO_TRANSLATOR;

	// !!! You might need to make this buffer bigger to test for your native format
	off_t position = inSource->Position();
	char header[sizeof(TranslatorBitmap)];
	status_t err = inSource->Read(header, sizeof(TranslatorBitmap));
	inSource->Seek(position, SEEK_SET);
	if (err < B_OK)
		return err;

	if (B_BENDIAN_TO_HOST_INT32(((TranslatorBitmap *)header)->magic) == B_TRANSLATOR_BITMAP) {
		if (PopulateInfoFromFormat(outInfo, B_TRANSLATOR_BITMAP) != B_OK)
			return B_NO_TRANSLATOR;
	} else {
		// First 3 bytes in jpg files are always the same from what i've seen so far
		// check them
		if (header[0] == (char)0xff && header[1] == (char)0xd8 && header[2] == (char)0xff) {
			if (PopulateInfoFromFormat(outInfo, JPEG_FORMAT) != B_OK)
				return B_NO_TRANSLATOR;

		} else
			return B_NO_TRANSLATOR;
	}

	return B_OK;
}


status_t
JPEGTranslator::DerivedTranslate(BPositionIO* inSource,
	const translator_info* inInfo, BMessage* ioExtension, uint32 outType,
	BPositionIO* outDestination, int32 baseType)
{
	// If no specific type was requested, convert to the interchange format
	if (outType == 0)
		outType = B_TRANSLATOR_BITMAP;

	// Setup a "breakpoint" since throwing exceptions does not seem to work
	// at all in an add-on. (?)
	// In the be_jerror.cpp we implement a handler for critical library errors
	// (be_error_exit()) and there we use the longjmp() function to return to
	// this place. If this happens, it is as if the setjmp() call is called
	// a second time, but this time the return value will be 1. The first
	// invokation will return 0.
	jmp_buf longJumpBuffer;
	int jmpRet = setjmp(longJumpBuffer);
	if (jmpRet == 1)
		return B_ERROR;

	try {
		// What action to take, based on the findings of Identify()
		if (outType == inInfo->type) {
			return Copy(inSource, outDestination);
		} else if (inInfo->type == B_TRANSLATOR_BITMAP
				&& outType == JPEG_FORMAT) {
			return Compress(inSource, outDestination, &longJumpBuffer);
		} else if (inInfo->type == JPEG_FORMAT
				&& outType == B_TRANSLATOR_BITMAP) {
			return Decompress(inSource, outDestination, ioExtension,
				&longJumpBuffer);
		}
	} catch (...) {
		syslog(LOG_ERR, "libjpeg encountered a critical error (caught C++ "
			"exception).\n");
		return B_ERROR;
	}

	return B_NO_TRANSLATOR;
}


/*!	The user has requested the same format for input and output, so just copy */
status_t
JPEGTranslator::Copy(BPositionIO* in, BPositionIO* out)
{
	int block_size = 65536;
	void* buffer = malloc(block_size);
	char temp[1024];
	if (buffer == NULL) {
		buffer = temp;
		block_size = 1024;
	}
	status_t err = B_OK;

	// Read until end of file or error
	while (1) {
		ssize_t to_read = block_size;
		err = in->Read(buffer, to_read);
		// Explicit check for EOF
		if (err == -1) {
			if (buffer != temp) free(buffer);
			return B_OK;
		}
		if (err <= B_OK) break;
		to_read = err;
		err = out->Write(buffer, to_read);
		if (err != to_read) if (err >= 0) err = B_DEVICE_FULL;
		if (err < B_OK) break;
	}

	if (buffer != temp) free(buffer);
	return (err >= 0) ? B_OK : err;
}


/*!	Encode into the native format */
status_t
JPEGTranslator::Compress(BPositionIO* in, BPositionIO* out,
	const jmp_buf* longJumpBuffer)
{
	using namespace conversion;

	// Read info about bitmap
	TranslatorBitmap header;
	status_t err = in->Read(&header, sizeof(TranslatorBitmap));
	if (err < B_OK)
		return err;
	else if (err < (int)sizeof(TranslatorBitmap))
		return B_ERROR;

	// Grab dimension, color space, and size information from the stream
	BRect bounds;
	bounds.left = B_BENDIAN_TO_HOST_FLOAT(header.bounds.left);
	bounds.top = B_BENDIAN_TO_HOST_FLOAT(header.bounds.top);
	bounds.right = B_BENDIAN_TO_HOST_FLOAT(header.bounds.right);
	bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(header.bounds.bottom);

	int32 in_row_bytes = B_BENDIAN_TO_HOST_INT32(header.rowBytes);

	int width = bounds.IntegerWidth() + 1;
	int height = bounds.IntegerHeight() + 1;

	// Function pointer to convert function
	// It will point to proper function if needed
	void (*converter)(uchar* inscanline, uchar* outscanline,
		int32 inRowBytes) = NULL;

	// Default color info
	J_COLOR_SPACE jpg_color_space = JCS_RGB;
	int jpg_input_components = 3;
	int32 out_row_bytes;
	int padding = 0;

	switch ((color_space)B_BENDIAN_TO_HOST_INT32(header.colors)) {
		case B_CMAP8:
			converter = convert_from_cmap8_to_24;
			padding = in_row_bytes - width;
			break;

		case B_GRAY1:
			if (fSettings->SetGetBool(JPEG_SET_GRAY1_AS_RGB24, NULL)) {
				converter = convert_from_gray1_to_24;
			} else {
				jpg_input_components = 1;
				jpg_color_space = JCS_GRAYSCALE;
				converter = convert_from_gray1_to_gray8;
			}
			padding = in_row_bytes - (width / 8);
			break;

		case B_GRAY8:
			jpg_input_components = 1;
			jpg_color_space = JCS_GRAYSCALE;
			padding = in_row_bytes - width;
			break;

		case B_RGB15:
		case B_RGBA15:
			converter = convert_from_15_to_24;
			padding = in_row_bytes - (width * 2);
			break;

		case B_RGB15_BIG:
		case B_RGBA15_BIG:
			converter = convert_from_15b_to_24;
			padding = in_row_bytes - (width * 2);
			break;

		case B_RGB16:
			converter = convert_from_16_to_24;
			padding = in_row_bytes - (width * 2);
			break;

		case B_RGB16_BIG:
			converter = convert_from_16b_to_24;
			padding = in_row_bytes - (width * 2);
			break;

		case B_RGB24:
			converter = convert_from_24_to_24;
			padding = in_row_bytes - (width * 3);
			break;

		case B_RGB24_BIG:
			padding = in_row_bytes - (width * 3);
			break;

		case B_RGB32:
		case B_RGBA32:
			converter = convert_from_32_to_24;
			padding = in_row_bytes - (width * 4);
			break;

		case B_RGB32_BIG:
		case B_RGBA32_BIG:
			converter = convert_from_32b_to_24;
			padding = in_row_bytes - (width * 4);
			break;

		case B_CMYK32:
			jpg_color_space = JCS_CMYK;
			jpg_input_components = 4;
			padding = in_row_bytes - (width * 4);
			break;

		default:
			syslog(LOG_ERR, "Wrong type: Color space not implemented.\n");
			return B_ERROR;
	}
	out_row_bytes = jpg_input_components * width;

	// Set basic things needed for jpeg writing
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = be_jpeg_std_error(&jerr, fSettings, longJumpBuffer);
	jpeg_create_compress(&cinfo);
	be_jpeg_stdio_dest(&cinfo, out);

	// Set basic values
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = jpg_input_components;
	cinfo.in_color_space = jpg_color_space;
	jpeg_set_defaults(&cinfo);

	// Set better accuracy
	cinfo.dct_method = JDCT_ISLOW;

	// This is needed to prevent some colors loss
	// With it generated jpegs are as good as from Fireworks (at last! :D)
	if (fSettings->SetGetBool(JPEG_SET_OPT_COLORS, NULL)) {
		int index = 0;
		while (index < cinfo.num_components) {
			cinfo.comp_info[index].h_samp_factor = 1;
			cinfo.comp_info[index].v_samp_factor = 1;
			// This will make file smaller, but with worse quality more or less
			// like with 93%-94% (but it's subjective opinion) on tested images
			// but with smaller size (between 92% and 93% on tested images)
			if (fSettings->SetGetBool(JPEG_SET_SMALL_FILES))
				cinfo.comp_info[index].quant_tbl_no = 1;
			// This will make bigger file, but also better quality ;]
			// from my tests it seems like useless - better quality with smaller
			// can be acheived without this
//			cinfo.comp_info[index].dc_tbl_no = 1;
//			cinfo.comp_info[index].ac_tbl_no = 1;
			index++;
		}
	}

	// Set quality
	jpeg_set_quality(&cinfo, fSettings->SetGetInt32(JPEG_SET_QUALITY, NULL), true);

	// Set progressive compression if needed
	// if not, turn on optimizing in libjpeg
	if (fSettings->SetGetBool(JPEG_SET_PROGRESSIVE, NULL))
		jpeg_simple_progression(&cinfo);
	else
		cinfo.optimize_coding = TRUE;

	// Set smoothing (effect like Blur)
	cinfo.smoothing_factor = fSettings->SetGetInt32(JPEG_SET_SMOOTHING, NULL);

	// Initialize compression
	jpeg_start_compress(&cinfo, TRUE);

	// Declare scanlines
	JSAMPROW in_scanline = NULL;
	JSAMPROW out_scanline = NULL;
	JSAMPROW writeline;
		// Pointer to in_scanline (default) or out_scanline (if there will be conversion)

	// Allocate scanline
	// Use libjpeg memory allocation functions, so in case of error it will free them itself
	in_scanline = (unsigned char *)(cinfo.mem->alloc_large)((j_common_ptr)&cinfo,
		JPOOL_PERMANENT, in_row_bytes);

	// We need 2nd scanline storage ony for conversion
	if (converter != NULL) {
		// There will be conversion, allocate second scanline...
		// Use libjpeg memory allocation functions, so in case of error it will free them itself
	    out_scanline = (unsigned char *)(cinfo.mem->alloc_large)((j_common_ptr)&cinfo,
	    	JPOOL_PERMANENT, out_row_bytes);
		// ... and make it the one to write to file
		writeline = out_scanline;
	} else
		writeline = in_scanline;

	while (cinfo.next_scanline < cinfo.image_height) {
		// Read scanline
		err = in->Read(in_scanline, in_row_bytes);
		if (err < in_row_bytes)
			return err < B_OK ? Error((j_common_ptr)&cinfo, err)
				: Error((j_common_ptr)&cinfo, B_ERROR);

		// Convert if needed
		if (converter != NULL)
			converter(in_scanline, out_scanline, in_row_bytes - padding);

		// Write scanline
	   	jpeg_write_scanlines(&cinfo, &writeline, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	return B_OK;
}


/*!	Decode the native format */
status_t
JPEGTranslator::Decompress(BPositionIO* in, BPositionIO* out,
	BMessage* ioExtension, const jmp_buf* longJumpBuffer)
{
	using namespace conversion;

	// Set basic things needed for jpeg reading
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = be_jpeg_std_error(&jerr, fSettings, longJumpBuffer);
	jpeg_create_decompress(&cinfo);
	be_jpeg_stdio_src(&cinfo, in);

	jpeg_save_markers(&cinfo, MARKER_EXIF, 131072);
		// make sure the EXIF tag is stored

	// Read info about image
	jpeg_read_header(&cinfo, TRUE);

	BMessage exif;

	// parse EXIF data and add it ioExtension, if any
	jpeg_marker_struct* marker = cinfo.marker_list;
	while (marker != NULL) {
		if (marker->marker == MARKER_EXIF
			&& !strncmp((char*)marker->data, "Exif", 4)) {
			if (ioExtension != NULL) {
				// Strip EXIF header from TIFF data
				ioExtension->AddData("exif", B_RAW_TYPE,
					(uint8*)marker->data + 6, marker->data_length - 6);
			}

			BMemoryIO io(marker->data + 6, marker->data_length - 6);
			convert_exif_to_message(io, exif);
		}
		marker = marker->next;
	}

	// Default color info
	color_space outColorSpace = B_RGB32;
	int outColorComponents = 4;

	// Function pointer to convert function
	// It will point to proper function if needed
	void (*converter)(uchar* inScanLine, uchar* outScanLine,
		int32 inRowBytes, int32 xStep) = convert_from_24_to_32;

	// If color space isn't rgb
	if (cinfo.out_color_space != JCS_RGB) {
		switch (cinfo.out_color_space) {
			case JCS_UNKNOWN:		/* error/unspecified */
				syslog(LOG_ERR, "From Type: Jpeg uses unknown color type\n");
				break;
			case JCS_GRAYSCALE:		/* monochrome */
				// Check if user wants to read only as RGB32 or not
				if (!fSettings->SetGetBool(JPEG_SET_ALWAYS_RGB32, NULL)) {
					// Grayscale
					outColorSpace = B_GRAY8;
					outColorComponents = 1;
					converter = translate_8;
				} else {
					// RGB
					cinfo.out_color_space = JCS_RGB;
					cinfo.output_components = 3;
					converter = convert_from_24_to_32;
				}
				break;
			case JCS_YCbCr:		/* Y/Cb/Cr (also known as YUV) */
				cinfo.out_color_space = JCS_RGB;
				converter = convert_from_24_to_32;
				break;
			case JCS_YCCK:		/* Y/Cb/Cr/K */
				// Let libjpeg convert it to CMYK
				cinfo.out_color_space = JCS_CMYK;
				// Fall through to CMYK since we need the same settings
			case JCS_CMYK:		/* C/M/Y/K */
				// Use proper converter
				if (fSettings->SetGetBool(JPEG_SET_PHOTOSHOP_CMYK))
					converter = convert_from_CMYK_to_32_photoshop;
				else
					converter = convert_from_CMYK_to_32;
				break;
			default:
				syslog(LOG_ERR,
						"From Type: Jpeg uses hmm... i don't know really :(\n");
				break;
		}
	}

	// Initialize decompression
	jpeg_start_decompress(&cinfo);

	// retrieve orientation from settings/EXIF
	int32 orientation;
	if (ioExtension == NULL
		|| ioExtension->FindInt32("exif:orientation", &orientation) != B_OK) {
		if (exif.FindInt32("Orientation", &orientation) != B_OK)
			orientation = 1;
	}

	if (orientation != 1 && converter == NULL)
		converter = translate_8;

	int32 outputWidth = orientation > 4 ? cinfo.output_height : cinfo.output_width;
	int32 outputHeight = orientation > 4 ? cinfo.output_width : cinfo.output_height;

	int32 destOffset = dest_index(outputWidth, outputHeight,
		0, 0, orientation) * outColorComponents;
	int32 xStep = dest_index(outputWidth, outputHeight,
		1, 0, orientation) * outColorComponents - destOffset;
	int32 yStep = dest_index(outputWidth, outputHeight,
		0, 1, orientation) * outColorComponents - destOffset;
	bool needAll = orientation != 1;

	// Initialize this bounds rect to the size of your image
	BRect bounds(0, 0, outputWidth - 1, outputHeight - 1);

#if 0
printf("destOffset = %ld, xStep = %ld, yStep = %ld, input: %ld x %ld, output: %ld x %ld, orientation %ld\n",
	destOffset, xStep, yStep, (int32)cinfo.output_width, (int32)cinfo.output_height,
	bounds.IntegerWidth() + 1, bounds.IntegerHeight() + 1, orientation);
#endif

	// Bytes count in one line of image (scanline)
	int32 inRowBytes = cinfo.output_width * cinfo.output_components;
	int32 rowBytes = (bounds.IntegerWidth() + 1) * outColorComponents;
	int32 dataSize = cinfo.output_width * cinfo.output_height
		* outColorComponents;

	// Fill out the B_TRANSLATOR_BITMAP's header
	TranslatorBitmap header;
	header.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	header.bounds.left = B_HOST_TO_BENDIAN_FLOAT(bounds.left);
	header.bounds.top = B_HOST_TO_BENDIAN_FLOAT(bounds.top);
	header.bounds.right = B_HOST_TO_BENDIAN_FLOAT(bounds.right);
	header.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT(bounds.bottom);
	header.colors = (color_space)B_HOST_TO_BENDIAN_INT32(outColorSpace);
	header.rowBytes = B_HOST_TO_BENDIAN_INT32(rowBytes);
	header.dataSize = B_HOST_TO_BENDIAN_INT32(dataSize);

	// Write out the header
	status_t err = out->Write(&header, sizeof(TranslatorBitmap));
	if (err < B_OK)
		return Error((j_common_ptr)&cinfo, err);
	else if (err < (int)sizeof(TranslatorBitmap))
		return Error((j_common_ptr)&cinfo, B_ERROR);

	// Declare scanlines
	JSAMPROW inScanLine = NULL;
	uint8* dest = NULL;
	uint8* destLine = NULL;

	// Allocate scanline
	// Use libjpeg memory allocation functions, so in case of error it will free them itself
    inScanLine = (unsigned char *)(cinfo.mem->alloc_large)((j_common_ptr)&cinfo,
    	JPOOL_PERMANENT, inRowBytes);

	// We need 2nd scanline storage only for conversion
	if (converter != NULL) {
		// There will be conversion, allocate second scanline...
		// Use libjpeg memory allocation functions, so in case of error it will
		// free them itself
	    dest = (uint8*)(cinfo.mem->alloc_large)((j_common_ptr)&cinfo,
	    	JPOOL_PERMANENT, needAll ? dataSize : rowBytes);
	    destLine = dest + destOffset;
	} else
		destLine = inScanLine;

	while (cinfo.output_scanline < cinfo.output_height) {
		// Read scanline
		jpeg_read_scanlines(&cinfo, &inScanLine, 1);

		// Convert if needed
		if (converter != NULL)
			converter(inScanLine, destLine, inRowBytes, xStep);

		if (!needAll) {
	  		// Write the scanline buffer to the output stream
			ssize_t bytesWritten = out->Write(destLine, rowBytes);
			if (bytesWritten < rowBytes) {
				return bytesWritten < B_OK
					? Error((j_common_ptr)&cinfo, bytesWritten)
					: Error((j_common_ptr)&cinfo, B_ERROR);
			}
		} else
			destLine += yStep;
	}

	if (needAll) {
		ssize_t bytesWritten = out->Write(dest, dataSize);
		if (bytesWritten < dataSize) {
			return bytesWritten < B_OK
				? Error((j_common_ptr)&cinfo, bytesWritten)
				: Error((j_common_ptr)&cinfo, B_ERROR);
		}
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	return B_OK;
}

/*! have the other PopulateInfoFromFormat() check both inputFormats & outputFormats */
status_t
JPEGTranslator::PopulateInfoFromFormat(translator_info* info,
	uint32 formatType, translator_id id)
{
	int32 formatCount;
	const translation_format* formats = OutputFormats(&formatCount);
	for (int i = 0; i <= 1 ;formats = InputFormats(&formatCount), i++) {
		if (PopulateInfoFromFormat(info, formatType,
			formats, formatCount) == B_OK) {
			info->translator = id;
			return B_OK;
		}
	}

	return B_ERROR;
}


status_t
JPEGTranslator::PopulateInfoFromFormat(translator_info* info,
	uint32 formatType, const translation_format* formats, int32 formatCount)
{
	for (int i = 0; i < formatCount; i++) {
		if (formats[i].type == formatType) {
			info->type = formatType;
			info->group = formats[i].group;
			info->quality = formats[i].quality;
			info->capability = formats[i].capability;
			BString str1(formats[i].name);
			str1.ReplaceFirst("Be Bitmap Format (JPEGTranslator)", 
				B_TRANSLATE("Be Bitmap Format (JPEGTranslator)"));
			strncpy(info->name, str1.String(), sizeof(info->name));
			strcpy(info->MIME,  formats[i].MIME);
			return B_OK;
		}
	}

	return B_ERROR;
}

/*!
	Frees jpeg alocated memory
	Returns given error (B_ERROR by default)
*/
status_t
JPEGTranslator::Error(j_common_ptr cinfo, status_t error)
{
	jpeg_destroy(cinfo);
	return error;
}


JPEGTranslator::JPEGTranslator()
	: BaseTranslator(sTranslatorName, sTranslatorInfo, sTranslatorVersion,
		sInputFormats,  kNumInputFormats,
		sOutputFormats, kNumOutputFormats,
		SETTINGS_FILE,
		sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_BITMAP, JPEG_FORMAT)
{}


BTranslator*
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (n == 0)
		return new JPEGTranslator();

	return NULL;
}


int
main(int, char**)
{
	BApplication app("application/x-vnd.Haiku-JPEGTranslator");
	JPEGTranslator* translator = new JPEGTranslator();
	if (LaunchTranslatorWindow(translator, sTranslatorName) == B_OK)
		app.Run();

	return 0;
}

