/*
 * Copyright 2006-2019, Haiku Inc. All rights reserved.
 * Copyright 2014, Dancsó Róbert <dancso.robert@d-rendszer.hu>
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@unet.nl>
 *		James Urquhart
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include "PartitionList.h"

#include <stdio.h>
#include <Catalog.h>
#include <ColumnTypes.h>
#include <DiskDevice.h>
#include <File.h>
#include <IconUtils.h>
#include <Locale.h>
#include <Path.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <driver_settings.h>

#include "EncryptionUtils.h"
#include "Support.h"
#include "MainWindow.h"
#include "icons.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PartitionList"


static const char* kUnavailableString = "";

enum {
	kDeviceColumn,
	kFilesystemColumn,
	kVolumeNameColumn,
	kMountedAtColumn,
	kSizeColumn,
	kFreeSizeColumn,
	kBlockSizeColumn,
	kParametersColumn,
	kPartitionTypeColumn,
};


// #pragma mark - BBitmapStringField


BBitmapStringField::BBitmapStringField(BBitmap* bitmap, const char* string)
	:
	Inherited(string),
	fBitmap(bitmap)
{
}


BBitmapStringField::~BBitmapStringField()
{
	delete fBitmap;
}


void
BBitmapStringField::SetBitmap(BBitmap* bitmap)
{
	delete fBitmap;
	fBitmap = bitmap;
	// TODO: cause a redraw?
}


// #pragma mark - PartitionColumn


float PartitionColumn::sTextMargin = 0.0;


PartitionColumn::PartitionColumn(const char* title, float width, float minWidth,
		float maxWidth, uint32 truncateMode, alignment align)
	:
	Inherited(title, width, minWidth, maxWidth, align),
	fTruncateMode(truncateMode)
{
	SetWantsEvents(true);
}


void
PartitionColumn::DrawField(BField* field, BRect rect, BView* parent)
{
	BBitmapStringField* bitmapField
		= dynamic_cast<BBitmapStringField*>(field);
	BStringField* stringField = dynamic_cast<BStringField*>(field);

	if (bitmapField) {
		const BBitmap* bitmap = bitmapField->Bitmap();

		// figure out the placement
		float x = 0.0;
		BRect r = bitmap ? bitmap->Bounds() : BRect(0, 0, 15, 15);
		float y = rect.top + ((rect.Height() - r.Height()) / 2);
		float width = 0.0;

		switch (Alignment()) {
			default:
			case B_ALIGN_LEFT:
			case B_ALIGN_CENTER:
				x = rect.left + sTextMargin;
				width = rect.right - (x + r.Width()) - (2 * sTextMargin);
				r.Set(x + r.Width(), rect.top, rect.right - width, rect.bottom);
				break;

			case B_ALIGN_RIGHT:
				x = rect.right - sTextMargin - r.Width();
				width = (x - rect.left - (2 * sTextMargin));
				r.Set(rect.left, rect.top, rect.left + width, rect.bottom);
				break;
		}

		if (width != bitmapField->Width()) {
			BString truncatedString(bitmapField->String());
			parent->TruncateString(&truncatedString, fTruncateMode, width + 2);
			bitmapField->SetClippedString(truncatedString.String());
			bitmapField->SetWidth(width);
		}

		// draw the bitmap
		if (bitmap) {
			parent->SetDrawingMode(B_OP_ALPHA);
			parent->DrawBitmap(bitmap, BPoint(x, y));
			parent->SetDrawingMode(B_OP_OVER);
		}

		// draw the string
		DrawString(bitmapField->ClippedString(), parent, r);

	} else if (stringField) {

		float width = rect.Width() - (2 * sTextMargin);

		if (width != stringField->Width()) {
			BString truncatedString(stringField->String());

			parent->TruncateString(&truncatedString, fTruncateMode, width + 2);
			stringField->SetClippedString(truncatedString.String());
			stringField->SetWidth(width);
		}

		DrawString(stringField->ClippedString(), parent, rect);
	}
}


float
PartitionColumn::GetPreferredWidth(BField *_field, BView* parent) const
{
	BBitmapStringField* bitmapField
		= dynamic_cast<BBitmapStringField*>(_field);
	BStringField* stringField = dynamic_cast<BStringField*>(_field);

	float parentWidth = Inherited::GetPreferredWidth(_field, parent);
	float width = 0.0;

	if (bitmapField) {
		const BBitmap* bitmap = bitmapField->Bitmap();
		BFont font;
		parent->GetFont(&font);
		width = font.StringWidth(bitmapField->String()) + 3 * sTextMargin;
		if (bitmap)
			width += bitmap->Bounds().Width();
		else
			width += 16;
	} else if (stringField) {
		BFont font;
		parent->GetFont(&font);
		width = font.StringWidth(stringField->String()) + 2 * sTextMargin;
	}
	return max_c(width, parentWidth);
}


bool
PartitionColumn::AcceptsField(const BField* field) const
{
	return dynamic_cast<const BStringField*>(field) != NULL;
}


void
PartitionColumn::InitTextMargin(BView* parent)
{
	BFont font;
	parent->GetFont(&font);
	sTextMargin = ceilf(font.Size() * 0.8);
}


// #pragma mark - PartitionListRow


static inline void
appendParameter(BString& string, bool append, const char* parameter)
{
	if (!append)
		return;
	if (string.Length() > 0)
		string.Append(", ");
	string.Append(parameter);
}


PartitionListRow::PartitionListRow(BPartition* partition)
	:
	Inherited(),
	fPartitionID(partition->ID()),
	fParentID(partition->Parent() ? partition->Parent()->ID() : -1),
	fOffset(partition->Offset()),
	fSize(partition->Size())
{
	BPath path;
	partition->GetPath(&path);

	// Device icon

	BVolume volume;
	partition->GetVolume(&volume);

	BVolume boot;
	BVolumeRoster().GetBootVolume(&boot);

	bool fBoot;
	bool fReadOnly;
	bool fShared;
	bool fEncrypted = false;

	fBoot = volume == boot;
	fReadOnly = volume.IsReadOnly();
	fShared = volume.IsShared();

	BString parameters;
	appendParameter(parameters, fBoot, B_TRANSLATE("Boot"));
	appendParameter(parameters, partition->Device()->IsFile(),
		B_TRANSLATE("Virtual"));
	appendParameter(parameters, fReadOnly, B_TRANSLATE("Read only"));
	appendParameter(parameters, volume.IsRemovable(), B_TRANSLATE("Removable"));
	appendParameter(parameters, fShared, B_TRANSLATE("Shared"));
	appendParameter(parameters, volume.KnowsMime(), B_TRANSLATE("MIMEs"));
	appendParameter(parameters, volume.KnowsAttr(), B_TRANSLATE("Attributes"));
	appendParameter(parameters, volume.KnowsQuery(), B_TRANSLATE("Queries"));

	const char* encrypter = EncryptionType(path.Path());
	if (encrypter != NULL) {
		appendParameter(parameters, true, encrypter);
		fEncrypted = true;
	}

	BBitmap *icon = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);

	if (fBoot)
		BIconUtils::GetVectorIcon(kLeaf, sizeof(kLeaf), icon);
	else if (fEncrypted)
		BIconUtils::GetVectorIcon(kEncrypted, sizeof(kEncrypted), icon);
	else if (fReadOnly)
		BIconUtils::GetVectorIcon(kReadOnly, sizeof(kReadOnly), icon);
	else if (fShared)
		BIconUtils::GetVectorIcon(kShared, sizeof(kShared), icon);
	else if (partition->Device()->IsFile())
		BIconUtils::GetVectorIcon(kFile, sizeof(kFile), icon);
	else if (partition->IsDevice()) {
		if (partition->GetIcon(icon, B_MINI_ICON) != B_OK) {
			delete icon;
			icon = NULL;
		}
	} else {
		delete icon;
		icon = NULL;
	}

	SetField(new BBitmapStringField(icon, path.Path()), kDeviceColumn);

	// File system & volume name

	BString partitionType(partition->Type());

	if (partition->ContainsFileSystem()) {
		SetField(new BStringField(partition->ContentType()), kFilesystemColumn);
		SetField(new BStringField(partition->ContentName()), kVolumeNameColumn);
	} else if (partition->IsDevice()) {
		SetField(new BStringField(kUnavailableString), kFilesystemColumn);
		if (partition->Name() != NULL && partition->Name()[0])
			SetField(new BStringField(partition->Name()), kVolumeNameColumn);
		else
			SetField(new BStringField(kUnavailableString), kVolumeNameColumn);
	} else if (partition->CountChildren() > 0) {
		SetField(new BStringField(kUnavailableString), kFilesystemColumn);
		SetField(new BStringField(kUnavailableString), kVolumeNameColumn);
	} else {
		if (!partitionType.IsEmpty()) {
			partitionType.Prepend("(");
			partitionType.Append(")");
			SetField(new BStringField(partitionType), kFilesystemColumn);
		} else
			SetField(new BStringField(kUnavailableString), kFilesystemColumn);

		SetField(new BStringField(kUnavailableString), kVolumeNameColumn);
	}

	// Mounted at

	if (partition->IsMounted() && partition->GetMountPoint(&path) == B_OK)
		SetField(new BStringField(path.Path()), kMountedAtColumn);
	else
		SetField(new BStringField(kUnavailableString), kMountedAtColumn);

	// Size

	if (fSize > 0) {
		char size[1024];
		SetField(new BStringField(string_for_size(partition->Size(), size,
			sizeof(size))), kSizeColumn);
	} else
		SetField(new BStringField(kUnavailableString), kSizeColumn);

	if (volume.FreeBytes() > 0) {
		char size[1024];
		SetField(new BStringField(string_for_size(volume.FreeBytes(), size,
			sizeof(size))), kFreeSizeColumn);
	} else
		SetField(new BStringField(kUnavailableString), kFreeSizeColumn);

	char blocksize[16];
	snprintf(blocksize, sizeof(blocksize), "%" B_PRIu32,
		partition->BlockSize());
	SetField(new BStringField(blocksize), kBlockSizeColumn);

	// Additional parameters

	if (partition->Parameters() != NULL) {
		// check parameters
		void* handle = parse_driver_settings_string(partition->Parameters());
		if (handle != NULL) {
			bool active = get_driver_boolean_parameter(handle, "active", false,
				true);
			appendParameter(parameters, active, B_TRANSLATE("Active"));

			delete_driver_settings(handle);
		}
	}

	SetField(new BStringField(parameters), kParametersColumn);

	// Partition type

	if (partitionType.IsEmpty())
		partitionType = partition->ContentType();
	SetField(new BStringField(partitionType), kPartitionTypeColumn);
}


PartitionListRow::PartitionListRow(partition_id parentID, partition_id id,
		off_t offset, off_t size)
	:
	Inherited(),
	fPartitionID(id),
	fParentID(parentID),
	fOffset(offset),
	fSize(size)
{
	// TODO: design icon for spaces on partitions
	SetField(new BBitmapStringField(NULL, "-"), kDeviceColumn);

	SetField(new BStringField(B_TRANSLATE("<empty or unknown>")),
		kFilesystemColumn);
	SetField(new BStringField(kUnavailableString), kVolumeNameColumn);

	SetField(new BStringField(kUnavailableString), kMountedAtColumn);

	char sizeString[1024];
	SetField(new BStringField(string_for_size(size, sizeString,
		sizeof(sizeString))), kSizeColumn);
}


const char*
PartitionListRow::DevicePath()
{
	BBitmapStringField* stringField
		= dynamic_cast<BBitmapStringField*>(GetField(kDeviceColumn));

	if (stringField == NULL)
		return NULL;

	return stringField->String();
}


// #pragma mark - PartitionListView


PartitionListView::PartitionListView(const BRect& frame, uint32 resizeMode)
	: Inherited(frame, "storagelist", resizeMode, 0, B_NO_BORDER, true)
{
	AddColumn(new PartitionColumn(B_TRANSLATE("Device"), 150, 50, 500,
		B_TRUNCATE_MIDDLE), kDeviceColumn);
	AddColumn(new PartitionColumn(B_TRANSLATE("File system"), 100, 50, 500,
		B_TRUNCATE_MIDDLE), kFilesystemColumn);
	AddColumn(new PartitionColumn(B_TRANSLATE("Volume name"), 130, 50, 500,
		B_TRUNCATE_MIDDLE), kVolumeNameColumn);
	AddColumn(new PartitionColumn(B_TRANSLATE("Mounted at"), 100, 50, 500,
		B_TRUNCATE_MIDDLE), kMountedAtColumn);
	AddColumn(new PartitionColumn(B_TRANSLATE("Size"), 80, 50, 500,
		B_TRUNCATE_END, B_ALIGN_RIGHT), kSizeColumn);
	AddColumn(new PartitionColumn(B_TRANSLATE("Free space"), 80, 50, 500,
		B_TRUNCATE_END, B_ALIGN_RIGHT), kFreeSizeColumn);
	AddColumn(new PartitionColumn(B_TRANSLATE("Block size"), 50, 50, 500,
		B_TRUNCATE_END, B_ALIGN_RIGHT), kBlockSizeColumn);
	AddColumn(new PartitionColumn(B_TRANSLATE("Parameters"), 100, 50, 500,
		B_TRUNCATE_END), kParametersColumn);
	AddColumn(new PartitionColumn(B_TRANSLATE("Partition type"), 200, 50, 500,
		B_TRUNCATE_END), kPartitionTypeColumn);

	SetSortingEnabled(false);
}


void
PartitionListView::AttachedToWindow()
{
	Inherited::AttachedToWindow();
	PartitionColumn::InitTextMargin(ScrollView());
}


bool
PartitionListView::InitiateDrag(BPoint rowPoint, bool wasSelected)
{
	PartitionListRow* draggedRow
		= dynamic_cast<PartitionListRow*>(RowAt(rowPoint));
	if (draggedRow == NULL)
		return false;

	const char* draggedPath = draggedRow->DevicePath();
	if (draggedPath == NULL)
		return false;

	BRect draggedRowRect;
	GetRowRect(draggedRow, &draggedRowRect);

	BMessage dragMessage(B_MIME_DATA);
	dragMessage.AddData("text/plain", B_MIME_TYPE, draggedPath,
		strlen(draggedPath));

	DragMessage(&dragMessage, draggedRowRect, NULL);
	return true;
}


void
PartitionListView::ExpandOrCollapse(BRow* row, bool expand)
{
	BColumnListView::ExpandOrCollapse(row, expand);
	Window()->PostMessage(MSG_UPDATE_ZOOM_LIMITS);
}


PartitionListRow*
PartitionListView::FindRow(partition_id id, PartitionListRow* parent)
{
	for (int32 i = 0; i < CountRows(parent); i++) {
		PartitionListRow* item
			= dynamic_cast<PartitionListRow*>(RowAt(i, parent));
		if (item != NULL && item->ID() == id)
			return item;
		if (CountRows(item) > 0) {
			// recurse into child rows
			item = FindRow(id, item);
			if (item)
				return item;
		}
	}

	return NULL;
}


PartitionListRow*
PartitionListView::AddPartition(BPartition* partition)
{
	PartitionListRow* partitionrow = FindRow(partition->ID());

	// forget about it if this partition is already in the listview
	if (partitionrow != NULL)
		return partitionrow;

	// create the row for this partition
	partitionrow = new PartitionListRow(partition);

	// see if this partition has a parent, or should have
	// a parent (add it in this case)
	PartitionListRow* parent = NULL;
	if (partition->Parent() != NULL) {
		// check if it is in the listview
		parent = FindRow(partition->Parent()->ID());
		// If parent of this partition is not yet in the list
		if (parent == NULL) {
			// add it
			parent = AddPartition(partition->Parent());
		}
	}

	// find a proper insertion index based on the on-disk offset
	int32 index = _InsertIndexForOffset(parent, partition->Offset());

	// add the row, parent may be NULL (add at top level)
	AddRow(partitionrow, index, parent);

	// make sure the row is initially expanded
	ExpandOrCollapse(partitionrow, true);

	return partitionrow;
}


PartitionListRow*
PartitionListView::AddSpace(partition_id parentID, partition_id id,
	off_t offset, off_t size)
{
	// the parent should already be in the listview
	PartitionListRow* parent = FindRow(parentID);
	if (!parent)
		return NULL;

	// create the row for this partition
	PartitionListRow* partitionrow = new PartitionListRow(parentID,
		id, offset, size);

	// find a proper insertion index based on the on-disk offset
	int32 index = _InsertIndexForOffset(parent, offset);

	// add the row, parent may be NULL (add at top level)
	AddRow(partitionrow, index, parent);

	// make sure the row is initially expanded
	ExpandOrCollapse(partitionrow, true);

	return partitionrow;
}


BSize
PartitionListView::PreferredSize()
{
	// Remove default size for parameters + partition type column
	BSize size = BColumnListView::PreferredSize();
	size.width -= ColumnAt(kParametersColumn)->Width()
		+ ColumnAt(kPartitionTypeColumn)->Width();
	return size;
}


int32
PartitionListView::_InsertIndexForOffset(PartitionListRow* parent,
	off_t offset) const
{
	int32 index = 0;
	int32 count = CountRows(parent);
	for (; index < count; index++) {
		const PartitionListRow* item
			= dynamic_cast<const PartitionListRow*>(RowAt(index, parent));
		if (item && item->Offset() > offset)
			break;
	}
	return index;
}


