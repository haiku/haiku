/*
 * Copyright 2013-214, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "FeaturedPackageView.h"

#include <stdio.h>

#include <Catalog.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <ScrollView.h>
#include <StringView.h>
#include <SpaceLayoutItem.h>

#include "BitmapView.h"
#include "MessagePackageListener.h"
#include "RatingView.h"
#include "ScrollableGroupView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FeaturedPackageView"


static const rgb_color kLightBlack = (rgb_color){ 60, 60, 60, 255 };


// #pragma mark - PackageView


class PackageView : public BGroupView {
public:
	PackageView()
		:
		BGroupView("package view", B_HORIZONTAL),
		fPackageListener(new(std::nothrow) MessagePackageListener(this))
	{
		fIconView = new BitmapView("package icon view");
		fTitleView = new BStringView("package title view", "");
		fPublisherView = new BStringView("package publisher view", "");

		// Title font
		BFont font;
		GetFont(&font);
		font_family family;
		font_style style;
		font.SetSize(ceilf(font.Size() * 1.5f));
		font.GetFamilyAndStyle(&family, &style);
		font.SetFamilyAndStyle(family, "Bold");
		fTitleView->SetFont(&font);

		// Publisher font
		GetFont(&font);
		font.SetSize(std::max(9.0f, floorf(font.Size() * 0.92f)));
		font.SetFamilyAndStyle(family, "Italic");
		fPublisherView->SetFont(&font);
		fPublisherView->SetHighColor(kLightBlack);

		// slightly bigger font
		GetFont(&font);
		font.SetSize(ceilf(font.Size() * 1.2f));

		// Version info
		fVersionInfo = new BStringView("package version info", "");
		fVersionInfo->SetFont(&font);
		fVersionInfo->SetHighColor(kLightBlack);

		BLayoutBuilder::Group<>(this)
			.Add(fIconView)
			.AddGroup(B_VERTICAL, 1.0f, 2.2f)
				.Add(fTitleView)
				.Add(fPublisherView)
				.SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET))
			.End()
			.AddGlue(0.1f)
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING, 2.0f)
				.Add(fVersionInfo)
				.AddGlue()
				.SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET))
			.End()
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
			case MSG_UPDATE_PACKAGE:
			{
				SetPackage(fPackageListener->Package());
				break;
			}
		}
	}

	void SetPackage(const PackageInfoRef& package)
	{
		fPackageListener->SetPackage(package);

		if (package->Icon().Get() != NULL) {
			fIconView->SetBitmap(
				package->Icon()->Bitmap(SharedBitmap::SIZE_32));
		} else
			fIconView->SetBitmap(NULL);

		fTitleView->SetText(package->Title());

		BString publisher = package->Publisher().Name();
		fPublisherView->SetText(publisher);

		BString version = B_TRANSLATE("%Version%");
		version.ReplaceAll("%Version%", package->Version().ToString());
		fVersionInfo->SetText(version);

		InvalidateLayout();
		Invalidate();
	}

	void Clear()
	{
		fPackageListener->SetPackage(PackageInfoRef(NULL));

		fIconView->SetBitmap(NULL);
		fTitleView->SetText("");
		fPublisherView->SetText("");
		fVersionInfo->SetText("");
	}

private:
	MessagePackageListener*			fPackageListener;

	BitmapView*						fIconView;

	BStringView*					fTitleView;
	BStringView*					fPublisherView;

	BStringView*					fVersionInfo;
};


// #pragma mark - FeaturedPackageView


FeaturedPackageView::FeaturedPackageView()
	:
	BView("featured package view", 0)
{
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	SetLayout(layout);

	ScrollableGroupView* containerView = new ScrollableGroupView();
	containerView->SetViewColor(255, 255, 255);
	fPackageListLayout = containerView->GroupLayout();

	BScrollView* scrollView = new BScrollView(
		"packages scroll view", containerView);

	BLayoutBuilder::Group<>(this)
		.Add(scrollView, 1.0f)
	;
}


FeaturedPackageView::~FeaturedPackageView()
{
}


void
FeaturedPackageView::AddPackage(const PackageInfoRef& package)
{
	PackageView* view = new PackageView();
	view->SetPackage(package);
	fPackageListLayout->AddView(view);
}


void
FeaturedPackageView::Clear()
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

