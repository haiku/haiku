/*****************************************************************************/
// TGATranslatorSettings
// TGATranslatorSettings.h
//
// The description goes here.
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

class TGATranslatorSettings {
public:
	TGATranslatorSettings();
	
	TGATranslatorSettings *Acquire();
	TGATranslatorSettings *Release();
	
	status_t LoadSettings();
	status_t LoadSettings(BMessage *pmsg);
	status_t SaveSettings();
	status_t GetConfigurationMessage(BMessage *pmsg);
	
	bool SetGetHeaderOnly(bool *pbHeaderOnly = NULL);
	bool SetGetDataOnly(bool *pbDataOnly = NULL);
	bool SetGetRLE(bool *pbRLE = NULL);
	
private:
	~TGATranslatorSettings();
		// private so that Release() must be used
		// to delete the object
	
	BLocker flock;
	int32 frefCount;
	BPath fsettingsPath;
	
	BMessage fmsgSettings;
		// the actual settings
};

#endif // #ifndef TGA_TRANSLATOR_SETTTINGS_H