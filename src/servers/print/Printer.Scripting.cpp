/*****************************************************************************/
// print_server Background Application.
//
// Version: 1.0.0d1
//
// The print_server manages the communication between applications and the
// printer and transport drivers.
//   
// Author
//   Ithamar R. Adema
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

#include "Printer.h"


#include "pr_server.h"

	// BeOS API
#include <PropertyInfo.h>
#include <Messenger.h>
#include <Message.h>
#include <AppDefs.h>

static property_info prop_list[] = {
	{ "Name", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Get name of printer" }, 
	{ "TransportAddon", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Get name of the transport add-on used for this printer" },
	{ "TransportConfig", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Get the transport configuration for this printer" },
	{ "PrinterAddon", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Get name of the printer add-on used for this printer" },
	{ "Comments", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Get comments about this printer" },
	{ 0 } // terminate list 
};

void Printer::HandleScriptingCommand(BMessage* msg)
{
	status_t rc = B_ERROR;
	BString propName;
	BString result;
	BMessage spec;
	int32 idx;

	if ((rc=msg->GetCurrentSpecifier(&idx,&spec)) == B_OK &&
		(rc=spec.FindString("property",&propName)) == B_OK) {
		switch(msg->what) {
			case B_GET_PROPERTY:
				if (propName == "Name")
					rc = fNode.ReadAttrString(PSRV_PRINTER_ATTR_PRT_NAME, &result);
				else if (propName == "TransportAddon")
					rc = fNode.ReadAttrString(PSRV_PRINTER_ATTR_TRANSPORT, &result);
				else if (propName == "TransportConfig")
					rc = fNode.ReadAttrString(PSRV_PRINTER_ATTR_TRANSPORT_ADDR, &result);
				else if (propName == "PrinterAddon")
					rc = fNode.ReadAttrString(PSRV_PRINTER_ATTR_DRV_NAME, &result);
				else if (propName == "Comments")
					rc = fNode.ReadAttrString(PSRV_PRINTER_ATTR_COMMENTS, &result);
				else { // If unknown scripting request, let superclas handle it
					Inherited::MessageReceived(msg);
					break;
				}

				BMessage reply(B_REPLY);
				reply.AddString("result", result);
				reply.AddInt32("error", rc);
				msg->SendReply(&reply);
				break;
		}
	}
	else {
			// If GetSpecifier failed
		if (idx == -1) {
			BMessage reply(B_REPLY);
			reply.AddMessenger("result", BMessenger(this));
			reply.AddInt32("error", B_OK);
			msg->SendReply(&reply);
		}
	}
}

BHandler* Printer::ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec,
	int32 form, const char* prop)
{
	BPropertyInfo prop_info(prop_list);
	BHandler* rc = this;
	
	int32 idx;
	switch( idx=prop_info.FindMatch(msg,0,spec,form,prop) ) {
		case B_ERROR:
			rc = Inherited::ResolveSpecifier(msg,index,spec,form,prop);
			break;
	}
	
	return rc;
}

status_t Printer::GetSupportedSuites(BMessage* msg)
{
	msg->AddString("suites", "application/x-vnd.OpenBeOS-printer");
	
	BPropertyInfo prop_info(prop_list);
	msg->AddFlat("messages", &prop_info);
	
	return Inherited::GetSupportedSuites(msg);
}
