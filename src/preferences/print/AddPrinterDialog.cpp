/*
 * Copyright 2002-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 *		Philippe Houdoin
 */


#include "AddPrinterDialog.h"
#include "PrinterListView.h"
#include "pr_server.h"
#include "Globals.h"
#include "Messages.h"

#include <Box.h>
#include <Button.h>
#include <TextControl.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Application.h>
#include <StorageKit.h>

#include <stdio.h>

AddPrinterDialog::AddPrinterDialog(BWindow *parent)
	: Inherited(BRect(78.0, 71.0, 400, 300), "Add Printer",
		B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_ZOOMABLE)
	, fPrintersPrefletMessenger(parent)
{
	BuildGUI(0);

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
			AddPrinter(msg);
			PostMessage(B_QUIT_REQUESTED);
			break;
			
		case B_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;
		
		case kNameChangedMsg:
			fNameText = fName->Text(); 
			Update();
			break;
		
		case kPrinterSelectedMsg:
			StorePrinter(msg);
			break;
			
		case kTransportSelectedMsg:
			HandleChangedTransport(msg);
			break;
		
		
		default:
			Inherited::MessageReceived(msg);
	}
}


void
AddPrinterDialog::AddPrinter(BMessage *msg)
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
AddPrinterDialog::StorePrinter(BMessage *msg)
{
	BString name;
	if (msg->FindString("name", &name) != B_OK)
		name = "";
		
	fPrinterText = name;
	Update();
}


void
AddPrinterDialog::HandleChangedTransport(BMessage *msg)
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
		for (int32 i = fTransport->CountItems()-1; i >= 0; i --) {
			BMenu* menu = fTransport->SubmenuAt(i);
			if (menu != NULL) {
				BMenuItem* item = menu->FindMarked();
				if (item != NULL) 
					item->SetMarked(false);								
			}
		}
	}
	Update();
}


void
AddPrinterDialog::BuildGUI(int stage)
{
	float w, h;
		// preferred size of current control

	const int32 kHMargin = 8;
	const int32 kVMargin = 8;

	#define NAME_LABEL	"Printer Name:"
	#define KIND_LABEL	"Printer Type:"
	#define PORT_LABEL	"Connected to:"


	BRect r = Bounds();

	BView *panel = new BView(r, "panel", B_FOLLOW_ALL, 0);
	AddChild(panel);
	panel->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	r.InsetBy(kHMargin, kVMargin);

	// add a "printer name" input field
	fName = new BTextControl(r, "printer_name",
							NAME_LABEL, B_EMPTY_STRING, NULL,
							B_FOLLOW_LEFT_RIGHT);
	fName->SetFont(be_bold_font);
	fName->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	panel->AddChild(fName);
	fName->SetModificationMessage(new BMessage(kNameChangedMsg));
	fName->GetPreferredSize(&w, &h);
	fName->ResizeTo(r.Width(), h);

	r.OffsetBy(0, h + 2 * kVMargin);

	// add a "driver" popup menu field
	fPrinter = new BPopUpMenu("<pick one>");
	BMenuField *printerMenuField = new BMenuField(r, "drivers_list", KIND_LABEL, fPrinter, 
							B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
							B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	printerMenuField->SetFont(be_plain_font);
	printerMenuField->SetAlignment(B_ALIGN_RIGHT);
	panel->AddChild(printerMenuField);
	printerMenuField->GetPreferredSize(&w, &h);
	printerMenuField->ResizeTo(r.Width(), h);
	FillMenu(fPrinter, "Print", kPrinterSelectedMsg);

	r.OffsetBy(0, printerMenuField->Bounds().Height() + kVMargin);

	// add a "connected to" (aka transports list) menu field
	fTransport = new BPopUpMenu("<pick one>");
	BMenuField *transportMenuField = new BMenuField(r, "transports_list", PORT_LABEL, fTransport, 
							B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
							B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	transportMenuField->SetFont(be_plain_font);
	transportMenuField->SetAlignment(B_ALIGN_RIGHT);
	panel->AddChild(transportMenuField);
	transportMenuField->GetPreferredSize(&w, &h);
	transportMenuField->ResizeTo(r.Width(), h);
	FillTransportMenu(fTransport);
		
	r.OffsetBy(0, transportMenuField->Bounds().Height() + kVMargin);
	
	// update dividers
	float divider = be_bold_font->StringWidth(NAME_LABEL "#");
	divider = max_c(divider, be_plain_font->StringWidth(NAME_LABEL "#"));	
	divider = max_c(divider, be_plain_font->StringWidth(PORT_LABEL "#"));

	fName->SetDivider(divider);
	printerMenuField->SetDivider(divider);
	transportMenuField->SetDivider(divider);
	
	// add a "OK" button, and make it default
	fOk = new BButton(r, NULL, "Add", new BMessage(B_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fOk->ResizeToPreferred();
	fOk->GetPreferredSize(&w, &h);
	// put the ok bottom at bottom right corner
	float x = panel->Bounds().right - w - kHMargin;
	float y = panel->Bounds().bottom - h - kVMargin;
	fOk->MoveTo(x, y);
	panel->AddChild(fOk);

	// add a "Cancel button	
	BButton *cancel = new BButton(r, NULL, "Cancel", new BMessage(B_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	cancel->ResizeToPreferred();
	cancel->GetPreferredSize(&w, &h);
	// put cancel button left next the ok button
	x = fOk->Frame().left - w - kVMargin;
	y = fOk->Frame().top;
	cancel->MoveTo(x, y);	
	panel->AddChild(cancel);

	SetDefaultButton(fOk);
	fOk->MakeDefault(true);

	// Auto resize window
	r.bottom = transportMenuField->Frame().bottom + fOk->Bounds().Height() + 2 * kVMargin;
	ResizeTo(r.right, r.bottom);
	
	SetSizeLimits(r.right, 10e5, r.bottom, 10e5);
	
	fName->MakeFocus(true);
	
	Update();
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
	B_BEOS_ADDONS_DIRECTORY,
	B_COMMON_ADDONS_DIRECTORY
	#if HAIKU_COMPATIBLE
	// TODO test 
	// , B_USER_ADDONS_DIRECTORY
	#endif
};

void
AddPrinterDialog::FillMenu(BMenu* menu, const char* path, uint32 what)
{
	for (uint32 i = 0; i < sizeof(gAddonDirs) / sizeof(directory_which); i ++) {
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
			if (strcmp(type, B_APP_MIME_TYPE) != 0)
				continue;

			BPath path;
			if (entry.GetPath(&path) != B_OK)
				continue;

			BMessage* msg = new BMessage(what);
			msg->AddString("name", path.Leaf());
			menu->AddItem(new BMenuItem(path.Leaf(), msg));
		}
	}
}

void AddPrinterDialog::FillTransportMenu(BMenu* menu)
{
	BMessenger msgr;
	if (GetPrinterServerMessenger(msgr) != B_OK)
		return;

	for (long idx=0; ; idx++) {
		BMessage reply, msg(B_GET_PROPERTY);
		msg.AddSpecifier("Transport", idx);
		if (msgr.SendMessage(&msg,&reply) != B_OK)
			break;

		BMessenger transport;
		if (reply.FindMessenger("result", &transport) != B_OK)
			break;

		// Got messenger to transport now
		msg.MakeEmpty();
		msg.what = B_GET_PROPERTY;
		msg.AddSpecifier("Name");
		if (transport.SendMessage(&msg,&reply) != B_OK)
			continue;

		BString transportName;
		if (reply.FindString("result",&transportName) != B_OK)
			continue;

		// Now get ports...
		BString portId, portName;
		status_t err;
		msg.MakeEmpty();
		msg.what = B_GET_PROPERTY;
		msg.AddSpecifier("Ports");
		if (transport.SendMessage(&msg,&reply) != B_OK ||
			reply.FindString("port_id", &portId) != B_OK) {
			// Can't find ports; so just show transport item, no menu
			BMessage* menuMsg = new BMessage(kTransportSelectedMsg);
			menuMsg->AddString("name", transportName);
			menu->AddItem(new BMenuItem(transportName.String(), menuMsg));
			continue;
		}

		// We have at least one port; so create submenu
		BMenu* transportMenu = new BMenu(transportName.String());
		menu->AddItem(transportMenu);
		transportMenu->SetRadioMode(true);
		menu->ItemAt(menu->IndexOf(transportMenu))->
			SetMessage(new BMessage(kTransportSelectedMsg));

		for (int32 pidx=0; reply.FindString("port_id", pidx, &portId) == B_OK; pidx++) {
			reply.FindString("port_name", pidx, &portName);

			if (!portName.Length())
				portName = portId;

			// Create menu item in submenu for port
			BMessage* portMsg = new BMessage(kTransportSelectedMsg);
			portMsg->AddString("name", transportName);
			portMsg->AddString("path", portId);
			transportMenu->AddItem(new BMenuItem(portName.String(), portMsg));
		}
	}
}

void
AddPrinterDialog::Update()
{
	fOk->SetEnabled(fNameText != "" && fPrinterText != "" && 
		(fTransportText != "" || fPrinterText == "Preview"));
	
	fTransport->SetEnabled(fPrinterText != "" && fPrinterText != "Preview");	
}

