/*
 *  Printers Preference Application.
 *  Copyright (C) 2001 OpenBeOS. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "AddPrinterDialog.h"
#include "PrinterListView.h"
#include "pr_server.h"
#include "Globals.h"
#include "Messages.h"

// BeOS API
#include <Box.h>
#include <Button.h>
#include <TextControl.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Application.h>

status_t AddPrinterDialog::Start() {
	AddPrinterDialog* dialog = new AddPrinterDialog();
	return B_OK;
}

AddPrinterDialog::AddPrinterDialog()
	: Inherited(BRect(78.0, 71.0, 400, 300), "Add Printer",
		B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BuildGUI(0);

	Show();
}


void AddPrinterDialog::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case B_OK:
			// TODO: create the new spooler!
			// fallback not to dialog cancelling case...
		case B_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;
			
		default:
			Inherited::MessageReceived(msg);
	}
}

void AddPrinterDialog::BuildGUI(int stage)
{
	BRect r, tr;
	float x, y, w, h;
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
	tc->SetAlignment(B_ALIGN_LEFT, B_ALIGN_LEFT);
	panel->AddChild(tc);
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
	mf = new BMenuField(r, "drivers_list", KIND_LABEL, pum, 
							B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
							B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	mf->SetFont(be_plain_font);
	mf->SetDivider(be_plain_font->StringWidth(NAME_LABEL "#"));
	bb->SetLabel(mf);
	mf->ResizeToPreferred();
	mf->GetPreferredSize(&w, &h);

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
	mf = new BMenuField(r, "transports_list", PORT_LABEL, pum, 
							B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
							B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	mf->SetFont(be_plain_font);
	mf->SetDivider(be_plain_font->StringWidth(PORT_LABEL "#"));
	bb->SetLabel(mf);
	mf->ResizeToPreferred();
	mf->GetPreferredSize(&w, &h);

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
	ok->MakeDefault(true);
	ok->ResizeToPreferred();
	ok->GetPreferredSize(&w, &h);
	x = r.right - w;
	ok->MoveTo(x, ok->Frame().top);	// put the ok bottom at bottom right corner
	panel->AddChild(ok);

	// add a "Cancel button	
	cancel = new BButton(r, NULL, "Cancel", new BMessage(B_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	cancel->ResizeToPreferred();
	cancel->GetPreferredSize(&w, &h);
	cancel->MoveTo(x - w - H_MARGIN, r.top);	// put cancel button left next the ok button
	panel->AddChild(cancel);

	// Auto resize window
	ResizeTo(ok->Frame().right + H_MARGIN, ok->Frame().bottom + V_MARGIN);
	
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
