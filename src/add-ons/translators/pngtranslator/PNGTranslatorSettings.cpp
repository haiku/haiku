/*****************************************************************************/
// PNGTranslatorSettings
// Written by Michael Wilber
//
// PNGTranslatorSettings.cpp
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

#include <png.h>
#include <File.h>
#include <FindDirectory.h>
#include <TranslatorFormats.h>
	// for B_TRANSLATOR_EXT_*
#include "PNGTranslatorSettings.h"

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
PNGTranslatorSettings::PNGTranslatorSettings()
	: flock("PNG Settings Lock")
{
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &fsettingsPath))
		fsettingsPath.SetTo("/tmp");
	fsettingsPath.Append(PNG_SETTINGS_FILENAME);
	
	frefCount = 1;
	
	// Default Settings
	// (Used when loading from the settings file or from
	// a BMessage fails)
	fmsgSettings.AddBool(B_TRANSLATOR_EXT_HEADER_ONLY, false);
	fmsgSettings.AddBool(B_TRANSLATOR_EXT_DATA_ONLY, false);
	fmsgSettings.AddInt32(PNG_SETTING_INTERLACE, PNG_INTERLACE_NONE);
		// interlacing is off by default
}

// ---------------------------------------------------------------
// Acquire
//
// Returns a pointer to the PNGTranslatorSettings and increments
// the reference count. 
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: pointer to this PNGTranslatorSettings object
// ---------------------------------------------------------------
PNGTranslatorSettings *
PNGTranslatorSettings::Acquire()
{
	PNGTranslatorSettings *psettings = NULL;
	
	flock.Lock();
	frefCount++;
	psettings = this;
	flock.Unlock();
	
	return psettings;
}

// ---------------------------------------------------------------
// Release
//
// Decrements the reference count and deletes the
// PNGTranslatorSettings if the reference count is zero.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: pointer to this PNGTranslatorSettings object if
// the reference count is greater than zero, returns NULL
// if the reference count is zero and the PNGTranslatorSettings
// object has been deleted
// ---------------------------------------------------------------
PNGTranslatorSettings *
PNGTranslatorSettings::Release()
{
	PNGTranslatorSettings *psettings = NULL;
	
	flock.Lock();
	frefCount--;
	if (frefCount > 0) {
		psettings = this;
		flock.Unlock();
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
PNGTranslatorSettings::~PNGTranslatorSettings()
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
PNGTranslatorSettings::LoadSettings()
{
	status_t result;
	
	flock.Lock();
	
	BFile settingsFile;
	result = settingsFile.SetTo(fsettingsPath.Path(), B_READ_ONLY);
	if (result == B_OK) {
		BMessage msg;
		result = msg.Unflatten(&settingsFile);
		if (result == B_OK)
			result = LoadSettings(&msg);
	}
	
	flock.Unlock();
	
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
PNGTranslatorSettings::LoadSettings(BMessage *pmsg)
{
	status_t result = B_BAD_VALUE;
	
	if (pmsg) {
		// Make certain that no PNG settings
		// are missing from the file
		int32 interlace;
		bool bheaderOnly, bdataOnly;
		
		flock.Lock();
		result = pmsg->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &bheaderOnly);
		if (result != B_OK)
			bheaderOnly = SetGetHeaderOnly();
			
		result = pmsg->FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &bdataOnly);
		if (result != B_OK)
			bdataOnly = SetGetDataOnly();
			
		result = pmsg->FindInt32(PNG_SETTING_INTERLACE, &interlace);
		if (result != B_OK)
			interlace = SetGetInterlace();
				
		if (bheaderOnly && bdataOnly)
			// "write header only" and "write data only"
			// are mutually exclusive
			result = B_BAD_VALUE;
		else {
			result = B_OK;
			
			result = fmsgSettings.ReplaceBool(
				B_TRANSLATOR_EXT_HEADER_ONLY, bheaderOnly);
			
			if (result == B_OK)
				result = fmsgSettings.ReplaceBool(
					B_TRANSLATOR_EXT_DATA_ONLY, bdataOnly);
		
			if (result == B_OK)		
				result = fmsgSettings.ReplaceInt32(PNG_SETTING_INTERLACE,
					interlace);
		}
		flock.Unlock();
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
PNGTranslatorSettings::SaveSettings()
{
	status_t result;
	
	flock.Lock();
	
	BFile settingsFile;
	result = settingsFile.SetTo(fsettingsPath.Path(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (result == B_OK)
		result = fmsgSettings.Flatten(&settingsFile);
		
	flock.Unlock();
	
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
PNGTranslatorSettings::GetConfigurationMessage(BMessage *pmsg)
{
	status_t result = B_BAD_VALUE;
	
	if (pmsg) {
		const char *kNames[] = {
			B_TRANSLATOR_EXT_HEADER_ONLY,
			B_TRANSLATOR_EXT_DATA_ONLY,
			PNG_SETTING_INTERLACE
		};
		const int32 klen = sizeof(kNames) / sizeof(const char *);
		int32 i;
		for (i = 0; i < klen; i++) {
			result = pmsg->RemoveName(kNames[i]);
			if (result != B_OK && result != B_NAME_NOT_FOUND)
				break;
		}
		if (i == klen) {
			flock.Lock();
			result = B_OK;
			
			result = pmsg->AddBool(B_TRANSLATOR_EXT_HEADER_ONLY,
				SetGetHeaderOnly());
			
			if (result == B_OK)
				result = pmsg->AddBool(B_TRANSLATOR_EXT_DATA_ONLY,
					SetGetDataOnly());
		
			if (result == B_OK)		
				result = pmsg->AddInt32(PNG_SETTING_INTERLACE,
					SetGetInterlace());
				
			flock.Unlock();
		}
	}
	
	return result;
}

// ---------------------------------------------------------------
// SetGetHeaderOnly
//
// Sets the state of the HeaderOnly setting (if pbHeaderOnly
// is not NULL) and returns the previous value of the
// HeaderOnly setting.
//
// If the HeaderOnly setting is true, only the header of
// the image will be output; the data will not be output.
//
// Preconditions:
//
// Parameters:	pbHeaderOnly	pointer to a bool specifying
//								the new value of the
//								HeaderOnly setting
//
// Postconditions:
//
// Returns: the prior value of the HeaderOnly setting
// ---------------------------------------------------------------
bool
PNGTranslatorSettings::SetGetHeaderOnly(bool *pbHeaderOnly)
{
	bool bprevValue = false;
		// set to default value in case FindBool fails
	
	flock.Lock();
	fmsgSettings.FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &bprevValue);
	if (pbHeaderOnly)
		fmsgSettings.ReplaceBool(B_TRANSLATOR_EXT_HEADER_ONLY, *pbHeaderOnly);
	flock.Unlock();
	
	return bprevValue;
}

// ---------------------------------------------------------------
// SetGetDataOnly
//
// Sets the state of the DataOnly setting (if pbDataOnly
// is not NULL) and returns the previous value of the
// DataOnly setting.
//
// If the DataOnly setting is true, only the data of
// the image will be output; the header will not be output.
//
// Preconditions:
//
// Parameters:	pbDataOnly		pointer to a bool specifying
//								the new value of the
//								DataOnly setting
//
// Postconditions:
//
// Returns: the prior value of the DataOnly setting
// ---------------------------------------------------------------
bool
PNGTranslatorSettings::SetGetDataOnly(bool *pbDataOnly)
{
	bool bprevValue = false;
		// set to default value in case FindBool fails
	
	flock.Lock();
	fmsgSettings.FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &bprevValue);
	if (pbDataOnly)
		fmsgSettings.ReplaceBool(B_TRANSLATOR_EXT_DATA_ONLY, *pbDataOnly);
	flock.Unlock();
	
	return bprevValue;
}

// ---------------------------------------------------------------
// SetGetInterlace
//
// Sets the state of the interlace setting (if pinterlace is
// not NULL) and returns the previous value of the interlace
// setting.
//
// If the interlace setting is PNG_INTERLACE_ADAM7, PNG images
// created by the PNGTranslator will be interlaced using the
// Adam7 method. If the setting is PNG_INTERLACE_NONE, PNG
// images created by the PNGTranslator will NOT be interlaced.
//
// Preconditions:
//
// Parameters:	pinterlace	pointer to int32 which specifies the
//							new value for the interlace setting
//
// Postconditions:
//
// Returns: the prior value of the interlace setting
// ---------------------------------------------------------------
int32
PNGTranslatorSettings::SetGetInterlace(int32 *pinterlace)
{
	int32 nprevValue = PNG_INTERLACE_NONE;
		// set to default value in case FindBool fails
	
	flock.Lock();
	
	fmsgSettings.FindInt32(PNG_SETTING_INTERLACE, &nprevValue);
	if (pinterlace && (*pinterlace == PNG_INTERLACE_NONE ||
		*pinterlace == PNG_INTERLACE_ADAM7))
		fmsgSettings.ReplaceInt32(PNG_SETTING_INTERLACE, *pinterlace);
		
	flock.Unlock();
	
	return nprevValue;
}

