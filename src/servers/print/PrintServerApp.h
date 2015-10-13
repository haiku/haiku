/*
 * Copyright 2001-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 *		Michael Pfeiffer
 */
#ifndef _PRINT_SERVER_APP_H
#define _PRINT_SERVER_APP_H


#include <Bitmap.h>
#include <Catalog.h>
#include <OS.h>
#include <Server.h>
#include <String.h>

#include "FolderWatcher.h"
#include "ResourceManager.h"
#include "Settings.h"


class Printer;
class Transport;


// The global BLocker for synchronisation.
extern BLocker *gLock;


// The print_server application.
class PrintServerApp : public BServer, public FolderListener {
private:
		typedef BServer Inherited;

public:
								PrintServerApp(status_t* error);
								~PrintServerApp();

			void				Acquire();
			void				Release();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* msg);
			void				NotifyPrinterDeletion(Printer* printer);

	// Scripting support, see PrintServerApp.Scripting.cpp
	virtual	status_t			GetSupportedSuites(BMessage* msg);
			void				HandleScriptingCommand(BMessage* msg);
			Printer*			GetPrinterFromSpecifier(BMessage* msg);
			Transport*			GetTransportFromSpecifier(BMessage* msg);
	virtual	BHandler*			ResolveSpecifier(BMessage* msg, int32 index,
									BMessage* specifier, int32 form,
									const char* property);

private:
			bool				OpenSettings(BFile& file, const char* name,
									bool forReading);
			void				LoadSettings();
			void				SaveSettings();

			status_t			SetupPrinterList();

			void				HandleSpooledJobs();

			status_t			SelectPrinter(const char* printerName);
			status_t			CreatePrinter(const char* printerName,
									const char* driverName,
									const char* connection,
									const char* transportName,
									const char* transportPath);

			void				RegisterPrinter(BDirectory* node);
			void				UnregisterPrinter(Printer* printer);

	// FolderListener
			void				EntryCreated(node_ref* node, entry_ref* entry);
			void				EntryRemoved(node_ref* node);
			void				AttributeChanged(node_ref* node);

			status_t			StoreDefaultPrinter();
			status_t			RetrieveDefaultPrinter();

			status_t			FindPrinterNode(const char* name, BNode& node);

		// "Classic" BeOS R5 support, see PrintServerApp.R5.cpp
	static	status_t			async_thread(void* data);
			void				AsyncHandleMessage(BMessage* msg);
			void				Handle_BeOSR5_Message(BMessage* msg);

private:
			ResourceManager		fResourceManager;
			Printer*			fDefaultPrinter;
			size_t				fIconSize;
			uint8*				fSelectedIcon;
			int32				fReferences;
			sem_id				fHasReferences;
			Settings*			fSettings;
			bool				fUseConfigWindow;
			FolderWatcher*		fFolder;
};


#endif	// _PRINT_SERVER_APP_H
