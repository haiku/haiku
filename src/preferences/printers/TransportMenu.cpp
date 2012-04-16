/*
 * Copyright 2002-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 *		Philippe Houdoin
 */
#include "TransportMenu.h"


#include <Catalog.h>
#include <MenuItem.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TransportMenu"


TransportMenu::TransportMenu(const char* title, uint32 what,
	const BMessenger& messenger, const BString& transportName)
	:
	BMenu(title),
	fWhat(what),
	fMessenger(messenger),
	fTransportName(transportName)
{
}


bool
TransportMenu::AddDynamicItem(add_state state)
{
	if (state != B_INITIAL_ADD)
		return false;

	BMenuItem* item = RemoveItem((int32)0);
	while (item != NULL) {
		delete item;
		item = RemoveItem((int32)0);
	}

	BMessage msg;
	msg.MakeEmpty();
	msg.what = B_GET_PROPERTY;
	msg.AddSpecifier("Ports");
	BMessage reply;
	if (fMessenger.SendMessage(&msg, &reply) != B_OK)
		return false;

	BString portId;
	BString portName;
	if (reply.FindString("port_id", &portId) != B_OK) {
		// Show error message in submenu
		BMessage* portMsg = new BMessage(fWhat);
		AddItem(new BMenuItem(
			B_TRANSLATE("No printer found!"), portMsg));
		return false;
	}

	// Add ports to submenu
	for (int32 i = 0; reply.FindString("port_id", i, &portId) == B_OK;
		i++) {
		if (reply.FindString("port_name", i, &portName) != B_OK
			|| !portName.Length())
			portName = portId;

		// Create menu item in submenu for port
		BMessage* portMsg = new BMessage(fWhat);
		portMsg->AddString("name", fTransportName);
		portMsg->AddString("path", portId);
		AddItem(new BMenuItem(portName.String(), portMsg));
	}

	return false;
}
