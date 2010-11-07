/*
 * Copyright 2001-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 *		Michael Pfeiffer
 */
#ifndef PRINTER_H
#define PRINTER_H

class Printer;

#include "BeUtils.h"
#include "ResourceManager.h"
#include "Jobs.h"

	// BeOS API
#include <Directory.h>
#include <Handler.h>
#include <image.h>
#include <Locker.h>
#include <PrintJob.h>
#include <String.h>

	// OpenTracker shared sources
#include "ObjectList.h"


class SpoolFolder : public Folder {
protected:
	void Notify(Job* job, int kind);

public:
	SpoolFolder(BLocker* locker, BLooper* looper, const BDirectory& spoolDir);
};


/*****************************************************************************/
// Printer
//
// This class represents one printer definition. It is manages all actions &
// data related to that printer.
/*****************************************************************************/
class Printer : public BHandler, public Object
{
	typedef BHandler Inherited;
public:
								Printer(const BDirectory* node, Resource* res);
	virtual						~Printer();

	virtual	void				MessageReceived(BMessage* message);
	virtual	status_t			GetSupportedSuites(BMessage* msg);
	virtual	BHandler*			ResolveSpecifier(BMessage* msg, int32 index,
									BMessage* spec, int32 form, const char* prop);

			// Static helper functions
	static	Printer*			Find(const BString& name);
	static	Printer*			Find(node_ref* node);
	static	Printer*			At(int32 idx);
	static	void				Remove(Printer* printer);
	static	int32				CountPrinters();

			status_t			Remove();
	static	status_t			FindPathToDriver(const char* driver,
									BPath* path);
	static	status_t			ConfigurePrinter(const char* driverName,
									const char* printerName);
			status_t			ConfigureJob(BMessage& ioSettings);
			status_t			ConfigurePage(BMessage& ioSettings);
			status_t			GetDefaultSettings(BMessage& configuration);

			// Try to start processing of next spooled job
			void				HandleSpooledJob();

			// Abort print_thread without processing spooled job
			void				AbortPrintThread();

			// Scripting support, see Printer.Scripting.cpp
			void				HandleScriptingCommand(BMessage* msg);

			void				GetName(BString& name);
			Resource*			GetResource() { return fResource; }

private:
			status_t			GetDriverName(BString* name);
			void				AddCurrentPrinter(BMessage& message);

			BDirectory*			SpoolDir() { return fPrinter.GetSpoolDir(); }

			void				ResetJobStatus();
			bool				HasCurrentPrinter(BString& name);
			bool				MoveJob(const BString& name);

			// Get next spooled job if any
			bool				FindSpooledJob();
			status_t			PrintSpooledJob(const char* spoolFile);
			void				PrintThread(Job* job);

	static	status_t			print_thread(void* data);
			void				StartPrintThread();

private:
			// the printer spooling directory
			SpoolFolder			fPrinter;
			// the resource required for processing a print job
			Resource*			fResource;
			// is printer add-on allowed to process multiple print job at once
			bool				fSinglePrintThread;
			// the next job to process
			Job*				fJob;
			// the current nmber of processing threads
			vint32				fProcessing;
			// stop processing
			bool				fAbort;
	static	BObjectList<Printer> sPrinters;
};

#endif
