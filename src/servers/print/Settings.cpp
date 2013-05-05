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
	if (printer != NULL)
		fPrinter = printer;
}


// Implementation of PrinterSettings

PrinterSettings::PrinterSettings(const char* printer,
		BMessage* pageSettings, BMessage* jobSettings)
	: fPrinter(printer)
{
	if (pageSettings != NULL)
		fPageSettings = *pageSettings;
	if (jobSettings != NULL)
		fJobSettings = *jobSettings;
}


// Implementation of Settings

Settings* Settings::sSingleton = NULL;

static const BRect kConfigWindowFrame(30, 30, 220, 120);

Settings::Settings()
	: fApps(true)     // owns AppSettings
	, fPrinters(true) // owns PrinterSettings
	, fUseConfigWindow(true)
	, fConfigWindowFrame(kConfigWindowFrame)
{
}


Settings::~Settings()
{
	sSingleton = NULL;
}


Settings* 
Settings::GetSettings()
{
	if (sSingleton == NULL)
		sSingleton = new Settings();
	return sSingleton;
}


void 
Settings::RemoveAppSettings(int i)
{
	delete fApps.RemoveItemAt(i);
}


void 
Settings::RemovePrinterSettings(int i)
{
	delete fPrinters.RemoveItemAt(i);
}


AppSettings* 
Settings::FindAppSettings(const char* mimeType)
{
	for (int i = AppSettingsCount() - 1; i >= 0; i --) {
		if (strcmp(AppSettingsAt(i)->GetMimeType(), mimeType) == 0) 
			return AppSettingsAt(i);
	}
	return NULL;
}


PrinterSettings* 
Settings::FindPrinterSettings(const char* printer)
{
	for (int i = PrinterSettingsCount() - 1; i >= 0; i --) {
		if (strcmp(PrinterSettingsAt(i)->GetPrinter(), printer) == 0) 
			return PrinterSettingsAt(i);
	}
	return NULL;
}


void 
Settings::Save(BFile* file)
{
	BMessage message;
	// store application settings
	for (int i = 0; i < AppSettingsCount(); i++) {
		AppSettings* app = AppSettingsAt(i);
		message.AddString("message", app->GetMimeType());
		message.AddString("p", app->GetPrinter());
	}
	// store printer settings
	for (int i = 0; i < PrinterSettingsCount(); i++) {
		PrinterSettings* p = PrinterSettingsAt(i);
		message.AddString("P", p->GetPrinter());
		message.AddMessage("S", p->GetPageSettings());
		message.AddMessage("J", p->GetJobSettings());
	}
	
	message.AddBool("UseConfigWindow", fUseConfigWindow);
	message.AddRect("ConfigWindowFrame", fConfigWindowFrame);
	message.AddString("DefaultPrinter", fDefaultPrinter);
	message.Flatten(file);
}


void 
Settings::Load(BFile* file)
{
	BMessage message;
	if (message.Unflatten(file) == B_OK) {
		// restore application settings
		BString mimetype;
		BString printer;
		for (int i = 0; message.FindString("message", i, &mimetype) == B_OK &&
			message.FindString("p", i, &printer ) == B_OK; i ++) {
			AddAppSettings(new AppSettings(mimetype.String(), printer.String()));
		}

		// restore printer settings
		BMessage page;
		BMessage job;
		for (int i = 0; message.FindString("P", i, &printer) == B_OK &&
			message.FindMessage("S", i, &page) == B_OK &&
			message.FindMessage("J", i, &job) == B_OK; i ++) {
			AddPrinterSettings(new PrinterSettings(printer.String(), &page, &job));
		}
		
		if (message.FindBool("UseConfigWindow", &fUseConfigWindow) != B_OK)
			fUseConfigWindow = true;
		
		if (message.FindRect("ConfigWindowFrame", &fConfigWindowFrame) != B_OK)
			fConfigWindowFrame = BRect(kConfigWindowFrame);
			
		if (message.FindString("DefaultPrinter", &fDefaultPrinter) != B_OK)
			fDefaultPrinter = "";
	}
}
