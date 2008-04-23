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


// #pragma mark -- PrinterListView


PrinterListView::PrinterListView(BRect frame)
	: Inherited(frame, "printers_list", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL),
	fFolder(NULL),
	fActivePrinter(NULL)
{
}


PrinterListView::~PrinterListView()
{
	while (!IsEmpty())
		delete RemoveItem(0L);
}


void
PrinterListView::BuildPrinterList()
{
	// clear list
	while (!IsEmpty())
		delete RemoveItem(0L);

	// Find directory containing printer definition nodes
	BPath path;
	if (find_directory(B_USER_PRINTERS_DIRECTORY, &path) != B_OK)
		return;

	BDirectory dir(path.Path());
	if (dir.InitCheck() != B_OK)
		return;

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

	SetSelectionMessage(new BMessage(kMsgPrinterSelected));
	SetInvocationMessage(new BMessage(kMsgMakeDefaultPrinter));
	SetTarget(Window());

	BPath path;
	if (find_directory(B_USER_PRINTERS_DIRECTORY, &path) != B_OK)
		return;

	BDirectory dir(path.Path());
	if (dir.InitCheck() != B_OK) {
		// directory has to exist in order to start watching it
		if (create_directory(path.Path(), 0777) != B_OK)
			return;
		dir.SetTo(path.Path());
	}

	fFolder = new FolderWatcher(Window(), dir, true);
	fFolder->SetListener(this);

	BuildPrinterList();

	// Select active printer
	BString activePrinterName(ActivePrinterName());
	for (int32 i = 0; i < CountItems(); i ++) {
		PrinterItem* item = dynamic_cast<PrinterItem*>(ItemAt(i));
		if (item != NULL && item->Name() == activePrinterName) {
			Select(i);
			fActivePrinter = item;
			break;
		}
	}
}


bool
PrinterListView::QuitRequested()
{
	delete fFolder;
	return true;
}


void
PrinterListView::AddPrinter(BDirectory& printer)
{
	BString state;
	node_ref node;
		// If the entry is a directory
	if (printer.InitCheck() == B_OK
		&& printer.GetNodeRef(&node) == B_OK
		&& FindItem(&node) == NULL
		&& printer.ReadAttrString(PSRV_PRINTER_ATTR_STATE, &state) == B_OK
		&& state == "free") {
			// Check it's Mime type for a spool director
		BNodeInfo info(&printer);
		char buffer[256];

		if (info.GetType(buffer) == B_OK
			&& strcmp(buffer, PSRV_PRINTER_FILETYPE) == 0) {
				// Yes, it is a printer definition node
			AddItem(new PrinterItem(dynamic_cast<PrintersWindow*>(Window()), printer));
		}
	}
}


PrinterItem*
PrinterListView::FindItem(node_ref* node) const
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
		if (item == fActivePrinter)
			fActivePrinter = NULL;

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


PrinterItem*
PrinterListView::SelectedItem() const
{
	return dynamic_cast<PrinterItem*>(ItemAt(CurrentSelection()));
}


PrinterItem*
PrinterListView::ActivePrinter() const
{
	return fActivePrinter;
}


void
PrinterListView::SetActivePrinter(PrinterItem* item)
{
	fActivePrinter = item;
}


// #pragma mark -- PrinterItem


BBitmap* PrinterItem::sIcon = NULL;
BBitmap* PrinterItem::sSelectedIcon = NULL;


PrinterItem::PrinterItem(PrintersWindow* window, const BDirectory& node)
	: BListItem(0, false),
	fFolder(NULL),
	fNode(node)
{
	BRect rect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1);
	if (sIcon == NULL) {
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		sIcon = new BBitmap(rect, B_RGBA32);
#else
		sIcon = new BBitmap(rect, B_CMAP8);
#endif
		BMimeType type(PSRV_PRINTER_FILETYPE);
		type.GetIcon(sIcon, B_LARGE_ICON);
	}

	if (sIcon && sIcon->IsValid() && sSelectedIcon == NULL) {
		BBitmap *checkMark = LoadBitmap("check_mark_icon", 'BBMP');
		sSelectedIcon = new BBitmap(rect, B_RGBA32, true);
		if (checkMark && checkMark->IsValid()
			&& sSelectedIcon && sSelectedIcon->IsValid()) {
			// draw check mark at bottom left over printer icon
			BView *view = new BView(rect, "offscreen", B_FOLLOW_ALL, B_WILL_DRAW);
			float y = rect.Height() - checkMark->Bounds().Height();
			sSelectedIcon->Lock();
			sSelectedIcon->AddChild(view);
			view->DrawBitmap(sIcon);
			view->SetDrawingMode(B_OP_ALPHA);
			view->DrawBitmap(checkMark, BPoint(0, y));
			view->Sync();
			view->RemoveSelf();
			sSelectedIcon->Unlock();
			delete view;
			delete checkMark;
		}
	}

	// Get Name of printer
	GetStringProperty(PSRV_PRINTER_ATTR_PRT_NAME, fName);
	GetStringProperty(PSRV_PRINTER_ATTR_COMMENTS, fComments);
	GetStringProperty(PSRV_PRINTER_ATTR_TRANSPORT, fTransport);
	GetStringProperty(PSRV_PRINTER_ATTR_DRV_NAME, fDriverName);

	BPath path;
	if (find_directory(B_USER_PRINTERS_DIRECTORY, &path) != B_OK)
		return;

	// Setup spool folder
	path.Append(fName.String());
	BDirectory dir(path.Path());
	if (dir.InitCheck() == B_OK) {
		fFolder = new SpoolFolder(window, this, dir);
		UpdatePendingJobs();
	}
}


PrinterItem::~PrinterItem()
{
	delete fFolder;
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

	SetHeight((height.ascent + height.descent + height.leading) * 3.0 + 8.0);
}


bool PrinterItem::Remove(BListView* view)
{
	BMessenger msgr;
	if (GetPrinterServerMessenger(msgr) == B_OK) {
		BMessage script(B_DELETE_PROPERTY);
		script.AddSpecifier("Printer", view->IndexOf(this));

		BMessage reply;
		if (msgr.SendMessage(&script,&reply) == B_OK)
			return true;
	}
	return false;
}


void PrinterItem::DrawItem(BView *owner, BRect /*bounds*/, bool complete)
{
	BListView* list = dynamic_cast<BListView*>(owner);
	if (list == NULL)
		return;

	BFont font;
	owner->GetFont(&font);

	font_height height;
	font.GetHeight(&height);

	float fntheight = height.ascent + height.descent + height.leading;

	BRect bounds = list->ItemFrame(list->IndexOf(this));

	rgb_color color = owner->ViewColor();
	rgb_color oldViewColor = color;
	rgb_color oldLowColor = owner->LowColor();
	rgb_color oldHighColor = owner->HighColor();

	if (IsSelected())
		color = tint_color(color, B_HIGHLIGHT_BACKGROUND_TINT);

	owner->SetViewColor(color);
	owner->SetLowColor(color);
	owner->SetHighColor(color);

	owner->FillRect(bounds);

	owner->SetLowColor(oldLowColor);
	owner->SetHighColor(oldHighColor);

	float x = B_LARGE_ICON + 8.0;
	BPoint iconPt(bounds.LeftTop() + BPoint(2.0, 2.0));
	BPoint namePt(iconPt + BPoint(x, fntheight));
	BPoint driverPt(iconPt + BPoint(x, fntheight * 2.0));
	BPoint defaultPt(iconPt + BPoint(x, fntheight * 3.0));

	float width = owner->StringWidth("No pending jobs.");
	BPoint pendingPt(bounds.right - width - 8.0, namePt.y);
	BPoint transportPt(bounds.right - width - 8.0, driverPt.y);
	BPoint commentPt(bounds.right - width - 8.0, defaultPt.y);


	drawing_mode mode = owner->DrawingMode();
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	owner->SetDrawingMode(B_OP_ALPHA);
#else
	owner->SetDrawingMode(B_OP_OVER);
#endif
	if (IsActivePrinter()) {
		if (sSelectedIcon && sSelectedIcon->IsValid())
			owner->DrawBitmap(sSelectedIcon, iconPt);
		else
			owner->DrawString("Default Printer", defaultPt);
	} else {
		if (sIcon && sIcon->IsValid())
			owner->DrawBitmap(sIcon, iconPt);
	}

	owner->SetDrawingMode(B_OP_OVER);

	// left of item
	owner->DrawString(fName.String(), fName.Length(), namePt);
	owner->DrawString(fDriverName.String(), fDriverName.Length(), driverPt);

	// right of item
	owner->DrawString(fPendingJobs.String(), 16, pendingPt);
	owner->DrawString(fTransport.String(), fTransport.Length(), transportPt);
	owner->DrawString(fComments.String(), fComments.Length(), commentPt);

	owner->SetDrawingMode(mode);
	owner->SetViewColor(oldViewColor);
}


bool
PrinterItem::IsActivePrinter() const
{
	return fName == ActivePrinterName();
}


bool
PrinterItem::HasPendingJobs() const
{
	return fFolder && fFolder->CountJobs() > 0;
}


SpoolFolder*
PrinterItem::Folder() const
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
			fPendingJobs << pendingJobs << " pending jobs.";
			return;
		}
	}
	fPendingJobs = "No pending jobs.";
}
