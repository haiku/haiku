/*
 * Copyright 2001-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 */
#include "Printer.h"


#include "pr_server.h"

	// BeOS API
#include <AppDefs.h>
#include <Catalog.h>
#include <Locale.h>
#include <Message.h>
#include <Messenger.h>
#include <PropertyInfo.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Printer Scripting"


static property_info prop_list[] = {
	{ "Name", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		B_TRANSLATE_MARK("Get name of printer") },
	{ "TransportAddon", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		B_TRANSLATE_MARK("Get name of the transport add-on used for this printer") },
	{ "TransportConfig", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		B_TRANSLATE_MARK("Get the transport configuration for this printer") },
	{ "PrinterAddon", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		B_TRANSLATE_MARK("Get name of the printer add-on used for this printer") },
	{ "Comments", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		B_TRANSLATE_MARK("Get comments about this printer") },
	{ 0 } // terminate list
};


void
Printer::HandleScriptingCommand(BMessage* msg)
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
					rc = SpoolDir()->ReadAttrString(PSRV_PRINTER_ATTR_PRT_NAME, &result);
				else if (propName == "TransportAddon")
					rc = SpoolDir()->ReadAttrString(PSRV_PRINTER_ATTR_TRANSPORT, &result);
				else if (propName == "TransportConfig")
					rc = SpoolDir()->ReadAttrString(PSRV_PRINTER_ATTR_TRANSPORT_ADDR, &result);
				else if (propName == "PrinterAddon")
					rc = SpoolDir()->ReadAttrString(PSRV_PRINTER_ATTR_DRV_NAME, &result);
				else if (propName == "Comments")
					rc = SpoolDir()->ReadAttrString(PSRV_PRINTER_ATTR_COMMENTS, &result);
				else { // If unknown scripting request, let super class handle it
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

BHandler*
Printer::ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec,
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

status_t
Printer::GetSupportedSuites(BMessage* msg)
{
	msg->AddString("suites", "application/x-vnd.OpenBeOS-printer");

	static bool localized = false;
	if (!localized) {
		localized = true;
		for (int i = 0; prop_list[i].name != NULL; i ++)
			prop_list[i].usage = B_TRANSLATE_NOCOLLECT(prop_list[i].usage);
	}

	BPropertyInfo prop_info(prop_list);
	msg->AddFlat("messages", &prop_info);

	return Inherited::GetSupportedSuites(msg);
}
