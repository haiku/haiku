/*****************************************************************************/
// print_server Background Application.
//
// Version: 1.0.0d1
//
// The print_server manages the communication between applications and the
// printer and transport drivers.
//
//
//
// TODO:
//	 Handle spooled jobs at startup
//	 Handle printer status (asked to print a spooled job but printer busy)
//   
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

	// BeOS API
#include <Handler.h>
#include <String.h>
#include <image.h>
#include <Node.h>

	// OpenTracker shared sources
#include "ObjectList.h"

/*****************************************************************************/
// Printer
//
// This class represents one printer definition. It is manages all actions &
// data related to that printer.
/*****************************************************************************/
class Printer : public BHandler
{
	typedef BHandler Inherited;
public:
	Printer(const BNode* node);
	~Printer();
	
		// Static helper functions
	static Printer* Find(const BString& name);
	static Printer* At(int32 idx);
	static int32 CountPrinters();
	
	status_t Remove();
	status_t ConfigurePrinter();
	status_t ConfigureJob(BMessage& ioSettings);
	status_t ConfigurePage(BMessage& ioSettings);
	status_t PrintSpooledJob(BFile* spoolFile, const BMessage& settings);

	void MessageReceived(BMessage* msg);

		// Scripting support, see Printer.Scripting.cpp
	status_t GetSupportedSuites(BMessage* msg);
	void HandleScriptingCommand(BMessage* msg);
	BHandler* ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec,
								int32 form, const char* prop);
private:
	status_t LoadPrinterAddon(image_id& id);

	BNode fNode;
	
	static BObjectList<Printer> sPrinters;
};

#endif
