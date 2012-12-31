/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
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
	BString                      fDefaultPrinter;
	
	static Settings* sSingleton;
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
	
	bool UseConfigWindow() const          { return fUseConfigWindow; }
	void SetUseConfigWindow(bool b)       { fUseConfigWindow = b; }
	BRect ConfigWindowFrame() const       { return fConfigWindowFrame; }
	void SetConfigWindowFrame(BRect r)    { fConfigWindowFrame = r; }	
	const char* DefaultPrinter() const    { return fDefaultPrinter.String(); }
	void SetDefaultPrinter(const char* n) { fDefaultPrinter = n; }	
		
	void Save(BFile* settings_file);
	void Load(BFile* settings_file);
};

#endif
