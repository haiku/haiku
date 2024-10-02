/*
 * Copyright 2013-214, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>.
 * Copyright 2020-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "FeaturedPackagesView.h"

#include <algorithm>
#include <vector>

#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <LayoutItem.h>
#include <Message.h>
#include <ScrollView.h>
#include <StringView.h>

#include "BitmapView.h"
#include "HaikuDepotConstants.h"
#include "LocaleUtils.h"
#include "Logger.h"
#include "MainWindow.h"
#include "MarkupTextView.h"
#include "MessagePackageListener.h"
#include "RatingUtils.h"
#include "RatingView.h"
#include "SharedIcons.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FeaturedPackagesView"

#define SIZE_ICON 64.0f

// If the space for the summary has less than this many "M" characters then the summary will not be
// displayed.
#define MINIMUM_M_COUNT_SUMMARY 10.0f

// The title area will be this many times the width of an "M".
#define M_COUNT_TITLE 10


// #pragma mark - PackageView


class StackedFeaturesPackageBandMetrics
{
public:
	StackedFeaturesPackageBandMetrics(float width, BFont* titleFont, BFont* metadataFont)
	{
		float padding = be_control_look->DefaultItemSpacing();
		BSize iconSize = BControlLook::ComposeIconSize(SIZE_ICON);

		font_height titleFontHeight;
		font_height metadataFontHeight;
		titleFont->GetHeight(&titleFontHeight);
		metadataFont->GetHeight(&metadataFontHeight);

		float totalTitleAndMetadataHeight = titleFontHeight.ascent + titleFontHeight.descent
			+ titleFontHeight.leading
			+ metadataFontHeight.leading
			+ 2.0 * (metadataFontHeight.ascent + metadataFontHeight.descent);

		fHeight = fmaxf(totalTitleAndMetadataHeight, iconSize.Height()) + 2.0 * padding;

		{
			float iconInset = (fHeight - iconSize.Width()) / 2.0;
			fIconRect = BRect(padding, iconInset, iconSize.Width() + padding,
				iconSize.Height() + iconInset);
		}

		{
			float titleWidthM = titleFont->StringWidth("M");

			float leftTitlePublisherAndChronologicalInfo = fIconRect.right + padding;
			float rightTitlePublisherAndChronologicalInfo = fminf(width, fIconRect.Size().Width()
				+ (2.0 * padding) + (titleWidthM * M_COUNT_TITLE));

// left, top, right bottom
			fTitleRect = BRect(leftTitlePublisherAndChronologicalInfo,
				(fHeight - totalTitleAndMetadataHeight) / 2.0,
				rightTitlePublisherAndChronologicalInfo,
				((fHeight - totalTitleAndMetadataHeight) / 2.0)
					+ titleFontHeight.ascent + titleFontHeight.descent);

			fPublisherRect = BRect(leftTitlePublisherAndChronologicalInfo,
				fTitleRect.bottom + titleFontHeight.leading,
				rightTitlePublisherAndChronologicalInfo,
				fTitleRect.bottom + titleFontHeight.leading
					+ metadataFontHeight.ascent + metadataFontHeight.descent);

			fChronologicalInfoRect = BRect(leftTitlePublisherAndChronologicalInfo,
				fPublisherRect.bottom + metadataFontHeight.leading,
				rightTitlePublisherAndChronologicalInfo,
				fPublisherRect.bottom + metadataFontHeight.leading
					+ metadataFontHeight.ascent + metadataFontHeight.descent);
        }

        // sort out the ratings display

		{
			BSize ratingStarSize = SharedIcons::IconStarBlue16Scaled()->Bitmap()->Bounds().Size();
			RatingStarsMetrics ratingStarsMetrics(ratingStarSize);

			fRatingStarsRect = BRect(BPoint(fTitleRect.right + padding,
				(fHeight - ratingStarsMetrics.Size().Height()) / 2), ratingStarsMetrics.Size());

			if (fRatingStarsRect.right > width)
				fRatingStarsRect = BRect();
			else {
				// Now sort out the position for the summary. This is reckoned as a container
				// rect because it would be nice to layout the text with newlines and not just a
				// single line.

				fSummaryContainerRect = BRect(fRatingStarsRect.right + (padding * 2.0), padding,
					width - padding, fHeight - (padding * 2.0));

				float metadataWidthM = metadataFont->StringWidth("M");

				if (fSummaryContainerRect.Size().Width() < MINIMUM_M_COUNT_SUMMARY * metadataWidthM)
					fSummaryContainerRect = BRect();
			}
		}
	}

	float Height()
	{
		return fHeight;
	}

	BRect IconRect()
	{
		return fIconRect;
	}

	BRect TitleRect()
	{
		return fTitleRect;
	}

	BRect PublisherRect()
	{
		return fPublisherRect;
	}

	BRect ChronologicalInfoRect()
	{
		return fChronologicalInfoRect;
	}

	BRect RatingStarsRect()
	{
		return fRatingStarsRect;
	}

	BRect SummaryContainerRect()
	{
		return fSummaryContainerRect;
	}

private:
			float 				fHeight;

			BRect				fIconRect;
			BRect				fTitleRect;
			BRect				fPublisherRect;
			BRect				fChronologicalInfoRect;

			BRect				fRatingStarsRect;

			BRect				fSummaryContainerRect;
};


class StackedFeaturedPackagesView : public BView {
public:
	StackedFeaturedPackagesView(Model& model)
		:
		BView("stacked featured packages view", B_WILL_DRAW | B_FRAME_EVENTS),
		fModel(model),
		fSelectedIndex(-1),
		fPackageListener(
			new(std::nothrow) OnePackageMessagePackageListener(this)),
		fLowestIndexAddedOrRemoved(-1)
	{
		SetEventMask(B_POINTER_EVENTS);

		fTitleFont = StackedFeaturedPackagesView::CreateTitleFont();
		fMetadataFont = StackedFeaturedPackagesView::CreateMetadataFont();
		fSummaryFont = StackedFeaturedPackagesView::CreateSummaryFont();
		fBandMetrics = CreateBandMetrics();

		Clear();
	}


	virtual ~StackedFeaturedPackagesView()
	{
		fPackageListener->SetPackage(PackageInfoRef(NULL));
		fPackageListener->ReleaseReference();
		delete fBandMetrics;
		delete fTitleFont;
		delete fMetadataFont;
		delete fSummaryFont;
	}

// #pragma mark - message handling and events

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_UPDATE_PACKAGE:
			{
				BString name;
				if (message->FindString("name", &name) != B_OK)
					HDINFO("expected 'name' key on package update message");
				else
					_HandleUpdatePackage(name);
				break;
			}

			case B_COLORS_UPDATED:
			{
				Invalidate();
				break;
			}

			default:
				BView::MessageReceived(message);
				break;
		}
	}


	virtual void MouseDown(BPoint where)
	{
		if (Window()->IsActive() && !IsHidden()) {
			BRect bounds = Bounds();
			BRect parentBounds = Parent()->Bounds();
			ConvertFromParent(&parentBounds);
			bounds = bounds & parentBounds;
			if (bounds.Contains(where)) {
				_MessageSelectIndex(_IndexOfY(where.y));
				MakeFocus();
			}
		}
	}


	virtual void KeyDown(const char* bytes, int32 numBytes)
	{
		char key = bytes[0];

		switch (key) {
			case B_RIGHT_ARROW:
			case B_DOWN_ARROW:
			{
				int32 lastIndex = static_cast<int32>(fPackages.size()) - 1;
				if (!IsEmpty() && fSelectedIndex != -1
						&& fSelectedIndex < lastIndex) {
					_MessageSelectIndex(fSelectedIndex + 1);
				}
				break;
			}
			case B_LEFT_ARROW:
			case B_UP_ARROW:
				if (fSelectedIndex > 0)
					_MessageSelectIndex( fSelectedIndex - 1);
				break;
			case B_PAGE_UP:
			{
				BRect bounds = Bounds();
				ScrollTo(0, fmaxf(0, bounds.top - bounds.Height()));
				break;
			}
			case B_PAGE_DOWN:
			{
				BRect bounds = Bounds();
				float height = fPackages.size() * fBandMetrics->Height();
				float maxScrollY = height - bounds.Height();
				float pageDownScrollY = bounds.top + bounds.Height();
				ScrollTo(0, fminf(maxScrollY, pageDownScrollY));
				break;
			}
			default:
				BView::KeyDown(bytes, numBytes);
				break;
		}
	}


	/*!	This method will send a message to the Window so that it can signal
		back to this and other views that a package has been selected.  This
		method won't actually change the state of this view directly.
	*/

	void _MessageSelectIndex(int32 index) const
	{
		if (index != -1) {
			BMessage message(MSG_PACKAGE_SELECTED);
			BString packageName = fPackages[index]->Name();
			message.AddString("name", packageName);
			Window()->PostMessage(&message);
		}
	}


	virtual void FrameResized(float width, float height)
	{
		BView::FrameResized(width, height);

		delete fBandMetrics;
		fBandMetrics = CreateBandMetrics();

		Invalidate();
	}


// #pragma mark - update / add / remove / clear data


	void UpdatePackage(uint32 changeMask, const PackageInfoRef& package)
	{
		// TODO; could optimize the invalidation?
		int32 index = _IndexOfPackage(package);
		if (index >= 0) {
			fPackages[index] = package;
			Invalidate(_RectOfIndex(index));
		}
	}


	void Clear()
	{
		for (std::vector<PackageInfoRef>::iterator it = fPackages.begin();
				it != fPackages.end(); it++) {
			(*it)->RemoveListener(fPackageListener);
		}
		fPackages.clear();
		fSelectedIndex = -1;

		Invalidate();
	}


	static BFont* CreateTitleFont()
	{
		BFont* font = new BFont(be_plain_font);
		font_family family;
		font_style style;
		font->SetSize(ceilf(font->Size() * 1.8f));
		font->GetFamilyAndStyle(&family, &style);
		font->SetFamilyAndStyle(family, "Bold");
		return font;
	}


    static BFont* CreateMetadataFont()
    {
		BFont* font = new BFont(be_plain_font);
		font_family family;
		font_style style;
		font->GetFamilyAndStyle(&family, &style);
		font->SetFamilyAndStyle(family, "Italic");
		return font;
	}


	static BFont* CreateSummaryFont()
	{
		return new BFont(be_plain_font);
	}


	StackedFeaturesPackageBandMetrics* CreateBandMetrics()
	{
		return new StackedFeaturesPackageBandMetrics(Bounds().Width(), fTitleFont, fMetadataFont);
	}


	bool IsEmpty() const
	{
		return fPackages.size() == 0;
	}


	void _HandleUpdatePackage(const BString& name)
	{
		int32 index = _IndexOfName(name);
		if (index != -1)
			Invalidate(_RectOfIndex(index));
	}


	static int _CmpProminences(int64 a, int64 b)
	{
		if (a <= 0)
			a = PROMINANCE_ORDERING_MAX;
		if (b <= 0)
			b = PROMINANCE_ORDERING_MAX;
		if (a == b)
			return 0;
		if (a > b)
			return 1;
		return -1;
	}


	/*! This method will return true if the packageA is ordered before
		packageB.
	*/

	static bool _IsPackageBefore(const PackageInfoRef& packageA,
		const PackageInfoRef& packageB)
	{
		if (!packageA.IsSet() || !packageB.IsSet())
			HDFATAL("unexpected NULL reference in a referencable");
		int c = _CmpProminences(packageA->Prominence(), packageB->Prominence());
		if (c == 0)
			c = packageA->Title().ICompare(packageB->Title());
		if (c == 0)
			c = packageA->Name().Compare(packageB->Name());
		return c < 0;
	}


	void BeginAddRemove()
	{
		fLowestIndexAddedOrRemoved = INT32_MAX;
	}


	void EndAddRemove()
	{
		if (fLowestIndexAddedOrRemoved < INT32_MAX) {
			if (fPackages.empty())
				Invalidate();
			else {
				BRect invalidRect = Bounds();
				invalidRect.top = _YOfIndex(fLowestIndexAddedOrRemoved);
				Invalidate(invalidRect);
			}
		}
	}


	void AddPackage(const PackageInfoRef& package)
	{
		// fPackages is sorted and for this reason it is possible to find the
		// insertion point by identifying the first item in fPackages that does
		// not return true from the method '_IsPackageBefore'.

		std::vector<PackageInfoRef>::iterator itInsertionPt
			= std::lower_bound(fPackages.begin(), fPackages.end(), package,
				&_IsPackageBefore);

		if (itInsertionPt == fPackages.end()
				|| package->Name() != (*itInsertionPt)->Name()) {
			int32 insertionIndex =
				std::distance<std::vector<PackageInfoRef>::const_iterator>(
					fPackages.begin(), itInsertionPt);
			if (fSelectedIndex >= insertionIndex)
				fSelectedIndex++;
			fPackages.insert(itInsertionPt, package);
			package->AddListener(fPackageListener);
			if (insertionIndex < fLowestIndexAddedOrRemoved)
				fLowestIndexAddedOrRemoved = insertionIndex;
		}
	}


	void RemovePackage(const PackageInfoRef& package)
	{
		int32 index = _IndexOfPackage(package);
		if (index >= 0) {
			if (fSelectedIndex == index)
				fSelectedIndex = -1;
			if (fSelectedIndex > index)
				fSelectedIndex--;
			fPackages[index]->RemoveListener(fPackageListener);
			fPackages.erase(fPackages.begin() + index);
			if (index < fLowestIndexAddedOrRemoved)
				fLowestIndexAddedOrRemoved = index;
		}
	}


// #pragma mark - selection and index handling


	void SelectPackage(const PackageInfoRef& package)
	{
		_SelectIndex(_IndexOfPackage(package));
	}


	void _SelectIndex(int32 index)
	{
		if (index != fSelectedIndex) {
			int32 previousSelectedIndex = fSelectedIndex;
			fSelectedIndex = index;
			if (fSelectedIndex >= 0)
				Invalidate(_RectOfIndex(fSelectedIndex));
			if (previousSelectedIndex >= 0)
				Invalidate(_RectOfIndex(previousSelectedIndex));
			_EnsureIndexVisible(index);
		}
	}


	int32 _IndexOfPackage(PackageInfoRef package) const
	{
		std::vector<PackageInfoRef>::const_iterator it
			= std::lower_bound(fPackages.begin(), fPackages.end(), package,
				&_IsPackageBefore);

		return (it == fPackages.end() || (*it)->Name() != package->Name())
			? -1 : it - fPackages.begin();
	}


	int32 _IndexOfName(const BString& name) const
	{
		// TODO; slow linear search.
		// the fPackages is not sorted on name and for this reason it is not
		// possible to do a binary search.
		for (uint32 i = 0; i < fPackages.size(); i++) {
			if (fPackages[i]->Name() == name)
				return i;
		}
		return -1;
	}


// #pragma mark - drawing and rendering


	virtual void Draw(BRect updateRect)
	{
		SetHighUIColor(B_LIST_BACKGROUND_COLOR);
		FillRect(updateRect);

		int32 iStart = _IndexRoundedOfY(updateRect.top);

		if (iStart != -1) {
			int32 iEnd = _IndexRoundedOfY(updateRect.bottom);
			for (int32 i = iStart; i <= iEnd; i++)
				_DrawPackageAtIndex(updateRect, i);
		}
	}


	void _DrawPackageAtIndex(BRect updateRect, int32 index)
	{
		_DrawPackage(updateRect, fPackages[index], _YOfIndex(index), index == fSelectedIndex);
	}


	void _DrawPackage(BRect updateRect, PackageInfoRef pkg, float y, bool selected)
	{
		if (selected) {
			SetLowUIColor(B_LIST_SELECTED_BACKGROUND_COLOR);
			FillRect(_RectOfY(y), B_SOLID_LOW);
		} else {
			SetLowUIColor(B_LIST_BACKGROUND_COLOR);
		}

		BRect iconRect = fBandMetrics->IconRect();
		BRect titleRect = fBandMetrics->TitleRect();
		BRect publisherRect = fBandMetrics->PublisherRect();
		BRect chronologicalInfoRect = fBandMetrics->ChronologicalInfoRect();
		BRect ratingStarsRect = fBandMetrics->RatingStarsRect();
		BRect summaryContainerRect = fBandMetrics->SummaryContainerRect();

		iconRect.OffsetBy(0.0, y);
		titleRect.OffsetBy(0.0, y);
		publisherRect.OffsetBy(0.0, y);
		chronologicalInfoRect.OffsetBy(0.0, y);
		ratingStarsRect.OffsetBy(0.0, y);
		summaryContainerRect.OffsetBy(0.0, y);

		// TODO; optimization; the updateRect may only cover some of this?
		_DrawPackageIcon(iconRect, pkg, selected);
		_DrawPackageTitle(titleRect, pkg, selected);
		_DrawPackagePublisher(publisherRect, pkg, selected);
		_DrawPackageChronologicalInfo(chronologicalInfoRect, pkg, selected);
		_DrawPackageRating(ratingStarsRect, pkg);
		_DrawPackageSummary(summaryContainerRect, pkg, selected);
	}


	void _DrawPackageIcon(BRect iconRect, PackageInfoRef pkg, bool selected)
	{
		if (!iconRect.IsValid())
			return;

		BitmapHolderRef icon;
		status_t iconResult = fModel.GetPackageIconRepository().GetIcon(pkg->Name(),
			iconRect.Width(), icon);

		if (iconResult == B_OK) {
			if (icon.IsSet()) {
				const BBitmap* bitmap = icon->Bitmap();

				if (bitmap != NULL && bitmap->IsValid()) {
					SetDrawingMode(B_OP_ALPHA);
					DrawBitmap(bitmap, bitmap->Bounds(), iconRect, B_FILTER_BITMAP_BILINEAR);
				}
			}
		}
	}


	void _DrawPackageTitle(BRect textRect, PackageInfoRef pkg, bool selected)
	{
		if (!textRect.IsValid())
			return;

		const BBitmap* installedIconBitmap = SharedIcons::IconInstalled16Scaled()->Bitmap();

		SetDrawingMode(B_OP_COPY);
		SetHighUIColor(selected ? B_LIST_SELECTED_ITEM_TEXT_COLOR : B_LIST_ITEM_TEXT_COLOR);
		SetFont(fTitleFont);

		font_height fontHeight;
		fTitleFont->GetHeight(&fontHeight);
		BPoint pt = textRect.LeftTop() + BPoint(0.0, + fontHeight.ascent);

		BString renderedText = pkg->Title();
		float installedIconAllowance = installedIconBitmap->Bounds().Width() * 1.5;
		TruncateString(&renderedText, B_TRUNCATE_END, textRect.Width() - installedIconAllowance);

		DrawString(renderedText, pt);

		if (pkg->State() == ACTIVATED) {
			float stringWidth = StringWidth(pkg->Title());
			BRect iconRect = BRect(
				BPoint(textRect.left + stringWidth + (installedIconBitmap->Bounds().Width() / 2.0),
				textRect.top + (textRect.Height() / 2.0)
					- (installedIconBitmap->Bounds().Height() / 2.0)),
				installedIconBitmap->Bounds().Size());
			SetDrawingMode(B_OP_ALPHA);
			DrawBitmap(installedIconBitmap, installedIconBitmap->Bounds(), iconRect,
				B_FILTER_BITMAP_BILINEAR);
		}
	}


	void _DrawPackageGenericTextSlug(BRect textRect, const BString& text, bool selected)
	{
		if (!textRect.IsValid())
			return;

		SetDrawingMode(B_OP_COPY);
		SetHighUIColor(selected ? B_LIST_SELECTED_ITEM_TEXT_COLOR : B_LIST_ITEM_TEXT_COLOR);
		SetFont(fMetadataFont);

		font_height fontHeight;
		fMetadataFont->GetHeight(&fontHeight);
		BPoint pt = textRect.LeftTop() + BPoint(0.0, + fontHeight.ascent);

		BString renderedText(text);
		TruncateString(&renderedText, B_TRUNCATE_END, textRect.Width());

		DrawString(renderedText, pt);
	}


	void _DrawPackagePublisher(BRect textRect, PackageInfoRef pkg, bool selected)
	{
		_DrawPackageGenericTextSlug(textRect, pkg->Publisher().Name(), selected);
	}


	void _DrawPackageChronologicalInfo(BRect textRect, PackageInfoRef pkg, bool selected)
	{
		BString versionCreateTimestampPresentation
			= LocaleUtils::TimestampToDateString(pkg->VersionCreateTimestamp());
		_DrawPackageGenericTextSlug(textRect, versionCreateTimestampPresentation, selected);
	}


	// TODO; show the sample size
	void _DrawPackageRating(BRect ratingRect, PackageInfoRef pkg)
	{
		if (!ratingRect.IsValid())
			return;

		UserRatingInfoRef userRatingInfo = pkg->UserRatingInfo();

		if (userRatingInfo.IsSet()) {
			UserRatingSummaryRef userRatingSummary = userRatingInfo->Summary();

			if (userRatingSummary.IsSet())
				RatingUtils::Draw(this, ratingRect.LeftTop(), userRatingSummary->AverageRating());
		}
	}


	// TODO; handle multi-line rendering of the text
	void _DrawPackageSummary(BRect textRect, PackageInfoRef pkg, bool selected)
	{
		if (!textRect.IsValid())
			return;

		SetDrawingMode(B_OP_COPY);
		SetHighUIColor(selected ? B_LIST_SELECTED_ITEM_TEXT_COLOR : B_LIST_ITEM_TEXT_COLOR);
		SetFont(fSummaryFont);

		font_height fontHeight;
		fSummaryFont->GetHeight(&fontHeight);

		// The text rect is a container into which later text can be made to flow multi-line. For
		// now just draw one line of the summary.

		BPoint pt = textRect.LeftTop() + BPoint(0.0,
			(textRect.Size().Height() / 2.0) - ((fontHeight.ascent + fontHeight.descent) / 2.0)
				+ fontHeight.ascent);

		BString summary(pkg->ShortDescription());
		TruncateString(&summary, B_TRUNCATE_END, textRect.Width());

		DrawString(summary, pt);
	}


// #pragma mark - geometry and scrolling


	/*!	This method will make sure that the package at the given index is
		visible.  If the whole of the package can be seen already then it will
		do nothing.  If the package is located above the visible region then it
		will scroll up to it.  If the package is located below the visible
		region then it will scroll down to it.
	*/

	void _EnsureIndexVisible(int32 index)
	{
		if (!_IsIndexEntirelyVisible(index)) {
			BRect bounds = Bounds();
			int32 indexOfCentreVisible = _IndexOfY(bounds.top + bounds.Height() / 2);
			if (index < indexOfCentreVisible)
				ScrollTo(0, _YOfIndex(index));
			else {
				float scrollPointY = (_YOfIndex(index) + fBandMetrics->Height()) - bounds.Height();
				ScrollTo(0, scrollPointY);
			}
		}
	}


	/*!	This method will return true if the package at the supplied index is
		entirely visible.
	*/

	bool _IsIndexEntirelyVisible(int32 index)
	{
		BRect bounds = Bounds();
		return bounds == (bounds | _RectOfIndex(index));
	}


	BRect _RectOfIndex(int32 index) const
	{
		if (index < 0)
			return BRect(0, 0, 0, 0);
		return _RectOfY(_YOfIndex(index));
	}


	/*!	Provides the top coordinate (offset from the top of view) of the package
		supplied.  If the package does not exist in the view then the coordinate
		returned will be B_SIZE_UNSET.
	*/

	float TopOfPackage(const PackageInfoRef& package)
	{
		if (package.IsSet()) {
			int index = _IndexOfPackage(package);
			if (-1 != index)
				return _YOfIndex(index);
		}
		return B_SIZE_UNSET;
	}


	BRect _RectOfY(float y) const
	{
		return BRect(0, y, Bounds().Width(), y + fBandMetrics->Height());
	}


	float _YOfIndex(int32 i) const
	{
		return i * fBandMetrics->Height();
	}


	/*! Finds the offset into the list of packages for the y-coord in the view's
		coordinate space.  If the y is above or below the list of packages then
		this will return -1 to signal this.
	*/

	int32 _IndexOfY(float y) const
	{
		if (fPackages.empty())
			return -1;
		int32 i = static_cast<int32>(y / fBandMetrics->Height());
		if (i < 0 || i >= static_cast<int32>(fPackages.size()))
			return -1;
		return i;
	}


	/*! Find the offset into the list of packages for the y-coord in the view's
		coordinate space.  If the y is above or below the list of packages then
		this will return the first or last package index respectively.  If there
		are no packages then this will return -1;
	*/

	int32 _IndexRoundedOfY(float y) const
	{
		if (fPackages.empty())
			return -1;
		int32 i = static_cast<int32>(y / fBandMetrics->Height());
		if (i < 0)
			return 0;
		return std::min(i, (int32) (fPackages.size() - 1));
	}


	virtual BSize PreferredSize()
	{
		return BSize(B_SIZE_UNLIMITED,
			fBandMetrics->Height() * static_cast<float>(fPackages.size()));
	}


private:
			Model&				fModel;
			std::vector<PackageInfoRef>
								fPackages;
			int32				fSelectedIndex;
			OnePackageMessagePackageListener*
								fPackageListener;
			int32				fLowestIndexAddedOrRemoved;
			StackedFeaturesPackageBandMetrics*
								fBandMetrics;

			BFont*				fTitleFont;
			BFont*				fMetadataFont;
			BFont*				fSummaryFont;
};


// #pragma mark - FeaturedPackagesView


FeaturedPackagesView::FeaturedPackagesView(Model& model)
	:
	BView(B_TRANSLATE("Featured packages"), 0),
	fModel(model)
{
	fPackagesView = new StackedFeaturedPackagesView(fModel);

	fScrollView = new BScrollView("featured packages scroll view",
		fPackagesView, 0, false, true, B_FANCY_BORDER);

	BLayoutBuilder::Group<>(this)
		.Add(fScrollView, 1.0f);
}


FeaturedPackagesView::~FeaturedPackagesView()
{
}


void
FeaturedPackagesView::BeginAddRemove()
{
	fPackagesView->BeginAddRemove();
}


void
FeaturedPackagesView::EndAddRemove()
{
	fPackagesView->EndAddRemove();
	_AdjustViews();
}


/*! This method will add the package into the list to be displayed.  The
    insertion will occur in alphabetical order.
*/

void
FeaturedPackagesView::AddPackage(const PackageInfoRef& package)
{
	fPackagesView->AddPackage(package);
}


void
FeaturedPackagesView::RemovePackage(const PackageInfoRef& package)
{
	fPackagesView->RemovePackage(package);
}


void
FeaturedPackagesView::Clear()
{
	HDINFO("did clear the featured packages view");
	fPackagesView->Clear();
	_AdjustViews();
}


void
FeaturedPackagesView::SelectPackage(const PackageInfoRef& package,
	bool scrollToEntry)
{
	fPackagesView->SelectPackage(package);

	if (scrollToEntry) {
		float offset = fPackagesView->TopOfPackage(package);
		if (offset != B_SIZE_UNSET)
			fPackagesView->ScrollTo(0, offset);
	}
}


void
FeaturedPackagesView::DoLayout()
{
	BView::DoLayout();
	_AdjustViews();
}


void
FeaturedPackagesView::_AdjustViews()
{
	fScrollView->FrameResized(fScrollView->Frame().Width(),
		fScrollView->Frame().Height());
}
