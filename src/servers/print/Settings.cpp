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


#include "Settings.h"

#include <StorageKit.h>

// Implementation of AppSettings

AppSettings::AppSettings(const char* mimetype, const char* printer) 
	: fMimeType(mimetype)
{
	if (printer) fPrinter = printer;
}


// Implementation of PrinterSettings

PrinterSettings::PrinterSettings(const char* printer, BMessage* pageSettings, BMessage* jobSettings)
	: fPrinter(printer)
{
	if (pageSettings) fPageSettings = *pageSettings;
	if (jobSettings) fJobSettings = *jobSettings;
}


// Implementation of Settings

Settings* Settings::fSingleton = NULL;

static const BRect kConfigWindowFrame(30, 30, 220, 120);

Settings::Settings()
	: fApps(true)     // owns AppSettings
	, fPrinters(true) // owns PrinterSettings
	, fUseConfigWindow(true)
	, fConfigWindowFrame(kConfigWindowFrame)
{
}

Settings::~Settings() {
	fSingleton = NULL;
}

Settings* Settings::GetSettings() {
	if (fSingleton == NULL) fSingleton = new Settings();
	return fSingleton;
}

void Settings::RemoveAppSettings(int i) {
	delete fApps.RemoveItemAt(i);
}

void Settings::RemovePrinterSettings(int i) {
	delete fPrinters.RemoveItemAt(i);
}

AppSettings* Settings::FindAppSettings(const char* mimeType) {
	for (int i = AppSettingsCount()-1; i >= 0; i --) {
		if (strcmp(AppSettingsAt(i)->GetMimeType(), mimeType) == 0) 
			return AppSettingsAt(i);
	}
	return NULL;
}

PrinterSettings* Settings::FindPrinterSettings(const char* printer) {
	for (int i = PrinterSettingsCount()-1; i >= 0; i --) {
		if (strcmp(PrinterSettingsAt(i)->GetPrinter(), printer) == 0) 
			return PrinterSettingsAt(i);
	}
	return NULL;
}

void Settings::Save(BFile* file) {
	BMessage m;
		// store application settings
	for (int i = 0; i < AppSettingsCount(); i++) {
		AppSettings* app = AppSettingsAt(i);
		m.AddString("m", app->GetMimeType());
		m.AddString("p", app->GetPrinter());
	}
		// store printer settings
	for (int i = 0; i < PrinterSettingsCount(); i++) {
		PrinterSettings* p = PrinterSettingsAt(i);
		m.AddString("P", p->GetPrinter());
		m.AddMessage("S", p->GetPageSettings());
		m.AddMessage("J", p->GetJobSettings());
	}
	
	m.AddBool("UseConfigWindow", fUseConfigWindow);
	m.AddRect("ConfigWindowFrame", fConfigWindowFrame);
	m.Flatten(file);
}

void Settings::Load(BFile* file) {
	BMessage m;
	if (m.Unflatten(file) == B_OK) {
			// restore application settings
		BString mimetype, printer;
		for (int i = 0; m.FindString("m", i, &mimetype) == B_OK &&
			m.FindString("p", i, &printer ) == B_OK; i ++) {
			AddAppSettings(new AppSettings(mimetype.String(), printer.String()));
		}
			// restore printer settings
		BMessage page, job;
		for (int i = 0; m.FindString("P", i, &printer) == B_OK &&
			m.FindMessage("S", i, &page) == B_OK &&
			m.FindMessage("J", i, &job) == B_OK; i ++) {
			AddPrinterSettings(new PrinterSettings(printer.String(), &page, &job));
		}
		
		if (m.FindBool("UseConfigWindow", &fUseConfigWindow) != B_OK)
			fUseConfigWindow = true;
		
		if (m.FindRect("ConfigWindowFrame", &fConfigWindowFrame) != B_OK)
			fConfigWindowFrame = BRect(kConfigWindowFrame);
	}
}
