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

#include "PrintServerApp.h"

#include "Printer.h"

	// BeOS API
#include <PropertyInfo.h>

	// ANSI C
#include <stdio.h>

static property_info prop_list[] = {
	{ "ActivePrinter", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Retrieve or select the active printer" }, 
	{ "Printer", { B_GET_PROPERTY }, { B_INDEX_SPECIFIER, B_NAME_SPECIFIER, B_REVERSE_INDEX_SPECIFIER },
		"Retrieve a specific printer" },
	{ "Printer", { B_CREATE_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Create a new printer" },
	{ "Printer", { B_DELETE_PROPERTY }, { B_INDEX_SPECIFIER, B_NAME_SPECIFIER, B_REVERSE_INDEX_SPECIFIER },
		"Delete a specific printer" },
	{ "Printers", { B_COUNT_PROPERTIES }, { B_DIRECT_SPECIFIER },
		"Return the number of available printers" },
	{ 0 } // terminate list 
};

void
PrintServerApp::HandleScriptingCommand(BMessage* msg)
{
	BString propName;
	BMessage spec;
	int32 idx;

	if (msg->GetCurrentSpecifier(&idx,&spec) == B_OK &&
		spec.FindString("property",&propName) == B_OK) {
		switch(msg->what) {
			case B_GET_PROPERTY:
				if (propName == "ActivePrinter") {
					BMessage reply(B_REPLY);
					reply.AddString("result", fDefaultPrinter ? fDefaultPrinter->Name() : "");
					reply.AddInt32("error", B_OK);
					msg->SendReply(&reply);
				}
				break;
	
			case B_SET_PROPERTY:
				if (propName == "ActivePrinter") {
					BString newActivePrinter;
					if (msg->FindString("data", &newActivePrinter) == B_OK) {
						BMessage reply(B_REPLY);
						reply.AddInt32("error", SelectPrinter(newActivePrinter.String()));
						msg->SendReply(&reply);
					}
				}
				break;
	
			case B_CREATE_PROPERTY:
				if (propName == "Printer") {
					BString name, driver, transport, config;

					if (msg->FindString("name", &name) == B_OK &&
						msg->FindString("driver", &driver) == B_OK &&
						msg->FindString("transport", &transport) == B_OK &&
						msg->FindString("config", &config) == B_OK) {
						BMessage reply(B_REPLY);
						reply.AddInt32("error", CreatePrinter(name.String(), driver.String(),
													"Local", transport.String(), config.String()));
						msg->SendReply(&reply);
					}
				}
				break;
	
			case B_DELETE_PROPERTY: {
					Printer* printer = GetPrinterFromSpecifier(&spec);
					status_t rc = B_BAD_VALUE;
					
					if (printer != NULL && (rc=printer->Remove()) == B_OK) {
						delete printer;
					}
					
					BMessage reply(B_REPLY);
					reply.AddInt32("error", rc);
					msg->SendReply(&reply);
				}
				break;
	
			case B_COUNT_PROPERTIES:
				if (propName == "Printers") {
					BMessage reply(B_REPLY);
					reply.AddInt32("result", Printer::CountPrinters());
					reply.AddInt32("error", B_OK);
					msg->SendReply(&reply);
				}
				break;
		}
	}
}

Printer* PrintServerApp::GetPrinterFromSpecifier(BMessage* msg)
{
	switch(msg->what) {
		case B_NAME_SPECIFIER: {
			BString name;
			if (msg->FindString("name", &name) == B_OK) {
				return Printer::Find(name.String());
			}
			break;
		}
		
		case B_INDEX_SPECIFIER: {
			int32 idx;
			if (msg->FindInt32("index", &idx) == B_OK) {
				return Printer::At(idx);
			}
			break;
		}

		case B_REVERSE_INDEX_SPECIFIER: {
			int32 idx;
			if (msg->FindInt32("index", &idx) == B_OK) {
				return Printer::At(Printer::CountPrinters() - idx);
			}
			break;
		}
	}
	
	return NULL;
}

BHandler*
PrintServerApp::ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec,
								int32 form, const char* prop)
{
	BPropertyInfo prop_info(prop_list);
	BHandler* rc = NULL;
		
	int32 idx;
	switch( idx=prop_info.FindMatch(msg,0,spec,form,prop) ) {
		case B_ERROR:
			rc = Inherited::ResolveSpecifier(msg,index,spec,form,prop);

			// GET Printer [arg]
		case 1:
			if ((rc=GetPrinterFromSpecifier(spec)) == NULL) {		
				BMessage reply(B_REPLY);
				reply.AddInt32("error", B_BAD_INDEX);
				msg->SendReply(&reply);
			}
			else
				msg->PopSpecifier();
			break;

		default:
			rc = this;
	}
	
	return rc;
}

status_t
PrintServerApp::GetSupportedSuites(BMessage* msg)
{
	msg->AddString("suites", "suite/vnd.OpenBeOS-printserver");
	
	BPropertyInfo prop_info(prop_list);
	msg->AddFlat("messages", &prop_info);
	
	return Inherited::GetSupportedSuites(msg);
}
