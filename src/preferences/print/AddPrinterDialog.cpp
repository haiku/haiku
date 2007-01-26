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


status_t
AddPrinterDialog::Start()
{
	new AddPrinterDialog();
	return B_OK;
}


AddPrinterDialog::AddPrinterDialog()
	: Inherited(BRect(78.0, 71.0, 400, 300), "Add Printer",
		B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BuildGUI(0);

	Show();
}


void
AddPrinterDialog::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case B_OK: {
				BMessage m(PSRV_MAKE_PRINTER);
				BMessenger msgr;
				if (GetPrinterServerMessenger(msgr) != B_OK) break;								
				
				BString transport, transportPath;
				if (fPrinterText != "Preview") {
					transport = fTransportText;
					transportPath = fTransportPathText;
				}
				
				m.AddString("driver", fPrinterText.String());			
				m.AddString("transport", transport.String());			
				m.AddString("transport path", transportPath.String());			
				m.AddString("printer name", fNameText.String());
				m.AddString("connection", "Local");
					// request print_server to create printer
				msgr.SendMessage(&m);

				PostMessage(B_QUIT_REQUESTED);
			} 
			break;
		case B_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;
		case MSG_NAME_CHANGED:
				fNameText = fName->Text(); Update();
			break;
		case MSG_PRINTER_SELECTED:
		case MSG_TRANSPORT_SELECTED:
			{
				BString name, path;
				if (msg->FindString("name", &name) != B_OK) {
					name = "";
				}	
				if (msg->what == MSG_PRINTER_SELECTED) {
					fPrinterText = name;
				} else {
					fTransportText = name;
					if (msg->FindString("path", &path) == B_OK) {
							// transport path selected
						fTransportPathText = path;
						void* pointer;
							// mark sub menu
						if (msg->FindPointer("source", &pointer) == B_OK) {
							BMenuItem* item = (BMenuItem*)pointer;
							BMenu* menu = item->Menu();
							int32 index = fTransport->IndexOf(menu);
							item = fTransport->ItemAt(index);
							if (item) item->SetMarked(true);
						}
					} else {
							// transport selected
						fTransportPathText = "";
							// remove mark from item in sub menu of transport sub menu
						for (int32 i = fTransport->CountItems()-1; i >= 0; i --) {
							BMenu* menu = fTransport->SubmenuAt(i);
							if (menu) {
								BMenuItem* item = menu->FindMarked();
								if (item) item->SetMarked(false);								
							}
						}
					}
				}
				Update();
			}
			break;
		
		
		default:
			Inherited::MessageReceived(msg);
	}
}


void
AddPrinterDialog::BuildGUI(int stage)
{
	BRect r, tr;
	float x, w, h;
	BButton * ok;
	BButton * cancel;
	BTextControl * tc;
	BPopUpMenu * pum;
	BMenuField * mf;
	BStringView * sv;
	BBox * bb;

#define H_MARGIN 8
#define V_MARGIN 8
#define LINE_MARGIN	3

#define NAME_LABEL	"Printer Name:"
#define KIND_LABEL	"Printer Type:"
#define PORT_LABEL	"Connected to:"
#define DRIVER_SETTINGS_AREA_TEXT "Driver settings area, if any." 
#define TRANSPORT_SETTINGS_AREA_TEXT "Transport settings area, if any." 

// ------------------------ First of all, create a nice grey backdrop
	BBox * panel = new BBox(Bounds(), "backdrop", B_FOLLOW_ALL,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
						B_PLAIN_BORDER);
	AddChild(panel);
	
	r = panel->Bounds();

	r.InsetBy(H_MARGIN, V_MARGIN);

	// add a "printer name" input field
	tc = new BTextControl(r, "printer_name",
							NAME_LABEL, B_EMPTY_STRING, NULL);
	fName = tc;
	tc->SetAlignment(B_ALIGN_LEFT, B_ALIGN_LEFT);
	panel->AddChild(tc);
	tc->SetModificationMessage(new BMessage(MSG_NAME_CHANGED));
	tc->SetFont(be_bold_font);
	tc->GetPreferredSize(&w, &h);
	tc->SetDivider(be_bold_font->StringWidth(NAME_LABEL "#"));
	tc->ResizeTo(tc->Bounds().Width(), h);

	r.OffsetBy(0, h + 2*V_MARGIN);

	// add a "driver" settings box
	bb = new BBox(r, "driver_box", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
						B_FANCY_BORDER);
	panel->AddChild(bb);

	pum = new BPopUpMenu("<pick one>");
	fPrinter = pum;
	mf = new BMenuField(r, "drivers_list", KIND_LABEL, pum, 
							B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
							B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	mf->SetFont(be_plain_font);
	mf->SetDivider(be_plain_font->StringWidth(NAME_LABEL "#"));
	bb->SetLabel(mf);
	mf->ResizeToPreferred();
	mf->GetPreferredSize(&w, &h);
	FillMenu(pum, "Print", MSG_PRINTER_SELECTED);

	tr = bb->Bounds();
	tr.top += h;
	tr.InsetBy(H_MARGIN, V_MARGIN);
	
	sv = new BStringView(tr, NULL, DRIVER_SETTINGS_AREA_TEXT);
	bb->AddChild(sv);
	sv->ResizeToPreferred();
	
	bb->ResizeTo(bb->Bounds().Width(), 200);
	
	r.OffsetBy(0, bb->Frame().Height() + V_MARGIN);

	// add a "transport" settings box
	bb = new BBox(r, "driver_box", B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
						B_FANCY_BORDER);
	panel->AddChild(bb);

	// add a "connected to" (aka transports list) menu field
	pum = new BPopUpMenu("<pick one>");
	fTransport = pum;
	mf = new BMenuField(r, "transports_list", PORT_LABEL, pum, 
							B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
							B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	mf->SetFont(be_plain_font);
	mf->SetDivider(be_plain_font->StringWidth(PORT_LABEL "#"));
	bb->SetLabel(mf);
	mf->ResizeToPreferred();
	mf->GetPreferredSize(&w, &h);
	FillMenu(pum, "Print/transport", MSG_TRANSPORT_SELECTED);

	tr = bb->Bounds();
	tr.top += h;
	tr.InsetBy(H_MARGIN, V_MARGIN);
	
	sv = new BStringView(tr, NULL, TRANSPORT_SETTINGS_AREA_TEXT);
	bb->AddChild(sv);
	sv->ResizeToPreferred();
	
	bb->ResizeTo(bb->Bounds().Width(), 100);
	
	r.OffsetBy(0, bb->Frame().Height() + V_MARGIN);
	
	// make some space before the buttons row
	r.OffsetBy(0, V_MARGIN);

	// add a "OK" button, and make it default
	ok 	= new BButton(r, NULL, "Add", new BMessage(B_OK), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	fOk = ok;
	ok->MakeDefault(true);
	ok->ResizeToPreferred();
	ok->GetPreferredSize(&w, &h);
	x = r.right - w;
	ok->MoveTo(x, ok->Frame().top);	// put the ok bottom at bottom right corner
	panel->AddChild(ok);
	SetDefaultButton(fOk);

	// add a "Cancel button	
	cancel = new BButton(r, NULL, "Cancel", new BMessage(B_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	cancel->ResizeToPreferred();
	cancel->GetPreferredSize(&w, &h);
	cancel->MoveTo(x - w - H_MARGIN, r.top);	// put cancel button left next the ok button
	panel->AddChild(cancel);

	// Auto resize window
	ResizeTo(ok->Frame().right + H_MARGIN, ok->Frame().bottom + V_MARGIN);
	
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
	// B_USER_ADDONS_DIRECTORY same as common directory
	// TODO: not in Haiku!
};


void
AddPrinterDialog::FillMenu(BMenu* menu, const char* path, uint32 what)
{
	for (uint32 i = 0; i < sizeof(gAddonDirs) / sizeof(directory_which); i ++) {
		BPath addonPath;
		if (find_directory(gAddonDirs[i], &addonPath) != B_OK) continue;
		if (addonPath.Append(path) != B_OK) continue;
		BDirectory dir(addonPath.Path());
		if (dir.InitCheck() != B_OK) continue;
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

			bool addMenuItem = true;
				// some hard coded special cases for transport add-ons
			if (menu == fTransport) {
				const char* transport = path.Leaf();
					// Network not implemented yet!
				if (strcmp(transport, "Network") == 0)
					continue;

				addMenuItem = false;

				if (strcmp(transport, "Serial Port") == 0) {
					AddPortSubMenu(menu, transport, "/dev/ports");					
				} else if (strcmp(transport, "Parallel Port") == 0) {
					AddPortSubMenu(menu, transport, "/dev/parallel");
				} else if (strcmp(transport, "USB Port") == 0) {
					AddPortSubMenu(menu, transport, "/dev/printer/usb");
				} else {
					addMenuItem = true;
				}
			} 

			if (addMenuItem) {
				BMessage* msg = new BMessage(what);
				msg->AddString("name", path.Leaf());
				menu->AddItem(new BMenuItem(path.Leaf(), msg));
			}
		}
	}
}


void
AddPrinterDialog::AddPortSubMenu(BMenu* menu, const char* transport, const char* port)
{
	BEntry entry(port);
	BDirectory dir(&entry);
	if (dir.InitCheck() != B_OK)
		return;

	BMenu* subMenu = NULL;
	BPath path;
	while (dir.GetNextEntry(&entry) == B_OK) {
		if (entry.GetPath(&path) != B_OK)
			continue;

		// lazily create sub menu 
		if (subMenu == NULL) {
			subMenu = new BMenu(transport);
			menu->AddItem(subMenu);
			subMenu->SetRadioMode(true);
			int32 index = menu->IndexOf(subMenu);
			BMenuItem* item = menu->ItemAt(index);
			if (item) item->SetMessage(new BMessage(MSG_TRANSPORT_SELECTED));
		}

		// setup menu item for port
		BMessage* msg = new BMessage(MSG_TRANSPORT_SELECTED);
		msg->AddString("name", transport);
		msg->AddString("path", path.Leaf());
		BMenuItem* item = new BMenuItem(path.Leaf(), msg);
		subMenu->AddItem(item);		
	}
}


void
AddPrinterDialog::Update()
{
	fOk->SetEnabled(fNameText != "" && fPrinterText != "" && 
		(fTransportText != "" || fPrinterText == "Preview"));
	fTransport->SetEnabled(fPrinterText != "" &&fPrinterText != "Preview");	
}

