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
// Copyright (c) 2001 OpenBeOS Project
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
	static Printer* Find(dev_t device, ino_t node);
	static Printer* At(int32 idx);
	static void Remove(Printer* printer);
	static int32 CountPrinters();
	
	status_t Remove();
	status_t ConfigurePrinter();
	status_t ConfigureJob(BMessage& ioSettings);
	status_t ConfigurePage(BMessage& ioSettings);
	void HandleSpooledJob();
	void AbortPrintThread();

	void MessageReceived(BMessage* msg);

		// Scripting support, see Printer.Scripting.cpp
	status_t GetSupportedSuites(BMessage* msg);
	void HandleScriptingCommand(BMessage* msg);
	BHandler* ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec,
								int32 form, const char* prop);
private:
	status_t LoadPrinterAddon(image_id& id);

	Folder fFolder;
	Resource* fResource;
	BDirectory fNode;
	bool fSinglePrintThread;
	Job* fJob;
	volatile vint32 fProcessing;
	bool fAbort;	
	
	static BObjectList<Printer> sPrinters;

	bool FindSpooledJob();
	void CloseJob();
	status_t PrintSpooledJob(BFile* spoolFile);
	void PrintThread(Job* job);
	static status_t print_thread(void* data);
	status_t StartPrintThread();
};

#endif
