/*****************************************************************************/
// Settings
//
// Author
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
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

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "BeUtils.h"
#include "ObjectList.h"

#include <String.h>

class AppSettings {
private:
	BString fMimeType; // application signature
	BString fPrinter;  // printer used by application (default == empty string)

public:
	AppSettings(const char* mimeType, const char* printer = NULL);
	
	const char* GetMimeType() const      { return fMimeType.String(); }
	bool UsesDefaultPrinter() const      { return fMimeType.Length() == 0; }
	const char* GetPrinter() const       { return fPrinter.String(); }
	void SetPrinter(const char* printer) { fPrinter = printer; }
	void SetDefaultPrinter()             { fPrinter = ""; }	
};


class PrinterSettings {
private:
	BString  fPrinter;
	BMessage fPageSettings; // default page settings
	BMessage fJobSettings;  // default job settings

public:
	PrinterSettings(const char* printer, BMessage* pageSettings = NULL, BMessage* jobSettings = NULL);
	
	const char* GetPrinter() const       { return fPrinter.String(); }
	BMessage* GetPageSettings()          { return &fPageSettings; }
	BMessage* GetJobSettings()           { return &fJobSettings; }

	void SetPrinter(const char* p)       { fPrinter = p; }
	void SetPageSettings(BMessage* s)    { fPageSettings = *s; }
	void SetJobSettings(BMessage* s)     { fJobSettings = *s; }
};

class Settings {
private:
	BObjectList<AppSettings>     fApps;
	BObjectList<PrinterSettings> fPrinters;
	bool                         fUseConfigWindow;
	BRect                        fConfigWindowFrame;
	
	static Settings* fSingleton;
	Settings();
	
public:
	static Settings* GetSettings();
	~Settings();
	
	int AppSettingsCount() const           { return fApps.CountItems(); }
	AppSettings* AppSettingsAt(int i)      { return fApps.ItemAt(i); }
	void AddAppSettings(AppSettings* s)    { fApps.AddItem(s); }
	void RemoveAppSettings(int i);
	AppSettings* FindAppSettings(const char* mimeType);
		
	int PrinterSettingsCount() const            { return fPrinters.CountItems(); }
	PrinterSettings* PrinterSettingsAt(int i)   { return fPrinters.ItemAt(i); }
	void AddPrinterSettings(PrinterSettings* s) { fPrinters.AddItem(s); }
	void RemovePrinterSettings(int i);
	PrinterSettings* FindPrinterSettings(const char* printer);
	
	bool UseConfigWindow() const       { return fUseConfigWindow; }
	void SetUseConfigWindow(bool b)    { fUseConfigWindow = b; }
	BRect ConfigWindowFrame() const    { return fConfigWindowFrame; }
	void SetConfigWindowFrame(BRect r) { fConfigWindowFrame = r; }	
		
	void Save(BFile* settings_file);
	void Load(BFile* settings_file);
};

#endif
