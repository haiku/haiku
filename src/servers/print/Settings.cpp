/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
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


Settings* 
Settings::GetSettings() {
	if (fSingleton == NULL) fSingleton = new Settings();
	return fSingleton;
}


void 
Settings::RemoveAppSettings(int i) {
	delete fApps.RemoveItemAt(i);
}


void 
Settings::RemovePrinterSettings(int i) {
	delete fPrinters.RemoveItemAt(i);
}


AppSettings* 
Settings::FindAppSettings(const char* mimeType) {
	for (int i = AppSettingsCount()-1; i >= 0; i --) {
		if (strcmp(AppSettingsAt(i)->GetMimeType(), mimeType) == 0) 
			return AppSettingsAt(i);
	}
	return NULL;
}


PrinterSettings* 
Settings::FindPrinterSettings(const char* printer) {
	for (int i = PrinterSettingsCount()-1; i >= 0; i --) {
		if (strcmp(PrinterSettingsAt(i)->GetPrinter(), printer) == 0) 
			return PrinterSettingsAt(i);
	}
	return NULL;
}


void 
Settings::Save(BFile* file) {
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
	m.AddString("DefaultPrinter", fDefaultPrinter);
	m.Flatten(file);
}


void 
Settings::Load(BFile* file) {
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
			
		if (m.FindString("DefaultPrinter", &fDefaultPrinter) != B_OK)
			fDefaultPrinter = "";
	}
}
