/*****************************************************************************/
// print_server Background Application.
//
// Version: 1.0.0d1
//
// The print_server manages the communication between applications and the
// printer and transport drivers.
//   
// Authors
//   Ithamar R. Adema
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001, 2002 OpenBeOS Project
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
	~Printer();

		// Static helper functions
	static Printer* Find(const BString& name);
	static Printer* Find(node_ref* node);
	static Printer* At(int32 idx);
	static void Remove(Printer* printer);
	static int32 CountPrinters();

	status_t Remove();
	status_t ConfigurePrinter();
	status_t ConfigureJob(BMessage& ioSettings);
	status_t ConfigurePage(BMessage& ioSettings);
	status_t GetDefaultSettings(BMessage& configuration);

		// Try to start processing of next spooled job
	void HandleSpooledJob();
		// Abort print_thread without processing spooled job
	void AbortPrintThread();

	void MessageReceived(BMessage* msg);

		// Scripting support, see Printer.Scripting.cpp
	status_t GetSupportedSuites(BMessage* msg);
	void HandleScriptingCommand(BMessage* msg);
	BHandler* ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec,
								int32 form, const char* prop);
								
	void GetName(BString& name);
	Resource* GetResource() { return fResource; }
	
private:
	status_t LoadPrinterAddon(image_id& id);
	void AddCurrentPrinter(BMessage* m);

	SpoolFolder fPrinter;      // the printer spooling directory
	Resource* fResource;       // the resource required for processing a print job
	bool fSinglePrintThread;   // is printer add-on allowed to process multiple print job at once
	Job* fJob;                 // the next job to process
	vint32 fProcessing;        // the current nmber of processing threads
	bool fAbort;	           // stop processing
	
	static BObjectList<Printer> sPrinters;

		// Accessor
	BDirectory* SpoolDir() { return fPrinter.GetSpoolDir(); }

	void ResetJobStatus();
	bool HasCurrentPrinter(BString& name);
	bool MoveJob(const BString& name);
		// Get next spooled job if any
	bool FindSpooledJob();
	status_t PrintSpooledJob(BFile* spoolFile);
	void PrintThread(Job* job);
	static status_t print_thread(void* data);
	void StartPrintThread();
};

#endif
