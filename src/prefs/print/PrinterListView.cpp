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

#include "PrinterListView.h"
#include "pr_server.h"

#include "Messages.h"
#include "Globals.h"

#include <Messenger.h>
#include <Bitmap.h>
#include <String.h>
#include <Alert.h>
#include <Mime.h>

PrinterListView::PrinterListView(BRect frame)
	: Inherited(frame, "printers_list", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL)
{
		// TODO: Fixup situation in which PrintServer is not running
	BMessenger msgr;
	if (GetPrinterServerMessenger(msgr) == B_OK)
	{
		BMessage count(B_COUNT_PROPERTIES);
		count.AddSpecifier("Printers");
	
		BMessage reply;
		msgr.SendMessage(&count, &reply);

		int32 countPrinters;
		if (reply.FindInt32("result", &countPrinters) == B_OK)
		{
			for (int32 printerIdx=0; printerIdx < countPrinters; printerIdx++)
			{
				BMessage getPrinter(B_GET_PROPERTY);
				getPrinter.AddSpecifier("Printer", printerIdx);
				msgr.SendMessage(&getPrinter, &reply);
				
				reply.PrintToStream();
				
				BMessenger thePrinter;
				if (reply.FindMessenger("result", &thePrinter) == B_OK)
					AddItem(new PrinterItem(thePrinter), CountItems());
			}
		}
	}
}

void PrinterListView::AttachedToWindow()
{
	Inherited::AttachedToWindow();

	SetSelectionMessage(new BMessage(MSG_PRINTER_SELECTED));
	SetInvocationMessage(new BMessage(MSG_MKDEF_PRINTER));
	SetTarget(Window());	
}

BBitmap* PrinterItem::sIcon = NULL;
BBitmap* PrinterItem::sSelectedIcon = NULL;

PrinterItem::PrinterItem(const BMessenger& thePrinter)
	: BListItem(0, false),
	fMessenger(thePrinter)
{
	if (sIcon == NULL)
	{
		sIcon = new BBitmap(BRect(0,0,B_LARGE_ICON-1,B_LARGE_ICON-1), B_CMAP8);
		BMimeType type(PSRV_PRINTER_FILETYPE);
		type.GetIcon(sIcon, B_LARGE_ICON);
	}

	if (sSelectedIcon == NULL)
	{
		sSelectedIcon = new BBitmap(BRect(0,0,B_LARGE_ICON-1,B_LARGE_ICON-1), B_CMAP8);
		BMimeType type(PRNT_SIGNATURE_TYPE);
		type.GetIcon(sSelectedIcon, B_LARGE_ICON);
	}

		// Get Name of printer
	GetStringProperty("Name", fName);
	GetStringProperty("Comments", fComments);
	GetStringProperty("TransportAddon", fTransport);
	GetStringProperty("PrinterAddon", fDriverName);
}

void PrinterItem::GetStringProperty(const char* propName, BString& outString)
{
	BMessage script(B_GET_PROPERTY);
	BMessage reply;
	script.AddSpecifier(propName);	
	fMessenger.SendMessage(&script,&reply);
	reply.FindString("result", &outString);
}

void PrinterItem::Update(BView *owner, const BFont *font)
{
	BListItem::Update(owner,font);
	
	font_height height;
	font->GetHeight(&height);
	
	SetHeight( (height.ascent+height.descent+height.leading) * 3.0 +4 );
}

void PrinterItem::Remove(BListView* view)
{
	BMessenger msgr;
	
	if (GetPrinterServerMessenger(msgr) == B_OK)
	{
		BMessage script(B_DELETE_PROPERTY);
		BMessage reply;
	
		script.AddSpecifier("Printer", view->IndexOf(this));	
	
		if (msgr.SendMessage(&script,&reply) == B_OK)
			view->RemoveItem(this);
	}
}

void PrinterItem::DrawItem(BView *owner, BRect /*bounds*/, bool complete)
{
	BListView* list = dynamic_cast<BListView*>(owner);
	if (list)
	{
		font_height height;
		BFont font;
		owner->GetFont(&font);
		font.GetHeight(&height);
		float fntheight = height.ascent+height.descent+height.leading;

		BRect bounds = list->ItemFrame(list->IndexOf(this));
		
		rgb_color color = owner->ViewColor();
		if ( IsSelected() ) 
			color = tint_color(color, B_HIGHLIGHT_BACKGROUND_TINT);
								
		rgb_color oldviewcolor = owner->ViewColor();
		rgb_color oldlowcolor = owner->LowColor();
		rgb_color oldcolor = owner->HighColor();
		owner->SetViewColor( color );
		owner->SetHighColor( color );
		owner->SetLowColor( color );
		owner->FillRect(bounds);
		owner->SetLowColor( oldlowcolor );
		owner->SetHighColor( oldcolor );

		BPoint iconPt = bounds.LeftTop() + BPoint(2,2);
		BPoint namePt = iconPt + BPoint(B_LARGE_ICON+8, fntheight);
		BPoint driverPt = iconPt + BPoint(B_LARGE_ICON+8, fntheight*2);
		BPoint commentPt = iconPt + BPoint(B_LARGE_ICON+8, fntheight*3);
		
		float width = owner->StringWidth("No jobs pending.");
		BPoint pendingPt(bounds.right - width -8, namePt.y);
		BPoint transportPt(bounds.right - width -8, driverPt.y);
		
		drawing_mode mode = owner->DrawingMode();
		owner->SetDrawingMode(B_OP_OVER);
			owner->DrawBitmap(IsActivePrinter() ? sSelectedIcon : sIcon, iconPt);
		
			// left of item
		owner->DrawString(fName.String(), fName.Length(), namePt);
		owner->DrawString(fDriverName.String(), fDriverName.Length(), driverPt);
		owner->DrawString(fComments.String(), fComments.Length(), commentPt);
		
			// right of item
		owner->DrawString("No jobs pending.", 16, pendingPt);
		owner->DrawString(fTransport.String(), fTransport.Length(), transportPt);
		
		owner->SetDrawingMode(mode);

		owner->SetViewColor(oldviewcolor);
	}
}

bool PrinterItem::IsActivePrinter()
{
	bool rc = false;
	BMessenger msgr;

	if (::GetPrinterServerMessenger(msgr) == B_OK)
	{
		BMessage reply, getNameOfActivePrinter(B_GET_PROPERTY);
		getNameOfActivePrinter.AddSpecifier("ActivePrinter");
	
		BString activePrinterName;
		if (msgr.SendMessage(&getNameOfActivePrinter, &reply) == B_OK &&
			reply.FindString("result", &activePrinterName) == B_OK) {
				// Compare name of active printer with name of 'this' printer item
			rc = (fName == activePrinterName);
		}
	}

	return rc;	
}

