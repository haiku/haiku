/*****************************************************************************/
// LiveSettings
// Written by Michael Wilber
//
// LiveSettings.h
//
// This class manages (saves/loads/locks/unlocks) a collection of BMessage
// based settings. This class allows you to share settings between different
// classes in different threads and receive notifications when the settings
// change. This class makes it easy to share settings between a Translator
// and its config panel or a Screen Saver and its config panel.
//
//
// Copyright (C) Haiku
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

#ifndef LIVE_SETTINGS_H
#define LIVE_SETTINGS_H

#include <Locker.h>
#include <Path.h>
#include <Message.h>
#include <String.h>
#include <vector>
#include "LiveSettingsObserver.h"
#include "LiveSetting.h"

class LiveSettings {
public:
	LiveSettings(const char *settingsFile, LiveSetting *defaults,
		int32 defCount);
	
	LiveSettings *Acquire();
		// increments the reference count, returns this
	LiveSettings *Release();
		// decrements the reference count, deletes this
		// when count reaches zero, returns this when 
		// ref count is greater than zero, NULL when
		// ref count is zero
		
	bool AddObserver(LiveSettingsObserver *observer);
		// returns true if observer was added sucessfully, 
		// false if observer already in the list or error
	bool RemoveObserver(LiveSettingsObserver *observer);
		// returns true if observer was removed successfully,
		// false if observer not found or error
	
	status_t LoadSettings();
	status_t LoadSettings(BMessage *pmsg);
	status_t SaveSettings();
	status_t GetConfigurationMessage(BMessage *pmsg);
	
	bool SetGetBool(const char *name, bool *pVal = NULL);
	int32 SetGetInt32(const char *name, int32 *pVal = NULL);
	
	void SetString(const char *name, const BString &str);
	void GetString(const char *name, BString &str);
	
private:
	const LiveSetting *FindLiveSetting(const char *name);
	~LiveSettings();
		// private so that Release() must be used
		// to delete the object
		
	void NotifySettingChanged(uint32 setting);
	
	template <class T>
	bool GetValue(const char *name, T &val);
	
	template <class T>
	bool SetValue(const char *name, const T &val);
	
	BLocker fLock;
	int32 fRefCount;
	BPath fSettingsPath;
		// where the settings file will be loaded from /
		// saved to
	
	BMessage fSettingsMsg;
		// the actual settings
		
	const LiveSetting *fDefaults;
	int32 fDefCount;
	
	typedef std::vector<LiveSettingsObserver *> ObserverList;
	ObserverList fObservers;
};

#endif // #ifndef LIVE_SETTTINGS_H

