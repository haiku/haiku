/*****************************************************************************/
// print_server Background Application.
//
// Version: 1.0.0d1
//
// The print_server manages the communication between applications and the
// printer and transport drivers.
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

#ifndef PRINTSERVERAPP_H
#define PRINTSERVERAPP_H

class PrintServerApp;

#include <Application.h>
#include <Bitmap.h>
#include <String.h>

/*****************************************************************************/
// PrintServerApp
//
// Application class for print_server.
/*****************************************************************************/
class Printer;
class PrintServerApp : public BApplication
{
	typedef BApplication Inherited;
public:
	PrintServerApp(status_t* err);
	bool QuitRequested();
	void MessageReceived(BMessage* msg);
	
		// Scripting support, see PrintServerApp.Scripting.cpp
	status_t GetSupportedSuites(BMessage* msg);
	void HandleScriptingCommand(BMessage* msg);
	Printer* GetPrinterFromSpecifier(BMessage* msg);
	BHandler* ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec,
								int32 form, const char* prop);
private:
	status_t SetupPrinterList();

	status_t  HandleSpooledJob(const char* jobname, const char* filepath);
	
	status_t SelectPrinter(const char* printerName);
	status_t CreatePrinter(	const char* printerName, const char* driverName,
							const char* connection, const char* transportName,
							const char* transportPath);
	
	status_t StoreDefaultPrinter();
	status_t RetrieveDefaultPrinter();
	
	status_t FindPrinterNode(const char* name, BNode& node);
	status_t FindPrinterDriver(const char* name, BPath& outPath);
	
	Printer* fDefaultPrinter;
	BBitmap fSelectedIconMini;
	BBitmap fSelectedIconLarge;

		// "Classic" BeOS R5 support, see PrintServerApp.R5.cpp
	void Handle_BeOSR5_Message(BMessage* msg);
};

#endif
