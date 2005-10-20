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

#include <jpeglib.h>


//----------------------------------------------------------------------------
//
//	Define & Global variables declaration
//
//----------------------------------------------------------------------------

// Settings
#define SETTINGS_FILE	"OpenJPEGTranslator"
#define SETTINGS_PATH	"/boot/home/config/settings"

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

// View labels
#define VIEW_LABEL_QUALITY "Output quality"
#define VIEW_LABEL_SMOOTHING "Output smoothing strength"
#define VIEW_LABEL_PROGRESSIVE "Use progressive compression"
#define VIEW_LABEL_OPTIMIZECOLORS "Prevent colors 'washing out'"
#define	VIEW_LABEL_SMALLERFILE "Make file smaller (sligthtly worse quality)"
#define	VIEW_LABEL_GRAY1ASRGB24 "Write Black&White images as RGB24"
#define	VIEW_LABEL_ALWAYSRGB32 "Read Greyscale images as RGB32"
#define	VIEW_LABEL_PHOTOSHOPCMYK "Use CMYK code with 0 for 100% ink coverage"
#define	VIEW_LABEL_SHOWREADERRORBOX "Show warning messages"

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
	uchar	Smoothing;			// default: 0
	uchar	Quality;			// default: 95
	bool	Progressive;		// default: true
	bool	OptimizeColors;		// default: true
	bool	SmallerFile;		// default: false	only used if (OptimizeColors == true)
	bool	B_GRAY1_as_B_RGB24;	// default: false	if false gray1 converted to gray8, else to rgb24
	// decompression
	bool	Always_B_RGB32;		// default: true
	bool	PhotoshopCMYK;		// default: true
	bool	ShowReadWarningBox;	// default: true
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
		BCheckBox			*alwaysrgb32;
		BCheckBox			*photoshopCMYK;
		BCheckBox			*showerrorbox;
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
		SSlider				*smoothing;
		BCheckBox			*progress;
		BCheckBox			*optimizecolors;
		BCheckBox			*smallerfile;
		BCheckBox			*gray1asrgb24;
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
	Settings->Smoothing = 0;
	Settings->Quality = 95;
	Settings->Progressive = true;
	Settings->OptimizeColors = true;
	Settings->SmallerFile = false;
	Settings->B_GRAY1_as_B_RGB24 = false;
	Settings->Always_B_RGB32 = true;
	Settings->PhotoshopCMYK = true;
	Settings->ShowReadWarningBox = true;
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
	if (!AreSettingsRunning && (new BAlert("Different settings file", "JPEG settings were set to default because of incompatible settings file.", "Configure settings", "OK", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go() == 0) {
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
EXTERN(struct jpeg_error_mgr *) be_jpeg_std_error (struct jpeg_error_mgr * err, SETTINGS * settings); // from "be_jerror.cpp"

//---------------------------------------------------
//	Main functions of translator :)
//---------------------------------------------------
status_t Copy(BPositionIO *in, BPositionIO *out);
status_t Compress(BPositionIO *in, BPositionIO *out);
status_t Decompress(BPositionIO *in, BPositionIO *out);
status_t Error(j_common_ptr cinfo, status_t error = B_ERROR);

//---------------------------------------------------
//	GRAY1 to GRAY8
//---------------------------------------------------
inline void
convert_from_gray1_to_gray8(uchar *in, uchar *out, int in_bytes)
{
	const color_map * map = system_colors();
	int32 index = 0;
	int32 index2 = 0;
	while (index < in_bytes) {
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

//---------------------------------------------------
//	GRAY1 to RGB 8:8:8
//---------------------------------------------------
inline void
convert_from_gray1_to_24(uchar *in, uchar *out, int in_bytes)
{
	const color_map * map = system_colors();
	int32 index = 0;
	int32 index2 = 0;
	while (index < in_bytes) {
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

//---------------------------------------------------
//	CMAP8 to RGB 8:8:8
//---------------------------------------------------
inline void
convert_from_cmap8_to_24(uchar *in, uchar *out, int in_bytes)
{
	const color_map * map = system_colors();
	int32 index = 0;
	int32 index2 = 0;
	while (index < in_bytes) {
		rgb_color color = map->color_list[in[index++]];
		
		out[index2++] = color.red;
		out[index2++] = color.green;
		out[index2++] = color.blue;
	}
}

//---------------------------------------------------
//	BGR 1:5:5:5 to RGB 8:8:8
//---------------------------------------------------
inline void
convert_from_15_to_24(uchar *in, uchar *out, int in_bytes)
{
	int32 index = 0;
	int32 index2 = 0;
	int16 in_pixel;
	while (index < in_bytes) {
		in_pixel = in[index] | (in[index+1] << 8);
		index += 2;
		
		out[index2++] = (((in_pixel & 0x7c00)) >> 7) | (((in_pixel & 0x7c00)) >> 12);
		out[index2++] = (((in_pixel & 0x3e0)) >> 2) | (((in_pixel & 0x3e0)) >> 7);
		out[index2++] = (((in_pixel & 0x1f)) << 3) | (((in_pixel & 0x1f)) >> 2);
	}
}

//---------------------------------------------------
//	RGB 1:5:5:5 to RGB 8:8:8
//---------------------------------------------------
inline void
convert_from_15b_to_24(uchar *in, uchar *out, int in_bytes)
{
	int32 index = 0;
	int32 index2 = 0;
	int16 in_pixel;
	while (index < in_bytes) {
		in_pixel = in[index+1] | (in[index] << 8);
		index += 2;
		
		out[index2++] = (((in_pixel & 0x7c00)) >> 7) | (((in_pixel & 0x7c00)) >> 12);
		out[index2++] = (((in_pixel & 0x3e0)) >> 2) | (((in_pixel & 0x3e0)) >> 7);
		out[index2++] = (((in_pixel & 0x1f)) << 3) | (((in_pixel & 0x1f)) >> 2);
	}
}

//---------------------------------------------------
//	BGR 5:6:5 to RGB 8:8:8
//---------------------------------------------------
inline void
convert_from_16_to_24(uchar *in, uchar *out, int in_bytes)
{
	int32 index = 0;
	int32 index2 = 0;
	int16 in_pixel;
	while (index < in_bytes) {
		in_pixel = in[index] | (in[index+1] << 8);
		index += 2;
		
		out[index2++] = (((in_pixel & 0xf800)) >> 8) | (((in_pixel & 0xf800)) >> 13);
		out[index2++] = (((in_pixel & 0x7e0)) >> 3) | (((in_pixel & 0x7e0)) >> 9);
		out[index2++] = (((in_pixel & 0x1f)) << 3) | (((in_pixel & 0x1f)) >> 2);
	}
}

//---------------------------------------------------
//	RGB 5:6:5 to RGB 8:8:8
//---------------------------------------------------
inline void
convert_from_16b_to_24(uchar *in, uchar *out, int in_bytes)
{
	int32 index = 0;
	int32 index2 = 0;
	int16 in_pixel;
	while (index < in_bytes) {
		in_pixel = in[index+1] | (in[index] << 8);
		index += 2;
		
		out[index2++] = (((in_pixel & 0xf800)) >> 8) | (((in_pixel & 0xf800)) >> 13);
		out[index2++] = (((in_pixel & 0x7e0)) >> 3) | (((in_pixel & 0x7e0)) >> 9);
		out[index2++] = (((in_pixel & 0x1f)) << 3) | (((in_pixel & 0x1f)) >> 2);
	}
}

//---------------------------------------------------
//	BGR 8:8:8 to RGB 8:8:8
//---------------------------------------------------
inline void
convert_from_24_to_24(uchar *in, uchar *out, int in_bytes)
{
	int32 index = 0;
	int32 index2 = 0;
	while (index < in_bytes) {
		out[index2++] = in[index+2];
		out[index2++] = in[index+1];
		out[index2++] = in[index];
		index+=3;
	}
}

//---------------------------------------------------
//	BGRx 8:8:8:8 to RGB 8:8:8
//---------------------------------------------------
inline void
convert_from_32_to_24(uchar *in, uchar *out, int in_bytes)
{
	int32 index = 0;
	int32 index2 = 0;
	while (index < in_bytes) {
		out[index2++] = in[index+2];
		out[index2++] = in[index+1];
		out[index2++] = in[index];
		index+=4;
	}
}

//---------------------------------------------------
//	xRGB 8:8:8:8 to RGB 8:8:8
//---------------------------------------------------
inline void
convert_from_32b_to_24(uchar *in, uchar *out, int in_bytes)
{
	int32 index = 0;
	int32 index2 = 0;
	while (index < in_bytes) {
		index++;
		out[index2++] = in[index++];
		out[index2++] = in[index++];
		out[index2++] = in[index++];
	}
}

//---------------------------------------------------
//	CMYK 8:8:8:8 to RGB32 8:8:8:8
//	Version for Adobe Photoshop files (0 for 100% ink)
//---------------------------------------------------
inline void
convert_from_CMYK_to_32_photoshop (uchar *in, uchar *out, int out_bytes)
{
	int32 index = 0;
	int32 index2 = 0;
	int32 black = 0;
	while (index < out_bytes) {
		black = in[index2+3];
		out[index++] = in[index2+2]*black/255;
		out[index++] = in[index2+1]*black/255;
		out[index++] = in[index2]*black/255;
		out[index++] = 255;
		index2 += 4;
	}
}

//---------------------------------------------------
//	CMYK 8:8:8:8 to RGB32 8:8:8:8
//	!!! UNTESTED !!!
//---------------------------------------------------
inline void
convert_from_CMYK_to_32 (uchar *in, uchar *out, int out_bytes)
{
	int32 index = 0;
	int32 index2 = 0;
	int32 black = 0;
	while (index < out_bytes) {
		black = 255 - in[index2+3];
		out[index++] = ((255-in[index2+2])*black)/255;
		out[index++] = ((255-in[index2+1])*black)/255;
		out[index++] = ((255-in[index2])*black)/255;
		out[index++] = 255;
		index2 += 4;
	}
}

//---------------------------------------------------
//	RGB24 8:8:8 to xRGB 8:8:8:8
//---------------------------------------------------
inline void
convert_from_24_to_32(uchar *in, uchar *out, int out_bytes)
{
	int32 index = 0;
	int32 index2 = 0;
	while (index < out_bytes) {
		out[index++] = in[index2+2];
		out[index++] = in[index2+1];
		out[index++] = in[index2];
		out[index++] = 255;
		index2 += 3;
	}
}


#endif // _JPEGTRANSLATOR_H_
