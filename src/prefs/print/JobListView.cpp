/*
 *  Printers Preference Application.
 *  Copyright (C) 2001, 2002 OpenBeOS. All Rights Reserved.
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

#include "JobListView.h"
#include "pr_server.h"

#include "Messages.h"
#include "Globals.h"
#include "Jobs.h"
#include "SpoolFolder.h"

#include <Messenger.h>
#include <Bitmap.h>
#include <String.h>
#include <Alert.h>
#include <Mime.h>
#include <Roster.h>

JobListView::JobListView(BRect frame)
	: Inherited(frame, "jobs_list", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL)
{
}

void JobListView::AttachedToWindow()
{
	Inherited::AttachedToWindow();

	SetSelectionMessage(new BMessage(MSG_JOB_SELECTED));
	SetTarget(Window());	
}


void JobListView::SetSpoolFolder(SpoolFolder* folder) 
{
	BPath path;

		// clear list
	const BListItem** items = Items();
	for (int i = CountItems() - 1; i >= 0; i --) {
		delete items[i];
	}
	MakeEmpty();
	if (folder == NULL) return;
	
		// Find directory containing printer definition nodes
	for (int32 i = 0; i < folder->CountJobs(); i ++) {
		Job* job = folder->JobAt(i);
		AddJob(job);
	}
}

JobItem* JobListView::Find(Job* job)
{
	const int32 n = CountItems();
	for (int32 i = 0; i < n; i ++) {
		JobItem* item = dynamic_cast<JobItem*>(ItemAt(i));
		if (item && item->GetJob() == job) return item;
	}
	return NULL;
}

JobItem* JobListView::SelectedItem() {
	BListItem* item = ItemAt(CurrentSelection());
	return dynamic_cast<JobItem*>(item);
}

void JobListView::AddJob(Job* job) 
{
	JobItem* item = new JobItem(job);
	AddItem(item);
	Invalidate();
}

void JobListView::RemoveJob(Job* job) 
{
	JobItem* item = Find(job);
	if (item) {
		RemoveItem(item);
		delete item;
		Invalidate();
	}
}

void JobListView::UpdateJob(Job* job)
{
	JobItem* item = Find(job);
	if (item) {
		item->Update();
		InvalidateItem(IndexOf(item));
	}
}

void JobListView::RestartJob()
{
	JobItem* item = SelectedItem();
	if (item && item->GetJob()->Status() == kFailed) {
			// setting the state changes the file attribute and
			// we will receive a notification from SpoolFolder
		item->GetJob()->SetStatus(kWaiting);
	}
}

void JobListView::CancelJob()
{
	JobItem* item = SelectedItem();
	if (item && item->GetJob()->Status() != kProcessing) {
		item->GetJob()->SetStatus(kFailed);
		item->GetJob()->Remove();
	}
}


// Implementation of JobItem

JobItem::JobItem(Job* job)
	: BListItem(0, false)
	, fJob(job)
	, fIcon(NULL)
{
	fJob->Acquire();

	Update();
}

JobItem::~JobItem() {
	fJob->Release();
}

void JobItem::Update()
{
	BNode node(&fJob->EntryRef());
	if (node.InitCheck() != B_OK) return;

	node.ReadAttrString(PSRV_SPOOL_ATTR_DESCRIPTION, &fName);

	BString mimeType;
	node.ReadAttrString(PSRV_SPOOL_ATTR_MIMETYPE, &mimeType);
	entry_ref ref;
	if (fIcon == NULL && be_roster->FindApp(mimeType.String(), &ref) == B_OK) {
		fIcon = new BBitmap(BRect(0,0,B_MINI_ICON-1,B_MINI_ICON-1), B_CMAP8);
		BMimeType type(mimeType.String());
		if (type.GetIcon(fIcon, B_MINI_ICON) != B_OK) {
			delete fIcon; fIcon = NULL;
		}
	}

	int32 pages;
	fPages = ""; fSize = ""; fStatus = "";
	
	if (node.ReadAttr(PSRV_SPOOL_ATTR_PAGECOUNT, B_INT32_TYPE, 0, &pages, sizeof(pages)) == sizeof(pages)) {
		fPages << pages;
		if (pages > 1) fPages << " pages.";
		else fPages << " page.";		
	} else {
		fPages = "??? pages.";
	}
	
	off_t size;
	if (node.GetSize(&size) == B_OK) {
		char buffer[80];
		sprintf(buffer, "%.2f KB", size / 1024.0);
		fSize = buffer;
	}
		
	switch (fJob->Status()) {
		case kWaiting: fStatus = "Waiting";
			break;
		case kProcessing: fStatus = "Processing";
			break;
		case kFailed: fStatus = "Failed";
			break;
		case kCompleted: fStatus = "Completed";
			break;
		default: fStatus = "Unkown status";
	}	
}	

void JobItem::Update(BView *owner, const BFont *font)
{
	BListItem::Update(owner, font);
	
	font_height height;
	font->GetHeight(&height);
	
	SetHeight( (height.ascent+height.descent+height.leading) * 2.0 +4 );
}

void JobItem::DrawItem(BView *owner, BRect, bool complete)
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

		BPoint iconPt(bounds.LeftTop() + BPoint(2, 2));
		BPoint leftTop(bounds.LeftTop() + BPoint(12+B_MINI_ICON, 2));
		BPoint namePt(leftTop + BPoint(0, fntheight));
		BPoint statusPt(leftTop + BPoint(0, fntheight*2));
		
		float width = owner->StringWidth(fPages.String());
		BPoint pagePt(bounds.RightTop() + BPoint(-width-32, fntheight));
		width = owner->StringWidth(fSize.String());
		BPoint sizePt(bounds.RightTop() + BPoint(-width-32, fntheight*2));
		
		drawing_mode mode = owner->DrawingMode();
		owner->SetDrawingMode(B_OP_OVER);

		if (fIcon) owner->DrawBitmap(fIcon, iconPt);
		
			// left of item
		owner->DrawString(fName.String(), fName.Length(), namePt);
		owner->DrawString(fStatus.String(), fStatus.Length(), statusPt);
		
			// right of item
		owner->DrawString(fPages.String(), fPages.Length(), pagePt);
		owner->DrawString(fSize.String(), fSize.Length(), sizePt);
		
		owner->SetDrawingMode(mode);

		owner->SetViewColor(oldviewcolor);
	}
}
