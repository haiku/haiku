/*****************************************************************************/
// TranslatorSettings
// Written by Michael Wilber, Haiku Translation Kit Team
//
// TranslatorSettings.cpp
//
// This class manages (saves/loads/locks/unlocks) the settings
// for a Translator.
//
//
// Copyright (c) 2004 Haiku Project
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
#include <TranslatorFormats.h>
	// for B_TRANSLATOR_EXT_*
#include "TranslatorSettings.h"

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
TranslatorSettings::TranslatorSettings(const char *settingsFile,
	const TranSetting *defaults, int32 defCount)
	: fLock("TranslatorSettings Lock")
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
	const TranSetting *defs = fDefaults;
	for (int32 i = 0; i < fDefCount; i++) {
		switch (defs[i].dataType) {
			case TRAN_SETTING_BOOL:
				fSettingsMsg.AddBool(defs[i].name,
					static_cast<bool>(defs[i].defaultVal));
				break;

			case TRAN_SETTING_INT32:
				fSettingsMsg.AddInt32(defs[i].name, defs[i].defaultVal);
				break;

			default:
				// ASSERT here? Erase the bogus setting entry instead?
				break;
		}
	}
}

// ---------------------------------------------------------------
// Acquire
//
// Returns a pointer to the TranslatorSettings and increments
// the reference count.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: pointer to this TranslatorSettings object
// ---------------------------------------------------------------
TranslatorSettings *
TranslatorSettings::Acquire()
{
	TranslatorSettings *psettings = NULL;

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
// TranslatorSettings if the reference count is zero.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: pointer to this TranslatorSettings object if
// the reference count is greater than zero, returns NULL
// if the reference count is zero and the TranslatorSettings
// object has been deleted
// ---------------------------------------------------------------
TranslatorSettings *
TranslatorSettings::Release()
{
	TranslatorSettings *psettings = NULL;

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
TranslatorSettings::~TranslatorSettings()
{
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
TranslatorSettings::LoadSettings()
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
TranslatorSettings::LoadSettings(BMessage *pmsg)
{
	if (pmsg == NULL)
		return B_BAD_VALUE;

	fLock.Lock();
	const TranSetting *defaults = fDefaults;
	for (int32 i = 0; i < fDefCount; i++) {
		switch (defaults[i].dataType) {
			case TRAN_SETTING_BOOL:
			{
				bool value;
				if (pmsg->FindBool(defaults[i].name, &value) != B_OK) {
					if (fSettingsMsg.HasBool(defaults[i].name))
						break;
					else
						value = static_cast<bool>(defaults[i].defaultVal);
				}

				fSettingsMsg.ReplaceBool(defaults[i].name, value);
				break;
			}

			case TRAN_SETTING_INT32:
			{
				int32 value;
				if (pmsg->FindInt32(defaults[i].name, &value) != B_OK) {
					if (fSettingsMsg.HasInt32(defaults[i].name))
						break;
					else
						value = defaults[i].defaultVal;
				}

				fSettingsMsg.ReplaceInt32(defaults[i].name, value);
				break;
			}

			default:
				// TODO: ASSERT here? Erase the bogus setting entry instead?
				break;
		}
	}

	fLock.Unlock();
	return B_OK;
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
TranslatorSettings::SaveSettings()
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
TranslatorSettings::GetConfigurationMessage(BMessage *pmsg)
{
	status_t result = B_BAD_VALUE;

	if (pmsg) {
		int32 i;
		for (i = 0; i < fDefCount; i++) {
			result = pmsg->RemoveName(fDefaults[i].name);
			if (result != B_OK && result != B_NAME_NOT_FOUND)
				break;
		}
		if (i == fDefCount) {
			fLock.Lock();
			result = B_OK;

			const TranSetting *defs = fDefaults;
			for (i = 0; i < fDefCount && result >= B_OK; i++) {
				switch (defs[i].dataType) {
					case TRAN_SETTING_BOOL:
						result = pmsg->AddBool(defs[i].name,
							SetGetBool(defs[i].name));
						break;

					case TRAN_SETTING_INT32:
						result = pmsg->AddInt32(defs[i].name,
							SetGetInt32(defs[i].name));
						break;

					default:
						// ASSERT here? Erase the bogus setting entry instead?
						break;
				}
			}

			fLock.Unlock();
		}
	}

	return result;
}

// ---------------------------------------------------------------
// FindTranSetting
//
// Returns a pointer to the TranSetting with the given name
//
//
// Preconditions:
//
// Parameters:	name	name of the TranSetting to find
//
//
// Postconditions:
//
// Returns: NULL if the TranSetting cannot be found, or a pointer
//          to the desired TranSetting if it is found
// ---------------------------------------------------------------
const TranSetting *
TranslatorSettings::FindTranSetting(const char *name)
{
	for (int32 i = 0; i < fDefCount; i++) {
		if (!strcmp(fDefaults[i].name, name))
			return fDefaults + i;
	}
	return NULL;
}

// ---------------------------------------------------------------
// SetGetBool
//
// Sets the state of the bool setting identified by the given name
//
//
// Preconditions:
//
// Parameters:	name	identifies the setting to set or get
//
//				pbool	the new value for the bool, or, if null,
//						it indicates that the caller wants to Get
//						rather than Set
//
// Postconditions:
//
// Returns: the prior value of the setting
// ---------------------------------------------------------------
bool
TranslatorSettings::SetGetBool(const char *name, bool *pbool)
{
	bool bprevValue;

	fLock.Lock();

	const TranSetting *def = FindTranSetting(name);
	if (def) {
		fSettingsMsg.FindBool(def->name, &bprevValue);
		if (pbool)
			fSettingsMsg.ReplaceBool(def->name, *pbool);
	} else
		bprevValue = false;

	fLock.Unlock();

	return bprevValue;
}

// ---------------------------------------------------------------
// SetGetInt32
//
// Sets the state of the int32 setting identified by the given name
//
//
// Preconditions:
//
// Parameters:	name	identifies the setting to set or get
//
//				pint32	the new value for the setting, or, if null,
//						it indicates that the caller wants to Get
//						rather than Set
//
// Postconditions:
//
// Returns: the prior value of the setting
// ---------------------------------------------------------------
int32
TranslatorSettings::SetGetInt32(const char *name, int32 *pint32)
{
	int32 prevValue;

	fLock.Lock();

	const TranSetting *def = FindTranSetting(name);
	if (def) {
		fSettingsMsg.FindInt32(def->name, &prevValue);
		if (pint32)
			fSettingsMsg.ReplaceInt32(def->name, *pint32);
	} else
		prevValue = 0;

	fLock.Unlock();

	return prevValue;
}

