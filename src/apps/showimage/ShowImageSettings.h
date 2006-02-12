/*****************************************************************************/
// ShowImageSettings
// Written by Michael Pfeiffer
//
// ShowImageSettings.cpp
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

#ifndef _ShowImageSettings_H
#define _ShowImageSettings_H

#include <File.h>
#include <Message.h>
#include <Locker.h>

class ShowImageSettings
{
public:
	ShowImageSettings();
	~ShowImageSettings();

	bool Lock();
	bool GetBool(const char* name, bool defaultValue);
	int32 GetInt32(const char* name, int32 defaultValue);
	float GetFloat(const char* name, float defaultValue);
	BRect GetRect(const char* name, BRect defaultValue);
	const char* GetString(const char* name, const char* defaultValue);
	void SetBool(const char* name, bool value);
	void SetInt32(const char* name, int32 value);
	void SetFloat(const char* name, float value);
	void SetRect(const char* name, BRect value);
	void SetString(const char* name, const char* value);
	void Unlock();
	
private:
	bool OpenSettingsFile(BFile* file, bool forReading);
	void Load();
	void Save();

	BLocker fLock;
	BMessage fSettings;
};

#endif
