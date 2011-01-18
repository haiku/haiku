/*****************************************************************************/
// TranslatorSettings
// Written by Michael Wilber, Haiku Translation Kit Team
//
// TranslatorSettings.h
//
// This class manages (saves/loads/locks/unlocks) the settings
// for a Translator.
//
//
// Copyright (c) 2004  Haiku, Inc.
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

#ifndef TRANSLATOR_SETTINGS_H
#define TRANSLATOR_SETTINGS_H

#include <Locker.h>
#include <Path.h>
#include <Message.h>

enum TranSettingType {
	TRAN_SETTING_INT32 = 0,
	TRAN_SETTING_BOOL
};

struct TranSetting {
	const char *name;
	TranSettingType dataType;
	int32 defaultVal;
};

class TranslatorSettings {
public:
	TranslatorSettings(const char *settingsFile, const TranSetting *defaults,
		int32 defCount);

	TranslatorSettings *Acquire();
		// increments the reference count, returns this
	TranslatorSettings *Release();
		// decrements the reference count, deletes this
		// when count reaches zero, returns this when
		// ref count is greater than zero, NULL when
		// ref count is zero

	status_t LoadSettings();
	status_t LoadSettings(BMessage *pmsg);
	status_t SaveSettings();
	status_t GetConfigurationMessage(BMessage *pmsg);

	bool SetGetBool(const char *name, bool *pbool = NULL);
	int32 SetGetInt32(const char *name, int32 *pint32 = NULL);

private:
	const TranSetting *FindTranSetting(const char *name);
	~TranslatorSettings();
		// private so that Release() must be used
		// to delete the object

	BLocker fLock;
	int32 fRefCount;
	BPath fSettingsPath;
		// where the settings file will be loaded from /
		// saved to

	BMessage fSettingsMsg;
		// the actual settings

	const TranSetting *fDefaults;
	int32 fDefCount;
};

#endif // #ifndef TRANSLATOR_SETTTINGS_H

