/*
 * Copyright 2001-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 *		Michael Pfeiffer
 */
#ifndef _PRINT_SERVER_APP_H
#define _PRINT_SERVER_APP_H

#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <OS.h>
#include <String.h>

#include "FolderWatcher.h"
#include "ResourceManager.h"
#include "Settings.h"

class Printer;
class Transport;

// The global BLocker for synchronisation.
extern BLocker *gLock;

// The print_server application.
class PrintServerApp : public BApplication, public FolderListener {
	private:
		typedef BApplication Inherited;

	public:
		PrintServerApp(status_t *err);
		~PrintServerApp();

		void Acquire();
		void Release();

		bool QuitRequested();
		void MessageReceived(BMessage *msg);
		void NotifyPrinterDeletion(Printer *printer);

		// Scripting support, see PrintServerApp.Scripting.cpp
		status_t GetSupportedSuites(BMessage *msg);
		void HandleScriptingCommand(BMessage *msg);
		Printer *GetPrinterFromSpecifier(BMessage *msg);
		Transport *GetTransportFromSpecifier(BMessage *msg);
		BHandler *ResolveSpecifier(BMessage *msg, int32 index, BMessage *spec,
			int32 form, const char *prop);
	private:
		bool OpenSettings(BFile &file, const char *name, bool forReading);
		void LoadSettings();
		void SaveSettings();

		status_t SetupPrinterList();

		void HandleSpooledJobs();

		status_t SelectPrinter(const char *printerName);
		status_t CreatePrinter(const char *printerName, const char *driverName,
			const char *connection, const char *transportName,
			const char *transportPath);

		void RegisterPrinter(BDirectory *node);
		void UnregisterPrinter(Printer *printer);

		// FolderListener
		void EntryCreated(node_ref *node, entry_ref *entry);
		void EntryRemoved(node_ref *node);
		void AttributeChanged(node_ref *node);

		status_t StoreDefaultPrinter();
		status_t RetrieveDefaultPrinter();

		status_t FindPrinterNode(const char *name, BNode &node);

		// "Classic" BeOS R5 support, see PrintServerApp.R5.cpp
		static status_t async_thread(void *data);
		void AsyncHandleMessage(BMessage *msg);
		void Handle_BeOSR5_Message(BMessage *msg);

		ResourceManager fResourceManager;
		Printer *fDefaultPrinter;
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		size_t fIconSize;
		uint8 *fSelectedIcon;
#else
		BBitmap *fSelectedIconMini;
		BBitmap *fSelectedIconLarge;
#endif
		vint32 fReferences;
		sem_id fHasReferences;
		Settings *fSettings;
		bool fUseConfigWindow;
		FolderWatcher *fFolder;
};

#endif
