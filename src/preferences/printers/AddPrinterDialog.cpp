/*
 * Copyright 2002-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 *		Philippe Houdoin
 */


#include "AddPrinterDialog.h"

#include <stdio.h>

#include <Button.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <GridLayout.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <Layout.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <MimeType.h>
#include <NodeInfo.h>
#include <Path.h>

#include "pr_server.h"
#include "Globals.h"
#include "Messages.h"
#include "PrinterListView.h"
#include "TransportMenu.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddPrinterDialog"


AddPrinterDialog::AddPrinterDialog(BWindow *parent)
	:
	Inherited(BRect(78, 71, 400, 300), B_TRANSLATE("Add printer"),
		B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fPrintersPrefletMessenger(parent)
{
	_BuildGUI(0);

	Show();
}


bool
AddPrinterDialog::QuitRequested()
{
	fPrintersPrefletMessenger.SendMessage(kMsgAddPrinterClosed);
	return Inherited::QuitRequested();
}


void
AddPrinterDialog::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case B_OK:
			_AddPrinter(msg);
			PostMessage(B_QUIT_REQUESTED);
			break;

		case B_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case kNameChangedMsg:
			fNameText = fName->Text();
			_Update();
			break;

		case kPrinterSelectedMsg:
			_StorePrinter(msg);
			break;

		case kTransportSelectedMsg:
			_HandleChangedTransport(msg);
			break;

		default:
			Inherited::MessageReceived(msg);
	}
}


void
AddPrinterDialog::_AddPrinter(BMessage *msg)
{
	BMessage m(PSRV_MAKE_PRINTER);
	BMessenger msgr;
	if (GetPrinterServerMessenger(msgr) != B_OK)
		return;

	BString transport;
	BString transportPath;
	if (fPrinterText != "Preview") {
		// Preview printer does not use transport add-on
		transport = fTransportText;
		transportPath = fTransportPathText;
	}

	m.AddString("driver", fPrinterText.String());
	m.AddString("transport", transport.String());
	m.AddString("transport path", transportPath.String());
	m.AddString("printer name", fNameText.String());
	m.AddString("connection", "Local");
	msgr.SendMessage(&m);
		// request print_server to create printer
}


void
AddPrinterDialog::_StorePrinter(BMessage *msg)
{
	BString name;
	if (msg->FindString("name", &name) != B_OK)
		name = "";

	fPrinterText = name;
	_Update();
}


void
AddPrinterDialog::_HandleChangedTransport(BMessage *msg)
{
	BString name;
	if (msg->FindString("name", &name) != B_OK) {
		name = "";
	}
	fTransportText = name;

	BString path;
	if (msg->FindString("path", &path) == B_OK) {
		// transport path selected
		fTransportPathText = path;

		// mark sub menu
		void* pointer;
		if (msg->FindPointer("source", &pointer) == B_OK) {
			BMenuItem* item = (BMenuItem*)pointer;

			// Update printer name with Transport Path if not filled in
			if (strlen(fName->Text()) == 0)
				fName->SetText(item->Label());

			BMenu* menu = item->Menu();
			int32 index = fTransport->IndexOf(menu);
			item = fTransport->ItemAt(index);
			if (item != NULL)
				item->SetMarked(true);
		}
	} else {
		// transport selected
		fTransportPathText = "";

		// remove mark from item in sub menu of transport sub menu
		for (int32 i = fTransport->CountItems() - 1; i >= 0; i --) {
			BMenu* menu = fTransport->SubmenuAt(i);
			if (menu != NULL) {
				BMenuItem* item = menu->FindMarked();
				if (item != NULL)
					item->SetMarked(false);
			}
		}
	}
	_Update();
}


void
AddPrinterDialog::_BuildGUI(int stage)
{
	// add a "printer name" input field
	fName = new BTextControl("printer_name", B_TRANSLATE("Printer name:"),
		B_EMPTY_STRING, NULL);
	fName->SetFont(be_bold_font);
	fName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fName->SetModificationMessage(new BMessage(kNameChangedMsg));

	// add a "driver" popup menu field
	fPrinter = new BPopUpMenu(B_TRANSLATE("<pick one>"));
	BMenuField *printerMenuField = new BMenuField("drivers_list",
		B_TRANSLATE("Printer type:"), fPrinter);
	printerMenuField->SetAlignment(B_ALIGN_RIGHT);
	_FillMenu(fPrinter, "Print", kPrinterSelectedMsg);

	// add a "connected to" (aka transports list) menu field
	fTransport = new BPopUpMenu(B_TRANSLATE("<pick one>"));
	BMenuField *transportMenuField = new BMenuField("transports_list",
		B_TRANSLATE("Connected to:"), fTransport);
	transportMenuField->SetAlignment(B_ALIGN_RIGHT);
	_FillTransportMenu(fTransport);

	// add a "OK" button
	fOk = new BButton(NULL, B_TRANSLATE("Add"), new BMessage((uint32)B_OK),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);

	// add a "Cancel button
	BButton *cancel = new BButton(NULL, B_TRANSLATE("Cancel"),
		new BMessage(B_CANCEL));

	SetLayout(new BGridLayout());

	AddChild(BGridLayoutBuilder(0, 4)
		.Add(fName->CreateLabelLayoutItem(), 0, 0)
		.Add(fName->CreateTextViewLayoutItem(), 1, 0)
		.Add(printerMenuField->CreateLabelLayoutItem(), 0, 1)
		.Add(printerMenuField->CreateMenuBarLayoutItem(), 1, 1)
		.Add(transportMenuField->CreateLabelLayoutItem(), 0, 2)
		.Add(transportMenuField->CreateMenuBarLayoutItem(), 1, 2)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.AddGlue()
			.Add(cancel)
			.Add(fOk)
			, 0, 3, 2)
		.SetInsets(8, 8, 8, 8)
		);

	SetDefaultButton(fOk);
	fOk->MakeDefault(true);

	fName->MakeFocus(true);

	_Update();
// Stage == 0
// init_icon 64x114  Add a Local or Network Printer
//                   ------------------------------
//                   I would like to add a...
//                              Local Printer
//                              Network Printer
// ------------------------------------------------
//                                Cancel   Continue

// Add local printer:

// Stage == 1
// local_icon        Add a Local Printer
//                   ------------------------------
//                   Printer Name: ________________
//                   Printer Type: pick one
//                   Connected to: pick one
// ------------------------------------------------
//                                Cancel        Add

// This seems to be hard coded into the preferences dialog:
// Don't show Network transport add-on in Printer Type menu.
// If Printer Type == Preview disable Connect to popup menu.
// If Printer Type == Serial Port add a submenu to menu item
//    with names in /dev/ports (if empty remove item from menu)
// If Printer Type == Parallel Port add a submenu to menu item
//    with names in /dev/parallel (if empty remove item from menu)

// Printer Driver Setup

// Dialog Info
// Would you like to make X the default printer?
//                                        No Yes

// Add network printer:

// Dialog Info
// Apple Talk networking isn't currenty enabled. If you
// wish to install a network printer you should enable
// AppleTalk in the Network preferences.
//                               Cancel   Open Network

// Stage == 2

// network_icon      Add a Network Printer
//                   ------------------------------
//                   Printer Name: ________________
//                   Printer Type: pick one
//              AppleTalk Printer: pick one
// ------------------------------------------------
//                                Cancel        Add
}


static directory_which gAddonDirs[] = {
	B_USER_ADDONS_DIRECTORY,
	B_COMMON_ADDONS_DIRECTORY,
	B_SYSTEM_ADDONS_DIRECTORY,
};


void
AddPrinterDialog::_FillMenu(BMenu* menu, const char* path, uint32 what)
{
	for (uint32 i = 0; i < sizeof(gAddonDirs) / sizeof(directory_which); i++) {
		BPath addonPath;
		if (find_directory(gAddonDirs[i], &addonPath) != B_OK)
			continue;

		if (addonPath.Append(path) != B_OK)
			continue;

		BDirectory dir(addonPath.Path());
		if (dir.InitCheck() != B_OK)
			continue;

		BEntry entry;
		while (dir.GetNextEntry(&entry, true) == B_OK) {
			if (!entry.IsFile())
				continue;

			BNode node(&entry);
			if (node.InitCheck() != B_OK)
				continue;

			BNodeInfo info(&node);
			if (info.InitCheck() != B_OK)
				continue;

			char type[B_MIME_TYPE_LENGTH + 1];
			info.GetType(type);
			BMimeType entryType(type);
			// filter non executable entries (like "transport" subfolder...)
			if (entryType == B_APP_MIME_TYPE) {
				BPath transportPath;
				if (entry.GetPath(&transportPath) != B_OK)
					continue;

				BMessage* msg = new BMessage(what);
				msg->AddString("name", transportPath.Leaf());
				menu->AddItem(new BMenuItem(transportPath.Leaf(), msg));
			}
		}
	}
}


void
AddPrinterDialog::_FillTransportMenu(BMenu* menu)
{
	BMessenger msgr;
	if (GetPrinterServerMessenger(msgr) != B_OK)
		return;

	for (long idx = 0; ; idx++) {
		BMessage reply, msg(B_GET_PROPERTY);
		msg.AddSpecifier("Transport", idx);
		if (msgr.SendMessage(&msg, &reply) != B_OK)
			break;

		BMessenger transport;
		if (reply.FindMessenger("result", &transport) != B_OK)
			break;

		// Got messenger to transport now
		msg.MakeEmpty();
		msg.what = B_GET_PROPERTY;
		msg.AddSpecifier("Name");
		if (transport.SendMessage(&msg, &reply) != B_OK)
			continue;

		BString transportName;
		if (reply.FindString("result", &transportName) != B_OK)
			continue;

		// Now get ports...
		BString portId, portName;
		int32 error;
		msg.MakeEmpty();
		msg.what = B_GET_PROPERTY;
		msg.AddSpecifier("Ports");
		if (transport.SendMessage(&msg, &reply) != B_OK ||
			reply.FindInt32("error", &error) != B_OK ||
			error != B_OK) {
			// Transport does not provide list of ports
			BMessage* menuMsg = new BMessage(kTransportSelectedMsg);
			menuMsg->AddString("name", transportName);
			menu->AddItem(new BMenuItem(transportName.String(), menuMsg));
			continue;
		}

		// Create submenu
		BMenu* transportMenu = new TransportMenu(transportName.String(),
			kTransportSelectedMsg, transport, transportName);
		menu->AddItem(transportMenu);
		transportMenu->SetRadioMode(true);
		menu->ItemAt(menu->IndexOf(transportMenu))->
			SetMessage(new BMessage(kTransportSelectedMsg));
	}
}


void
AddPrinterDialog::_Update()
{
	fOk->SetEnabled(fNameText != "" && fPrinterText != ""
		&& (fTransportText != "" || fPrinterText == "Preview"));

	fTransport->SetEnabled(fPrinterText != "Preview");
}

