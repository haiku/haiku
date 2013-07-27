/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageListView.h"

#include <algorithm>
#include <stdio.h>

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageListView"


// #pragma mark - BBitmapStringField


// TODO: Code-duplication with DriveSetup PartitionList.cpp
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


// #pragma mark - PackageListView


enum {
	kNameColumn,
	kRatingColumn,
	kDescriptionColumn,
	kSizeColumn,
	kStatusColumn,
};


PackageListView::PackageListView()
	:
	BColumnListView("package list view", 0, B_FANCY_BORDER, true)
{
	AddColumn(new PackageColumn(B_TRANSLATE("Name"), 150, 50, 500,
		B_TRUNCATE_MIDDLE), kNameColumn);
	AddColumn(new PackageColumn(B_TRANSLATE("Rating"), 100, 50, 500,
		B_TRUNCATE_MIDDLE), kRatingColumn);
	AddColumn(new PackageColumn(B_TRANSLATE("Description"), 130, 50, 500,
		B_TRUNCATE_MIDDLE), kDescriptionColumn);
	AddColumn(new PackageColumn(B_TRANSLATE("Size"), 100, 50, 500,
		B_TRUNCATE_END), kSizeColumn);
	AddColumn(new PackageColumn(B_TRANSLATE("Status"), 100, 50, 500,
		B_TRUNCATE_END), kStatusColumn);
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
