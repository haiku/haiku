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


//----------------------------------------------------------------------------
//
//	Include
//
//----------------------------------------------------------------------------

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

//----------------------------------------------------------------------------
//
//	Define & Global variables declaration
//
//----------------------------------------------------------------------------

// Settings
#define SETTINGS_FILE	"OpenJPEG2000Translator"
#define SETTINGS_PATH	"/boot/home/config/settings"

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

// This will be used true if Settings are running, else false
extern bool AreSettingsRunning;

//----------------------------------------------------------------------------
//
//	Classes
//
//----------------------------------------------------------------------------

//---------------------------------------------------
//	Settings storage structure
//---------------------------------------------------
struct SETTINGS
{
	// compression
	jpr_uchar_t	Quality;			// default: 25
	bool	JPC;				// default: false	// compress to JPC or JP2?
	bool	B_GRAY1_as_B_RGB24;	// default: false	// copress gray 1 as rgb24 or grayscale?
	// decompression
	bool	B_GRAY8_as_B_RGB32;	// default: true
};

//---------------------------------------------------
//	Slider used in TranslatorView
//	With status showing actual value
//---------------------------------------------------
class SSlider : public BSlider
{
	public:
							SSlider(BRect frame, const char *name, const char *label, BMessage *message, int32 minValue, int32 maxValue, orientation posture = B_HORIZONTAL, thumb_style thumbType = B_BLOCK_THUMB, uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP, uint32 flags = B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS);
		char*				UpdateText() const;
		void				ResizeToPreferred();

	private:
		char				statusLabel[12];
};


//---------------------------------------------------
//	Basic view class with resizing to needed size
//---------------------------------------------------
class SView : public BView
{
	public:
							SView(const char *name, float x = 0, float y = 0)
								:BView( BRect(x,y,x,y), name, B_FOLLOW_NONE, B_WILL_DRAW)
								{
									preferredWidth = 0;
									preferredHeight = 0;
									SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR));
									SetFont(be_plain_font);
								};
		void				GetPreferredSize(float *width, float *height)
								{
									*width = preferredWidth;
									*height = preferredHeight;
								}
		inline float		GetPreferredWidth() { return preferredWidth; };
		inline float		GetPreferredHeight() { return preferredHeight; };
		inline void			ResizePreferredBy(float width, float height) { preferredWidth += width; preferredHeight += height; };
		inline void			ResizeToPreferred() { ResizeTo(preferredWidth, preferredHeight); };
		void				AddChild(BView *child, BView *before = NULL)
								{
									BView::AddChild(child, before);
									child->ResizeToPreferred();
									BRect frame = child->Frame();
									if (frame.right > preferredWidth)
										preferredWidth = frame.right;
									if (frame.bottom > preferredHeight)
										preferredHeight = frame.bottom;
								}

	private:
		float				preferredWidth;
		float				preferredHeight;
};

//---------------------------------------------------
//	Configuration view for reading settings
//---------------------------------------------------
class TranslatorReadView : public SView
{
	public:
							TranslatorReadView(const char *name, SETTINGS *settings, float x = 0, float y = 0);
		void				AttachedToWindow();
		void				MessageReceived(BMessage *message);

	private:
		SETTINGS			*Settings;
		BCheckBox			*grayasrgb32;
};

//---------------------------------------------------
//	Configuration view for writing settings
//---------------------------------------------------
class TranslatorWriteView : public SView
{
	public:
							TranslatorWriteView(const char *name, SETTINGS *settings, float x = 0, float y = 0);
		void				AttachedToWindow();
		void				MessageReceived(BMessage *message);

	private:
		SETTINGS			*Settings;
		SSlider				*quality;
		BCheckBox			*gray1asrgb24;
		BCheckBox			*jpc;
};

//---------------------------------------------------
//	About view
//---------------------------------------------------
class TranslatorAboutView : public SView
{
	public:
							TranslatorAboutView(const char *name, float x = 0, float y = 0);
};

//---------------------------------------------------
//	Configuration view
//---------------------------------------------------
class TranslatorView : public SView
{
	public:
							TranslatorView(const char *name);
							~TranslatorView() { AreSettingsRunning = false; };
		void				AttachedToWindow();
		void				Draw(BRect updateRect);
		void				MouseDown(BPoint where);

	private:
		SETTINGS			Settings;
		int32				tabWidth;
		int32				tabHeight;
		int32				activeChild;
};

//---------------------------------------------------
//	Window used for configuration
//---------------------------------------------------
class TranslatorWindow : public BWindow
{
	public:
							TranslatorWindow(bool quit_on_close = true);
};


//----------------------------------------------------------------------------
//
//	Functions :: Settings
//
//----------------------------------------------------------------------------

//---------------------------------------------------
//	Make Settings to defaults
//---------------------------------------------------
inline void
LoadDefaultSettings(SETTINGS *Settings)
{
	Settings->Quality = 25;
	Settings->JPC = false;
	Settings->B_GRAY1_as_B_RGB24 = false;
	Settings->B_GRAY8_as_B_RGB32 = true;
}

//---------------------------------------------------
//	Save Settings to config file
//---------------------------------------------------
inline void
SaveSettings(SETTINGS *Settings)
{
	// Make path to settings file
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK) {
		path.SetTo(SETTINGS_PATH);
		path.Append(SETTINGS_FILE);
	} else
		path.Append(SETTINGS_FILE);
	
	// Open settings file (create it if there's no file) and write settings			
	FILE *file = NULL;
	if ((file = fopen( path.Path(), "wb+"))) {
		fwrite(Settings, sizeof(SETTINGS), 1, file);
		fclose(file);
	}
}

//---------------------------------------------------
//	Return true if Settings were run, false if not
//---------------------------------------------------
inline bool
SettingsChangedAlert()
{
	// If settings view wasn't already initialized (settings not running)
	// and user wants to run settings
	if (!AreSettingsRunning && (new BAlert("Different settings file", "JPEG2000 settings were set to default because of incompatible settings file.", "Configure settings", "OK", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go() == 0) {
		// Create settings window (with no quit on close!), launch it and wait until it's closed
		status_t err;
		TranslatorWindow *window = new TranslatorWindow(false);
		window->Show();
		wait_for_thread(window->Thread(), &err);
		return true;
	}

	return false;
}

//---------------------------------------------------
//	Load settings from config file
//	If can't find it make them default and try to save
//---------------------------------------------------
inline void
LoadSettings(SETTINGS *Settings)
{
	// Make path to settings file
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK) {
		path.SetTo(SETTINGS_PATH);
		path.Append(SETTINGS_FILE);
	} else
		path.Append(SETTINGS_FILE);

	// Open settings file (create it if there's no file) and write settings			
	FILE *file = NULL;
	if ((file = fopen( path.Path(), "rb"))) {
		if ( !fread(Settings, sizeof(SETTINGS), 1, file)) {
			// Settings struct has changed size
			// Load default settings, and Save them
			fclose(file);
			LoadDefaultSettings(Settings);
			SaveSettings(Settings);
			// Tell user settings were changed to default, and ask to run settings panel or not
			if (SettingsChangedAlert())
				// User configured settings, load them again
				LoadSettings(Settings);
		} else
			fclose(file);
	} else if ((file = fopen( path.Path(), "wb+"))) {
		LoadDefaultSettings(Settings);
		fwrite(Settings, sizeof(SETTINGS), 1, file);
		fclose(file);
		// Tell user settings were changed to default, and ask to run settings panel or not
		if (SettingsChangedAlert())
			// User configured settings, load them again
			LoadSettings(Settings);
	}
}


//----------------------------------------------------------------------------
//
//	Functions
//
//----------------------------------------------------------------------------

//---------------------------------------------------
//	Main functions of translator :)
//---------------------------------------------------
status_t Copy(BPositionIO *in, BPositionIO *out);
status_t Compress(BPositionIO *in, BPositionIO *out);
status_t Decompress(BPositionIO *in, BPositionIO *out);
status_t Error(jas_stream_t *stream, jas_image_t *image, jas_matrix_t **pixels, int32 pixels_count, jpr_uchar_t *scanline, status_t error = B_ERROR);

//---------------------------------------------------
//	Make RGB32 scanline from *pixels[3]
//---------------------------------------------------
inline void
read_rgb24_to_rgb32(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width)
	{
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[2], x);
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[1], x);
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[0], x);
		scanline[index++] = 255;
		x++;
	}
}

//---------------------------------------------------
//	Make gray scanline from *pixels[1]
//---------------------------------------------------
inline void
read_gray_to_rgb32(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	jpr_uchar_t color = 0;
	while (x < width)
	{
		color = (jpr_uchar_t)jas_matrix_getv(pixels[0], x++);
		scanline[index++] = color;
		scanline[index++] = color;
		scanline[index++] = color;
		scanline[index++] = 255;
	}
}

//---------------------------------------------------
//	Make RGBA32 scanline from *pixels[4]
//	(just read data to scanline)
//---------------------------------------------------
inline void
read_rgba32(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width)
	{
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[2], x);
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[1], x);
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[0], x);
		scanline[index++] = (jpr_uchar_t)jas_matrix_getv(pixels[3], x);
		x++;
	}
}

//---------------------------------------------------
//	Make gray scanline from *pixels[1]
//	(just read data to scanline)
//---------------------------------------------------
inline void
read_gray(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 x = 0;
	while (x < width)
	{
		scanline[x] = (jpr_uchar_t)jas_matrix_getv(pixels[0], x);
		x++;
	}
}

//---------------------------------------------------
//	Make *pixels[1] from gray1 scanline
//---------------------------------------------------
inline void
write_gray1_to_gray(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 x = 0;
	int32 index = 0;
	while (x < (width/8))
	{
		unsigned char c = scanline[x++];
		for (int b = 128; b; b = b >> 1) {
			if (c & b)
				jas_matrix_setv(pixels[0], index++, 0);
			else
				jas_matrix_setv(pixels[0], index++, 255);
		}
	}
}

//---------------------------------------------------
//	Make *pixels[3] from gray1 scanline
//---------------------------------------------------
inline void
write_gray1_to_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 x = 0;
	int32 index = 0;
	while (x < (width/8))
	{
		unsigned char c = scanline[x++];
		for (int b = 128; b; b = b >> 1) {
			if (c & b) {
				jas_matrix_setv(pixels[0], index, 0);
				jas_matrix_setv(pixels[1], index, 0);
				jas_matrix_setv(pixels[2], index, 0);
			}
			else {
				jas_matrix_setv(pixels[0], index, 255);
				jas_matrix_setv(pixels[1], index, 255);
				jas_matrix_setv(pixels[2], index, 255);
			}
			index++;
		}
	}
}

//---------------------------------------------------
//	Make *pixels[3] from cmap8 scanline
//---------------------------------------------------
inline void
write_cmap8_to_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	const color_map *map = system_colors();
	int32 x = 0;
	while (x < width)
	{
		rgb_color color = map->color_list[scanline[x]];

		jas_matrix_setv(pixels[0], x, color.red);
		jas_matrix_setv(pixels[1], x, color.green);
		jas_matrix_setv(pixels[2], x, color.blue);
		x++;
	}
}

//---------------------------------------------------
//	Make *pixels[1] from gray scanline
//	(just write data to pixels)
//---------------------------------------------------
inline void
write_gray(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 x = 0;
	while (x < width)
	{
		jas_matrix_setv(pixels[0], x, scanline[x]);
		x++;
	}
}

//---------------------------------------------------
//	Make *pixels[3] from RGB15/RGBA15 scanline
//	(just write data to pixels)
//---------------------------------------------------
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

//---------------------------------------------------
//	Make *pixels[3] from RGB15/RGBA15 bigendian scanline
//	(just write data to pixels)
//---------------------------------------------------
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

//---------------------------------------------------
//	Make *pixels[3] from RGB16/RGBA16 scanline
//	(just write data to pixels)
//---------------------------------------------------
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

//---------------------------------------------------
//	Make *pixels[3] from RGB16/RGBA16 bigendian scanline
//	(just write data to pixels)
//---------------------------------------------------
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

//---------------------------------------------------
//	Make *pixels[3] from RGB24 scanline
//	(just write data to pixels)
//---------------------------------------------------
inline void
write_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width)
	{
		jas_matrix_setv(pixels[2], x, scanline[index++]);
		jas_matrix_setv(pixels[1], x, scanline[index++]);
		jas_matrix_setv(pixels[0], x, scanline[index++]);
		x++;
	}
}

//---------------------------------------------------
//	Make *pixels[3] from RGB24 bigendian scanline
//	(just write data to pixels)
//---------------------------------------------------
inline void
write_rgb24b(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width)
	{
		jas_matrix_setv(pixels[0], x, scanline[index++]);
		jas_matrix_setv(pixels[1], x, scanline[index++]);
		jas_matrix_setv(pixels[2], x, scanline[index++]);
		x++;
	}
}

//---------------------------------------------------
//	Make *pixels[3] from RGB32 scanline
//	(just write data to pixels)
//---------------------------------------------------
inline void
write_rgb32_to_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width)
	{
		jas_matrix_setv(pixels[2], x, scanline[index++]);
		jas_matrix_setv(pixels[1], x, scanline[index++]);
		jas_matrix_setv(pixels[0], x, scanline[index++]);
		index++;
		x++;
	}
}

//---------------------------------------------------
//	Make *pixels[3] from RGB32 bigendian scanline
//	(just write data to pixels)
//---------------------------------------------------
inline void
write_rgb32b_to_rgb24(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width)
	{
		index++;
		jas_matrix_setv(pixels[0], x, scanline[index++]);
		jas_matrix_setv(pixels[1], x, scanline[index++]);
		jas_matrix_setv(pixels[2], x, scanline[index++]);
		x++;
	}
}

//---------------------------------------------------
//	Make *pixels[4] from RGBA32 scanline
//	(just write data to pixels)
//	!!! UNTESTED !!!
//---------------------------------------------------
inline void
write_rgba32(jas_matrix_t **pixels, jpr_uchar_t *scanline, int width)
{
	int32 index = 0;
	int32 x = 0;
	while (x < width)
	{
		jas_matrix_setv(pixels[3], x, scanline[index++]);
		jas_matrix_setv(pixels[2], x, scanline[index++]);
		jas_matrix_setv(pixels[1], x, scanline[index++]);
		jas_matrix_setv(pixels[0], x, scanline[index++]);
		x++;
	}
}

//---------------------------------------------------
//	Make *pixels[4] from RGBA32 bigendian scanline
//	(just write data to pixels)
//	!!! UNTESTED !!!
//---------------------------------------------------
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

#endif // _JP2TRANSLATOR_H_
