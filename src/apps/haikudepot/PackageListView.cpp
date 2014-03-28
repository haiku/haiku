/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, <rene@gollent.com>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageListView.h"

#include <algorithm>
#include <stdio.h>

#include <Autolock.h>
#include <Catalog.h>
#include <ScrollBar.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageListView"


static const char* skPackageStateAvailable = B_TRANSLATE_MARK("Available");
static const char* skPackageStateUninstalled = B_TRANSLATE_MARK("Uninstalled");
static const char* skPackageStateActive = B_TRANSLATE_MARK("Active");
static const char* skPackageStateInactive = B_TRANSLATE_MARK("Inactive");
static const char* skPackageStatePending = B_TRANSLATE_MARK(
	"Pending" B_UTF8_ELLIPSIS);


inline BString
package_state_to_string(PackageInfoRef ref)
{
	switch (ref->State()) {
		case NONE:
			return B_TRANSLATE(skPackageStateAvailable);
		case INSTALLED:
			return B_TRANSLATE(skPackageStateInactive);
		case ACTIVATED:
			return B_TRANSLATE(skPackageStateActive);
		case UNINSTALLED:
			return B_TRANSLATE(skPackageStateUninstalled);
		case DOWNLOADING:
		{
			BString data;
			data.SetToFormat("%3.2f%%", ref->DownloadProgress() * 100.0);
			return data;
		}
		case PENDING:
			return B_TRANSLATE(skPackageStatePending);
	}

	return B_TRANSLATE("Unknown");
}


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


class RatingField : public BField {
public:
								RatingField(float rating);
	virtual						~RatingField();

			void				SetRating(float rating);
			float				Rating() const
									{ return fRating; }
private:
			float				fRating;
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
	virtual	int					CompareFields(BField* field1, BField* field2);
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
								PackageRow(const PackageInfoRef& package,
									PackageListener* listener);
	virtual						~PackageRow();

			const PackageInfoRef& Package() const
									{ return fPackage; }

			void				UpdateTitle();
			void				UpdateState();
			void				UpdateRating();

private:
			PackageInfoRef		fPackage;
			PackageInfoListenerRef fPackageListener;
};


enum {
	MSG_UPDATE_PACKAGE		= 'updp'
};


class PackageListener : public PackageInfoListener {
public:
	PackageListener(PackageListView* view)
		:
		fView(view)
	{
	}

	virtual ~PackageListener()
	{
	}

	virtual void PackageChanged(const PackageInfoEvent& event)
	{
		BMessenger messenger(fView);
		if (!messenger.IsValid())
			return;

		const PackageInfo& package = *event.Package().Get();

		BMessage message(MSG_UPDATE_PACKAGE);
		message.AddString("title", package.Title());
		message.AddUInt32("changes", event.Changes());

		messenger.SendMessage(&message);
	}

private:
	PackageListView*	fView;
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


// #pragma mark - RatingField


RatingField::RatingField(float rating)
	:
	fRating(0.0f)
{
	SetRating(rating);
}


RatingField::~RatingField()
{
}


void
RatingField::SetRating(float rating)
{
	if (rating < 0.0f)
		rating = 0.0f;
	if (rating > 5.0f)
		rating = 5.0f;

	if (rating == fRating)
		return;

	fRating = rating;
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
	RatingField* ratingField = dynamic_cast<RatingField*>(field);

	if (bitmapField != NULL) {
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
		if (bitmap != NULL) {
			parent->SetDrawingMode(B_OP_ALPHA);
			parent->DrawBitmap(bitmap, BPoint(x, y));
			parent->SetDrawingMode(B_OP_OVER);
		}

		// draw the string
		DrawString(bitmapField->ClippedString(), parent, r);

	} else if (stringField != NULL) {

		float width = rect.Width() - (2 * sTextMargin);

		if (width != stringField->Width()) {
			BString truncatedString(stringField->String());

			parent->TruncateString(&truncatedString, fTruncateMode, width + 2);
			stringField->SetClippedString(truncatedString.String());
			stringField->SetWidth(width);
		}

		DrawString(stringField->ClippedString(), parent, rect);

	} else if (ratingField != NULL) {

		const float kDefaultTextMargin = 8;

		float width = rect.Width() - (2 * kDefaultTextMargin);

		BString string = "★★★★★";
		float stringWidth = parent->StringWidth(string);
		bool drawOverlay = true;

		if (width < stringWidth) {
			string.SetToFormat("%.1f", ratingField->Rating());
			drawOverlay = false;
			stringWidth = parent->StringWidth(string);
		}

		switch (Alignment()) {
			default:
			case B_ALIGN_LEFT:
				rect.left += kDefaultTextMargin;
				break;
			case B_ALIGN_CENTER:
				rect.left = rect.left + (width - stringWidth) / 2.0f;
				break;

			case B_ALIGN_RIGHT:
				rect.left = rect.right - (stringWidth + kDefaultTextMargin);
				break;
		}

		rect.left = floorf(rect.left);
		rect.right = rect.left + stringWidth;

		if (drawOverlay)
			parent->SetHighColor(0, 170, 255);

		font_height	fontHeight;
		parent->GetFontHeight(&fontHeight);
		float y = rect.top + (rect.Height()
			- (fontHeight.ascent + fontHeight.descent)) / 2
			+ fontHeight.ascent;

		parent->DrawString(string, BPoint(rect.left, y));

		if (drawOverlay) {
			rect.left = ceilf(rect.left
				+ (ratingField->Rating() / 5.0f) * rect.Width());

			rgb_color color = parent->LowColor();
			color.alpha = 190;
			parent->SetHighColor(color);

			parent->SetDrawingMode(B_OP_ALPHA);
			parent->FillRect(rect, B_SOLID_HIGH);

		}
	}
}


int
PackageColumn::CompareFields(BField* field1, BField* field2)
{
	BStringField* stringField1 = dynamic_cast<BStringField*>(field1);
	BStringField* stringField2 = dynamic_cast<BStringField*>(field2);
	if (stringField1 != NULL && stringField2 != NULL) {
		// TODO: Locale aware string compare... not too important if
		// package names are not translated.
		return strcmp(stringField1->String(), stringField2->String());
	}

	RatingField* ratingField1 = dynamic_cast<RatingField*>(field1);
	RatingField* ratingField2 = dynamic_cast<RatingField*>(field2);
	if (ratingField1 != NULL && ratingField2 != NULL) {
		if (ratingField1->Rating() > ratingField2->Rating())
			return -1;
		else if (ratingField1->Rating() < ratingField2->Rating())
			return 1;
		return 0;
	}

	return Inherited::CompareFields(field1, field2);
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
	return dynamic_cast<const BStringField*>(field) != NULL
		|| dynamic_cast<const RatingField*>(field) != NULL;
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


PackageRow::PackageRow(const PackageInfoRef& packageRef,
		PackageListener* packageListener)
	:
	Inherited(ceilf(be_plain_font->Size() * 1.8f)),
	fPackage(packageRef),
	fPackageListener(packageListener)
{
	if (packageRef.Get() == NULL)
		return;

	PackageInfo& package = *packageRef.Get();

	// Package icon and title
	// NOTE: The icon BBitmap is referenced by the fPackage member.
	UpdateTitle();

	// Rating
	UpdateRating();

	// Description
	SetField(new BStringField(package.ShortDescription()), kDescriptionColumn);

	// Size
	// TODO: Store package size
	SetField(new BStringField("0 KiB"), kSizeColumn);

	// Status
	SetField(new BStringField(package_state_to_string(fPackage)),
		kStatusColumn);

	package.AddListener(fPackageListener);
}


PackageRow::~PackageRow()
{
	if (fPackage.Get() != NULL)
		fPackage->RemoveListener(fPackageListener);
}


void
PackageRow::UpdateTitle()
{
	if (fPackage.Get() == NULL)
		return;

	const BBitmap* icon = NULL;
	if (fPackage->Icon().Get() != NULL)
		icon = fPackage->Icon()->Bitmap(SharedBitmap::SIZE_16);
	SetField(new BBitmapStringField(icon, fPackage->Title()), kTitleColumn);
}


void
PackageRow::UpdateState()
{
	if (fPackage.Get() == NULL)
		return;

	SetField(new BStringField(package_state_to_string(fPackage)),
		kStatusColumn);
}


void
PackageRow::UpdateRating()
{
	if (fPackage.Get() == NULL)
		return;
	RatingSummary summary = fPackage->CalculateRatingSummary();
	SetField(new RatingField(summary.averageRating), kRatingColumn);
}


// #pragma mark - ItemCountView


class PackageListView::ItemCountView : public BView {
public:
	ItemCountView()
		:
		BView("item count view", B_WILL_DRAW),
		fItemCount(0)
	{
		BFont font(be_plain_font);
		font.SetSize(9.0f);
		SetFont(&font);

		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		SetHighColor(tint_color(LowColor(), B_DARKEN_4_TINT));
	}

	virtual BSize MinSize()
	{
		BString label(_GetLabel());
		return BSize(StringWidth(label) + 10, B_H_SCROLL_BAR_HEIGHT);
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual BSize MaxSize()
	{
		return MinSize();
	}

	virtual void Draw(BRect updateRect)
	{
		FillRect(updateRect, B_SOLID_LOW);

		BString label(_GetLabel());

		font_height fontHeight;
		GetFontHeight(&fontHeight);

		BRect bounds(Bounds());
		float width = StringWidth(label);

		BPoint offset;
		offset.x = bounds.left + (bounds.Width() - width) / 2.0f;
		offset.y = bounds.top + (bounds.Height()
			- (fontHeight.ascent + fontHeight.descent)) / 2.0f
			+ fontHeight.ascent;

		DrawString(label, offset);
	}

	void SetItemCount(int32 count)
	{
		if (count == fItemCount)
			return;
		fItemCount = count;
		InvalidateLayout();
		Invalidate();
	}

private:
	BString _GetLabel() const
	{
		BString label;
		if (fItemCount == 1)
			label = B_TRANSLATE("1 item");
		else
			label.SetToFormat(B_TRANSLATE("%ld items"), fItemCount);
		return label;
	}

	int32		fItemCount;
};


// #pragma mark - PackageListView


PackageListView::PackageListView(BLocker* modelLock)
	:
	BColumnListView("package list view", 0, B_FANCY_BORDER, true),
	fModelLock(modelLock),
	fPackageListener(new(std::nothrow) PackageListener(this))
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

	fItemCountView = new ItemCountView();
	AddStatusView(fItemCountView);
}


PackageListView::~PackageListView()
{
	Clear();
	delete fPackageListener;
}


void
PackageListView::AttachedToWindow()
{
	BColumnListView::AttachedToWindow();

	PackageColumn::InitTextMargin(ScrollView());
}


void
PackageListView::AllAttached()
{
	BColumnListView::AllAttached();

	SetSortingEnabled(true);
	SetSortColumn(ColumnAt(0), false, true);
}


void
PackageListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_UPDATE_PACKAGE:
		{
			BString title;
			uint32 changes;
			if (message->FindString("title", &title) != B_OK
				|| message->FindUInt32("changes", &changes) != B_OK) {
				break;
			}

			BAutolock _(fModelLock);
			PackageRow* row = _FindRow(title);
			if (row != NULL) {
				if ((changes & PKG_CHANGED_RATINGS) != 0)
					row->UpdateRating();
				if ((changes & PKG_CHANGED_STATE) != 0)
					row->UpdateState();
				if ((changes & PKG_CHANGED_ICON) != 0)
					row->UpdateTitle();
			}
			break;
		}

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

	PackageRow* selected = dynamic_cast<PackageRow*>(CurrentSelection());
	if (selected != NULL)
		message.AddString("title", selected->Package()->Title());

	Window()->PostMessage(&message);
}


void
PackageListView::AddPackage(const PackageInfoRef& package)
{
	PackageRow* packageRow = _FindRow(package);

	// forget about it if this package is already in the listview
	if (packageRow != NULL)
		return;

	BAutolock _(fModelLock);

	// create the row for this package
	packageRow = new PackageRow(package, fPackageListener);

	// add the row, parent may be NULL (add at top level)
	AddRow(packageRow);

	// make sure the row is initially expanded
	ExpandOrCollapse(packageRow, true);

	fItemCountView->SetItemCount(CountRows());
}


PackageRow*
PackageListView::_FindRow(const PackageInfoRef& package, PackageRow* parent)
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


PackageRow*
PackageListView::_FindRow(const BString& packageTitle, PackageRow* parent)
{
	for (int32 i = CountRows(parent) - 1; i >= 0; i--) {
		PackageRow* row = dynamic_cast<PackageRow*>(RowAt(i, parent));
		if (row != NULL && row->Package().Get() != NULL
			&& row->Package()->Title() == packageTitle) {
			return row;
		}
		if (CountRows(row) > 0) {
			// recurse into child rows
			row = _FindRow(packageTitle, row);
			if (row != NULL)
				return row;
		}
	}

	return NULL;
}

