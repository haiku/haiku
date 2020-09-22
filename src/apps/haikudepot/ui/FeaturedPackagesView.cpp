/*
 * Copyright 2013-214, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>.
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "FeaturedPackagesView.h"

#include <algorithm>
#include <vector>

#include <Bitmap.h>
#include <Catalog.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <LayoutItem.h>
#include <Message.h>
#include <ScrollView.h>
#include <StringView.h>
#include <SpaceLayoutItem.h>

#include "BitmapView.h"
#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "MainWindow.h"
#include "MarkupTextView.h"
#include "MessagePackageListener.h"
#include "RatingUtils.h"
#include "RatingView.h"
#include "ScrollableGroupView.h"
#include "SharedBitmap.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FeaturedPackagesView"


#define HEIGHT_PACKAGE 84.0f
#define SIZE_ICON 64.0f
#define X_POSITION_RATING 350.0f
#define X_POSITION_SUMMARY 500.0f
#define WIDTH_RATING 100.0f
#define Y_PROPORTION_TITLE 0.4f
#define Y_PROPORTION_PUBLISHER 0.7f
#define PADDING 8.0f


static BitmapRef sInstalledIcon(new(std::nothrow)
	SharedBitmap(RSRC_INSTALLED), true);


// #pragma mark - PackageView


class StackedFeaturedPackagesView : public BView {
public:
	StackedFeaturedPackagesView(Model& model)
		:
		BView("stacked featured packages view", B_WILL_DRAW | B_FRAME_EVENTS),
		fModel(model),
		fSelectedIndex(-1),
		fPackageListener(
			new(std::nothrow) OnePackageMessagePackageListener(this))
	{
		SetEventMask(B_POINTER_EVENTS);
		Clear();
	}


	virtual ~StackedFeaturedPackagesView()
	{
		fPackageListener->SetPackage(PackageInfoRef(NULL));
		fPackageListener->ReleaseReference();
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
				if (!IsEmpty() && fSelectedIndex != -1
						&& fSelectedIndex < fPackages.size() - 1) {
					_MessageSelectIndex(fSelectedIndex + 1);
				}
				break;
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
				float height = fPackages.size() * HEIGHT_PACKAGE;
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
			message.AddString("name", fPackages[index]->Name());
			Window()->PostMessage(&message);
		}
	}


	virtual void FrameResized(float width, float height)
	{
		BView::FrameResized(width, height);

		// because the summary text will wrap, a resize of the frame will
		// result in all of the summary area needing to be redrawn.

		BRect rectToInvalidate = Bounds();
		rectToInvalidate.left = X_POSITION_SUMMARY;
		Invalidate(rectToInvalidate);
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
		if (packageA.Get() == NULL || packageB.Get() == NULL)
			HDFATAL("unexpected NULL reference in a referencable");
		int c = _CmpProminences(packageA->Prominence(), packageB->Prominence());
		if (c == 0)
			c = packageA->Title().ICompare(packageB->Title());
		if (c == 0)
			c = packageA->Name().Compare(packageB->Name());
		return c < 0;
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
			Invalidate(_RectOfIndex(insertionIndex)
				| _RectOfIndex(fPackages.size() - 1));
			package->AddListener(fPackageListener);
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
			if (fPackages.empty())
				Invalidate();
			else {
				Invalidate(_RectOfIndex(index)
					| _RectOfIndex(fPackages.size() - 1));
			}
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
		_DrawPackage(updateRect, fPackages[index], index, _YOfIndex(index),
			index == fSelectedIndex);
	}


	void _DrawPackage(BRect updateRect, PackageInfoRef pkg, int index, float y,
		bool selected)
	{
		if (selected) {
			SetLowUIColor(B_LIST_SELECTED_BACKGROUND_COLOR);
			FillRect(_RectOfY(y), B_SOLID_LOW);
		} else {
			SetLowUIColor(B_LIST_BACKGROUND_COLOR);
		}
		// TODO; optimization; the updateRect may only cover some of this?
		_DrawPackageIcon(updateRect, pkg, y, selected);
		_DrawPackageTitle(updateRect, pkg, y, selected);
		_DrawPackagePublisher(updateRect, pkg, y, selected);
		_DrawPackageRating(updateRect, pkg, y, selected);
		_DrawPackageSummary(updateRect, pkg, y, selected);
	}


	void _DrawPackageIcon(BRect updateRect, PackageInfoRef pkg, float y,
		bool selected)
	{
		BitmapRef icon;
		status_t iconResult = fModel.GetPackageIconRepository().GetIcon(
			pkg->Name(), BITMAP_SIZE_64, icon);

		if (iconResult == B_OK) {
			if (icon.Get() != NULL) {
				float inset = (HEIGHT_PACKAGE - SIZE_ICON) / 2.0;
				BRect targetRect = BRect(inset, y + inset, SIZE_ICON + inset,
					y + SIZE_ICON + inset);
				const BBitmap* bitmap = icon->Bitmap(BITMAP_SIZE_64);
				SetDrawingMode(B_OP_ALPHA);
				DrawBitmap(bitmap, bitmap->Bounds(), targetRect,
					B_FILTER_BITMAP_BILINEAR);
			}
		}
	}


	void _DrawPackageTitle(BRect updateRect, PackageInfoRef pkg, float y,
		bool selected)
	{
		static BFont* sFont = NULL;

		if (sFont == NULL) {
			sFont = new BFont(be_plain_font);
			GetFont(sFont);
  			font_family family;
			font_style style;
			sFont->SetSize(ceilf(sFont->Size() * 1.8f));
			sFont->GetFamilyAndStyle(&family, &style);
			sFont->SetFamilyAndStyle(family, "Bold");
		}

		SetDrawingMode(B_OP_COPY);
		SetHighUIColor(selected ? B_LIST_SELECTED_ITEM_TEXT_COLOR
			: B_LIST_ITEM_TEXT_COLOR);
		SetFont(sFont);
		BPoint pt(HEIGHT_PACKAGE, y + (HEIGHT_PACKAGE * Y_PROPORTION_TITLE));
		DrawString(pkg->Title(), pt);

		if (pkg->State() == ACTIVATED) {
			const BBitmap* bitmap = sInstalledIcon->Bitmap(
				BITMAP_SIZE_16);
			float stringWidth = StringWidth(pkg->Title());
			float offsetX = pt.x + stringWidth + PADDING;
			BRect targetRect(offsetX, pt.y - 16, offsetX + 16, pt.y);
			SetDrawingMode(B_OP_ALPHA);
			DrawBitmap(bitmap, bitmap->Bounds(), targetRect,
				B_FILTER_BITMAP_BILINEAR);
		}
	}


	void _DrawPackagePublisher(BRect updateRect, PackageInfoRef pkg, float y,
		bool selected)
	{
		static BFont* sFont = NULL;

		if (sFont == NULL) {
			sFont = new BFont(be_plain_font);
			font_family family;
			font_style style;
			sFont->SetSize(std::max(9.0f, floorf(sFont->Size() * 0.92f)));
			sFont->GetFamilyAndStyle(&family, &style);
			sFont->SetFamilyAndStyle(family, "Italic");
		}

		SetDrawingMode(B_OP_COPY);
		SetHighUIColor(selected ? B_LIST_SELECTED_ITEM_TEXT_COLOR
			: B_LIST_ITEM_TEXT_COLOR);
		SetFont(sFont);

		float maxTextWidth = (X_POSITION_RATING - HEIGHT_PACKAGE) - PADDING;
		BString publisherName(pkg->Publisher().Name());
		TruncateString(&publisherName, B_TRUNCATE_END, maxTextWidth);

		DrawString(publisherName, BPoint(HEIGHT_PACKAGE,
			y + (HEIGHT_PACKAGE * Y_PROPORTION_PUBLISHER)));
	}


	// TODO; show the sample size
	void _DrawPackageRating(BRect updateRect, PackageInfoRef pkg, float y,
		bool selected)
	{
		BPoint at(X_POSITION_RATING,
			y + (HEIGHT_PACKAGE - SIZE_RATING_STAR) / 2.0f);
		RatingUtils::Draw(this, at,
			pkg->CalculateRatingSummary().averageRating);
	}


	// TODO; handle multi-line rendering of the text
	void _DrawPackageSummary(BRect updateRect, PackageInfoRef pkg, float y,
		bool selected)
	{
		BRect bounds = Bounds();

		SetDrawingMode(B_OP_COPY);
		SetHighUIColor(selected ? B_LIST_SELECTED_ITEM_TEXT_COLOR
			: B_LIST_ITEM_TEXT_COLOR);
		SetFont(be_plain_font);

		float maxTextWidth = bounds.Width() - X_POSITION_SUMMARY - PADDING;
		BString summary(pkg->ShortDescription());
		TruncateString(&summary, B_TRUNCATE_END, maxTextWidth);

		DrawString(summary, BPoint(X_POSITION_SUMMARY,
			y + (HEIGHT_PACKAGE * 0.5)));
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
			int32 indexOfCentreVisible = _IndexOfY(
				bounds.top + bounds.Height() / 2);
			if (index < indexOfCentreVisible)
				ScrollTo(0, _YOfIndex(index));
			else {
				float scrollPointY = (_YOfIndex(index) + HEIGHT_PACKAGE)
					- bounds.Height();
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
		if (package.Get() != NULL) {
			int index = _IndexOfPackage(package);
			if (-1 != index)
				return _YOfIndex(index);
		}
		return B_SIZE_UNSET;
	}


	BRect _RectOfY(float y) const
	{
		return BRect(0, y, Bounds().Width(), y + HEIGHT_PACKAGE);
	}


	float _YOfIndex(int32 i) const
	{
		return i * HEIGHT_PACKAGE;
	}


	/*! Finds the offset into the list of packages for the y-coord in the view's
		coordinate space.  If the y is above or below the list of packages then
		this will return -1 to signal this.
	*/

	int32 _IndexOfY(float y) const
	{
		if (fPackages.empty())
			return -1;
		int32 i = y / HEIGHT_PACKAGE;
		if (i < 0 || i >= fPackages.size())
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
		int32 i = y / HEIGHT_PACKAGE;
		if (i < 0)
			return 0;
		return std::min(i, (int32) (fPackages.size() - 1));
	}


	virtual BSize PreferredSize()
	{
		return BSize(B_SIZE_UNLIMITED, HEIGHT_PACKAGE * fPackages.size());
	}


private:
			Model&				fModel;
			std::vector<PackageInfoRef>
								fPackages;
			int32				fSelectedIndex;
			OnePackageMessagePackageListener*
								fPackageListener;
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


/*! This method will add the package into the list to be displayed.  The
    insertion will occur in alphabetical order.
*/

void
FeaturedPackagesView::AddPackage(const PackageInfoRef& package)
{
	fPackagesView->AddPackage(package);
	_AdjustViews();
}


void
FeaturedPackagesView::RemovePackage(const PackageInfoRef& package)
{
	fPackagesView->RemovePackage(package);
	_AdjustViews();
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


void
FeaturedPackagesView::CleanupIcons()
{
	sInstalledIcon.Unset();
}
