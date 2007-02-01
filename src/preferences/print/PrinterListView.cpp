/*****************************************************************************/
// Printers Preference Application.
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001-2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include "PrinterListView.h"
#include "pr_server.h"

#include "Messages.h"
#include "Globals.h"
#include "PrintersWindow.h"
#include "SpoolFolder.h"

#include <Messenger.h>
#include <Bitmap.h>
#include <String.h>
#include <Alert.h>
#include <Mime.h>
#include <StorageKit.h>

// Implementation of PrinterListView

PrinterListView::PrinterListView(BRect frame)
	: Inherited(frame, "printers_list", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL),
	fFolder(NULL)
{
}


void
PrinterListView::BuildPrinterList()
{	
	// clear list
	const BListItem** items = Items();
	for (int32 i = CountItems()-1; i >= 0; i --)
		delete items[i];
	MakeEmpty();
	
	// Find directory containing printer definition nodes
	BPath path;
	status_t rc = ::find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	if (rc != B_OK) 
		return;
			
	BDirectory dir(path.Path());
	rc = dir.InitCheck();
	if (rc != B_OK) 
		return;
		
	fNode = dir;
	BEntry entry;
	while(dir.GetNextEntry(&entry) == B_OK) {
		BDirectory printer(&entry);
		AddPrinter(printer);
	}
}


void 
PrinterListView::AttachedToWindow()
{
	Inherited::AttachedToWindow();

	SetSelectionMessage(new BMessage(MSG_PRINTER_SELECTED));
	SetInvocationMessage(new BMessage(MSG_MKDEF_PRINTER));
	SetTarget(Window());	

	BPath path;	
	bool hasPrintersDirectory = find_directory(B_USER_PRINTERS_DIRECTORY, &path) == B_OK;
	
	if (hasPrintersDirectory) {
		create_directory(path.Path(), 0777);
			// directory has to exist in order to start watching it
	}

	BuildPrinterList();

	// Select active printer
	for (int32 i = 0; i < CountItems(); i ++) {
		PrinterItem* item = dynamic_cast<PrinterItem*>(ItemAt(i));
		if (item != NULL && item->IsActivePrinter()) {
			Select(i); break;
		}
	}

	if (hasPrintersDirectory) {
		BDirectory dir(path.Path());
		if (dir.InitCheck() == B_OK) {
			fFolder = new FolderWatcher(this->Window(), dir, true);
			fFolder->SetListener(this);
		}
	}	
}


bool 
PrinterListView::QuitRequested() 
{
	delete fFolder; fFolder = NULL;
	return true;
}


void 
PrinterListView::AddPrinter(BDirectory& printer) 
{
	BString state;
	node_ref node;
		// If the entry is a directory
	if (printer.InitCheck() == B_OK && 
		printer.GetNodeRef(&node) == B_OK &&
		FindItem(&node) == NULL &&
		printer.ReadAttrString(PSRV_PRINTER_ATTR_STATE, &state) == B_OK &&
		state == "free") {
			// Check it's Mime type for a spool director
		BNodeInfo info(&printer);
		char buffer[256];
		
		if (info.GetType(buffer) == B_OK &&
			strcmp(buffer, PSRV_PRINTER_FILETYPE) == 0) {
				// Yes, it is a printer definition node
			AddItem(new PrinterItem(dynamic_cast<PrintersWindow*>(Window()), printer));
		}
	}
}


PrinterItem* PrinterListView::FindItem(node_ref* node) 
{
	for (int32 i = CountItems()-1; i >= 0; i --) {
		PrinterItem* item = dynamic_cast<PrinterItem*>(ItemAt(i));
		node_ref ref;
		if (item && item->Node()->GetNodeRef(&ref) == B_OK && ref == *node) {
			return item;
		}
	}
	return NULL;
}


void 
PrinterListView::EntryCreated(node_ref* node, entry_ref* entry) 
{
	BDirectory printer(node);
	AddPrinter(printer);
}


void PrinterListView::EntryRemoved(node_ref* node) 
{
	PrinterItem* item = FindItem(node);
	if (item) {
		RemoveItem(item);
		delete item;
	}
}

void 
PrinterListView::AttributeChanged(node_ref* node) 
{
	BDirectory printer(node);
	AddPrinter(printer);
}

void 
PrinterListView::UpdateItem(PrinterItem* item)
{
	item->UpdatePendingJobs();
	InvalidateItem(IndexOf(item));
}

PrinterItem* PrinterListView::SelectedItem()
{
	BListItem* item = ItemAt(CurrentSelection());
	return dynamic_cast<PrinterItem*>(item);
}


BBitmap* PrinterItem::sIcon = NULL;
BBitmap* PrinterItem::sSelectedIcon = NULL;


PrinterItem::PrinterItem(PrintersWindow* window, const BDirectory& node)
	: BListItem(0, false),
	fFolder(NULL),
	fNode(node)
{
	if (sIcon == NULL) {
		sIcon = new BBitmap(BRect(0,0,B_LARGE_ICON-1,B_LARGE_ICON-1), B_CMAP8);
		BMimeType type(PSRV_PRINTER_FILETYPE);
		type.GetIcon(sIcon, B_LARGE_ICON);
	}

	if (sSelectedIcon == NULL) {
		sSelectedIcon = new BBitmap(BRect(0,0,B_LARGE_ICON-1,B_LARGE_ICON-1), B_CMAP8);
		BMimeType type(PRNT_SIGNATURE_TYPE);
		type.GetIcon(sSelectedIcon, B_LARGE_ICON);
	}

	// Get Name of printer
	GetStringProperty(PSRV_PRINTER_ATTR_PRT_NAME, fName);
	GetStringProperty(PSRV_PRINTER_ATTR_COMMENTS, fComments);
	GetStringProperty(PSRV_PRINTER_ATTR_TRANSPORT, fTransport);
	GetStringProperty(PSRV_PRINTER_ATTR_DRV_NAME, fDriverName);

	BPath path;
	// Setup spool folder
	if (find_directory(B_USER_PRINTERS_DIRECTORY, &path) == B_OK) {
		path.Append(fName.String());
		BDirectory dir(path.Path());
		if (dir.InitCheck() != B_OK) return;
		fFolder = new SpoolFolder(window, this, dir);
	}
	UpdatePendingJobs();
}


PrinterItem::~PrinterItem() 
{
	delete fFolder; fFolder = NULL;
}


void 
PrinterItem::GetStringProperty(const char* propName, BString& outString)
{
	fNode.ReadAttrString(propName, &outString);
}


void 
PrinterItem::Update(BView *owner, const BFont *font)
{
	BListItem::Update(owner,font);
	
	font_height height;
	font->GetHeight(&height);
	
	SetHeight( (height.ascent+height.descent+height.leading) * 3.0 +4 );
}


bool PrinterItem::Remove(BListView* view)
{
	BMessenger msgr;
	
	if (GetPrinterServerMessenger(msgr) == B_OK)
	{
		BMessage script(B_DELETE_PROPERTY);
		BMessage reply;
	
		script.AddSpecifier("Printer", view->IndexOf(this));	
	
		if (msgr.SendMessage(&script,&reply) == B_OK) {
			return true;
		}
	}
	return false;
}


void PrinterItem::DrawItem(BView *owner, BRect /*bounds*/, bool complete)
{
	BListView* list = dynamic_cast<BListView*>(owner);
	if (list == NULL)
		return;

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
	
	float width = owner->StringWidth("No pending jobs.");
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
	owner->DrawString(fPendingJobs.String(), 16, pendingPt);
	owner->DrawString(fTransport.String(), fTransport.Length(), transportPt);
	
	owner->SetDrawingMode(mode);

	owner->SetViewColor(oldviewcolor);
}


bool 
PrinterItem::IsActivePrinter()
{
	BMessenger msgr;
	if (::GetPrinterServerMessenger(msgr) != B_OK)
		return false;
		
	BMessage getNameOfActivePrinter(B_GET_PROPERTY);
	getNameOfActivePrinter.AddSpecifier("ActivePrinter");

	BString activePrinterName;
	BMessage reply;
	return msgr.SendMessage(&getNameOfActivePrinter, &reply) == B_OK 
		&& reply.FindString("result", &activePrinterName) == B_OK
		&& fName == activePrinterName;
}


bool
PrinterItem::HasPendingJobs() 
{
	return fFolder && fFolder->CountJobs() > 0;
}


SpoolFolder* PrinterItem::Folder() 
{
	return fFolder;
}


BDirectory* 
PrinterItem::Node() 
{
	return &fNode;
}


void
PrinterItem::UpdatePendingJobs()
{
	if (fFolder) {
		uint32 pendingJobs = fFolder->CountJobs();
		if (pendingJobs == 1) {
			fPendingJobs = "1 pending job.";
			return;
		} else if (pendingJobs > 1) {
			fPendingJobs << pendingJobs << " pending jobs.h";
			return;
		}
	}
	fPendingJobs = "No pending jobs.";
}
