/*****************************************************************************/
// TIFFTranslatorSettings
// Adopted by Stephan Aßmus, <stippi@yellowbites.com>
// from TGATranslatorSettings written by
// Michael Wilber, OBOS Translation Kit Team
//
// TIFFTranslatorSettings.h
//
// This class manages (saves/loads/locks/unlocks) the settings
// for the TIFFTranslator.
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

#ifndef TIFF_TRANSLATOR_SETTINGS_H
#define TIFF_TRANSLATOR_SETTINGS_H

#include <Locker.h>
#include <Path.h>
#include <Message.h>

#define TIFF_SETTINGS_FILENAME "TIFFTranslator_Settings"

// TIFF Translator Settings
#define TIFF_SETTING_COMPRESSION "tiff /compression"

class TIFFTranslatorSettings {
public:
	TIFFTranslatorSettings();
	
	TIFFTranslatorSettings *Acquire();
		// increments the reference count, returns this
	TIFFTranslatorSettings *Release();
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
	uint32 SetGetCompression(uint32 *pCompression = NULL);
		// sets / gets Compression setting
		// specifies what compression will be used
		// when the TIFFTranslator creates TIFF images
	
private:
	~TIFFTranslatorSettings();
		// private so that Release() must be used
		// to delete the object
	
	BLocker		fLock;
	int32		fRefCount;
	BPath		fSettingsPath;
		// where the settings file will be loaded from /
		// saved to
	
	BMessage	fSettingsMSG;
		// the actual settings
};

#endif // #ifndef TIFF_TRANSLATOR_SETTTINGS_H
