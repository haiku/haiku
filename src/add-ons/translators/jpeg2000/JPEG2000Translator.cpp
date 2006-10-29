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


#include "JPEG2000Translator.h"
#include "jp2_cod.h"
#include "jpc_cs.h"

#include <TabView.h>


// Set these accordingly
#define JP2_ACRONYM "JP2"
#define JP2_FORMAT 'JP2 '
#define JP2_MIME_STRING "image/jp2"
#define JP2_DESCRIPTION "JPEG2000 image"

// The translation kit's native file type
#define B_TRANSLATOR_BITMAP_MIME_STRING "image/x-be-bitmap"
#define B_TRANSLATOR_BITMAP_DESCRIPTION "Be Bitmap image"

// Translation Kit required globals
char translatorName[] = "JPEG2000 Images";
char translatorInfo[] = "©2002-2003, Shard\n"
	"©2005-2006, Haiku\n"
	"\n"
	"Based on JasPer library:\n"
	"© 1999-2000, Image Power, Inc. and\n"
	"the University of British Columbia, Canada.\n"
	"© 2001-2003 Michael David Adams.\n"
	"          http://www.ece.uvic.ca/~mdadams/jasper/\n"
	"\n"
	"ImageMagick's jp2 codec was used as \"tutorial\".\n"
	"          http://www.imagemagick.org/\n";

int32 translatorVersion = 0x100;

// Define the formats we know how to read
translation_format inputFormats[] = {
	{ JP2_FORMAT, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		JP2_MIME_STRING, JP2_DESCRIPTION },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		B_TRANSLATOR_BITMAP_MIME_STRING, B_TRANSLATOR_BITMAP_DESCRIPTION },
	{},
};

// Define the formats we know how to write
translation_format outputFormats[] = {
	{ JP2_FORMAT, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		JP2_MIME_STRING, JP2_DESCRIPTION },
	{ B_TRANSLATOR_BITMAP, B_TRANSLATOR_BITMAP, 0.5, 0.5,
		B_TRANSLATOR_BITMAP_MIME_STRING, B_TRANSLATOR_BITMAP_DESCRIPTION },
	{},
};

bool gAreSettingsRunning = false;


//!	Make settings to defaults
void
LoadDefaultSettings(jpeg_settings *settings)
{
	settings->Quality = 25;
	settings->JPC = false;
	settings->B_GRAY1_as_B_RGB24 = false;
	settings->B_GRAY8_as_B_RGB32 = true;
}


//!	Save settings to config file
void
SaveSettings(jpeg_settings *settings)
{
	// Make path to settings file
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return;

	path.Append(SETTINGS_FILE);

	// Open settings file (create it if there's no file) and write settings			
	FILE *file = NULL;
	if ((file = fopen(path.Path(), "wb+"))) {
		fwrite(settings, sizeof(jpeg_settings), 1, file);
		fclose(file);
	}
}


//!	Return true if settings were run, false if not
bool
SettingsChangedAlert()
{
	// If settings view wasn't already initialized (settings not running)
	// and user wants to run settings
	if (!gAreSettingsRunning && (new BAlert("Different settings file", "JPEG2000 settings were set to default because of incompatible settings file.", "Configure settings", "OK", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go() == 0) {
		// Create settings window (with no quit on close!), launch it and wait until it's closed
		status_t err;
		TranslatorWindow *window = new TranslatorWindow(false);
		window->Show();
		wait_for_thread(window->Thread(), &err);
		return true;
	}

	return false;
}


/*!
	Load settings from config file
	If can't find it make them default and try to save
*/
void
LoadSettings(jpeg_settings *settings)
{
	// Make path to settings file
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK) {
		LoadDefaultSettings(settings);
		return;
	}

	path.Append(SETTINGS_FILE);

	// Open settings file (create it if there's no file) and write settings			
	FILE *file = NULL;
	if ((file = fopen(path.Path(), "rb"))) {
		if (!fread(settings, sizeof(jpeg_settings), 1, file)) {
			// settings struct has changed size
			// Load default settings, and Save them
			fclose(file);
			LoadDefaultSettings(settings);
			SaveSettings(settings);
			// Tell user settings were changed to default, and ask to run settings panel or not
			if (SettingsChangedAlert())
				// User configured settings, load them again
				LoadSettings(settings);
		} else
			fclose(file);
	} else if ((file = fopen(path.Path(), "wb+"))) {
		LoadDefaultSettings(settings);
		fwrite(settings, sizeof(jpeg_settings), 1, file);
		fclose(file);
		// Tell user settings were changed to default, and ask to run settings panel or not
		if (SettingsChangedAlert())
			// User configured settings, load them again
			LoadSettings(settings);
	}
}


//	#pragma mark - conversion routines


//!	Make RGB32 scanline from *pixels[3]
inline void
read_rgb24_to_rgb32(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width) {
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[2], x);
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[1], x);
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[0], x);
		scanline[index++] = 255;
		x++;
	}
}


//!	Make gray scanline from *pixels[1]
inline void
read_gray_to_rgb32(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	jpr_uchar_t color = 0;
	while (x < width) {
		color = (jpr_uchar_t)jas_matrix_getv(pixels[0], x++);
		scanline[index++] = color;
		scanline[index++] = color;
		scanline[index++] = color;
		scanline[index++] = 255;
	}
}


/*!
	Make RGBA32 scanline from *pixels[4]
	(just read data to scanline)
*/
inline void
read_rgba32(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width) {
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[2], x);
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[1], x);
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[0], x);
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[3], x);
		x++;
	}
}


/*!
	Make gray scanline from *pixels[1]
	(just read data to scanline)
*/
inline void
read_gray(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 x = 0;
	while (x < width) {
		scanline[x] = (jpr_uchar_t)jas_matrix_getv(pixels[0], x);
		x++;
	}
}


//!	Make *pixels[1] from gray1 scanline
inline void
write_gray1_to_gray(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 x = 0;
	int32 index = 0;
	while (x < (width/8)) {
		unsigned char c = scanline[x++];
		for (int b = 128; b; b = b >> 1) {
			if (c & b)
				jas_matrix_setv(pixels[0], index++, 0);
			else
				jas_matrix_setv(pixels[0], index++, 255);
		}
	}
}


//!	Make *pixels[3] from gray1 scanline
inline void
write_gray1_to_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 x = 0;
	int32 index = 0;
	while (x < (width/8)) {
		unsigned char c = scanline[x++];
		for (int b = 128; b; b = b >> 1) {
			if (c & b) {
				jas_matrix_setv(pixels[0], index, 0);
				jas_matrix_setv(pixels[1], index, 0);
				jas_matrix_setv(pixels[2], index, 0);
			} else {
				jas_matrix_setv(pixels[0], index, 255);
				jas_matrix_setv(pixels[1], index, 255);
				jas_matrix_setv(pixels[2], index, 255);
			}
			index++;
		}
	}
}


//!	Make *pixels[3] from cmap8 scanline
inline void
write_cmap8_to_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	const color_map *map = system_colors();
	int32 x = 0;
	while (x < width) {
		rgb_color color = map->color_list[scanline[x]];

		jas_matrix_setv(pixels[0], x, color.red);
		jas_matrix_setv(pixels[1], x, color.green);
		jas_matrix_setv(pixels[2], x, color.blue);
		x++;
	}
}


/*!
	Make *pixels[1] from gray scanline
	(just write data to pixels)
*/
inline void
write_gray(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 x = 0;
	while (x < width) {
		jas_matrix_setv(pixels[0], x, scanline[x]);
		x++;
	}
}


/*!
	Make *pixels[3] from RGB15/RGBA15 scanline
	(just write data to pixels)
*/
inline void
write_rgb15_to_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 x = 0;
	int32 index = 0;
	int16 in_pixel;
	while (x < width) {
		in_pixel = scanline[index] | (scanline[index+1] << 8);
		index += 2;

		jas_matrix_setv(pixels[0], x, (char)(((in_pixel & 0x7c00)) >> 7) | (((in_pixel & 0x7c00)) >> 12));
		jas_matrix_setv(pixels[1], x, (char)(((in_pixel & 0x3e0)) >> 2) | (((in_pixel & 0x3e0)) >> 7));
		jas_matrix_setv(pixels[2], x, (char)(((in_pixel & 0x1f)) << 3) | (((in_pixel & 0x1f)) >> 2));
		x++;
	}
}


/*!
	Make *pixels[3] from RGB15/RGBA15 bigendian scanline
	(just write data to pixels)
*/
inline void
write_rgb15b_to_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 x = 0;
	int32 index = 0;
	int16 in_pixel;
	while (x < width) {
		in_pixel = scanline[index+1] | (scanline[index] << 8);
		index += 2;

		jas_matrix_setv(pixels[0], x, (char)(((in_pixel & 0x7c00)) >> 7) | (((in_pixel & 0x7c00)) >> 12));
		jas_matrix_setv(pixels[1], x, (char)(((in_pixel & 0x3e0)) >> 2) | (((in_pixel & 0x3e0)) >> 7));
		jas_matrix_setv(pixels[2], x, (char)(((in_pixel & 0x1f)) << 3) | (((in_pixel & 0x1f)) >> 2));
		x++;
	}
}


/*!
	Make *pixels[3] from RGB16/RGBA16 scanline
	(just write data to pixels)
*/
inline void
write_rgb16_to_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 x = 0;
	int32 index = 0;
	int16 in_pixel;
	while (x < width) {
		in_pixel = scanline[index] | (scanline[index+1] << 8);
		index += 2;

		jas_matrix_setv(pixels[0], x, (char)(((in_pixel & 0xf800)) >> 8) | (((in_pixel & 0x7c00)) >> 12));
		jas_matrix_setv(pixels[1], x, (char)(((in_pixel & 0x7e0)) >> 3) | (((in_pixel & 0x7e0)) >> 9));
		jas_matrix_setv(pixels[2], x, (char)(((in_pixel & 0x1f)) << 3) | (((in_pixel & 0x1f)) >> 2));
		x++;
	}
}


/*!
	Make *pixels[3] from RGB16/RGBA16 bigendian scanline
	(just write data to pixels)
*/
inline void
write_rgb16b_to_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 x = 0;
	int32 index = 0;
	int16 in_pixel;
	while (x < width) {
		in_pixel = scanline[index+1] | (scanline[index] << 8);
		index += 2;

		jas_matrix_setv(pixels[0], x, (char)(((in_pixel & 0xf800)) >> 8) | (((in_pixel & 0xf800)) >> 13));
		jas_matrix_setv(pixels[1], x, (char)(((in_pixel & 0x7e0)) >> 3) | (((in_pixel & 0x7e0)) >> 9));
		jas_matrix_setv(pixels[2], x, (char)(((in_pixel & 0x1f)) << 3) | (((in_pixel & 0x1f)) >> 2));
		x++;
	}
}


/*!
	Make *pixels[3] from RGB24 scanline
	(just write data to pixels)
*/
inline void
write_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width) {
		jas_matrix_setv(pixels[2], x, scanline[index++]);
		jas_matrix_setv(pixels[1], x, scanline[index++]);
		jas_matrix_setv(pixels[0], x, scanline[index++]);
		x++;
	}
}


/*!
	Make *pixels[3] from RGB24 bigendian scanline
	(just write data to pixels)
*/
inline void
write_rgb24b(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width) {
		jas_matrix_setv(pixels[0], x, scanline[index++]);
		jas_matrix_setv(pixels[1], x, scanline[index++]);
		jas_matrix_setv(pixels[2], x, scanline[index++]);
		x++;
	}
}


/*!
	Make *pixels[3] from RGB32 scanline
	(just write data to pixels)
*/
inline void
write_rgb32_to_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width) {
		jas_matrix_setv(pixels[2], x, scanline[index++]);
		jas_matrix_setv(pixels[1], x, scanline[index++]);
		jas_matrix_setv(pixels[0], x, scanline[index++]);
		index++;
		x++;
	}
}


/*!
	Make *pixels[3] from RGB32 bigendian scanline
	(just write data to pixels)
*/
inline void
write_rgb32b_to_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width) {
		index++;
		jas_matrix_setv(pixels[0], x, scanline[index++]);
		jas_matrix_setv(pixels[1], x, scanline[index++]);
		jas_matrix_setv(pixels[2], x, scanline[index++]);
		x++;
	}
}


/*!
	Make *pixels[4] from RGBA32 scanline
	(just write data to pixels)
	!!! UNTESTED !!!
*/
inline void
write_rgba32(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width) {
		jas_matrix_setv(pixels[3], x, scanline[index++]);
		jas_matrix_setv(pixels[2], x, scanline[index++]);
		jas_matrix_setv(pixels[1], x, scanline[index++]);
		jas_matrix_setv(pixels[0], x, scanline[index++]);
		x++;
	}
}


/*!
	Make *pixels[4] from RGBA32 bigendian scanline
	(just write data to pixels)
	!!! UNTESTED !!!
*/
inline void
write_rgba32b(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width)
	{
		jas_matrix_setv(pixels[0], x, scanline[index++]);
		jas_matrix_setv(pixels[1], x, scanline[index++]);
		jas_matrix_setv(pixels[2], x, scanline[index++]);
		jas_matrix_setv(pixels[3], x, scanline[index++]);
		x++;
	}
}


//	#pragma mark -	jasper I/O


static int
Read(jas_stream_obj_t *object, char *buffer, const int length)
{
	return (*(BPositionIO**)object)->Read(buffer, length);
}


static int
Write(jas_stream_obj_t *object, char *buffer, const int length)
{
	return (*(BPositionIO**)object)->Write(buffer, length);
}


static long
Seek(jas_stream_obj_t *object, long offset, int origin)
{
	return (*(BPositionIO**)object)->Seek(offset, origin);
}


static int
Close(jas_stream_obj_t *object)
{
	return 0;
}


static jas_stream_ops_t positionIOops = {
	Read,
	Write,
	Seek,
	Close
};


static jas_stream_t *
jas_stream_positionIOopen(BPositionIO *positionIO)
{
	jas_stream_t *stream;

	stream = (jas_stream_t *)malloc(sizeof(jas_stream_t));
	if (stream == (jas_stream_t *)NULL)
		return (jas_stream_t *)NULL;

	memset(stream, 0, sizeof(jas_stream_t));
	stream->rwlimit_ = -1;
	stream->obj_=(jas_stream_obj_t *)malloc(sizeof(BPositionIO*));
	if (stream->obj_ == (jas_stream_obj_t *)NULL)
		return (jas_stream_t *)NULL;

	*((BPositionIO**)stream->obj_) = positionIO;
	stream->ops_ = (&positionIOops);
	stream->openmode_ = JAS_STREAM_READ | JAS_STREAM_WRITE | JAS_STREAM_BINARY;
	stream->bufbase_ = stream->tinybuf_;
	stream->bufsize_ = 1;
	stream->bufstart_ = (&stream->bufbase_[JAS_STREAM_MAXPUTBACK]);
	stream->ptr_ = stream->bufstart_;
	stream->bufmode_ |= JAS_STREAM_UNBUF & JAS_STREAM_BUFMODEMASK;

	return stream;
}


//	#pragma mark - SView


SView::SView(const char *name, float x, float y)
	: BView(BRect(x, y, x, y), name, B_FOLLOW_NONE, B_WILL_DRAW)
{
	fPreferredWidth = 0;
	fPreferredHeight = 0;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ViewColor());

	SetFont(be_plain_font);
}


void
SView::GetPreferredSize(float* _width, float* _height)
{
	if (_width)
		*_width = fPreferredWidth;
	if (_height)
		*_height = fPreferredHeight;
}


void
SView::ResizeToPreferred()
{
	ResizeTo(fPreferredWidth, fPreferredHeight);
}


void
SView::ResizePreferredBy(float width, float height)
{
	fPreferredWidth += width;
	fPreferredHeight += height;
}


void
SView::AddChild(BView *child, BView *before)
{
	BView::AddChild(child, before);
	child->ResizeToPreferred();
	BRect frame = child->Frame();

	if (frame.right > fPreferredWidth)
		fPreferredWidth = frame.right;
	if (frame.bottom > fPreferredHeight)
		fPreferredHeight = frame.bottom;
}


//	#pragma mark -


SSlider::SSlider(BRect frame, const char *name, const char *label,
		BMessage *message, int32 minValue, int32 maxValue, orientation posture,
		thumb_style thumbType, uint32 resizingMode, uint32 flags)
	: BSlider(frame, name, label, message, minValue, maxValue,
		posture, thumbType, resizingMode, flags)
{
	rgb_color barColor = { 0, 0, 229, 255 };
	UseFillColor(true, &barColor);
}


//!	Update status string - show actual value
char*
SSlider::UpdateText() const
{
	snprintf(fStatusLabel, sizeof(fStatusLabel), "%ld", Value());
	return fStatusLabel;
}


//!	BSlider::ResizeToPreferred + Resize width if it's too small to show label and status
void
SSlider::ResizeToPreferred()
{
	int32 width = (int32)ceil(StringWidth(Label()) + StringWidth("9999"));
	if (width < 230)
		width = 230;

	float w, h;
	GetPreferredSize(&w, &h);
	ResizeTo(width, h);
}


//	#pragma mark -


TranslatorReadView::TranslatorReadView(const char *name, jpeg_settings *settings,
		float x, float y)
	: SView(name, x, y),
	fSettings(settings)
{
	fGrayAsRGB32 = new BCheckBox(BRect(10, GetPreferredHeight(), 10,
		GetPreferredHeight()), "grayasrgb32", VIEW_LABEL_GRAYASRGB32,
		new BMessage(VIEW_MSG_SET_GRAYASRGB32));
	fGrayAsRGB32->SetFont(be_plain_font);
	if (fSettings->B_GRAY8_as_B_RGB32)
		fGrayAsRGB32->SetValue(1);

	AddChild(fGrayAsRGB32);

	ResizeToPreferred();
}


void
TranslatorReadView::AttachedToWindow()
{
	fGrayAsRGB32->SetTarget(this);
}


void
TranslatorReadView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case VIEW_MSG_SET_GRAYASRGB32:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				fSettings->B_GRAY8_as_B_RGB32 = value;
				SaveSettings(fSettings);
			}
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}


//	#pragma mark - TranslatorWriteView


TranslatorWriteView::TranslatorWriteView(const char *name, jpeg_settings *settings,
		float x, float y)
	: SView(name, x, y),
	fSettings(settings)
{
	fQualitySlider = new SSlider(BRect(10, GetPreferredHeight(), 10,
		GetPreferredHeight()), "quality", VIEW_LABEL_QUALITY,
		new BMessage(VIEW_MSG_SET_QUALITY), 0, 100);
	fQualitySlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fQualitySlider->SetHashMarkCount(10);
	fQualitySlider->SetLimitLabels("Low", "High");
	fQualitySlider->SetFont(be_plain_font);
	fQualitySlider->SetValue(fSettings->Quality);
	AddChild(fQualitySlider);

	fGrayAsRGB24 = new BCheckBox(BRect(10, GetPreferredHeight() + 5, 25,
		GetPreferredHeight() + 5), "gray1asrgb24", VIEW_LABEL_GRAY1ASRGB24,
		new BMessage(VIEW_MSG_SET_GRAY1ASRGB24));
	fGrayAsRGB24->SetFont(be_plain_font);
	if (fSettings->B_GRAY1_as_B_RGB24)
		fGrayAsRGB24->SetValue(1);

	AddChild(fGrayAsRGB24);

	fCodeStreamOnly = new BCheckBox(BRect(10, GetPreferredHeight() + 5, 10,
		GetPreferredHeight()), "codestreamonly", VIEW_LABEL_JPC,
		new BMessage(VIEW_MSG_SET_JPC));
	fCodeStreamOnly->SetFont(be_plain_font);
	if (fSettings->JPC)
		fCodeStreamOnly->SetValue(1);

	AddChild(fCodeStreamOnly);

	ResizeToPreferred();
}


void
TranslatorWriteView::AttachedToWindow()
{
	fQualitySlider->SetTarget(this);
	fGrayAsRGB24->SetTarget(this);
	fCodeStreamOnly->SetTarget(this);
}


void
TranslatorWriteView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case VIEW_MSG_SET_QUALITY:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				fSettings->Quality = value;
				SaveSettings(fSettings);
			}
			break;
		}
		case VIEW_MSG_SET_GRAY1ASRGB24:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				fSettings->B_GRAY1_as_B_RGB24 = value;
				SaveSettings(fSettings);
			}
			break;
		}
		case VIEW_MSG_SET_JPC:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK) {
				fSettings->JPC = value;
				SaveSettings(fSettings);
			}
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}


//	#pragma mark -


TranslatorAboutView::TranslatorAboutView(const char *name, float x, float y)
	: SView(name, x, y)
{
	BStringView *title = new BStringView(BRect(10, 0, 10, 0), "Title",
		translatorName);
	title->SetFont(be_bold_font);

	AddChild(title);

	BRect rect = title->Bounds();
	float space = title->StringWidth("    ");

	char versionString[16];
	sprintf(versionString, "v%d.%d.%d", (int)(translatorVersion >> 8),
		(int)((translatorVersion >> 4) & 0xf), (int)(translatorVersion & 0xf));

	BStringView *version = new BStringView(BRect(rect.right+space, rect.top,
		rect.right+space, rect.top), "Version", versionString);
	version->SetFont(be_plain_font);
	version->SetFontSize(9);
	// Make version be in the same line as title
	version->ResizeToPreferred();
	version->MoveBy(0, rect.bottom-version->Frame().bottom);

	AddChild(version);

	// Now for each line in translatorInfo add a BStringView
	char* current = translatorInfo;
	int32 index = 1;
	while (current != NULL && current[0]) {
		char text[128];
		char* newLine = strchr(current, '\n');
		if (newLine == NULL) {
			strlcpy(text, current, sizeof(text));
			current = NULL;
		} else {
			strlcpy(text, current, min_c((int32)sizeof(text), newLine + 1 - current));
			current = newLine + 1;
		}

		BStringView* string = new BStringView(BRect(10, GetPreferredHeight(),
			10, GetPreferredHeight()), "copyright", text);
		if (index > 3)
			string->SetFontSize(9);
		AddChild(string);

		index++;
	}

	ResizeToPreferred();
}


//	#pragma mark -


TranslatorView::TranslatorView(const char *name)
	: SView(name),
	fTabWidth(30),
	fActiveChild(0)
{
	// Set global var to true
	gAreSettingsRunning = true;

	// Load settings to global settings struct
	LoadSettings(&fSettings);

	font_height fontHeight;
	GetFontHeight(&fontHeight);
	fTabHeight = (int32)ceilf(fontHeight.ascent + fontHeight.descent + fontHeight.leading) + 7;
	// Add left and top margins
	float top = fTabHeight + 20;
	float left = 0;

	// This will remember longest string width
	int32 nameWidth = 0;

	SView *view = new TranslatorWriteView("Write", &fSettings, left, top);
	AddChild(view);
	nameWidth = (int32)StringWidth(view->Name());
	fTabs.AddItem(new BTab(view));

	view = new TranslatorReadView("Read", &fSettings, left, top);
	AddChild(view);
	if (nameWidth < StringWidth(view->Name()))
		nameWidth = (int32)StringWidth(view->Name());
	fTabs.AddItem(new BTab(view));

	view = new TranslatorAboutView("About", left, top);
	AddChild(view);
	if (nameWidth < StringWidth(view->Name()))
		nameWidth = (int32)StringWidth(view->Name());
	fTabs.AddItem(new BTab(view));

	fTabWidth += nameWidth;
	if (fTabWidth * CountChildren() > GetPreferredWidth())
		ResizePreferredBy((fTabWidth * CountChildren()) - GetPreferredWidth(), 0);

	// Add right and bottom margins
	ResizePreferredBy(10, 15);

	ResizeToPreferred();

	// Make TranslatorView resize itself with parent
	SetFlags(Flags() | B_FOLLOW_ALL);
}


TranslatorView::~TranslatorView()
{
	gAreSettingsRunning = false;

	BTab* tab;
	while ((tab = (BTab*)fTabs.RemoveItem((int32)0)) != NULL) {
		delete tab;
	}
}


//!	Attached to window - resize parent to preferred
void
TranslatorView::AttachedToWindow()
{
	// Hide all children except first one
	BView *child;
	int32 index = 1;
	while ((child = ChildAt(index++)) != NULL)
		child->Hide();

}


BRect
TranslatorView::_TabFrame(int32 index) const
{
	return BRect(index * fTabWidth, 10, (index + 1) * fTabWidth, 10 + fTabHeight);
}


void
TranslatorView::Draw(BRect updateRect)
{
	// This is needed because DataTranslations app hides children
	// after user changes translator
	if (ChildAt(fActiveChild)->IsHidden())
		ChildAt(fActiveChild)->Show();

	// Clear
	SetHighColor(ViewColor());
	BRect frame = _TabFrame(0);
	FillRect(BRect(frame.left, frame.top, Bounds().right, frame.bottom - 1));

	int32 index = 0;
	BTab* tab;
	while ((tab = (BTab*)fTabs.ItemAt(index)) != NULL) {
		tab_position position;
		if (fActiveChild == index)
			position = B_TAB_FRONT;
		else if (index == 0)
			position = B_TAB_FIRST;
		else
			position = B_TAB_ANY;

		tab->DrawTab(this, _TabFrame(index), position, index + 1 != fActiveChild);
		index++;
	}

	// Draw bottom edge
	SetHighColor(tint_color(ViewColor(), B_LIGHTEN_MAX_TINT));

	BRect selectedFrame = _TabFrame(fActiveChild);
	float offset = ceilf(frame.Height() / 2.0);

	if (selectedFrame.left > frame.left) {
		StrokeLine(BPoint(frame.left, frame.bottom),
			BPoint(selectedFrame.left, frame.bottom));
	}
	if (selectedFrame.right + offset < Bounds().right) {
		StrokeLine(BPoint(selectedFrame.right + offset, frame.bottom),
			BPoint(Bounds().right, frame.bottom));
	}
}


//!	MouseDown, check if on tab, if so change tab if needed
void
TranslatorView::MouseDown(BPoint where)
{
	BRect frame = _TabFrame(fTabs.CountItems() - 1);
	frame.left = 0;
	if (!frame.Contains(where))
		return;

	for (int32 index = fTabs.CountItems(); index-- > 0;) {
		if (!_TabFrame(index).Contains(where))
			continue;

		if (fActiveChild != index) {
			// Hide current visible child
			ChildAt(fActiveChild)->Hide();

			// This loop is needed because it looks like in DataTranslations
			// view gets hidden more than one time when user changes translator
			while (ChildAt(index)->IsHidden()) {
				ChildAt(index)->Show();
			}

			// Remember which one is currently visible
			fActiveChild = index;
			Invalidate(frame);
			break;
		}
	}
}


//	#pragma mark -


TranslatorWindow::TranslatorWindow(bool quitOnClose)
	: BWindow(BRect(100, 100, 100, 100), "JPEG Settings", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BRect extent(0, 0, 0, 0);
	BView *config = NULL;
	MakeConfig(NULL, &config, &extent);

	AddChild(config);
	ResizeTo(extent.Width(), extent.Height());

	// Make application quit after this window close
	if (quitOnClose)
		SetFlags(Flags() | B_QUIT_ON_WINDOW_CLOSE);
}


//	#pragma mark -


//!	Hook to create and return our configuration view
status_t
MakeConfig(BMessage *ioExtension, BView **outView, BRect *outExtent)
{
	*outView = new TranslatorView("TranslatorView");
	*outExtent = (*outView)->Frame();
	return B_OK;
}


//!	Determine whether or not we can handle this data
status_t
Identify(BPositionIO *inSource, const translation_format *inFormat, BMessage *ioExtension, translator_info *outInfo, uint32 outType)
{
	if ((outType != 0) && (outType != B_TRANSLATOR_BITMAP) && outType != JP2_FORMAT)
		return B_NO_TRANSLATOR;

	// !!! You might need to make this buffer bigger to test for your native format
	off_t position = inSource->Position();
	uint8 header[sizeof(TranslatorBitmap)];
	status_t err = inSource->Read(header, sizeof(TranslatorBitmap));
	inSource->Seek(position, SEEK_SET);
	if (err < B_OK)
		return err;

	if (B_BENDIAN_TO_HOST_INT32(((TranslatorBitmap *)header)->magic) == B_TRANSLATOR_BITMAP) {
		outInfo->type = inputFormats[1].type;
		outInfo->translator = 0;
		outInfo->group = inputFormats[1].group;
		outInfo->quality = inputFormats[1].quality;
		outInfo->capability = inputFormats[1].capability;
		strcpy(outInfo->name, inputFormats[1].name);
		strcpy(outInfo->MIME, inputFormats[1].MIME);
	} else {
		if ((((header[4] << 24) | (header[5] << 16) | (header[6] << 8) | header[7]) == JP2_BOX_JP) ||	// JP2
			(header[0] == (JPC_MS_SOC >> 8) && header[1] == (JPC_MS_SOC & 0xff)))	// JPC
		{
			outInfo->type = inputFormats[0].type;
			outInfo->translator = 0;
			outInfo->group = inputFormats[0].group;
			outInfo->quality = inputFormats[0].quality;
			outInfo->capability = inputFormats[0].capability;
			strcpy(outInfo->name, inputFormats[0].name);
			strcpy(outInfo->MIME, inputFormats[0].MIME);
			return B_OK;
		} else
			return B_NO_TRANSLATOR;
	}

	return B_OK;
}


//!	Arguably the most important method in the add-on
status_t
Translate(BPositionIO *inSource, const translator_info *inInfo, BMessage *ioExtension, uint32 outType, BPositionIO *outDestination)
{
	// If no specific type was requested, convert to the interchange format
	if (outType == 0)
		outType = B_TRANSLATOR_BITMAP;

	// What action to take, based on the findings of Identify()
	if (outType == inInfo->type)
		return Copy(inSource, outDestination);
	if (inInfo->type == B_TRANSLATOR_BITMAP && outType == JP2_FORMAT)
		return Compress(inSource, outDestination);
	if (inInfo->type == JP2_FORMAT && outType == B_TRANSLATOR_BITMAP)
		return Decompress(inSource, outDestination);

	return B_NO_TRANSLATOR;
}


//!	The user has requested the same format for input and output, so just copy
status_t
Copy(BPositionIO *in, BPositionIO *out)
{
	int block_size = 65536;
	void *buffer = malloc(block_size);
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
			if (buffer != temp)
				free(buffer);
			return B_OK;
		}
		if (err <= B_OK) break;
		to_read = err;
		err = out->Write(buffer, to_read);
		if (err != to_read) if (err >= 0) err = B_DEVICE_FULL;
		if (err < B_OK) break;
	}

	if (buffer != temp)
		free(buffer);
	return (err >= 0) ? B_OK : err;
}


//!	Encode into the native format
status_t
Compress(BPositionIO *in, BPositionIO *out)
{
	// Load settings
	jpeg_settings settings;
	LoadSettings(&settings);

	// Read info about bitmap
	TranslatorBitmap header;
	status_t err = in->Read(&header, sizeof(TranslatorBitmap));
	if (err < B_OK)
		return err;
	if (err < (int)sizeof(TranslatorBitmap))
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

	// Function pointer to write function
	// It MUST point to proper function
	void (*converter)(jas_matrix_t **pixels, jpr_uchar_t *inscanline, int width) = write_rgba32;

	// Default color info
	int out_color_space = JAS_IMAGE_CS_RGB;
	int out_color_components = 3;

	switch ((color_space)B_BENDIAN_TO_HOST_INT32(header.colors)) {
		case B_GRAY1:
			if (settings.B_GRAY1_as_B_RGB24) {
				converter = write_gray1_to_rgb24;
			} else {
				out_color_components = 1;
				out_color_space = JAS_IMAGE_CS_GRAY;
				converter = write_gray1_to_gray;
			}
			break;

		case B_CMAP8:
			converter = write_cmap8_to_rgb24;
			break;

		case B_GRAY8:
			out_color_components = 1;
			out_color_space = JAS_IMAGE_CS_GRAY;
			converter = write_gray;
			break;

		case B_RGB15:
		case B_RGBA15:
			converter = write_rgb15_to_rgb24;
			break;

		case B_RGB15_BIG:
		case B_RGBA15_BIG:
			converter = write_rgb15b_to_rgb24;
			break;

		case B_RGB16:
			converter = write_rgb16_to_rgb24;
			break;

		case B_RGB16_BIG:
			converter = write_rgb16b_to_rgb24;
			break;

		case B_RGB24:
			converter = write_rgb24;
			break;

		case B_RGB24_BIG:
			converter = write_rgb24b;
			break;

		case B_RGB32:
			converter = write_rgb32_to_rgb24;
			break;

		case B_RGB32_BIG:
			converter = write_rgb32b_to_rgb24;
			break;

		case B_RGBA32:
		/*
			// In theory it should be possible to write 4 color components
			// to jp2, so it should be possible to have transparency.
			// Unfortunetly libjasper does not agree with that
			// For now i don't know how to modify it :(

			out_color_components = 4;
			converter = write_rgba32;
		*/
			converter = write_rgb32_to_rgb24;
			break;

		case B_RGBA32_BIG:
		/*
			// In theory it should be possible to write 4 color components
			// to jp2, so it should be possible to have transparency.
			// Unfortunetly libjasper does not agree with that
			// For now i don't know how to modify it :(

			out_color_components = 4;
			converter = write_rgba32b;
		*/
			converter = write_rgb32b_to_rgb24;
			break;

		default:
			(new BAlert("Error", "Unknown color space.", "Quit"))->Go();
			return B_ERROR;
	}

	jas_image_t *image;
	jas_stream_t *outs;
	jas_matrix_t *pixels[4];
	jas_image_cmptparm_t component_info[4];

	if (jas_init())
		return B_ERROR;

	if (!(outs = jas_stream_positionIOopen(out)))
		return B_ERROR;

	int32 i = 0;
	for (i = 0; i < (long)out_color_components; i++) {
		(void) memset(component_info + i, 0, sizeof(jas_image_cmptparm_t));
		component_info[i].hstep = 1;
		component_info[i].vstep = 1;
		component_info[i].width = (unsigned int)width;
		component_info[i].height = (unsigned int)height;
		component_info[i].prec = (unsigned int)8;
	}

	image = jas_image_create((short)out_color_components, component_info, out_color_space);
	if (image == (jas_image_t *)NULL)
		return Error(outs, NULL, NULL, 0, NULL, B_ERROR);

	jpr_uchar_t *in_scanline = (jpr_uchar_t*) malloc(in_row_bytes);
	if (in_scanline == NULL)
		return Error(outs, image, NULL, 0, NULL, B_ERROR);

	for (i = 0; i < (long)out_color_components; i++) {
		pixels[i] = jas_matrix_create(1, (unsigned int)width);
		if (pixels[i] == (jas_matrix_t *)NULL)
			return Error(outs, image, pixels, i+1, in_scanline, B_ERROR);
	}

	int32 y = 0;
	for (y = 0; y < (long)height; y++) {
		err = in->Read(in_scanline, in_row_bytes);
		if (err < in_row_bytes)
			return (err < B_OK) ? Error(outs, image, pixels, out_color_components, in_scanline, err) : Error(outs, image, pixels, out_color_components, in_scanline, B_ERROR);

		converter(pixels, in_scanline, width);

		for (i = 0; i < (long)out_color_components; i++)
			(void)jas_image_writecmpt(image, (short)i, 0, (unsigned int)y, (unsigned int)width, 1, pixels[i]);
	}

	char opts[16];
	sprintf(opts, "rate=%1f", (float)settings.Quality / 100.0);
	if (jas_image_encode(image, outs, jas_image_strtofmt(settings.JPC ? (char*)"jpc" : (char*)"jp2"), opts))
		return Error(outs, image, pixels, out_color_components, in_scanline, err);

	free(in_scanline);

	for (i = 0; i < (long)out_color_components; i++)
		jas_matrix_destroy(pixels[i]);

	jas_stream_close(outs);
	jas_image_destroy(image);
	jas_image_clearfmts();

	return B_OK;
}


//!	Decode the native format
status_t
Decompress(BPositionIO *in, BPositionIO *out)
{
	jpeg_settings settings;
	LoadSettings(&settings);

	jas_image_t *image;
	jas_stream_t *ins;
	jas_matrix_t *pixels[4];

	if (jas_init())
		return B_ERROR;

	if (!(ins = jas_stream_positionIOopen(in)))
		return B_ERROR;

	if (!(image = jas_image_decode(ins, -1, 0)))
		return Error(ins, NULL, NULL, 0, NULL, B_ERROR);

	// Default color info
	color_space out_color_space;
	int out_color_components;
	int	in_color_components = jas_image_numcmpts(image);

	// Function pointer to read function
	// It MUST point to proper function
	void (*converter)(jas_matrix_t **pixels, jpr_uchar_t *outscanline, int width) = NULL;

	switch (jas_image_colorspace(image)) {
		case JAS_IMAGE_CS_RGB:
			out_color_components = 4;
			if (in_color_components == 3) {
				out_color_space = B_RGB32;
				converter = read_rgb24_to_rgb32;
			} else if (in_color_components == 4) {
				out_color_space = B_RGBA32;
				converter = read_rgba32;
			} else {
				(new BAlert("Error", "Other than RGB with 3 or 4 color components not implemented.", "Quit"))->Go();
				return Error(ins, image, NULL, 0, NULL, B_ERROR);
			}
			break;
		case JAS_IMAGE_CS_GRAY:
			if (settings.B_GRAY8_as_B_RGB32) {
				out_color_space = B_RGB32;
				out_color_components = 4;
				converter = read_gray_to_rgb32;
			} else {
				out_color_space = B_GRAY8;
				out_color_components = 1;
				converter = read_gray;
			}
			break;
		case JAS_IMAGE_CS_YCBCR:
			(new BAlert("Error", "color space YCBCR not implemented yet.", "Quit"))->Go();
			return Error(ins, image, NULL, 0, NULL, B_ERROR);
			break;
		case JAS_IMAGE_CS_UNKNOWN:
		default:
			(new BAlert("Error", "color space unknown.", "Quit"))->Go();
			return Error(ins, image, NULL, 0, NULL, B_ERROR);
			break;
	}

	float width = (float)jas_image_width(image);
	float height = (float)jas_image_height(image);

	// Bytes count in one line of image (scanline)
	int64 out_row_bytes = (int32)width * out_color_components;
		// NOTE: things will go wrong if "out_row_bytes" wouldn't fit into 32 bits

	// !!! Initialize this bounds rect to the size of your image
	BRect bounds(0, 0, width-1, height-1);

	// Fill out the B_TRANSLATOR_BITMAP's header
	TranslatorBitmap header;
	header.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	header.bounds.left = B_HOST_TO_BENDIAN_FLOAT(bounds.left);
	header.bounds.top = B_HOST_TO_BENDIAN_FLOAT(bounds.top);
	header.bounds.right = B_HOST_TO_BENDIAN_FLOAT(bounds.right);
	header.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT(bounds.bottom);
	header.colors = (color_space)B_HOST_TO_BENDIAN_INT32(out_color_space);
	header.rowBytes = B_HOST_TO_BENDIAN_INT32(out_row_bytes);
	header.dataSize = B_HOST_TO_BENDIAN_INT32((int32)(out_row_bytes * height));

	// Write out the header
	status_t err = out->Write(&header, sizeof(TranslatorBitmap));
	if (err < B_OK)
		return Error(ins, image, NULL, 0, NULL, err);
	if (err < (int)sizeof(TranslatorBitmap))
		return Error(ins, image, NULL, 0, NULL, B_ERROR);

	jpr_uchar_t *out_scanline = (jpr_uchar_t*) malloc(out_row_bytes);
	if (out_scanline == NULL)
		return Error(ins, image, NULL, 0, NULL, B_ERROR);

	int32 i = 0;
	for (i = 0; i < (long)in_color_components; i++) {
		pixels[i] = jas_matrix_create(1, (unsigned int)width);
		if (pixels[i] == (jas_matrix_t *)NULL)
			return Error(ins, image, pixels, i+1, out_scanline, B_ERROR);
	}

	int32 y = 0;
	for (y = 0; y < (long)height; y++) {
		for (i = 0; i < (long)in_color_components; i++)
			(void)jas_image_readcmpt(image, (short)i, 0, (unsigned int)y, (unsigned int)width, 1, pixels[i]);

		converter(pixels, out_scanline, (int32)width);

		err = out->Write(out_scanline, out_row_bytes);
		if (err < out_row_bytes)
			return (err < B_OK) ? Error(ins, image, pixels, in_color_components, out_scanline, err) : Error(ins, image, pixels, in_color_components, out_scanline, B_ERROR);
	}

	free(out_scanline);

	for (i = 0; i < (long)in_color_components; i++)
		jas_matrix_destroy(pixels[i]);

	jas_stream_close(ins);
	jas_image_destroy(image);
	jas_image_clearfmts();

	return B_OK;
}


/*!
	Frees jpeg alocated memory
	Returns given error (B_ERROR by default)
*/
status_t
Error(jas_stream_t *stream, jas_image_t *image, jas_matrix_t **pixels, int32 pixels_count, jpr_uchar_t *scanline, status_t error)
{
	if (pixels) {
		int32 i;
		for (i = 0; i < (long)pixels_count; i++) {
			if (pixels[i] != NULL)
				jas_matrix_destroy(pixels[i]);
		}
	}
	if (stream)
		jas_stream_close(stream);
	if (image)
		jas_image_destroy(image);

	jas_image_clearfmts();
	free(scanline);

	return error;
}


//	#pragma mark -


int
main()
{
	BApplication app("application/x-vnd.Haiku-JPEG2000Translator");

	TranslatorWindow *window = new TranslatorWindow();
	window->Show();

	app.Run();
	return 0;
}

