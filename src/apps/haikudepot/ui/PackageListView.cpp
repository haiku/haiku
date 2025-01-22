/*
 * Copyright 2018-2025, Andrew Lindesay, <apl@lindesay.co.nz>.
 * Copyright 2017, Julian Harnath, <julian.harnath@rwth-aachen.de>.
 * Copyright 2015, Axel Dörfler, <axeld@pinc-software.de>.
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, <rene@gollent.com>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageListView.h"

#include <algorithm>
#include <stdio.h>

#include <Autolock.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <NumberFormat.h>
#include <ScrollBar.h>
#include <StringForSize.h>
#include <StringFormat.h>
#include <Window.h>
#include <package/hpkg/Strings.h>

#include "LocaleUtils.h"
#include "Logger.h"
#include "PackageUtils.h"
#include "RatingUtils.h"
#include "SharedIcons.h"
#include "WorkStatusView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageListView"


static const char* skPackageStateAvailable = B_TRANSLATE_MARK("Available");
static const char* skPackageStateUninstalled = B_TRANSLATE_MARK("Uninstalled");
static const char* skPackageStateActive = B_TRANSLATE_MARK("Active");
static const char* skPackageStateInactive = B_TRANSLATE_MARK("Inactive");
static const char* skPackageStatePending = B_TRANSLATE_MARK("Pending" B_UTF8_ELLIPSIS);

static const BString sSizeNoValue = B_TRANSLATE_CONTEXT("-", "no package size");


inline BString
package_state_to_string(PackageInfoRef package)
{
	static BNumberFormat numberFormat;

	switch (PackageUtils::State(package)) {
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
			float fraction = PackageUtils::DownloadProgress(package);
			if (numberFormat.FormatPercent(data, fraction) != B_OK) {
				HDERROR("unable to format the percentage");
				data = "???";
			}
			return data;
		}
		case PENDING:
			return B_TRANSLATE(skPackageStatePending);
	}

	return B_TRANSLATE("Unknown");
}


class LazyStringField : public BField {
public:
								LazyStringField();
	virtual						~LazyStringField();

			const char*			String();

			void				SetClippedString(const char* string);
			bool				HasClippedString() const;
			const char*			ClippedString() const;
			void				SetWidth(float);
			float				Width() const;

protected:
	virtual	const BString		CreateString() const = 0;
			void				EvictCache();

private:
			BString				fCachedValue;
			bool				fIsCached;
			float				fWidth;
            BString				fClippedString;
            bool				fHasClippedString;
};


class PackageIconAndTitleField : public BStringField {
	typedef BStringField Inherited;
public:
								PackageIconAndTitleField(
									const char* packageName,
									const char* string,
									bool isActivated,
									bool isNativeDesktop);
	virtual						~PackageIconAndTitleField();

			const BString		PackageName() const
									{ return fPackageName; }

			bool				IsActivated() const
									{ return fIsActivated; }

			bool				IsNativeDesktop() const
									{ return fIsNativeDesktop; }

private:
			const BString		fPackageName;
			const bool			fIsActivated;
			const bool			fIsNativeDesktop;
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


class SizeField : public LazyStringField {
public:
								SizeField(double size);
	virtual						~SizeField();

			void				SetSize(double size);
			double				Size() const
									{ return fSize; }

protected:
	virtual	const BString		CreateString() const;

private:
			double				fSize;
};


class DateField : public LazyStringField {
public:
								DateField(uint64 millisSinceEpoc);
	virtual						~DateField();

			void				SetMillisSinceEpoc(uint64 millisSinceEpoc);
			uint64				MillisSinceEpoc() const
									{ return fMillisSinceEpoc; }

protected:
	virtual	const BString		CreateString() const;

private:
			uint64				fMillisSinceEpoc;
};


// BColumn for PackageListView which knows how to render
// a PackageIconAndTitleField
class PackageColumn : public BTitledColumn {
	typedef BTitledColumn Inherited;
public:
								PackageColumn(Model* model,
									const char* title,
									float width, float minWidth,
									float maxWidth, uint32 truncateMode,
									alignment align = B_ALIGN_LEFT);
	virtual						~PackageColumn();

	virtual	void				DrawField(BField* field, BRect rect,
									BView* parent);
	virtual	int					CompareFields(BField* field1, BField* field2);
	virtual float				GetPreferredWidth(BField* field,
									BView* parent) const;

	virtual	bool				AcceptsField(const BField* field) const;

	static	void				InitTextMargin(BView* parent);

private:
			Model*				fModel;
			uint32				fTruncateMode;
			RatingStarsMetrics*	fRatingsMetrics;
	static	float				sTextMargin;
};


// BRow for the PackageListView
class PackageRow : public BRow {
	typedef BRow Inherited;
public:
								PackageRow(const PackageInfoRef& package);
	virtual						~PackageRow();

			const PackageInfoRef&
								Package() const
									{ return fPackage; }
			void				SetPackage(const PackageInfoRef& value);

			void				UpdateIconAndTitle();
			void				UpdateSummary();
			void				UpdateState();
			void				UpdateRating();
			void				UpdateSize();
			void				UpdateRepository();
			void				UpdateVersion();
			void				UpdateVersionCreateTimestamp();

			PackageRow*&		NextInHash()
									{ return fNextInHash; }

private:
			PackageInfoRef		fPackage;
			PackageRow*			fNextInHash;
				// link for BOpenHashTable
};


// #pragma mark - LazyStringField


LazyStringField::LazyStringField()
	:
	fCachedValue(),
	fIsCached(false),
	fWidth(-1.0f),
	fClippedString(),
	fHasClippedString(false)
{
}


LazyStringField::~LazyStringField()
{
}


const char*
LazyStringField::String()
{
	if (!fIsCached) {
		EvictCache();
		fCachedValue = CreateString();
		fIsCached = true;
	}
	return fCachedValue.String();
}


void
LazyStringField::SetClippedString(const char* string)
{
	fClippedString = string;
	fHasClippedString = true;
}


bool
LazyStringField::HasClippedString() const
{
	return fHasClippedString;
}


const char*
LazyStringField::ClippedString() const
{
	return fClippedString.String();
}


void
LazyStringField::SetWidth(float value)
{
	fWidth = value;
}


float
LazyStringField::Width() const
{
	return fWidth;
}


void
LazyStringField::EvictCache()
{
	fCachedValue = "";
	fIsCached = false;
	fWidth = -1.0f;
	fClippedString = "";
	fHasClippedString = false;
}


// #pragma mark - PackageIconAndTitleField


PackageIconAndTitleField::PackageIconAndTitleField(const char* packageName, const char* string,
	bool isActivated, bool isNativeDesktop)
	:
	Inherited(string),
	fPackageName(packageName),
	fIsActivated(isActivated),
	fIsNativeDesktop(isNativeDesktop)
{
}


PackageIconAndTitleField::~PackageIconAndTitleField()
{
}


// #pragma mark - RatingField


RatingField::RatingField(float rating)
	:
	fRating(RATING_MISSING)
{
	SetRating(rating);
}


RatingField::~RatingField()
{
}


void
RatingField::SetRating(float rating)
{
	fRating = rating;
}


// #pragma mark - SizeField


SizeField::SizeField(double size)
	:
	fSize(-1.0)
{
	SetSize(size);
}


SizeField::~SizeField()
{
}


void
SizeField::SetSize(double size)
{
	if (size < 0.0)
		size = 0.0;

	if (size == fSize)
		return;

	fSize = size;
	EvictCache();
}


const BString
SizeField::CreateString() const
{
	if (fSize == 0.0)
		return sSizeNoValue;

	// TODO; don't keep making the buffer on the stack.
	char buffer[256];
	return string_for_size(fSize, buffer, sizeof(buffer));
}


// #pragma mark - DateField


DateField::DateField(uint64 millisSinceEpoc)
	:
	fMillisSinceEpoc(0)
{
	SetMillisSinceEpoc(millisSinceEpoc);
}


DateField::~DateField()
{
}


void
DateField::SetMillisSinceEpoc(uint64 millisSinceEpoc)
{
	if (millisSinceEpoc == fMillisSinceEpoc)
		return;

	fMillisSinceEpoc = millisSinceEpoc;
	EvictCache();
}


const BString
DateField::CreateString() const
{
	if (fMillisSinceEpoc == 0)
		return B_TRANSLATE_CONTEXT("-", "no package publish");
	return LocaleUtils::TimestampToDateString(fMillisSinceEpoc);
}


// #pragma mark - PackageColumn


// TODO: Code-duplication with DriveSetup PartitionList.cpp


float PackageColumn::sTextMargin = 0.0;


PackageColumn::PackageColumn(Model* model, const char* title, float width, float minWidth,
	float maxWidth, uint32 truncateMode, alignment align)
	:
	Inherited(title, width, minWidth, maxWidth, align),
	fModel(model),
	fTruncateMode(truncateMode)
{
	SetWantsEvents(true);

	BSize ratingStarSize = SharedIcons::IconStarBlue12Scaled()->Bitmap()->Bounds().Size();
	fRatingsMetrics = new RatingStarsMetrics(ratingStarSize);
}


PackageColumn::~PackageColumn()
{
	delete fRatingsMetrics;
}


void
PackageColumn::DrawField(BField* field, BRect rect, BView* parent)
{
	PackageIconAndTitleField* packageIconAndTitleField
		= dynamic_cast<PackageIconAndTitleField*>(field);
	BStringField* stringField = dynamic_cast<BStringField*>(field);
	LazyStringField* lazyStringField = dynamic_cast<LazyStringField*>(field);
	RatingField* ratingField = dynamic_cast<RatingField*>(field);

	if (packageIconAndTitleField != NULL) {

		// TODO (andponlin) factor this out as this method is getting too large.

		BSize iconSize = BControlLook::ComposeIconSize(16);
		BSize trailingIconSize = BControlLook::ComposeIconSize(8);
		float trailingIconPaddingFactor = 0.2f;
		BRect iconRect;
		BRect titleRect;
		float titleTextWidth = 0.0f;
		float textMargin = 8.0f; // copied from ColumnTypes.cpp

		std::vector<BitmapHolderRef> trailingIconBitmaps;

		if (packageIconAndTitleField->IsActivated())
			trailingIconBitmaps.push_back(SharedIcons::IconInstalled16Scaled());

		if (packageIconAndTitleField->IsNativeDesktop())
			trailingIconBitmaps.push_back(SharedIcons::IconNative16Scaled());

		// If there is not enough space then drop the "activated" indicator in order to make more
		// room for the title.

		float trailingIconsWidth = static_cast<float>(trailingIconBitmaps.size())
			* (trailingIconSize.Width() * (1.0 + trailingIconPaddingFactor));

		if (!trailingIconBitmaps.empty()) {
			static float sMinimalTextPart = -1.0;

			if (sMinimalTextPart < 0.0)
				sMinimalTextPart = parent->StringWidth("M") * 5.0;

			float minimalWidth
				= iconSize.Width() + trailingIconsWidth + sTextMargin + sMinimalTextPart;

			if (rect.Width() <= minimalWidth)
				trailingIconBitmaps.clear();
		}

		// Calculate the location of the icon.

		iconRect = BRect(BPoint(rect.left + sTextMargin,
							 rect.top + ((rect.Height() - iconSize.Height()) / 2) - 1),
			iconSize);

		// Calculate the location of the title text.

		titleRect = rect;
		titleRect.left = iconRect.right;
		titleRect.right -= trailingIconsWidth;

		// Figure out if the text needs to be truncated.

		float textWidth = titleRect.Width() - (2.0 * textMargin);
		BString truncatedString(packageIconAndTitleField->String());
		parent->TruncateString(&truncatedString, fTruncateMode, textWidth);
		packageIconAndTitleField->SetClippedString(truncatedString.String());
		titleTextWidth = parent->StringWidth(truncatedString);

		// Draw the icon.

		PackageIconRepositoryRef iconRepository = fModel->IconRepository();
		status_t bitmapResult = B_OK;
		BitmapHolderRef bitmapHolderRef;

		if (iconRepository.IsSet()) {
			BString packageName = packageIconAndTitleField->PackageName();
			bitmapResult
				= iconRepository->GetIcon(packageName, iconSize.Width() + 1, bitmapHolderRef);
		} else {
			bitmapResult = B_NOT_INITIALIZED;
		}

		if (bitmapResult == B_OK) {
			if (bitmapHolderRef.IsSet()) {
				const BBitmap* bitmap = bitmapHolderRef->Bitmap();
				if (bitmap != NULL && bitmap->IsValid()) {
					parent->SetDrawingMode(B_OP_ALPHA);
					parent->DrawBitmap(bitmap, bitmap->Bounds(), iconRect,
						B_FILTER_BITMAP_BILINEAR);
					parent->SetDrawingMode(B_OP_OVER);
				}
			}
		}

		// Draw the title.

		DrawString(packageIconAndTitleField->ClippedString(), parent, titleRect);

		// Draw the trailing icons

		if (!trailingIconBitmaps.empty()) {

			BRect trailingIconRect(
				BPoint(titleRect.left + titleTextWidth + textMargin, iconRect.top),
				trailingIconSize);

			parent->SetDrawingMode(B_OP_ALPHA);

			for (std::vector<BitmapHolderRef>::iterator it = trailingIconBitmaps.begin();
				it != trailingIconBitmaps.end(); it++) {
				const BBitmap* bitmap = (*it)->Bitmap();
				BRect bitmapBounds = bitmap->Bounds();

				BRect trailingIconAlignedRect
					= BRect(BPoint(ceilf(trailingIconRect.LeftTop().x) + 0.5,
								ceilf(trailingIconRect.LeftTop().y) + 0.5),
						trailingIconRect.Size());

				parent->DrawBitmap(bitmap, bitmapBounds, trailingIconAlignedRect,
					B_FILTER_BITMAP_BILINEAR);

				trailingIconRect.OffsetBy(
					trailingIconSize.Width() * (1.0 + trailingIconPaddingFactor), 0);
			}

			parent->SetDrawingMode(B_OP_OVER);
		}

	} else if (stringField != NULL) {

		float width = rect.Width() - (2 * sTextMargin);

		if (width != stringField->Width()) {
			BString truncatedString(stringField->String());
			parent->TruncateString(&truncatedString, fTruncateMode, width + 2);
			stringField->SetClippedString(truncatedString.String());
			stringField->SetWidth(width);
		}

		DrawString(stringField->ClippedString(), parent, rect);

	} else if (lazyStringField != NULL) {
		float width = rect.Width() - (2 * sTextMargin);

		if (width != lazyStringField->Width()) {
			BString truncatedString(lazyStringField->String());
			parent->TruncateString(&truncatedString, fTruncateMode, width + 2);
			lazyStringField->SetClippedString(truncatedString.String());
			lazyStringField->SetWidth(width);
		}

		DrawString(lazyStringField->ClippedString(), parent, rect);
	} else if (ratingField != NULL) {
		float width = rect.Width();
		float padding = be_control_look->ComposeSpacing(B_USE_SMALL_SPACING);
		bool isRatingValid = ratingField->Rating() >= RATING_MIN;

		if (!isRatingValid || width < fRatingsMetrics->Size().Width() + padding * 2.0) {
			BString ratingAsText = "-";

			if (isRatingValid)
				ratingAsText.SetToFormat("%.1f", ratingField->Rating());

			float ratingAsTextWidth = parent->StringWidth(ratingAsText);

			if (ratingAsTextWidth + padding * 2.0 < width) {
				font_height fontHeight;
				parent->GetFontHeight(&fontHeight);
				float fullHeight = fontHeight.ascent + fontHeight.descent;
				float y = rect.top + (rect.Height() - fullHeight) / 2 + fontHeight.ascent;
				parent->DrawString(ratingAsText, BPoint(rect.left + padding, y));
			}
		} else {
			const BBitmap* starIcon = SharedIcons::IconStarBlue12Scaled()->Bitmap();
			float ratingsStarsHeight = fRatingsMetrics->Size().Height();
			BPoint starsPt(floorf(rect.LeftTop().x + padding),
				floorf(rect.LeftTop().y + (rect.Size().Height() / 2.0) - ratingsStarsHeight / 2.0));
			RatingUtils::Draw(parent, starsPt, ratingField->Rating(), starIcon);
		}
	}
}


int
PackageColumn::CompareFields(BField* field1, BField* field2)
{
	DateField* dateField1 = dynamic_cast<DateField*>(field1);
	DateField* dateField2 = dynamic_cast<DateField*>(field2);
	if (dateField1 != NULL && dateField2 != NULL) {
		if (dateField1->MillisSinceEpoc() > dateField2->MillisSinceEpoc())
			return -1;
		else if (dateField1->MillisSinceEpoc() < dateField2->MillisSinceEpoc())
			return 1;
		return 0;
	}

	SizeField* sizeField1 = dynamic_cast<SizeField*>(field1);
	SizeField* sizeField2 = dynamic_cast<SizeField*>(field2);
	if (sizeField1 != NULL && sizeField2 != NULL) {
		if (sizeField1->Size() > sizeField2->Size())
			return -1;
		else if (sizeField1->Size() < sizeField2->Size())
			return 1;
		return 0;
	}

	BStringField* stringField1 = dynamic_cast<BStringField*>(field1);
	BStringField* stringField2 = dynamic_cast<BStringField*>(field2);
	if (stringField1 != NULL && stringField2 != NULL) {
		// TODO: Locale aware string compare... not too important if
		// package names are not translated.
		return strcasecmp(stringField1->String(), stringField2->String());
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
PackageColumn::GetPreferredWidth(BField* _field, BView* parent) const
{
	PackageIconAndTitleField* packageIconAndTitleField
		= dynamic_cast<PackageIconAndTitleField*>(_field);
	BStringField* stringField = dynamic_cast<BStringField*>(_field);
	LazyStringField* lazyStringField = dynamic_cast<LazyStringField*>(_field);

	float parentWidth = Inherited::GetPreferredWidth(_field, parent);
	float width = 0.0;

	if (packageIconAndTitleField) {
		BFont font;
		parent->GetFont(&font);
		width = font.StringWidth(packageIconAndTitleField->String()) + 3 * sTextMargin;
		width += 16;
			// for the icon; always 16px
	} else if (stringField) {
		BFont font;
		parent->GetFont(&font);
		width = font.StringWidth(stringField->String()) + 2 * sTextMargin;
	} else if (lazyStringField) {
		BFont font;
		parent->GetFont(&font);
		width = font.StringWidth(lazyStringField->String()) + 2 * sTextMargin;
	}
	return max_c(width, parentWidth);
}


bool
PackageColumn::AcceptsField(const BField* field) const
{
	return dynamic_cast<const BStringField*>(field) != NULL
		|| dynamic_cast<const RatingField*>(field) != NULL
		|| dynamic_cast<const LazyStringField*>(field) != NULL;
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
	kRepositoryColumn,
	kVersionColumn,
	kVersionCreateTimestampColumn,
};


PackageRow::PackageRow(const PackageInfoRef& packageRef)
	:
	Inherited(ceilf(be_plain_font->Size() * 1.8f)),
	fPackage(packageRef),
	fNextInHash(NULL)
{
	if (!packageRef.IsSet())
		return;

	// Package icon and title
	// NOTE: The icon BBitmap is referenced by the fPackage member.
	UpdateIconAndTitle();

	UpdateRating();
	UpdateSummary();
	UpdateSize();
	UpdateState();
	UpdateRepository();
	UpdateVersion();
	UpdateVersionCreateTimestamp();
}


PackageRow::~PackageRow()
{
}


void
PackageRow::SetPackage(const PackageInfoRef& value)
{
	fPackage = value;
}


void
PackageRow::UpdateIconAndTitle()
{
	if (!fPackage.IsSet())
		return;

	PackageIconAndTitleField* existingField
		= static_cast<PackageIconAndTitleField*>(GetField(kTitleColumn));

	BString packageName = fPackage->Name();
	BString title;
	PackageUtils::TitleOrName(fPackage, title);
	bool isActivated = PackageUtils::State(fPackage) == ACTIVATED;
	bool isNativeDesktop = PackageUtils::IsNativeDesktop(fPackage);

	if (existingField != NULL && existingField->PackageName() == packageName
		&& existingField->String() == title && existingField->IsActivated() == isActivated
		&& existingField->IsNativeDesktop() == isNativeDesktop) {
		return;
	}

	BField* field = new PackageIconAndTitleField(packageName, title, isActivated, isNativeDesktop);
	SetField(field, kTitleColumn);
}


void
PackageRow::UpdateState()
{
	if (!fPackage.IsSet())
		return;

	BStringField* existingStringField = static_cast<BStringField*>(GetField(kStatusColumn));
	BString updatedStatus = package_state_to_string(fPackage);

	if (existingStringField != NULL && existingStringField->String() == updatedStatus)
		return;

	SetField(new BStringField(updatedStatus), kStatusColumn);
}


void
PackageRow::UpdateSummary()
{
	if (!fPackage.IsSet())
		return;

	BStringField* existingStringField = static_cast<BStringField*>(GetField(kDescriptionColumn));
	BString updatedSummary;
	PackageUtils::Summary(fPackage, updatedSummary);

	if (existingStringField != NULL && existingStringField->String() == updatedSummary)
		return;

	// TODO; `kDescriptionColumn` seems wrong here?
	SetField(new BStringField(updatedSummary), kDescriptionColumn);
}


void
PackageRow::UpdateRating()
{
	if (!fPackage.IsSet())
		return;

	PackageUserRatingInfoRef userRatingInfo = fPackage->UserRatingInfo();
	UserRatingSummaryRef userRatingSummary;
	float averageRating = RATING_MISSING;

	if (userRatingInfo.IsSet())
		userRatingSummary = userRatingInfo->Summary();

	if (userRatingSummary.IsSet())
		averageRating = userRatingSummary->AverageRating();

	RatingField* existingRatingField = static_cast<RatingField*>(GetField(kRatingColumn));

	if (existingRatingField != NULL && existingRatingField->Rating() == averageRating)
		return;

	SetField(new RatingField(averageRating), kRatingColumn);
}


void
PackageRow::UpdateSize()
{
	if (!fPackage.IsSet())
		return;

	SizeField* existingSizeField = static_cast<SizeField*>(GetField(kSizeColumn));
	double updatedSize = static_cast<double>(PackageUtils::Size(fPackage));

	if (existingSizeField != NULL && existingSizeField->Size() == updatedSize)
		return;

	SetField(new SizeField(updatedSize), kSizeColumn);
}


void
PackageRow::UpdateRepository()
{
	if (!fPackage.IsSet())
		return;

	BStringField* existingDepotNameField = static_cast<BStringField*>(GetField(kRepositoryColumn));
	BString depotName = PackageUtils::DepotName(fPackage);

	if (existingDepotNameField != NULL && existingDepotNameField->String() == depotName)
		return;

	SetField(new BStringField(depotName), kRepositoryColumn);
}


void
PackageRow::UpdateVersion()
{
	if (!fPackage.IsSet())
		return;

	PackageVersionRef version = PackageUtils::Version(fPackage);
	BString versionString;

	if (version.IsSet())
		versionString = version->ToString();

	BStringField* existingStringField = static_cast<BStringField*>(GetField(kVersionColumn));

	if (existingStringField != NULL && existingStringField->String() == versionString)
		return;

	SetField(new BStringField(versionString), kVersionColumn);
}


void
PackageRow::UpdateVersionCreateTimestamp()
{
	if (!fPackage.IsSet())
		return;

	DateField* existingDateField = static_cast<DateField*>(GetField(kVersionCreateTimestampColumn));
	PackageVersionRef version = PackageUtils::Version(fPackage);
	uint64 millisSinceEpoc = 0;

	if (version.IsSet())
		millisSinceEpoc = version->CreateTimestamp();

	if (existingDateField != NULL && existingDateField->MillisSinceEpoc() == millisSinceEpoc)
		return;

	SetField(new DateField(millisSinceEpoc), kVersionCreateTimestampColumn);
}


// #pragma mark - ItemCountView


class PackageListView::ItemCountView : public BView {
public:
	ItemCountView()
		:
		BView("item count view", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
		fItemCount(0),
		fInvalidated(false)
	{
		BFont font(be_plain_font);
		font.SetSize(font.Size() * 0.75f);
		SetFont(&font);

		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		SetLowUIColor(ViewUIColor());
		SetHighUIColor(LowUIColor(), B_DARKEN_4_TINT);

		// constantly calculating the size is expensive so here a sensible
		// upper limit on the number of packages is arbitrarily chosen.
		fMinSize
			= BSize(StringWidth(_DeriveLabel(999999)) + 10, be_control_look->GetScrollBarWidth());
	}

	virtual BSize MinSize()
	{
		return fMinSize;
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
		if (fInvalidated) {
			fLabel = _DeriveLabel(fItemCount);
			fInvalidated = false;
		}

		FillRect(updateRect, B_SOLID_LOW);

		font_height fontHeight;
		GetFontHeight(&fontHeight);

		BRect bounds(Bounds());
		float width = StringWidth(fLabel);

		BPoint offset;
		offset.x = bounds.left + (bounds.Width() - width) / 2.0f;
		offset.y = bounds.top + (bounds.Height() - (fontHeight.ascent + fontHeight.descent)) / 2.0f
			+ fontHeight.ascent;

		DrawString(fLabel, offset);
	}

	void SetItemCount(int32 count)
	{
		if (count == fItemCount)
			return;

		fItemCount = count;
		if (!fInvalidated) {
			Invalidate();
			fInvalidated = true;
		}
	}

private:
		/*! This method is hit quite often when the list of packages in the
			table-view are updated.  Derivation of the plural for some
			languages such as Russian can be slow so this method should be
			called sparingly.
		*/
	BString _DeriveLabel(int32 count) const
	{
		static BStringFormat format(B_TRANSLATE("{0, plural, one{# item} other{# items}}"));
		BString label;
		format.Format(label, count);
		return label;
	}

private:
	int32		fItemCount;
	BString		fLabel;
	BSize		fMinSize;

	bool		fInvalidated;
};


// #pragma mark - PackageListView::RowByNameHashDefinition


struct PackageListView::RowByNameHashDefinition {
	typedef const char*	KeyType;
	typedef	PackageRow	ValueType;

	size_t HashKey(const char* key) const
	{
		return BString::HashValue(key);
	}

	size_t Hash(PackageRow* value) const
	{
		return HashKey(value->Package()->Name().String());
	}

	bool Compare(const char* key, PackageRow* value) const
	{
		return value->Package()->Name() == key;
	}

	ValueType*& GetLink(PackageRow* value) const
	{
		return value->NextInHash();
	}
};


// #pragma mark - PackageListView


PackageListView::PackageListView(Model* model)
	:
	BColumnListView(B_TRANSLATE("All packages"), 0, B_FANCY_BORDER, true),
	fModel(model),
	fRowByNameTable(new RowByNameTable()),
	fWorkStatusView(NULL),
	fIgnoreSelectionChanged(false)
{
	float scale = be_plain_font->Size() / 12.f;
	float spacing = be_control_look->DefaultItemSpacing() * 2;

	AddColumn(new PackageColumn(fModel, B_TRANSLATE("Name"), 150 * scale, 50 * scale, 300 * scale,
				  B_TRUNCATE_MIDDLE),
		kTitleColumn);
	AddColumn(new PackageColumn(fModel, B_TRANSLATE("Rating"), 80 * scale, 50 * scale, 100 * scale,
				  B_TRUNCATE_MIDDLE),
		kRatingColumn);
	AddColumn(new PackageColumn(fModel, B_TRANSLATE("Description"), 300 * scale, 80 * scale,
				  1000 * scale, B_TRUNCATE_MIDDLE),
		kDescriptionColumn);
	PackageColumn* sizeColumn = new PackageColumn(fModel, B_TRANSLATE("Size"),
		spacing + StringWidth("9999.99 KiB"), 50 * scale, 140 * scale, B_TRUNCATE_END);
	sizeColumn->SetAlignment(B_ALIGN_RIGHT);
	AddColumn(sizeColumn, kSizeColumn);
	AddColumn(new PackageColumn(fModel, B_TRANSLATE("Status"),
				  spacing + StringWidth(B_TRANSLATE("Available")), 60 * scale, 140 * scale,
				  B_TRUNCATE_END),
		kStatusColumn);

	AddColumn(new PackageColumn(fModel, B_TRANSLATE("Repository"), 120 * scale, 50 * scale,
				  200 * scale, B_TRUNCATE_MIDDLE),
		kRepositoryColumn);
	SetColumnVisible(kRepositoryColumn, false);
		// invisible by default

	float widthWithPlacboVersion = spacing + StringWidth("8.2.3176-2");
		// average sort of version length as model
	AddColumn(new PackageColumn(fModel, B_TRANSLATE("Version"), widthWithPlacboVersion,
				  widthWithPlacboVersion, widthWithPlacboVersion + (50 * scale), B_TRUNCATE_MIDDLE),
		kVersionColumn);

	float widthWithPlaceboDate
		= spacing + StringWidth(LocaleUtils::TimestampToDateString(static_cast<uint64>(1000)));
	AddColumn(new PackageColumn(fModel, B_TRANSLATE("Date"), widthWithPlaceboDate,
				  widthWithPlaceboDate, widthWithPlaceboDate + (50 * scale), B_TRUNCATE_END),
		kVersionCreateTimestampColumn);

	fItemCountView = new ItemCountView();
	AddStatusView(fItemCountView);
}


PackageListView::~PackageListView()
{
	Clear();
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
PackageListView::HandleIconsChanged()
{
	// Go through the rows and in each case where there is an icon for the
	// package then update that row.

	for (int32 i = CountRows() - 1; i >= 0; i--) {
		PackageRow* row = static_cast<PackageRow*>(RowAt(i));
		InvalidateRow(row);
	}
}


void
PackageListView::HandlePackagesChanged(const PackageInfoEvents& events)
{
	for (int32 i = events.CountEvents() - 1; i >= 0; i--)
		_HandlePackageChanged(events.EventAtIndex(i));
}


void
PackageListView::_HandlePackageChanged(const PackageInfoEvent& event)
{
	uint32 changes = event.Changes();
	PackageInfoRef package = event.Package();

	if (!package.IsSet() || 0 == changes)
		return;

	PackageRow* row = _FindRow(package->Name());

	if (row != NULL) {
		PackageInfoRef previousPackage = row->Package();
		row->SetPackage(package);

		if ((changes & PKG_CHANGED_LOCALIZED_TEXT) != 0
			|| (changes & PKG_CHANGED_LOCAL_INFO) != 0) {
			row->UpdateIconAndTitle();
			row->UpdateSummary();
		}

		if ((changes & PKG_CHANGED_RATINGS) != 0)
			row->UpdateRating();

		if ((changes & PKG_CHANGED_LOCAL_INFO) != 0) {
			row->UpdateState();
			row->UpdateSize();
		}

		if ((changes & PKG_CHANGED_CORE_INFO) != 0)
			row->UpdateVersionCreateTimestamp();
	}
}


void
PackageListView::SelectionChanged()
{
	BColumnListView::SelectionChanged();

	if (fIgnoreSelectionChanged)
		return;

	BMessage message(MSG_PACKAGE_SELECTED);

	PackageRow* selected = dynamic_cast<PackageRow*>(CurrentSelection());
	if (selected != NULL)
		message.AddString("name", selected->Package()->Name());

	Window()->PostMessage(&message);
}


void
PackageListView::Clear()
{
	fItemCountView->SetItemCount(0);
	BColumnListView::Clear();
	fRowByNameTable->Clear();
}


/*!	Only keep the packages in the supplied list and remove all others.
 */
void
PackageListView::RetainPackages(const std::vector<PackageInfoRef>& packages)
{
	if (packages.empty()) {
		Clear();
		fRowByNameTable->Clear();
		return;
	}

	std::vector<PackageInfoRef>::const_iterator it;
	std::set<BString> packagesNames;
	std::vector<PackageInfoRef> removedPackages;

	for (it = packages.begin(); it != packages.end(); it++) {
		PackageInfoRef package = *it;
		packagesNames.insert(package->Name());
	}

	for (int32 i = CountRows() - 1; i >= 0; i--) {
		PackageRow* row = static_cast<PackageRow*>(RowAt(i));
		PackageInfoRef package = row->Package();

		if (packagesNames.find(package->Name()) == packagesNames.end())
			removedPackages.push_back(package);
	}

	AddRemovePackages(packages, removedPackages);
}


void
PackageListView::AddRemovePackages(const std::vector<PackageInfoRef>& addedPackages,
	const std::vector<PackageInfoRef>& removedPackages)
{
	std::vector<PackageInfoRef>::const_iterator it;

	if (!addedPackages.empty()) {
		BList addedRows;

		for (it = addedPackages.begin(); it != addedPackages.end(); it++) {
			PackageInfoRef package = *it;
			PackageRow* packageRow = _FindRow(package);

			if (packageRow == NULL) {
				packageRow = new PackageRow(package);
				addedRows.AddItem(packageRow);
				fRowByNameTable->Insert(packageRow);
			}
		}

		if (!addedRows.IsEmpty()) {
			AddRows(&addedRows, -1, NULL);
			BRow* row = static_cast<BRow*>(addedRows.ItemAt(0));
			ExpandOrCollapse(row, true);
		}
	}

	if (!removedPackages.empty()) {
		BList removedRows;

		for (it = removedPackages.begin(); it != removedPackages.end(); it++) {
			PackageInfoRef package = *it;
			PackageRow* packageRow = _FindRow(package);

			if (packageRow != NULL)
				removedRows.AddItem(packageRow);
		}

		if (!removedRows.IsEmpty())
			RemoveRows(&removedRows);

		for (int32 i = removedRows.CountItems() - 1; i >= 0; i--) {
			PackageRow* packageRow = static_cast<PackageRow*>(removedRows.ItemAt(i));
			fRowByNameTable->Remove(packageRow);
			delete packageRow;
		}
	}

	fItemCountView->SetItemCount(CountRows());
}


void
PackageListView::_AddPackage(const PackageInfoRef& package)
{
	PackageRow* packageRow = _FindRow(package);

	// forget about it if this package is already in the listview
	if (packageRow != NULL)
		return;

	// create the row for this package
	packageRow = new PackageRow(package);

	// add the row, parent may be NULL (add at top level)
	AddRow(packageRow);

	// add to hash table for quick lookup of row by package name
	fRowByNameTable->Insert(packageRow);

	// make sure the row is initially expanded
	ExpandOrCollapse(packageRow, true);
}


void
PackageListView::_RemovePackage(const PackageInfoRef& package)
{
	PackageRow* packageRow = _FindRow(package);
	if (packageRow == NULL)
		return;

	fRowByNameTable->Remove(packageRow);

	RemoveRow(packageRow);
	delete packageRow;
}


void
PackageListView::SelectPackage(const PackageInfoRef& package)
{
	fIgnoreSelectionChanged = true;

	PackageRow* row = _FindRow(package);
	BRow* selected = CurrentSelection();
	if (row != selected)
		DeselectAll();
	if (row != NULL) {
		AddToSelection(row);
		SetFocusRow(row, false);
		ScrollTo(row);
	}

	fIgnoreSelectionChanged = false;
}


void
PackageListView::AttachWorkStatusView(WorkStatusView* view)
{
	fWorkStatusView = view;
}


PackageRow*
PackageListView::_FindRow(const PackageInfoRef& package)
{
	if (!package.IsSet())
		return NULL;
	return fRowByNameTable->Lookup(package->Name().String());
}


PackageRow*
PackageListView::_FindRow(const BString& packageName)
{
	return fRowByNameTable->Lookup(packageName.String());
}
