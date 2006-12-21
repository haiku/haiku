/*
 * Copyright 2001-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 *		Michael Pfeiffer
 */
#ifndef PRINTSERVERAPP_H
#define PRINTSERVERAPP_H

#include "ResourceManager.h"
#include "Settings.h"
#include "FolderWatcher.h"

class PrintServerApp;

#include <Application.h>
#include <Bitmap.h>
#include <OS.h>
#include <String.h>

// global BLocker for synchronisation
extern BLocker* gLock;

/*****************************************************************************/
// PrintServerApp
//
// Application class for print_server.
/*****************************************************************************/
class Printer;
class PrintServerApp : public BApplication, public FolderListener
{
	typedef BApplication Inherited;
public:
	PrintServerApp(status_t* err);
	~PrintServerApp();
		
	void Acquire();
	void Release();
	
	bool QuitRequested();
	void MessageReceived(BMessage* msg);
	void NotifyPrinterDeletion(Printer* printer);
	
		// Scripting support, see PrintServerApp.Scripting.cpp
	status_t GetSupportedSuites(BMessage* msg);
	void HandleScriptingCommand(BMessage* msg);
	Printer* GetPrinterFromSpecifier(BMessage* msg);
	BHandler* ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec,
								int32 form, const char* prop);
private:
	bool OpenSettings(BFile& file, const char* name, bool forReading);
	void LoadSettings();
	void SaveSettings();

	status_t SetupPrinterList();

	void     HandleSpooledJobs();
	
	status_t SelectPrinter(const char* printerName);
	status_t CreatePrinter(	const char* printerName, const char* driverName,
							const char* connection, const char* transportName,
							const char* transportPath);

	void     RegisterPrinter(BDirectory* node);
	void     UnregisterPrinter(Printer* printer);
	// FolderListener
	void     EntryCreated(node_ref* node, entry_ref* entry);
	void     EntryRemoved(node_ref* node);
	void     AttributeChanged(node_ref* node);
		
	status_t StoreDefaultPrinter();
	status_t RetrieveDefaultPrinter();
	
	status_t FindPrinterNode(const char* name, BNode& node);
	status_t FindPrinterDriver(const char* name, BPath& outPath);
	
	ResourceManager fResourceManager;
	Printer* fDefaultPrinter;
	BBitmap* fSelectedIconMini;
	BBitmap* fSelectedIconLarge;
	vint32   fReferences; 
	sem_id   fHasReferences;
	Settings*fSettings;
	bool     fUseConfigWindow;
	FolderWatcher* fFolder;
	
		// "Classic" BeOS R5 support, see PrintServerApp.R5.cpp
	static status_t async_thread(void* data);
	void AsyncHandleMessage(BMessage* msg);
	void Handle_BeOSR5_Message(BMessage* msg);

};

#endif
