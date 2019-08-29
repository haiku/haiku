/*
 * Copyright 2007-2010 Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Dancsó Róbert <dancso.robert@d-rendszer.hu>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "DiskView.h"

#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <DiskDeviceVisitor.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <HashMap.h>
#include <IconUtils.h>
#include <LayoutItem.h>
#include <LayoutUtils.h>
#include <Locale.h>
#include <PartitioningInfo.h>
#include <Path.h>
#include <Resources.h>
#include <String.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "icons.h"
#include "EncryptionUtils.h"
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
			int32 level, partition_id id, BPartition* partition)
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

		SetExplicitMinSize(BSize(font.Size() + 20, 30));

		BVolume volume;
		partition->GetVolume(&volume);

		BVolume boot;
		BVolumeRoster().GetBootVolume(&boot);
		fBoot = volume == boot;
		fReadOnly = volume.IsReadOnly();
		fShared = volume.IsShared();
		fEncrypted = false;

		_ComputeFullName(partition, name);

		fUsed = 100.0 / ((float)volume.Capacity()
				/ (volume.Capacity() - volume.FreeBytes()));
		if (fUsed < 0)
			fUsed = 100.0;

		fIcon = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);

		fBFS = BString(partition->ContentType()) == "Be File System";

		if (fBoot)
			BIconUtils::GetVectorIcon(kLeaf, sizeof(kLeaf), fIcon);
		else if (fEncrypted)
			BIconUtils::GetVectorIcon(kEncrypted, sizeof(kEncrypted), fIcon);
		else if (fReadOnly)
			BIconUtils::GetVectorIcon(kReadOnly, sizeof(kReadOnly), fIcon);
		else if (fShared)
			BIconUtils::GetVectorIcon(kShared, sizeof(kShared), fIcon);
		else if (fVirtual)
			BIconUtils::GetVectorIcon(kFile, sizeof(kFile), fIcon);
		else {
			delete fIcon;
			fIcon = NULL;
		}
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
			if (fReadOnly)
				SetHighColor(make_color(255, 128, 128));
			else if (fBoot || fBFS)
				SetHighColor(make_color(128, 255, 128));
			else
				SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			StrokeRect(b, B_SOLID_HIGH);
			b.InsetBy(1, 1);
			StrokeRect(b, B_SOLID_HIGH);
			b.InsetBy(1, 1);
		} else if (fLevel >= 0) {
			StrokeRect(b, B_SOLID_HIGH);
			b.InsetBy(1, 1);
		}

		if (CountChildren() == 0)
			SetLowColor(make_color(255, 255, 255));
		FillRect(b, B_SOLID_LOW);

		if (fEncrypted && CountChildren() == 0) {
			SetHighColor(make_color(192, 192, 192));
			StrokeRect(b, B_SOLID_HIGH);
			b.InsetBy(1, 1);
			SetLowColor(make_color(224, 224, 0));
			BRect e(BPoint(15, b.top), b.RightBottom());
			e.InsetBy(1, 1);
			FillRect(e, kStripes);
		}

		// prevent the text from moving when border width changes
		if (!fSelected)
			b.InsetBy(1, 1);

		BRect used(b.LeftTop(), BSize(b.Width() / (100.0 / fUsed), b.Height()));

		SetLowColor(make_color(172, 172, 255));

		if (fReadOnly)
			SetLowColor(make_color(255, 172, 172));
		else if (fBoot || fBFS)
			SetLowColor(make_color(190, 255, 190));
		if (CountChildren() == 0)
			FillRect(used, B_SOLID_LOW);

		if (fIcon && CountChildren() == 0) {
			BPoint where = b.RightBottom() - BPoint(fIcon->Bounds().Width(),
				fIcon->Bounds().Height());
			SetDrawingMode(B_OP_OVER);
			DrawBitmap(fIcon, where);
			SetDrawingMode(B_OP_COPY);
		}

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

	bool IsEncrypted()
	{
		return fEncrypted;
	}

private:
	void _SetMouseOver(bool mouseOver)
	{
		if (fMouseOver == mouseOver)
			return;
		fMouseOver = mouseOver;
		Invalidate();
	}

	void _ComputeFullName(BPartition* partition, const char* name)
	{
		BString fullName(name);
		fVirtual = partition->Device()->IsFile();
		if (fVirtual)
			fullName << " (" << B_TRANSLATE("Virtual") << ")";

		while (partition != NULL) {
			BPath path;
			partition->GetPath(&path);

			const char* encrypter = EncryptionType(path.Path());
			if (encrypter != NULL) {
				fullName << " (" << encrypter << ")";
				fEncrypted = true;
				break;
			}
			partition = partition->Parent();
		}

		SetName(fullName);
	}


private:
	partition_id	fID;
	float			fWeight;
	off_t			fOffset;
	int32			fLevel;
	bool			fSelected;
	bool			fMouseOver;
	BGroupLayout*	fGroupLayout;

	bool			fBoot;
	bool			fReadOnly;
	bool			fShared;
	bool			fEncrypted;
	bool			fVirtual;
	float			fUsed;
	bool			fBFS;
	BBitmap*		fIcon;
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
		const char* name;
		if (device->Name() != NULL && device->Name()[0] != '\0')
			name = device->Name();
		else
			name = B_TRANSLATE("Device");

		if (BString(device->ContentName()).Length() > 0)
			name = device->ContentName();

		PartitionView* view = new PartitionView(name, 1.0,
			device->Offset(), 0, device->ID(), device);
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
					partition->ID(), partition);
				name << buffer;
			}
		}
		partition_id id = partition->ID();
		PartitionView* view = new PartitionView(name.String(), scale, offset,
			level, id, partition);
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
				PartitionView* view = new PartitionView(
					B_TRANSLATE("<empty or unknown>"), scale, offset,
					parentView->Level() + 1, id, partition);

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
	SetHighUIColor(B_PANEL_BACKGROUND_COLOR, B_DARKEN_2_TINT);
	SetLowUIColor(B_PANEL_BACKGROUND_COLOR, 1.221f);

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
	rgb_color color = LowColor();
	if (color.Brightness() > 100)
		color = tint_color(color, B_DARKEN_4_TINT);
	else
		color = tint_color(color, B_LIGHTEN_2_TINT);

	SetHighColor(color);
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
	if (count == 1) {
		BMessage message(MSG_SELECTED_PARTITION_ID);
		message.AddInt32("partition_id", 0);
		Window()->PostMessage(&message);
	}
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

