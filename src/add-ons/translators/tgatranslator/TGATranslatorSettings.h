/*****************************************************************************/
// TGATranslatorSettings
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TGATranslatorSettings.h
//
// This class manages (saves/loads/locks/unlocks) the settings
// for the TGATranslator.
//
//
// Copyright (c) 2002 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef TGA_TRANSLATOR_SETTINGS_H
#define TGA_TRANSLATOR_SETTINGS_H

#include <Locker.h>
#include <Path.h>
#include <Message.h>

#define TGA_SETTINGS_FILENAME "TGATranslator_Settings"

// TGA Translator Settings
#define TGA_SETTING_RLE "tga /rle"
#define TGA_SETTING_IGNORE_ALPHA "tga /ignore_alpha"

class TGATranslatorSettings {
public:
	TGATranslatorSettings();
	
	TGATranslatorSettings *Acquire();
		// increments the reference count, returns this
	TGATranslatorSettings *Release();
		// decrements the reference count, deletes this
		// when count reaches zero, returns this when 
		// ref count is greater than zero, NULL when
		// ref count is zero
	
	status_t LoadSettings();
	status_t LoadSettings(BMessage *pmsg);
	status_t SaveSettings();
	status_t GetConfigurationMessage(BMessage *pmsg);
	
	bool SetGetHeaderOnly(bool *pbHeaderOnly = NULL);
		// sets / gets HeaderOnly setting
		// specifies if only the image header should be
		// outputted
	bool SetGetDataOnly(bool *pbDataOnly = NULL);
		// sets / gets DataOnly setting
		// specifiees if only the image data should be
		// outputted
	bool SetGetRLE(bool *pbRLE = NULL);
		// sets / gets RLE setting
		// specifies if RLE compression will be used
		// when the TGATranslator creates TGA images
	bool SetGetIgnoreAlpha(bool *pbIgnoreAlpha = NULL);
		// sets / gets ignore alpha setting
		// specifies whether or not TGATranslator uses
		// the alpha data from TGA files
	
private:
	~TGATranslatorSettings();
		// private so that Release() must be used
		// to delete the object
	
	BLocker flock;
	int32 frefCount;
	BPath fsettingsPath;
		// where the settings file will be loaded from /
		// saved to
	
	BMessage fmsgSettings;
		// the actual settings
};

#endif // #ifndef TGA_TRANSLATOR_SETTTINGS_H
