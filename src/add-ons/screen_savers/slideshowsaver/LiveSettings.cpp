/*****************************************************************************/
// LiveSettings
// Written by Michael Wilber
//
// LiveSettings.cpp
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

#include <string.h>
#include <File.h>
#include <FindDirectory.h>
#include "LiveSettings.h"

// ---------------------------------------------------------------
// Constructor
//
// Sets the default settings, location for the settings file
// and sets the reference count to 1
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
LiveSettings::LiveSettings(const char *settingsFile,
	LiveSetting *defaults, int32 defCount)
	: fLock("LiveSettings Lock")
{
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &fSettingsPath))
		fSettingsPath.SetTo("/tmp");
	fSettingsPath.Append(settingsFile);
	
	fRefCount = 1;
	
	if (defCount > 0) {
		fDefaults = defaults;
		fDefCount = defCount;
	} else {
		fDefaults = NULL;
		fDefCount = 0;
	}
	
	// Add Default Settings
	// (Used when loading from the settings file or from
	// a BMessage fails)
	const LiveSetting *defs = fDefaults;
	for (int32 i = 0; i < fDefCount; i++)
		defs[i].AddReplaceValue(&fSettingsMsg);
}

// ---------------------------------------------------------------
// Acquire
//
// Returns a pointer to the LiveSettings and increments
// the reference count. 
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: pointer to this LiveSettings object
// ---------------------------------------------------------------
LiveSettings *
LiveSettings::Acquire()
{
	LiveSettings *psettings = NULL;
	
	fLock.Lock();
	fRefCount++;
	psettings = this;
	fLock.Unlock();
	
	return psettings;
}

// ---------------------------------------------------------------
// Release
//
// Decrements the reference count and deletes the
// LiveSettings if the reference count is zero.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: pointer to this LiveSettings object if
// the reference count is greater than zero, returns NULL
// if the reference count is zero and the LiveSettings
// object has been deleted
// ---------------------------------------------------------------
LiveSettings *
LiveSettings::Release()
{
	LiveSettings *psettings = NULL;
	
	fLock.Lock();
	fRefCount--;
	if (fRefCount > 0) {
		psettings = this;
		fLock.Unlock();
	} else
		delete this;
			// delete this object and
			// release locks
	
	return psettings;	
}

// ---------------------------------------------------------------
// Destructor
//
// Does nothing!
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
LiveSettings::~LiveSettings()
{
	fObservers.clear();
}

// returns true if observer was added sucessfully, 
// false if observer already in the list or error
bool
LiveSettings::AddObserver(LiveSettingsObserver *observer)
{
	fLock.Lock();
	
	bool bAdd = true;
	ObserverList::iterator I = fObservers.begin();
	while (I != fObservers.end()) {
		if (*I == observer) {
			bAdd = false;
			break;
		}
		I++;
	}
	
	if (bAdd == true)
		fObservers.push_back(observer);
	
	fLock.Unlock();
	
	return bAdd;
}

// returns true if observer was removed successfully,
// false if observer not found or error		
bool
LiveSettings::RemoveObserver(LiveSettingsObserver *observer)
{
	fLock.Lock();
	
	bool bRemove = false;
	ObserverList::iterator I = fObservers.begin();
	while (I != fObservers.end()) {
		if (*I == observer) {
			bRemove = true;
			break;
		}
		I++;
	}
	
	if (bRemove == true)
		fObservers.erase(I);
	
	fLock.Unlock();
	
	return bRemove;
}

void
LiveSettings::NotifySettingChanged(uint32 setting)
{
	fLock.Lock();
	ObserverList listCopy = fObservers;
	fLock.Unlock();
	
	ObserverList::iterator I = listCopy.begin();
	while (I != listCopy.end()) {
		(*I)->SettingChanged(setting);
		I++;
	}
}		

// ---------------------------------------------------------------
// LoadSettings
//
// Loads the settings by reading them from the default
// settings file.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: B_OK if there were no errors or an error code from
// BFile::SetTo() or BMessage::Unflatten() if there were errors
// ---------------------------------------------------------------
status_t
LiveSettings::LoadSettings()
{
	status_t result;
	
	fLock.Lock();
	
	// Don't try to open the settings file if there are
	// no settings that need to be loaded
	if (fDefCount > 0) {
		BFile settingsFile;
		result = settingsFile.SetTo(fSettingsPath.Path(), B_READ_ONLY);
		if (result == B_OK) {
			BMessage msg;
			result = msg.Unflatten(&settingsFile);
			if (result == B_OK)
				result = LoadSettings(&msg);
		}
	} else
		result = B_OK;
	
	fLock.Unlock();
	
	return result;
}

// ---------------------------------------------------------------
// LoadSettings
//
// Loads the settings from a BMessage passed to the function.
//
// Preconditions:
//
// Parameters:	pmsg	pointer to BMessage that contains the
//						settings
//
// Postconditions:
//
// Returns: B_BAD_VALUE if pmsg is NULL or invalid options
// have been found, B_OK if there were no
// errors or an error code from BMessage::FindBool() or
// BMessage::ReplaceBool() if there were other errors
// ---------------------------------------------------------------
status_t
LiveSettings::LoadSettings(BMessage *pmsg)
{
	status_t result = B_OK;
	
	if (pmsg) {
		
		fLock.Lock();
		
		const LiveSetting *defs = fDefaults;
		for (int32 i = 0; i < fDefCount; i++)
			defs[i].AddReplaceValue(&fSettingsMsg, pmsg);

		fLock.Unlock();
	}
	
	return result;
}

// ---------------------------------------------------------------
// SaveSettings
//
// Saves the settings as a flattened BMessage to the default
// settings file
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: B_OK if no errors or an error code from BFile::SetTo()
// or BMessage::Flatten() if there were errors
// ---------------------------------------------------------------
status_t
LiveSettings::SaveSettings()
{
	status_t result;
	
	fLock.Lock();
	
	// Only write out settings file if there are
	// actual settings stored by this object
	if (fDefCount > 0) {
		BFile settingsFile;
		result = settingsFile.SetTo(fSettingsPath.Path(),
			B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (result == B_OK)
			result = fSettingsMsg.Flatten(&settingsFile);
	} else
		result = B_OK;
		
	fLock.Unlock();
	
	return result;
}

// ---------------------------------------------------------------
// GetConfigurationMessage
//
// Saves the current settings to the BMessage passed to the
// function
//
// Preconditions:
//
// Parameters:	pmsg	pointer to BMessage where the settings
//						will be stored
//
// Postconditions:
//
// Returns: B_OK if there were no errors or an error code from
// BMessage::RemoveName() or BMessage::AddBool() if there were
// errors
// ---------------------------------------------------------------
status_t
LiveSettings::GetConfigurationMessage(BMessage *pmsg)
{
	status_t result = B_BAD_VALUE;
	
	if (pmsg) {
		int32 i;
		for (i = 0; i < fDefCount; i++) {
			result = pmsg->RemoveName(fDefaults[i].GetName());
			if (result != B_OK && result != B_NAME_NOT_FOUND)
				break;
		}
		if (i == fDefCount) {
			fLock.Lock();
			result = B_OK;
			
			BString tempStr;
			const LiveSetting *defs = fDefaults;
			for (i = 0; i < fDefCount && result >= B_OK; i++)
				defs[i].AddReplaceValue(pmsg, &fSettingsMsg);
				
			fLock.Unlock();
		}
	}
	
	return result;
}

// ---------------------------------------------------------------
// FindLiveSetting
//
// Returns a pointer to the LiveSetting with the given name
//
//
// Preconditions:
//
// Parameters:	name	name of the LiveSetting to find
//
//
// Postconditions:
//
// Returns: NULL if the LiveSetting cannot be found, or a pointer
//          to the desired LiveSetting if it is found
// ---------------------------------------------------------------
const LiveSetting *
LiveSettings::FindLiveSetting(const char *name)
{
	for (int32 i = 0; i < fDefCount; i++) {
		if (!strcmp(fDefaults[i].GetName(), name))
			return fDefaults + i;
	}
	return NULL;
}

bool
LiveSettings::SetGetBool(const char *name, bool *pVal /*= NULL*/)
{
	bool bPrevVal = false;
	GetValue<bool>(name, bPrevVal);
	if (pVal != NULL)
		SetValue<bool>(name, *pVal);

	return bPrevVal;
}

int32
LiveSettings::SetGetInt32(const char *name, int32 *pVal /*= NULL*/)
{
	int32 iPrevVal = 0;
	GetValue<int32>(name, iPrevVal);
	if (pVal != NULL)
		SetValue<int32>(name, *pVal);
	
	return iPrevVal;
}

void
LiveSettings::SetString(const char *name, const BString &str)
{
	SetValue<BString>(name, str);
}
	
void
LiveSettings::GetString(const char *name, BString &str)
{
	GetValue<BString>(name, str);
}

template <class T>
bool
LiveSettings::SetValue(const char *name, const T &val)
{
	bool bResult = false, bChanged = false;
	fLock.Lock();
	
	const LiveSetting *def = FindLiveSetting(name);
	if (def) {
		bResult = def->AddReplaceValue(&fSettingsMsg, val);
		bChanged = true;
	}
		
	fLock.Unlock();
	
	if (bChanged == true)
		NotifySettingChanged(def->GetId());
		
	return bResult;
}

template <class T>
bool
LiveSettings::GetValue(const char *name, T &val)
{
	fLock.Lock();
	
	bool bResult = false;
	const LiveSetting *def = FindLiveSetting(name);
	if (def)
		bResult = def->GetValue(&fSettingsMsg, val);
		
	fLock.Unlock();
	
	return bResult;
}
