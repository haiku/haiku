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
#include <ListView.h>
#include <ScrollView.h>
#include <Application.h>

status_t AddPrinterDialog::Start() {
	AddPrinterDialog* dialog = new AddPrinterDialog();
	return B_OK;
}

AddPrinterDialog::AddPrinterDialog()
	: Inherited(BRect(78.0, 71.0, 561.0, 409.0), "Add Printer", B_MODAL_WINDOW, B_NOT_H_RESIZABLE)
{
	BuildGUI(0);

	Show();
}


void AddPrinterDialog::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		default:
			Inherited::MessageReceived(msg);
	}
}

void AddPrinterDialog::BuildGUI(int stage)
{
	const float boxInset = 10.0;
	BRect r(Bounds());

// ------------------------ First of all, create a nice grey backdrop
	BBox * backdrop = new BBox(Bounds(), "backdrop", B_FOLLOW_ALL_SIDES,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
						B_PLAIN_BORDER);
	AddChild(backdrop);

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
