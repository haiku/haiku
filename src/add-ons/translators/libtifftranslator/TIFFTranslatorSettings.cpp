/*****************************************************************************/
// TIFFTranslatorSettings
// Adopted by Stephan Aßmus, <stippi@yellowbites.com>
// from TGATranslatorSettings written by
// Michael Wilber, OBOS Translation Kit Team
//
// TIFFTranslatorSettings.cpp
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

#include "tiff.h"

#include <File.h>
#include <FindDirectory.h>
#include <TranslatorFormats.h>
	// for B_TRANSLATOR_EXT_*
#include "TIFFTranslatorSettings.h"

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
TIFFTranslatorSettings::TIFFTranslatorSettings()
	: fLock("TIFF Settings Lock")
{
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &fSettingsPath) < B_OK)
		fSettingsPath.SetTo("/tmp");
	fSettingsPath.Append(TIFF_SETTINGS_FILENAME);
	
	fRefCount = 1;
	
	// Default Settings
	// (Used when loading from the settings file or from
	// a BMessage fails)
	fSettingsMSG.AddBool(B_TRANSLATOR_EXT_HEADER_ONLY, false);
	fSettingsMSG.AddBool(B_TRANSLATOR_EXT_DATA_ONLY, false);
	fSettingsMSG.AddInt32(TIFF_SETTING_COMPRESSION, COMPRESSION_LZW);
		// compression is set to LZW by default
}

// ---------------------------------------------------------------
// Acquire
//
// Returns a pointer to the TIFFTranslatorSettings and increments
// the reference count. 
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: pointer to this TIFFTranslatorSettings object
// ---------------------------------------------------------------
TIFFTranslatorSettings *
TIFFTranslatorSettings::Acquire()
{
	TIFFTranslatorSettings *psettings = NULL;
	
	if (fLock.Lock()) {
		fRefCount++;
		psettings = this;
		fLock.Unlock();
	}
	
	return psettings;
}

// ---------------------------------------------------------------
// Release
//
// Decrements the reference count and deletes the
// TIFFTranslatorSettings if the reference count is zero.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: pointer to this TIFFTranslatorSettings object if
// the reference count is greater than zero, returns NULL
// if the reference count is zero and the TIFFTranslatorSettings
// object has been deleted
// ---------------------------------------------------------------
TIFFTranslatorSettings *
TIFFTranslatorSettings::Release()
{
	TIFFTranslatorSettings *psettings = NULL;
	
	if (fLock.Lock()) {
		fRefCount--;
		if (fRefCount > 0) {
			psettings = this;
			fLock.Unlock();
		} else
			delete this;
				// delete this object and
				// release locks
	}
	
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
TIFFTranslatorSettings::~TIFFTranslatorSettings()
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
TIFFTranslatorSettings::LoadSettings()
{
	status_t result = B_ERROR;
	
	if (fLock.Lock()) {
	
		BFile settingsFile;
		result = settingsFile.SetTo(fSettingsPath.Path(), B_READ_ONLY);
		if (result >= B_OK) {
			BMessage msg;
			result = msg.Unflatten(&settingsFile);
			if (result >= B_OK)
				result = LoadSettings(&msg);
		}
		
		fLock.Unlock();
	}
	
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
TIFFTranslatorSettings::LoadSettings(BMessage *pmsg)
{
	status_t result = B_BAD_VALUE;
	
	if (pmsg) {
		// Make certain that no TIFF settings
		// are missing from the file
		bool bheaderOnly, bdataOnly;
		uint32 compression;

		result = B_ERROR;
		
		if (fLock.Lock()) {

			result = pmsg->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &bheaderOnly);
			if (result < B_OK)
				bheaderOnly = SetGetHeaderOnly();
			result = pmsg->FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &bdataOnly);
			if (result < B_OK)
				bdataOnly = SetGetDataOnly();
			result = pmsg->FindInt32(TIFF_SETTING_COMPRESSION, (int32*)&compression);
			if (result < B_OK)
				compression = SetGetCompression();
					
			if (bheaderOnly && bdataOnly)
				// "write header only" and "write data only"
				// are mutually exclusive
				result = B_BAD_VALUE;
			else {
				result = B_OK;
				
				result = fSettingsMSG.ReplaceBool(
					B_TRANSLATOR_EXT_HEADER_ONLY, bheaderOnly);
				if (result >= B_OK)
					result = fSettingsMSG.ReplaceBool(
						B_TRANSLATOR_EXT_DATA_ONLY, bdataOnly);
				if (result >= B_OK)
					result = fSettingsMSG.ReplaceInt32(TIFF_SETTING_COMPRESSION,
													   compression);
			}
			fLock.Unlock();
		}
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
TIFFTranslatorSettings::SaveSettings()
{
	status_t result = B_ERROR;
	
	if (fLock.Lock()) {
	
		BFile settingsFile;
		result = settingsFile.SetTo(fSettingsPath.Path(),
			B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (result >= B_OK)
			result = fSettingsMSG.Flatten(&settingsFile);
			
		fLock.Unlock();
	}
	
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
TIFFTranslatorSettings::GetConfigurationMessage(BMessage *pmsg)
{
	status_t result = B_BAD_VALUE;
	
	if (pmsg) {
		const char *kNames[] = {
			B_TRANSLATOR_EXT_HEADER_ONLY,
			B_TRANSLATOR_EXT_DATA_ONLY,
			TIFF_SETTING_COMPRESSION
		};
		const int32 klen = sizeof(kNames) / sizeof(const char *);
		int32 i;
		for (i = 0; i < klen; i++) {
			result = pmsg->RemoveName(kNames[i]);
			if (result < B_OK && result != B_NAME_NOT_FOUND)
				break;
		}
		if (i == klen && fLock.Lock()) {
			result = B_OK;
			
			result = pmsg->AddBool(B_TRANSLATOR_EXT_HEADER_ONLY,
				SetGetHeaderOnly());
			if (result >= B_OK)
				result = pmsg->AddBool(B_TRANSLATOR_EXT_DATA_ONLY,
					SetGetDataOnly());
			if (result >= B_OK)		
				result = pmsg->AddInt32(TIFF_SETTING_COMPRESSION, SetGetCompression());
				
			fLock.Unlock();
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
TIFFTranslatorSettings::SetGetHeaderOnly(bool *pbHeaderOnly)
{
	bool bprevValue;
	
	if (fLock.Lock()) {
		fSettingsMSG.FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &bprevValue);
		if (pbHeaderOnly)
			fSettingsMSG.ReplaceBool(B_TRANSLATOR_EXT_HEADER_ONLY, *pbHeaderOnly);
		fLock.Unlock();
	}
	
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
TIFFTranslatorSettings::SetGetDataOnly(bool *pbDataOnly)
{
	bool bprevValue;
	
	if (fLock.Lock()) {
		fSettingsMSG.FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &bprevValue);
		if (pbDataOnly)
			fSettingsMSG.ReplaceBool(B_TRANSLATOR_EXT_DATA_ONLY, *pbDataOnly);
		fLock.Unlock();
	}
	
	return bprevValue;
}

// ---------------------------------------------------------------
// SetGetRLE
//
// Sets the state of the RLE setting (if pbRLE is not NULL)
// and returns the previous value of the RLE setting.
//
// If the RLE setting is true, TIFF images created by the 
// TIFFTranslator will be RLE compressed.
//
// Preconditions:
//
// Parameters:	pbRLE	pointer to bool which specifies
//						the new value for the RLE setting
//
// Postconditions:
//
// Returns: the prior value of the RLE setting
// ---------------------------------------------------------------
uint32
TIFFTranslatorSettings::SetGetCompression(uint32 *pCompression)
{
	uint32 prevValue;
	
	if (fLock.Lock()) {
		fSettingsMSG.FindInt32(TIFF_SETTING_COMPRESSION, (int32*)&prevValue);
		if (pCompression)
			fSettingsMSG.ReplaceInt32(TIFF_SETTING_COMPRESSION, *pCompression);
		fLock.Unlock();
	}
	
	return prevValue;
}

