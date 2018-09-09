/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */


#include "JobListView.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>
#include <MimeType.h>
#include <Roster.h>
#include <StringFormat.h>
#include <Window.h>

#include "pr_server.h"
#include "Globals.h"
#include "Jobs.h"
#include "Messages.h"
#include "SpoolFolder.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "JobListView"


// #pragma mark -- JobListView


JobListView::JobListView(BRect frame)
	:
	Inherited(frame, "jobs_list", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE)
{
}


JobListView::~JobListView()
{
	while (!IsEmpty())
		delete RemoveItem((int32)0);
}


void
JobListView::AttachedToWindow()
{
	Inherited::AttachedToWindow();

	SetSelectionMessage(new BMessage(kMsgJobSelected));
	SetTarget(Window());
}


void
JobListView::SetSpoolFolder(SpoolFolder* folder)
{
	// clear list
	while (!IsEmpty())
		delete RemoveItem((int32)0);

	if (folder == NULL)
		return;

	// Find directory containing printer definition nodes
	for (int32 i = 0; i < folder->CountJobs(); i++)
		AddJob(folder->JobAt(i));
}


JobItem*
JobListView::FindJob(Job* job) const
{
	const int32 n = CountItems();
	for (int32 i = 0; i < n; i++) {
		JobItem* item = dynamic_cast<JobItem*>(ItemAt(i));
		if (item && item->GetJob() == job)
			return item;
	}
	return NULL;
}


JobItem*
JobListView::SelectedItem() const
{
	return dynamic_cast<JobItem*>(ItemAt(CurrentSelection()));
}


void
JobListView::AddJob(Job* job)
{
	AddItem(new JobItem(job));
	Invalidate();
}


void
JobListView::RemoveJob(Job* job)
{
	JobItem* item = FindJob(job);
	if (item) {
		RemoveItem(item);
		delete item;
		Invalidate();
	}
}


void
JobListView::UpdateJob(Job* job)
{
	JobItem* item = FindJob(job);
	if (item) {
		item->Update();
		InvalidateItem(IndexOf(item));
	}
}


void
JobListView::RestartJob()
{
	JobItem* item = SelectedItem();
	if (item && item->GetJob()->Status() == kFailed) {
		// setting the state changes the file attribute and
		// we will receive a notification from SpoolFolder
		item->GetJob()->SetStatus(kWaiting);
	}
}


void
JobListView::CancelJob()
{
	JobItem* item = SelectedItem();
	if (item && item->GetJob()->Status() != kProcessing) {
		item->GetJob()->SetStatus(kFailed);
		item->GetJob()->Remove();
	}
}


// #pragma mark -- JobItem


JobItem::JobItem(Job* job)
	:
	BListItem(0, false),
	fJob(job),
	fIcon(NULL)
{
	fJob->Acquire();
	Update();
}


JobItem::~JobItem()
{
	fJob->Release();
	delete fIcon;
}


void
JobItem::Update()
{
	BNode node(&fJob->EntryRef());
	if (node.InitCheck() != B_OK)
		return;

	node.ReadAttrString(PSRV_SPOOL_ATTR_DESCRIPTION, &fName);

	BString mimeType;
	node.ReadAttrString(PSRV_SPOOL_ATTR_MIMETYPE, &mimeType);

	entry_ref ref;
	if (fIcon == NULL && be_roster->FindApp(mimeType.String(), &ref) == B_OK) {
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		font_height fontHeight;
		be_plain_font->GetHeight(&fontHeight);
		float height = (fontHeight.ascent + fontHeight.descent
			+ fontHeight.leading) * 2.0;
		BRect rect(0, 0, height, height);
		fIcon = new BBitmap(rect, B_RGBA32);
#else
		BRect rect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1);
		fIcon = new BBitmap(rect, B_CMAP8);
#endif
		BMimeType type(mimeType.String());
		if (type.GetIcon(fIcon, B_MINI_ICON) != B_OK) {
			delete fIcon;
			fIcon = NULL;
		}
	}

	fPages = "";
	int32 pages;
	static BStringFormat format(B_TRANSLATE("{0, plural, "
		"=-1{??? pages}"
		"=1{# page}"
		"other{# pages}}"));

	if (node.ReadAttr(PSRV_SPOOL_ATTR_PAGECOUNT,
		B_INT32_TYPE, 0, &pages, sizeof(pages)) == sizeof(pages)) {
		format.Format(fPages, pages);
	} else {
		// unknown page count, probably the printer is paginating without
		// software help.
		format.Format(fPages, -1);
	}

	fSize = "";
	off_t size;
	if (node.GetSize(&size) == B_OK) {
		char buffer[80];
		snprintf(buffer, sizeof(buffer), B_TRANSLATE("%.2f KB"),
			size / 1024.0);
		fSize = buffer;
	}

	fStatus = "";
	switch (fJob->Status()) {
		case kWaiting:
			fStatus = B_TRANSLATE("Waiting");
			break;

		case kProcessing:
			fStatus = B_TRANSLATE("Processing");
			break;

		case kFailed:
			fStatus = B_TRANSLATE("Failed");
			break;

		case kCompleted:
			fStatus = B_TRANSLATE("Completed");
			break;

		default:
			fStatus = B_TRANSLATE("Unknown status");
	}
}


void
JobItem::Update(BView *owner, const BFont *font)
{
	BListItem::Update(owner, font);

	font_height height;
	font->GetHeight(&height);

	SetHeight((height.ascent + height.descent + height.leading) * 2.0 + 8.0);
}


void
JobItem::DrawItem(BView *owner, BRect, bool complete)
{
	BListView* list = dynamic_cast<BListView*>(owner);
	if (list) {
		BFont font;
		owner->GetFont(&font);

		font_height height;
		font.GetHeight(&height);
		float fntheight = height.ascent + height.descent + height.leading;

		BRect bounds = list->ItemFrame(list->IndexOf(this));

		rgb_color color = owner->ViewColor();
		rgb_color oldLowColor = owner->LowColor();
		rgb_color oldHighColor = owner->HighColor();

		if (IsSelected())
			color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);

		owner->SetHighColor(color);
		owner->SetLowColor(color);

		owner->FillRect(bounds);

		owner->SetLowColor(oldLowColor);
		owner->SetHighColor(oldHighColor);

		BPoint iconPt(bounds.LeftTop() + BPoint(2.0, 2.0));
		float iconHeight = B_MINI_ICON;
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		if (fIcon)
			iconHeight = fIcon->Bounds().Height();
#endif
		BPoint leftTop(bounds.LeftTop() + BPoint(12.0 + iconHeight, 2.0));
		BPoint namePt(leftTop + BPoint(0.0, fntheight));
		BPoint statusPt(leftTop + BPoint(0.0, fntheight * 2.0));

		float x = owner->StringWidth(fPages.String()) + 32.0;
		BPoint pagePt(bounds.RightTop() + BPoint(-x, fntheight));
		BPoint sizePt(bounds.RightTop() + BPoint(-x, fntheight * 2.0));

		drawing_mode mode = owner->DrawingMode();
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	owner->SetDrawingMode(B_OP_ALPHA);
#else
	owner->SetDrawingMode(B_OP_OVER);
#endif

		if (fIcon)
			owner->DrawBitmap(fIcon, iconPt);

		// left of item
		BString name = fName;
		owner->TruncateString(&name, B_TRUNCATE_MIDDLE, pagePt.x - namePt.x);
		owner->DrawString(name.String(), name.Length(), namePt);
		BString status = fStatus;
		owner->TruncateString(&status, B_TRUNCATE_MIDDLE, sizePt.x - statusPt.x);
		owner->DrawString(status.String(), status.Length(), statusPt);

		// right of item
		owner->DrawString(fPages.String(), fPages.Length(), pagePt);
		owner->DrawString(fSize.String(), fSize.Length(), sizePt);

		owner->SetDrawingMode(mode);
	}
}
