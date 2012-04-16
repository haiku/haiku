/*
 * Copyright 2007-2010 Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "DiskView.h"

#include <stdio.h>

#include <DiskDeviceVisitor.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <HashMap.h>
#include <LayoutItem.h>
#include <Locale.h>
#include <PartitioningInfo.h>
#include <String.h>

#include "MainWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DiskView"

using BPrivate::HashMap;
using BPrivate::HashKey32;

static const pattern	kStripes		= { { 0xc7, 0x8f, 0x1f, 0x3e,
											  0x7c, 0xf8, 0xf1, 0xe3 } };

static const float		kLayoutInset	= 6;


class PartitionView : public BView {
public:
	PartitionView(const char* name, float weight, off_t offset,
			int32 level, partition_id id)
		:
		BView(name, B_WILL_DRAW | B_SUPPORTS_LAYOUT | B_FULL_UPDATE_ON_RESIZE),
		fID(id),
		fWeight(weight),
		fOffset(offset),
		fLevel(level),
		fSelected(false),
		fMouseOver(false),
		fGroupLayout(new BGroupLayout(B_HORIZONTAL, kLayoutInset))
	{
		SetLayout(fGroupLayout);

		SetViewColor(B_TRANSPARENT_COLOR);
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		base = tint_color(base, B_LIGHTEN_2_TINT);
		base = tint_color(base, 1 + 0.13 * (level - 1));
		SetLowColor(base);
		SetHighColor(tint_color(base, B_DARKEN_1_TINT));

		BFont font;
		GetFont(&font);
		font.SetSize(ceilf(font.Size() * 0.85));
		font.SetRotation(90.0);
		SetFont(&font);

		fGroupLayout->SetInsets(kLayoutInset, kLayoutInset + font.Size(),
			kLayoutInset, kLayoutInset);

		SetExplicitMinSize(BSize(font.Size() + 6, 30));
	}

	virtual void MouseDown(BPoint where)
	{
		BMessage message(MSG_SELECTED_PARTITION_ID);
		message.AddInt32("partition_id", fID);
		Window()->PostMessage(&message);
	}

	virtual void MouseMoved(BPoint where, uint32 transit, const BMessage*)
	{
		uint32 buttons;
		if (Window()->CurrentMessage()->FindInt32("buttons",
				(int32*)&buttons) < B_OK)
			buttons = 0;

		_SetMouseOver(buttons == 0
			&& (transit == B_ENTERED_VIEW || transit == B_INSIDE_VIEW));
	}

	virtual void Draw(BRect updateRect)
	{
		if (fMouseOver) {
			float tint = (B_NO_TINT + B_LIGHTEN_1_TINT) / 2.0;
			SetHighColor(tint_color(HighColor(), tint));
			SetLowColor(tint_color(LowColor(), tint));
		}

		BRect b(Bounds());
		if (fSelected) {
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			StrokeRect(b, B_SOLID_HIGH);
			b.InsetBy(1, 1);
			StrokeRect(b, B_SOLID_HIGH);
			b.InsetBy(1, 1);
		} else if (fLevel > 0) {
			StrokeRect(b, B_SOLID_HIGH);
			b.InsetBy(1, 1);
		}

		FillRect(b, B_SOLID_LOW);

		// prevent the text from moving when border width changes
		if (!fSelected)
			b.InsetBy(1, 1);

		float width;
		BFont font;
		GetFont(&font);

		font_height fh;
		font.GetHeight(&fh);

		// draw the partition label, but only if we have no child partition
		// views
		BPoint textOffset;
		if (CountChildren() > 0) {
			font.SetRotation(0.0);
			SetFont(&font);
			width = b.Width();
			textOffset = b.LeftTop();
			textOffset.x += 3;
			textOffset.y += ceilf(fh.ascent);
		} else {
			width = b.Height();
			textOffset = b.LeftBottom();
			textOffset.x += ceilf(fh.ascent);
		}

		BString name(Name());
		font.TruncateString(&name, B_TRUNCATE_END, width);

		SetHighColor(tint_color(LowColor(), B_DARKEN_4_TINT));
		DrawString(name.String(), textOffset);
	}

	float Weight() const
	{
		return fWeight;
	}

	off_t Offset() const
	{
		return fOffset;
	}

	int32 Level() const
	{
		return fLevel;
	}

	BGroupLayout* GroupLayout() const
	{
		return fGroupLayout;
	}

	void SetSelected(bool selected)
	{
		if (fSelected == selected)
			return;

		fSelected = selected;
		Invalidate();
	}

private:
	void _SetMouseOver(bool mouseOver)
	{
		if (fMouseOver == mouseOver)
			return;
		fMouseOver = mouseOver;
		Invalidate();
	}

private:
	partition_id	fID;
	float			fWeight;
	off_t			fOffset;
	int32			fLevel;
	bool			fSelected;
	bool			fMouseOver;
	BGroupLayout*	fGroupLayout;
};


class DiskView::PartitionLayout : public BDiskDeviceVisitor {
public:
	PartitionLayout(BView* view, SpaceIDMap& spaceIDMap)
		:
		fView(view),
		fViewMap(),
		fSelectedPartition(-1),
		fSpaceIDMap(spaceIDMap)
	{
	}

	virtual bool Visit(BDiskDevice* device)
	{
		PartitionView* view = new PartitionView(B_TRANSLATE("Device"), 1.0,
			device->Offset(), 0, device->ID());
		fViewMap.Put(device->ID(), view);
		fView->GetLayout()->AddView(view);
		_AddSpaces(device, view);
		return false;
	}

	virtual bool Visit(BPartition* partition, int32 level)
	{
		if (!partition->Parent()
			|| !fViewMap.ContainsKey(partition->Parent()->ID()))
			return false;

		// calculate size factor within parent frame
		off_t offset = partition->Offset();
//		off_t parentOffset = partition->Parent()->Offset();
		off_t size = partition->Size();
		off_t parentSize = partition->Parent()->Size();
		double scale = (double)size / parentSize;

		BString name = partition->ContentName();
		if (name.Length() == 0) {
			if (partition->CountChildren() > 0)
				name << partition->Type();
			else {
				char buffer[64];
				snprintf(buffer, 64, B_TRANSLATE("Partition %ld"),
					partition->ID());
				name << buffer;
			}
		}
		partition_id id = partition->ID();
		PartitionView* view = new PartitionView(name.String(), scale, offset,
			level, id);
		view->SetSelected(id == fSelectedPartition);
		PartitionView* parent = fViewMap.Get(partition->Parent()->ID());
		BGroupLayout* layout = parent->GroupLayout();
		layout->AddView(_FindInsertIndex(view, layout), view, scale);

		fViewMap.Put(partition->ID(), view);
		_AddSpaces(partition, view);

		return false;
	}

	void SetSelectedPartition(partition_id id)
	{
		if (fSelectedPartition == id)
			return;

		if (fViewMap.ContainsKey(fSelectedPartition)) {
			PartitionView* view = fViewMap.Get(fSelectedPartition);
			view->SetSelected(false);
		}

		fSelectedPartition = id;

		if (fViewMap.ContainsKey(fSelectedPartition)) {
			PartitionView* view = fViewMap.Get(fSelectedPartition);
			view->SetSelected(true);
		}
	}

	void Unset()
	{
		fViewMap.Clear();
	}

 private:
	void _AddSpaces(BPartition* partition, PartitionView* parentView)
	{
		// add any available space on the partition
		BPartitioningInfo info;
		if (partition->GetPartitioningInfo(&info) >= B_OK) {
			off_t parentSize = partition->Size();
			off_t offset;
			off_t size;
			for (int32 i = 0;
				info.GetPartitionableSpaceAt(i, &offset, &size) >= B_OK;
				i++) {
				// TODO: remove again once Disk Device API is fixed
				if (!is_valid_partitionable_space(size))
					continue;
				//
				double scale = (double)size / parentSize;
				partition_id id
					= fSpaceIDMap.SpaceIDFor(partition->ID(), offset);
				PartitionView* view = new PartitionView(B_TRANSLATE("<empty>"),
					scale, offset, parentView->Level() + 1, id);

				fViewMap.Put(id, view);
				BGroupLayout* layout = parentView->GroupLayout();
				layout->AddView(_FindInsertIndex(view, layout), view, scale);
			}
		}
	}
	int32 _FindInsertIndex(PartitionView* view, BGroupLayout* layout) const
	{
		int32 insertIndex = 0;
		int32 count = layout->CountItems();
		for (int32 i = 0; i < count; i++) {
			BLayoutItem* item = layout->ItemAt(i);
			if (!item)
				break;
			PartitionView* sibling
				= dynamic_cast<PartitionView*>(item->View());
			if (sibling && sibling->Offset() > view->Offset())
				break;
			insertIndex++;
		}
		return insertIndex;
	}

	typedef	HashKey32<partition_id>					PartitionKey;
	typedef HashMap<PartitionKey, PartitionView* >	PartitionViewMap;

	BView*				fView;
	PartitionViewMap	fViewMap;
	partition_id		fSelectedPartition;
	SpaceIDMap&			fSpaceIDMap;
};


// #pragma mark -


DiskView::DiskView(const BRect& frame, uint32 resizeMode,
		SpaceIDMap& spaceIDMap)
	:
	Inherited(frame, "diskview", resizeMode,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fDiskCount(0),
	fDisk(NULL),
	fSpaceIDMap(spaceIDMap),
	fPartitionLayout(new PartitionLayout(this, fSpaceIDMap))
{
	BGroupLayout* layout = new BGroupLayout(B_HORIZONTAL, kLayoutInset);
	SetLayout(layout);

	SetViewColor(B_TRANSPARENT_COLOR);
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetHighColor(tint_color(base, B_DARKEN_2_TINT));
	SetLowColor(tint_color(base, (B_DARKEN_2_TINT + B_DARKEN_1_TINT) / 2));

#ifdef HAIKU_TARGET_PLATFORM_LIBBE_TEST
	PartitionView* view;
	float scale = 1.0;
	view = new PartitionView("Disk", scale, 0, 0, -1);
	layout->AddView(view, scale);

	layout = view->GroupLayout();

	scale = 0.3;
	view = new PartitionView("Primary", scale, 1, 50, -1);
	layout->AddView(view, scale);
	scale = 0.7;
	view = new PartitionView("Extended", scale, 1, 100, -1);
	layout->AddView(view, scale);

	layout = view->GroupLayout();

	scale = 0.2;
	view = new PartitionView("Logical", scale, 2, 200, -1);
	layout->AddView(view, scale);

	scale = 0.5;
	view = new PartitionView("Logical", scale, 2, 250, -1);
	layout->AddView(view, scale);

	scale = 0.005;
	view = new PartitionView("Logical", scale, 2, 290, -1);
	layout->AddView(view, scale);

	scale = 0.295;
	view = new PartitionView("Logical", scale, 2, 420, -1);
	layout->AddView(view, scale);
#endif
}


DiskView::~DiskView()
{
	SetDisk(NULL, -1);
	delete fPartitionLayout;
}


void
DiskView::Draw(BRect updateRect)
{
	BRect bounds(Bounds());

	if (fDisk)
		return;

	FillRect(bounds, kStripes);

	const char* helpfulMessage;
	if (fDiskCount == 0)
		helpfulMessage = B_TRANSLATE("No disk devices have been recognized.");
	else
		helpfulMessage =
			B_TRANSLATE("Select a partition from the list below.");

	float width = StringWidth(helpfulMessage);
	font_height fh;
	GetFontHeight(&fh);
	BRect messageBounds(bounds);
	messageBounds.InsetBy((bounds.Width() - width - fh.ascent * 2) / 2.0,
		(bounds.Height() - (fh.ascent + fh.descent) * 2) / 2.0);

	FillRoundRect(messageBounds, 4, 4, B_SOLID_LOW);
	SetHighColor(tint_color(HighColor(), B_DARKEN_4_TINT));
	BPoint textOffset;
	textOffset.x = messageBounds.left + fh.ascent;
	textOffset.y = (messageBounds.top + messageBounds.bottom
		- fh.ascent - fh.descent) / 2 + fh.ascent;
	DrawString(helpfulMessage, textOffset);
}


void
DiskView::SetDiskCount(int32 count)
{
	fDiskCount = count;
}


void
DiskView::SetDisk(BDiskDevice* disk, partition_id selectedPartition)
{
	if (fDisk != disk) {
		fDisk = disk;
		ForceUpdate();
	}

	fPartitionLayout->SetSelectedPartition(selectedPartition);
}


void
DiskView::ForceUpdate()
{
	while (BView* view = ChildAt(0)) {
		view->RemoveSelf();
		delete view;
	}

	fPartitionLayout->Unset();

	if (fDisk) {
		// we need to prepare the disk for modifications, otherwise
		// we cannot get information about available spaces on the
		// device or any of its child partitions
		// TODO: cancelling modifications here is of course undesired
		// once we hold off the real modifications until an explicit
		// command to write them to disk...
		bool prepared = fDisk->PrepareModifications() == B_OK;
		fDisk->VisitEachDescendant(fPartitionLayout);
		if (prepared)
			fDisk->CancelModifications();
	}

	Invalidate();
}

