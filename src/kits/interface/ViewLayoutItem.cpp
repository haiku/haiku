/*
 * Copyright 2010, Haiku, Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ViewLayoutItem.h"

#include <new>

#include <Layout.h>
#include <View.h>
#include <ViewPrivate.h>


namespace {
	const char* const kViewField = "BViewLayoutItem:view";
}


BViewLayoutItem::BViewLayoutItem(BView* view)
	:
	fView(view),
	fAncestorsVisible(true)
{
}


BViewLayoutItem::BViewLayoutItem(BMessage* from)
	:
	BLayoutItem(BUnarchiver::PrepareArchive(from)),
	fView(NULL),
	fAncestorsVisible(true)
{
	BUnarchiver unarchiver(from);
	unarchiver.Finish(unarchiver.FindObject<BView>(kViewField, 0,
		BUnarchiver::B_DONT_ASSUME_OWNERSHIP, fView));
}


BViewLayoutItem::~BViewLayoutItem()
{
}


BSize
BViewLayoutItem::MinSize()
{
	return fView->MinSize();
}


BSize
BViewLayoutItem::MaxSize()
{
	return fView->MaxSize();
}


BSize
BViewLayoutItem::PreferredSize()
{
	return fView->PreferredSize();
}


BAlignment
BViewLayoutItem::Alignment()
{
	return fView->LayoutAlignment();
}


void
BViewLayoutItem::SetExplicitMinSize(BSize size)
{
	fView->SetExplicitMinSize(size);
}


void
BViewLayoutItem::SetExplicitMaxSize(BSize size)
{
	fView->SetExplicitMaxSize(size);
}


void
BViewLayoutItem::SetExplicitPreferredSize(BSize size)
{
	fView->SetExplicitPreferredSize(size);
}


void
BViewLayoutItem::SetExplicitAlignment(BAlignment alignment)
{
	fView->SetExplicitAlignment(alignment);
}


bool
BViewLayoutItem::IsVisible()
{
	int16 showLevel = BView::Private(fView).ShowLevel();
	return showLevel - (fAncestorsVisible ? 0 : 1) <= 0;
}


void
BViewLayoutItem::SetVisible(bool visible)
{
	if (visible != IsVisible()) {
		if (visible)
			fView->Show();
		else
			fView->Hide();
	}
}


BRect
BViewLayoutItem::Frame()
{
	return fView->Frame();
}


void
BViewLayoutItem::SetFrame(BRect frame)
{
	fView->MoveTo(frame.LeftTop());
	fView->ResizeTo(frame.Width(), frame.Height());
}


bool
BViewLayoutItem::HasHeightForWidth()
{
	return fView->HasHeightForWidth();
}


void
BViewLayoutItem::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	fView->GetHeightForWidth(width, min, max, preferred);
}


BView*
BViewLayoutItem::View()
{
	return fView;
}


void
BViewLayoutItem::Relayout(bool immediate)
{
	if (immediate)
		fView->Layout(false);
	else
		fView->Relayout();
}


status_t
BViewLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BLayoutItem::Archive(into, deep);

	return archiver.Finish(err);
}


status_t
BViewLayoutItem::AllArchived(BMessage* into) const
{
	BArchiver archiver(into);
	status_t err = BLayoutItem::AllArchived(into);

	if (err == B_OK) {
		if (archiver.IsArchived(fView))
			err = archiver.AddArchivable(kViewField, fView);
		else
			err = B_NAME_NOT_FOUND;
	}

	return err;
}


status_t
BViewLayoutItem::AllUnarchived(const BMessage* from)
{
	if (!fView)
		return B_ERROR;

	return BLayoutItem::AllUnarchived(from);
}


BArchivable*
BViewLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BViewLayoutItem"))
		return new(std::nothrow) BViewLayoutItem(from);
	return NULL;
}


void
BViewLayoutItem::LayoutInvalidated(bool children)
{
	fView->InvalidateLayout(children);
}


void
BViewLayoutItem::AncestorVisibilityChanged(bool shown)
{
	if (fAncestorsVisible == shown)
		return;

	fAncestorsVisible = shown;
	if (shown)
		fView->Show();
	if (!shown)
		fView->Hide();
}

