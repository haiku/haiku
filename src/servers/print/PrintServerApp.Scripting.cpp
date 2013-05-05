/*
 * Copyright 2001-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 */


#include "PrintServerApp.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>
#include <PropertyInfo.h>

#include "Transport.h"
#include "Printer.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrintServerApp Scripting"


static property_info prop_list[] = {
	{ "ActivePrinter", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		B_TRANSLATE_MARK("Retrieve or select the active printer") },
	{ "Printer", { B_GET_PROPERTY }, { B_INDEX_SPECIFIER, B_NAME_SPECIFIER,
		B_REVERSE_INDEX_SPECIFIER },
		B_TRANSLATE_MARK("Retrieve a specific printer") },
	{ "Printer", { B_CREATE_PROPERTY }, { B_DIRECT_SPECIFIER },
		B_TRANSLATE_MARK("Create a new printer") },
	{ "Printer", { B_DELETE_PROPERTY }, { B_INDEX_SPECIFIER, B_NAME_SPECIFIER,
		B_REVERSE_INDEX_SPECIFIER },
		B_TRANSLATE_MARK("Delete a specific printer") },
	{ "Printers", { B_COUNT_PROPERTIES }, { B_DIRECT_SPECIFIER },
		B_TRANSLATE_MARK("Return the number of available printers") },
	{ "Transport", { B_GET_PROPERTY }, { B_INDEX_SPECIFIER, B_NAME_SPECIFIER,
		B_REVERSE_INDEX_SPECIFIER },
		B_TRANSLATE_MARK("Retrieve a specific transport") },
	{ "Transports", { B_COUNT_PROPERTIES }, { B_DIRECT_SPECIFIER },
		B_TRANSLATE_MARK("Return the number of available transports") },
	{ "UseConfigWindow", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		B_TRANSLATE_MARK("Show configuration window") },
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
					reply.AddString("result", fDefaultPrinter
						? fDefaultPrinter->Name() : "");
					reply.AddInt32("error", B_OK);
					msg->SendReply(&reply);
				} else if (propName == "UseConfigWindow") {
					BMessage reply(B_REPLY);
					reply.AddString("result", fUseConfigWindow
						? "true" : "false");
					reply.AddInt32("error", B_OK);
					msg->SendReply(&reply);
				}
				break;

			case B_SET_PROPERTY:
				if (propName == "ActivePrinter") {
					BString newActivePrinter;
					if (msg->FindString("data", &newActivePrinter) == B_OK) {
						BMessage reply(B_REPLY);
						reply.AddInt32("error",
							SelectPrinter(newActivePrinter.String()));
						msg->SendReply(&reply);
					}
				} else if (propName == "UseConfigWindow") {
					bool useConfigWindow;
					if (msg->FindBool("data", &useConfigWindow) == B_OK) {
						fUseConfigWindow = useConfigWindow;
						BMessage reply(B_REPLY);
						reply.AddInt32("error", fUseConfigWindow);
						msg->SendReply(&reply);
					}
				}
				break;

			case B_CREATE_PROPERTY:
				if (propName == "Printer") {
					BString name, driver, transport, config;

					if (msg->FindString("name", &name) == B_OK
						&& msg->FindString("driver", &driver) == B_OK
						&& msg->FindString("transport", &transport) == B_OK
						&& msg->FindString("config", &config) == B_OK) {
						BMessage reply(B_REPLY);
						reply.AddInt32("error", CreatePrinter(name.String(),
							driver.String(), "Local", transport.String(),
							config.String()));
						msg->SendReply(&reply);
					}
				}
				break;

			case B_DELETE_PROPERTY: {
					Printer* printer = GetPrinterFromSpecifier(&spec);
					status_t rc = B_BAD_VALUE;

					if (printer != NULL)
						rc=printer->Remove();

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
				} else if (propName == "Transports") {
					BMessage reply(B_REPLY);
					reply.AddInt32("result", Transport::CountTransports());
					reply.AddInt32("error", B_OK);
					msg->SendReply(&reply);
				}
				break;
		}
	}
}


Printer*
PrintServerApp::GetPrinterFromSpecifier(BMessage* msg)
{
	switch(msg->what) {
		case B_NAME_SPECIFIER:
		{
			BString name;
			if (msg->FindString("name", &name) == B_OK)
				return Printer::Find(name.String());
			break;
		}

		case B_INDEX_SPECIFIER:
		{
			int32 idx;
			if (msg->FindInt32("index", &idx) == B_OK)
				return Printer::At(idx);
			break;
		}

		case B_REVERSE_INDEX_SPECIFIER:
		{
			int32 idx;
			if (msg->FindInt32("index", &idx) == B_OK)
				return Printer::At(Printer::CountPrinters() - idx);
			break;
		}
	}

	return NULL;
}


Transport*
PrintServerApp::GetTransportFromSpecifier(BMessage* msg)
{
	switch(msg->what) {
		case B_NAME_SPECIFIER:
		{
			BString name;
			if (msg->FindString("name", &name) == B_OK)
				return Transport::Find(name);
			break;
		}

		case B_INDEX_SPECIFIER:
		{
			int32 idx;
			if (msg->FindInt32("index", &idx) == B_OK)
				return Transport::At(idx);
			break;
		}

		case B_REVERSE_INDEX_SPECIFIER:
		{
			int32 idx;
			if (msg->FindInt32("index", &idx) == B_OK)
				return Transport::At(Transport::CountTransports() - idx);
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

		case 1:
			// GET Printer [arg]
			if ((rc=GetPrinterFromSpecifier(spec)) == NULL) {
				BMessage reply(B_REPLY);
				reply.AddInt32("error", B_BAD_INDEX);
				msg->SendReply(&reply);
			}
			else
				msg->PopSpecifier();
			break;

		case 5:
			// GET Transport [arg]
			if ((rc=GetTransportFromSpecifier(spec)) == NULL) {
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
