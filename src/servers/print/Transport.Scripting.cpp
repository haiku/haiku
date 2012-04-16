/*
 * Copyright 2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 */


#include "Transport.h"

#include <AppDefs.h>
#include <Catalog.h>
#include <Locale.h>
#include <Message.h>
#include <Messenger.h>
#include <PropertyInfo.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Transport Scripting"


static property_info prop_list[] = {
	{ "Name", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		B_TRANSLATE_MARK("Get name of transport") },
	{ "Ports", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		B_TRANSLATE_MARK("Get currently available ports/devices") },
	{ 0 } // terminate list
};


void
Transport::HandleScriptingCommand(BMessage* msg)
{
	status_t rc = B_ERROR;
	BString propName;
	BString result;
	BMessage spec;
	int32 idx;

	if ((rc=msg->GetCurrentSpecifier(&idx,&spec)) == B_OK
		&& (rc=spec.FindString("property",&propName)) == B_OK) {
		switch(msg->what) {
			case B_GET_PROPERTY:
				if (propName == "Name")
					result = Name();
				else if (propName == "Ports") {
					// Need to duplicate messaging code, as our result is a
					// complex bmessage, not a string :(
					BMessage reply(B_REPLY);
					rc = ListAvailablePorts(&reply);
					reply.AddInt32("error", rc);
					msg->SendReply(&reply);
					break;
				} else {
					// If unknown scripting request, let superclas handle it
					Inherited::MessageReceived(msg);
					break;
				}

				BMessage reply(B_REPLY);
				reply.AddString("result", result);
				reply.AddInt32("error", rc);
				msg->SendReply(&reply);
				break;
		}
	} else {
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
Transport::ResolveSpecifier(BMessage* msg, int32 index, BMessage* spec,
	int32 form, const char* prop)
{
	BPropertyInfo prop_info(prop_list);
	BHandler* rc = this;

	int32 idx;
	switch (idx=prop_info.FindMatch(msg,0,spec,form,prop)) {
		case B_ERROR:
			rc = Inherited::ResolveSpecifier(msg,index,spec,form,prop);
			break;
	}

	return rc;
}


status_t
Transport::GetSupportedSuites(BMessage* msg)
{
	msg->AddString("suites", "application/x-vnd.OpenBeOS-transport");

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
