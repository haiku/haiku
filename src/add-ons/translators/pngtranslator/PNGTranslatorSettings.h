/*****************************************************************************/
// PNGTranslatorSettings
// Written by Michael Wilber
//
// PNGTranslatorSettings.h
//
// This class manages (saves/loads/locks/unlocks) the settings
// for the PNGTranslator.
//
//
// Copyright (c) 2003 OpenBeOS Project
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

#ifndef PNG_TRANSLATOR_SETTINGS_H
#define PNG_TRANSLATOR_SETTINGS_H

#include <Locker.h>
#include <Path.h>
#include <Message.h>

#define PNG_SETTINGS_FILENAME "PNGTranslator_Settings"

// PNG Translator Settings
#define PNG_SETTING_INTERLACE "png /interlace"

class PNGTranslatorSettings {
public:
	PNGTranslatorSettings();
	
	PNGTranslatorSettings *Acquire();
		// increments the reference count, returns this
	PNGTranslatorSettings *Release();
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
	int32 SetGetInterlace(int32 *pinterlace = NULL);
		// sets /gets Interlace setting
		// specifies if interlacing will be used
		// when the PNGTranslator creates PNG images
	
private:
	~PNGTranslatorSettings();
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

#endif // #ifndef PNG_TRANSLATOR_SETTTINGS_H

