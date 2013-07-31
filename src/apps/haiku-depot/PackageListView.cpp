/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageListView.h"

#include <algorithm>
#include <stdio.h>

#include <Catalog.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageListView"


// A field type displaying both a bitmap and a string so that the
// tree display looks nicer (both text and bitmap are indented)
// TODO: Code-duplication with DriveSetup PartitionList.h
class BBitmapStringField : public BStringField {
	typedef BStringField Inherited;
public:
								BBitmapStringField(const BBitmap* bitmap,
									const char* string);
	virtual						~BBitmapStringField();

			void				SetBitmap(const BBitmap* bitmap);
			const BBitmap*		Bitmap() const
									{ return fBitmap; }

private:
			const BBitmap*		fBitmap;
};


// BColumn for PackageListView which knows how to render
// a BBitmapStringField
// TODO: Code-duplication with DriveSetup PartitionList.h
class PackageColumn : public BTitledColumn {
	typedef BTitledColumn Inherited;
public:
								PackageColumn(const char* title,
									float width, float minWidth,
									float maxWidth, uint32 truncateMode,
									alignment align = B_ALIGN_LEFT);

	virtual	void				DrawField(BField* field, BRect rect,
									BView* parent);
	virtual float				GetPreferredWidth(BField* field,
									BView* parent) const;

	virtual	bool				AcceptsField(const BField* field) const;

	static	void				InitTextMargin(BView* parent);

private:
			uint32				fTruncateMode;
	static	float				sTextMargin;
};


// BRow for the PartitionListView
class PackageRow : public BRow {
	typedef BRow Inherited;
public:
								PackageRow(const PackageInfo& package);

			const PackageInfo&	Package() const
									{ return fPackage; }

private:
			PackageInfo			fPackage;
};


// #pragma mark - BBitmapStringField


// TODO: Code-duplication with DriveSetup PartitionList.cpp
BBitmapStringField::BBitmapStringField(const BBitmap* bitmap,
		const char* string)
	:
	Inherited(string),
	fBitmap(bitmap)
{
}


BBitmapStringField::~BBitmapStringField()
{
}


void
BBitmapStringField::SetBitmap(const BBitmap* bitmap)
{
	fBitmap = bitmap;
	// TODO: cause a redraw?
}


// #pragma mark - PackageColumn


// TODO: Code-duplication with DriveSetup PartitionList.cpp


float PackageColumn::sTextMargin = 0.0;


PackageColumn::PackageColumn(const char* title, float width, float minWidth,
		float maxWidth, uint32 truncateMode, alignment align)
	:
	Inherited(title, width, minWidth, maxWidth, align),
	fTruncateMode(truncateMode)
{
	SetWantsEvents(true);
}


void
PackageColumn::DrawField(BField* field, BRect rect, BView* parent)
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
PackageColumn::GetPreferredWidth(BField *_field, BView* parent) const
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
PackageColumn::AcceptsField(const BField* field) const
{
	return dynamic_cast<const BStringField*>(field) != NULL;
}


void
PackageColumn::InitTextMargin(BView* parent)
{
	BFont font;
	parent->GetFont(&font);
	sTextMargin = ceilf(font.Size() * 0.8);
}


// #pragma mark - PackageRow


enum {
	kTitleColumn,
	kRatingColumn,
	kDescriptionColumn,
	kSizeColumn,
	kStatusColumn,
};


PackageRow::PackageRow(const PackageInfo& package)
	:
	Inherited(),
	fPackage(package)
{
	// Package icon and title
	// NOTE: The icon BBitmap is referenced by the fPackage member.
	const BBitmap* icon = NULL;
	if (package.Icon().Get() != NULL)
		icon = package.Icon()->Bitmap(SharedBitmap::SIZE_16);
	SetField(new BBitmapStringField(icon, package.Title()), kTitleColumn);

	// Rating
	// TODO: Method to compute and cache rating
	SetField(new BStringField("n/a"), kRatingColumn);

	// Description
	SetField(new BStringField(package.ShortDescription()), kDescriptionColumn);

	// Size
	// TODO: Store package size
	SetField(new BStringField("0 KiB"), kSizeColumn);

	// Status
	// TODO: Fetch info about installed/deactivated/unintalled/...
	SetField(new BStringField("n/a"), kStatusColumn);
}


// #pragma mark - PackageListView


PackageListView::PackageListView()
	:
	BColumnListView("package list view", 0, B_FANCY_BORDER, true)
{
	AddColumn(new PackageColumn(B_TRANSLATE("Name"), 150, 50, 300,
		B_TRUNCATE_MIDDLE), kTitleColumn);
	AddColumn(new PackageColumn(B_TRANSLATE("Rating"), 80, 50, 100,
		B_TRUNCATE_MIDDLE), kRatingColumn);
	AddColumn(new PackageColumn(B_TRANSLATE("Description"), 300, 80, 1000,
		B_TRUNCATE_MIDDLE), kDescriptionColumn);
	AddColumn(new PackageColumn(B_TRANSLATE("Size"), 60, 50, 100,
		B_TRUNCATE_END), kSizeColumn);
	AddColumn(new PackageColumn(B_TRANSLATE("Status"), 60, 60, 100,
		B_TRUNCATE_END), kStatusColumn);

	SetSortingEnabled(true);
}


PackageListView::~PackageListView()
{
}


void
PackageListView::AttachedToWindow()
{
	PackageColumn::InitTextMargin(ScrollView());
}


void
PackageListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BColumnListView::MessageReceived(message);
			break;
	}
}


void
PackageListView::SelectionChanged()
{
	BColumnListView::SelectionChanged();
	
	BMessage message(MSG_PACKAGE_SELECTED);
	
	BRow* selected = CurrentSelection();
	int32 index = -1;
	if (selected != NULL)
		index = IndexOf(selected);
	message.AddInt32("index", index);
	
	Window()->PostMessage(&message);
}


void
PackageListView::AddPackage(const PackageInfo& package)
{
	PackageRow* packageRow = _FindRow(package);

	// forget about it if this package is already in the listview
	if (packageRow != NULL)
		return;

	// create the row for this package
	packageRow = new PackageRow(package);

	// add the row, parent may be NULL (add at top level)
	AddRow(packageRow);

	// make sure the row is initially expanded
	ExpandOrCollapse(packageRow, true);
}


PackageRow*
PackageListView::_FindRow(const PackageInfo& package, PackageRow* parent)
{
	for (int32 i = CountRows(parent) - 1; i >= 0; i--) {
		PackageRow* row = dynamic_cast<PackageRow*>(RowAt(i, parent));
		if (row != NULL && row->Package() == package)
			return row;
		if (CountRows(row) > 0) {
			// recurse into child rows
			row = _FindRow(package, row);
			if (row != NULL)
				return row;
		}
	}

	return NULL;
}

