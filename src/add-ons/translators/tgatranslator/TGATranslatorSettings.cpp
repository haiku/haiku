/*****************************************************************************/
// TGATranslatorSettings
// TGATranslatorSettings.cpp
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

#include <File.h>
#include <FindDirectory.h>
#include <TranslatorFormats.h>
	// for B_TRANSLATOR_EXT_*
#include "TGATranslatorSettings.h"

TGATranslatorSettings::TGATranslatorSettings()
	: flock("TGA Settings Lock")
{
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &fsettingsPath))
		fsettingsPath.SetTo("/tmp");
	fsettingsPath.Append(TGA_SETTINGS_FILENAME);
	
	frefCount = 1;
	
	// Default Settings
	// (Used when loading from the settings file or from
	// a BMessage fails)
	fmsgSettings.AddBool(B_TRANSLATOR_EXT_HEADER_ONLY, false);
	fmsgSettings.AddBool(B_TRANSLATOR_EXT_DATA_ONLY, false);
	fmsgSettings.AddBool(TGA_SETTING_RLE, false);
		// RLE compression is off by default
}

TGATranslatorSettings *
TGATranslatorSettings::Acquire()
{
	TGATranslatorSettings *psettings = NULL;
	
	flock.Lock();
	frefCount++;
	psettings = this;
	flock.Unlock();
	
	return psettings;
}

TGATranslatorSettings *
TGATranslatorSettings::Release()
{
	TGATranslatorSettings *psettings = NULL;
	
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

TGATranslatorSettings::~TGATranslatorSettings()
{
}
	
status_t
TGATranslatorSettings::LoadSettings()
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

status_t
TGATranslatorSettings::LoadSettings(BMessage *pmsg)
{
	status_t result = B_BAD_VALUE;
	
	if (pmsg) {
		// Make certain that no TGA settings
		// are missing from the file
		bool bheaderOnly, bdataOnly, brle;
		
		flock.Lock();
		result = pmsg->FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &bheaderOnly);
		if (result != B_OK)
			bheaderOnly = SetGetHeaderOnly();
			
		result = pmsg->FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &bdataOnly);
		if (result != B_OK)
			bdataOnly = SetGetDataOnly();
			
		result = pmsg->FindBool(TGA_SETTING_RLE, &brle);
		if (result != B_OK)
			brle = SetGetRLE();
				
		if (bheaderOnly && bdataOnly)
			// "write header only" and "write data only"
			// are mutually exclusive
			result = B_BAD_VALUE;
		else {
			fmsgSettings.ReplaceBool(
				B_TRANSLATOR_EXT_HEADER_ONLY, bheaderOnly);
				
			fmsgSettings.ReplaceBool(
				B_TRANSLATOR_EXT_DATA_ONLY, bdataOnly);
				
			fmsgSettings.ReplaceBool(TGA_SETTING_RLE, brle);
			
			result = B_OK;
		}
		flock.Unlock();
	}
	
	return result;
}

status_t
TGATranslatorSettings::SaveSettings()
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

status_t
TGATranslatorSettings::GetConfigurationMessage(BMessage *pmsg)
{
	status_t result = B_BAD_VALUE;
	
	if (pmsg) {
		const char *kNames[] = {
			B_TRANSLATOR_EXT_HEADER_ONLY,
			B_TRANSLATOR_EXT_DATA_ONLY,
			TGA_SETTING_RLE
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
			
			pmsg->AddBool(B_TRANSLATOR_EXT_HEADER_ONLY,
				SetGetHeaderOnly());
				
			pmsg->AddBool(B_TRANSLATOR_EXT_DATA_ONLY,
				SetGetDataOnly());
				
			pmsg->AddBool(TGA_SETTING_RLE,
				SetGetRLE());
				
			flock.Unlock();
			result = B_OK;
		}
	}
	
	return result;
}

bool
TGATranslatorSettings::SetGetHeaderOnly(bool *pbHeaderOnly)
{
	bool bprevValue;
	
	flock.Lock();
	fmsgSettings.FindBool(B_TRANSLATOR_EXT_HEADER_ONLY, &bprevValue);
	if (pbHeaderOnly)
		fmsgSettings.ReplaceBool(B_TRANSLATOR_EXT_HEADER_ONLY, *pbHeaderOnly);
	flock.Unlock();
	
	return bprevValue;
}

bool
TGATranslatorSettings::SetGetDataOnly(bool *pbDataOnly)
{
	bool bprevValue;
	
	flock.Lock();
	fmsgSettings.FindBool(B_TRANSLATOR_EXT_DATA_ONLY, &bprevValue);
	if (pbDataOnly)
		fmsgSettings.ReplaceBool(B_TRANSLATOR_EXT_DATA_ONLY, *pbDataOnly);
	flock.Unlock();
	
	return bprevValue;
}
	
bool
TGATranslatorSettings::SetGetRLE(bool *pbRLE)
{
	bool bprevValue;
	
	flock.Lock();
	fmsgSettings.FindBool(TGA_SETTING_RLE, &bprevValue);
	if (pbRLE)
		fmsgSettings.ReplaceBool(TGA_SETTING_RLE, *pbRLE);
	flock.Unlock();
	
	return bprevValue;
}