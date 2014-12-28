/*
 * Copyright 2013-214, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "FeaturedPackagesView.h"

#include <stdio.h>

#include <Catalog.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <ScrollView.h>
#include <StringView.h>
#include <SpaceLayoutItem.h>

#include "BitmapView.h"
#include "MainWindow.h"
#include "MarkupTextView.h"
#include "MessagePackageListener.h"
#include "RatingView.h"
#include "ScrollableGroupView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FeaturedPackagesView"


static const rgb_color kLightBlack = (rgb_color){ 60, 60, 60, 255 };

static BitmapRef sInstalledIcon(new(std::nothrow) SharedBitmap(504), true);


// #pragma mark - PackageView


class PackageView : public BGroupView {
public:
	PackageView()
		:
		BGroupView("package view", B_HORIZONTAL),
		fPackageListener(
			new(std::nothrow) OnePackageMessagePackageListener(this)),
		fSelected(false)
	{
		SetViewColor(255, 255, 255);
		SetEventMask(B_POINTER_EVENTS);
		
		fIconView = new BitmapView("package icon view");
		fInstalledIconView = new BitmapView("installed icon view");
		fTitleView = new BStringView("package title view", "");
		fPublisherView = new BStringView("package publisher view", "");

		// Title font
		BFont font;
		GetFont(&font);
		font_family family;
		font_style style;
		font.SetSize(ceilf(font.Size() * 1.8f));
		font.GetFamilyAndStyle(&family, &style);
		font.SetFamilyAndStyle(family, "Bold");
		fTitleView->SetFont(&font);

		// Publisher font
		GetFont(&font);
		font.SetSize(std::max(9.0f, floorf(font.Size() * 0.92f)));
		font.SetFamilyAndStyle(family, "Italic");
		fPublisherView->SetFont(&font);
		fPublisherView->SetHighColor(kLightBlack);

		// Summary text view
		fSummaryView = new MarkupTextView("package summary");
		fSummaryView->SetSelectionEnabled(false);

		// Rating view
		fRatingView = new RatingView("package rating view");

		fAvgRating = new BStringView("package average rating", "");
		fAvgRating->SetFont(&font);
		fAvgRating->SetHighColor(kLightBlack);

		fVoteInfo = new BStringView("package vote info", "");
		// small font
		GetFont(&font);
		font.SetSize(std::max(9.0f, floorf(font.Size() * 0.85f)));
		fVoteInfo->SetFont(&font);
		fVoteInfo->SetHighColor(kLightBlack);

		BLayoutBuilder::Group<>(this)
			.Add(fIconView)
			.AddGroup(B_VERTICAL, 1.0f, 2.2f)
				.AddGroup(B_HORIZONTAL)
					.Add(fTitleView)
					.Add(fInstalledIconView)
					.AddGlue()
				.End()
				.Add(fPublisherView)
				.SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET))
			.End()
			.AddGlue(0.1f)
			.AddGroup(B_HORIZONTAL, 0.8f)
				.Add(fRatingView)
				.Add(fAvgRating)
				.Add(fVoteInfo)
			.End()
			.AddGlue(0.2f)
			.Add(fSummaryView, 2.0f)

			.SetInsets(B_USE_WINDOW_INSETS)
		;

		Clear();
	}

	virtual ~PackageView()
	{
		fPackageListener->SetPackage(PackageInfoRef(NULL));
		delete fPackageListener;
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case B_MOUSE_WHEEL_CHANGED:
				Window()->PostMessage(message, Parent());
				break;

			case MSG_UPDATE_PACKAGE:
				SetPackage(fPackageListener->Package());
				break;
		}
	}
	
	virtual void MouseDown(BPoint where)
	{
		BRect bounds = Bounds();
		BRect parentBounds = Parent()->Bounds();
		ConvertFromParent(&parentBounds);
		bounds = bounds & parentBounds;
		
		if (bounds.Contains(where) && Window()->IsActive()) {
			BMessage message(MSG_PACKAGE_SELECTED);
			message.AddString("title", PackageTitle());
			Window()->PostMessage(&message);
		}
	}

	void SetPackage(const PackageInfoRef& package)
	{
		fPackageListener->SetPackage(package);

		if (package->Icon().Get() != NULL) {
			fIconView->SetBitmap(
				package->Icon()->Bitmap(SharedBitmap::SIZE_64));
		} else
			fIconView->SetBitmap(NULL);

		if (package->State() == ACTIVATED) {
			fInstalledIconView->SetBitmap(
				sInstalledIcon->Bitmap(SharedBitmap::SIZE_16));
		} else
			fInstalledIconView->SetBitmap(NULL);

		fTitleView->SetText(package->Title());

		BString publisher = package->Publisher().Name();
		fPublisherView->SetText(publisher);

		BString summary = package->ShortDescription();
		fSummaryView->SetText(summary);

		RatingSummary ratingSummary = package->CalculateRatingSummary();

		fRatingView->SetRating(ratingSummary.averageRating);

		if (ratingSummary.ratingCount > 0) {
			BString avgRating;
			avgRating.SetToFormat("%.1f", ratingSummary.averageRating);
			fAvgRating->SetText(avgRating);

			BString votes;
			votes.SetToFormat("%d", ratingSummary.ratingCount);

			BString voteInfo(B_TRANSLATE("(%Votes%)"));
			voteInfo.ReplaceAll("%Votes%", votes);

			fVoteInfo->SetText(voteInfo);
		} else {
			fAvgRating->SetText("");
			fVoteInfo->SetText("");
		}

		InvalidateLayout();
		Invalidate();
	}

	void Clear()
	{
		fPackageListener->SetPackage(PackageInfoRef(NULL));

		fIconView->SetBitmap(NULL);
		fInstalledIconView->SetBitmap(NULL);
		fTitleView->SetText("");
		fPublisherView->SetText("");
		fSummaryView->SetText("");
		fRatingView->SetRating(-1.0f);
		fAvgRating->SetText("");
		fVoteInfo->SetText("");
	}

	const char* PackageTitle() const
	{
		return fTitleView->Text();
	}

	void SetSelected(bool selected)
	{
		if (fSelected == selected)
			return;
		fSelected = selected;
		
		rgb_color bgColor;
		if (fSelected)
			bgColor = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		else
			bgColor = (rgb_color){ 255, 255, 255, 255 };
		
		List<BView*, true> views;

		views.Add(this);
		views.Add(fIconView);
		views.Add(fInstalledIconView);
		views.Add(fTitleView);
		views.Add(fPublisherView);
		views.Add(fSummaryView);
		views.Add(fRatingView);
		views.Add(fAvgRating);
		views.Add(fVoteInfo);
		
		for (int32 i = 0; i < views.CountItems(); i++) {
			BView* view = views.ItemAtFast(i);
		
			view->SetViewColor(bgColor);
			view->SetLowColor(bgColor);
			view->Invalidate();
		}
	}

private:
	OnePackageMessagePackageListener* fPackageListener;

	BitmapView*						fIconView;
	BitmapView*						fInstalledIconView;

	BStringView*					fTitleView;
	BStringView*					fPublisherView;

	MarkupTextView*					fSummaryView;

	RatingView*						fRatingView;
	BStringView*					fAvgRating;
	BStringView*					fVoteInfo;

	bool							fSelected;

};


// #pragma mark - FeaturedPackagesView


FeaturedPackagesView::FeaturedPackagesView()
	:
	BView("featured package view", 0)
{
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	SetLayout(layout);

	ScrollableGroupView* containerView = new ScrollableGroupView();
	containerView->SetViewColor(255, 255, 255);
	fPackageListLayout = containerView->GroupLayout();

	BScrollView* scrollView = new BScrollView(
		"featured packages scroll view", containerView,
		0, false, true, B_FANCY_BORDER);

	BScrollBar* scrollBar = scrollView->ScrollBar(B_VERTICAL);
	if (scrollBar != NULL)
		scrollBar->SetSteps(10.0f, 20.0f);

	BLayoutBuilder::Group<>(this)
		.Add(scrollView, 1.0f)
	;
}


FeaturedPackagesView::~FeaturedPackagesView()
{
}


void
FeaturedPackagesView::AddPackage(const PackageInfoRef& package)
{
	// Find insertion index (alphabetical)
	int32 index = 0;
	for (int32 i = 0; BLayoutItem* item = fPackageListLayout->ItemAt(i); i++) {
		PackageView* view = dynamic_cast<PackageView*>(item->View());
		if (view == NULL)
			break;

		BString title = view->PackageTitle();
		if (title == package->Title()) {
			// Don't add packages more than once
			return;
		}

		if (title.Compare(package->Title()) < 0)
			index++;
	}

	PackageView* view = new PackageView();
	view->SetPackage(package);

	fPackageListLayout->AddView(index, view);
}


void
FeaturedPackagesView::Clear()
{
	for (int32 i = fPackageListLayout->CountItems() - 1;
			BLayoutItem* item = fPackageListLayout->ItemAt(i); i--) {
		BView* view = dynamic_cast<PackageView*>(item->View());
		if (view != NULL) {
			view->RemoveSelf();
			delete view;
		}
	}
}


void
FeaturedPackagesView::SelectPackage(const PackageInfoRef& package)
{
	BString selectedTitle;
	if (package.Get() != NULL)
		selectedTitle = package->Title();
	
	for (int32 i = 0; BLayoutItem* item = fPackageListLayout->ItemAt(i); i++) {
		PackageView* view = dynamic_cast<PackageView*>(item->View());
		if (view == NULL)
			break;

		BString title = view->PackageTitle();
		view->SetSelected(title == selectedTitle);
	}
}


void
FeaturedPackagesView::CleanupIcons()
{
	sInstalledIcon.Unset();
}
