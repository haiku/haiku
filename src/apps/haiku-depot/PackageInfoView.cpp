/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfoView.h"

#include <algorithm>
#include <stdio.h>

#include <Bitmap.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <TabView.h>
#include <ScrollView.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>

#include "PackageManager.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageInfoView"


static const rgb_color kLightBlack = (rgb_color){ 60, 60, 60, 255 };
static const float kContentTint = (B_NO_TINT + B_LIGHTEN_1_TINT) / 2.0f;


class BitmapView : public BView {
public:
	BitmapView(const char* name)
		:
		BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
		fBitmap(NULL)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetDrawingMode(B_OP_OVER);
	}
	
	virtual ~BitmapView()
	{
	}

	virtual void AttachedToWindow()
	{
		BView* parent = Parent();
		if (parent != NULL)
			SetLowColor(parent->ViewColor());
	}

	virtual void Draw(BRect updateRect)
	{
		BRect bounds(Bounds());
		FillRect(updateRect, B_SOLID_LOW);
		
		if (fBitmap == NULL)
			return;

		BRect bitmapBounds = fBitmap->Bounds();
		if (bitmapBounds.Width() <= 0.0f || bitmapBounds.Height() <= 0.0f)
			return;
		
		float hScale = bounds.Width() / bitmapBounds.Width();
		float vScale = bounds.Height() / bitmapBounds.Height();
		
		float scale = std::min(hScale, vScale);
		
		float width = bitmapBounds.Width() * scale;
		float height = bitmapBounds.Height() * scale;
		
		switch (LayoutAlignment().horizontal) {
			case B_ALIGN_LEFT:
				break;
			case B_ALIGN_RIGHT:
				bounds.left = floorf(bounds.right - width);
				break;
			default:
			case B_ALIGN_HORIZONTAL_CENTER:
				bounds.left = floorf(bounds.left
					+ (bounds.Width() - width) / 2.0f);
				break;
		}
		switch (LayoutAlignment().vertical) {
			case B_ALIGN_TOP:
				break;
			case B_ALIGN_BOTTOM:
				bounds.top = floorf(bounds.bottom - height);
				break;
			default:
			case B_ALIGN_VERTICAL_CENTER:
				bounds.top = floorf(bounds.top
					+ (bounds.Height() - height) / 2.0f);
				break;
		}
		
		bounds.right = ceilf(bounds.left + width);
		bounds.bottom = ceilf(bounds.top + height);

		DrawBitmap(fBitmap, bitmapBounds, bounds, B_FILTER_BITMAP_BILINEAR);
	}

	virtual BSize MinSize()
	{
		BSize size(0.0f, 0.0f);
		
		if (fBitmap != NULL) {
			BRect bounds = fBitmap->Bounds();
			size.width = bounds.Width();
			size.height = bounds.Height();
		}
		
		return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
	}

	virtual BSize PreferredSize()
	{
		BSize size = MinSize();
		return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
	}
	
	virtual BSize MaxSize()
	{
		BSize size = MinSize();
		return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
	}

//	virtual	bool HasHeightForWidth()
//	{
//		return true;
//	}
//
//	virtual void GetHeightForWidth(float width, float* min, float* max,
//		float* preferred)
//	{
//		float height = width;
//		
//		if (fBitmap != NULL) {
//			BRect bounds = fBitmap->Bounds();
//			if (bounds.Width() > 0.0f && bounds.Height() > 0.0f)
//				height = (width / bounds.Width()) * bounds.Height();
//		}
//
//		if (min != NULL)
//			*min = height;
//		if (max != NULL)
//			*max = height;
//		if (preferred != NULL)
//			*preferred = height;
//	}
	
	void SetBitmap(const BBitmap* bitmap)
	{
		if (bitmap == fBitmap)
			return;

		BSize size = MinSize();

		fBitmap = bitmap;
		
		BSize newSize = MinSize();
		if (size != newSize)
			InvalidateLayout();
		
		Invalidate();
	}

private:
	const BBitmap*	fBitmap;
};


//! Layouts the scrollbar so it looks nice with no border and the document
// window look.
class CustomScrollView : public BScrollView {
public:
	CustomScrollView(const char* name, BView* target)
		:
		BScrollView(name, target, 0, false, true, B_NO_BORDER)
	{
	}

	virtual void DoLayout()
	{
		BRect innerFrame = Bounds();
		innerFrame.right -= B_V_SCROLL_BAR_WIDTH + 1;
		
		BView* target = Target();
		if (target != NULL) {
			Target()->MoveTo(innerFrame.left, innerFrame.top);
			Target()->ResizeTo(innerFrame.Width(), innerFrame.Height());
		}
		
		BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
		if (scrollBar != NULL) {
			BRect rect = innerFrame;
			rect.left = rect.right + 1;
			rect.right = rect.left + B_V_SCROLL_BAR_WIDTH;
			rect.bottom -= B_H_SCROLL_BAR_HEIGHT;
	
			scrollBar->MoveTo(rect.left, rect.top);
			scrollBar->ResizeTo(rect.Width(), rect.Height());
		}
	}
};


// #pragma mark - AboutView


class RatingView : public BView {
public:
	RatingView()
		:
		BView("package rating view", B_WILL_DRAW),
		fStarBitmap(501),
		fRating(-1.0f)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}
	
	virtual ~RatingView()
	{
	}

	virtual void Draw(BRect updateRect)
	{
		FillRect(updateRect, B_SOLID_LOW);
		
		if (fRating < 0.0f)
			return;
		
		const BBitmap* star = fStarBitmap.Bitmap(SharedBitmap::SIZE_16);
		if (star == NULL) {
			fprintf(stderr, "No star icon found in application resources.\n");
			return;
		}
		
		SetDrawingMode(B_OP_OVER);
		
		float x = 0;
		for (int i = 0; i < 5; i++) {
			DrawBitmap(star, BPoint(x, 0));
			x += 16 + 2;
		}

		if (fRating >= 5.0f)
			return;

		SetDrawingMode(B_OP_OVER);
		
		BRect rect(Bounds());
		rect.left = ceilf(rect.left + (fRating / 5.0f) * rect.Width());
		
		rgb_color color = LowColor();
		color.alpha = 190;
		SetHighColor(color);
		
		SetDrawingMode(B_OP_ALPHA);
		FillRect(rect, B_SOLID_HIGH);
	}

	virtual BSize MinSize()
	{
		return BSize(16 * 5 + 2 * 4, 16 + 2);
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual BSize MaxSize()
	{
		return MinSize();
	}

	void SetRating(float rating)
	{
		fRating = rating;
		Invalidate();
	}

private:
	SharedBitmap	fStarBitmap;
	float			fRating;
};


// #pragma mark - TitleView


enum {
	MSG_PACKAGE_ACTION			= 'pkga',
};


class TitleView : public BGroupView {
public:
	TitleView()
		:
		BGroupView("title view", B_HORIZONTAL)
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
		fPublisherView->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT,
			B_ALIGN_VERTICAL_UNSET));

		// Rating view
		fRatingView = new RatingView();

		fAvgRating = new BStringView("package average rating", "");
		// small font
		GetFont(&font);
		font.SetSize(ceilf(font.Size() * 1.2f));
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
			.AddGroup(B_VERTICAL, 1.0f)
				.Add(fTitleView)
				.Add(fPublisherView)
			.End()
			.AddGlue(0.2f)
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
				.Add(fRatingView)
				.Add(fAvgRating)
				.Add(fVoteInfo)
			.End()
			.AddGlue(3.0f)
		;
	
		Clear();
	}
	
	virtual ~TitleView()
	{
	}

	void SetPackage(const PackageInfo& package)
	{
		if (package.Icon().Get() != NULL)
			fIconView->SetBitmap(package.Icon()->Bitmap(SharedBitmap::SIZE_32));
		else
			fIconView->SetBitmap(NULL);

		fTitleView->SetText(package.Title());
		
		BString publisher = B_TRANSLATE("by %Publisher%");
		publisher.ReplaceAll("%Publisher%", package.Publisher().Name());
		fPublisherView->SetText(publisher);

		RatingSummary ratingSummary = package.CalculateRatingSummary();

		fRatingView->SetRating(ratingSummary.averageRating);

		BString avgRating;
		avgRating.SetToFormat("%.1f", ratingSummary.averageRating);
		fAvgRating->SetText(avgRating);

		BString votes;
		votes.SetToFormat("%d", ratingSummary.ratingCount);

		BString voteInfo(B_TRANSLATE("(%Votes%)"));
		voteInfo.ReplaceAll("%Votes%", votes);

		fVoteInfo->SetText(voteInfo);

		InvalidateLayout();
		Invalidate();
	}

	void Clear()
	{
		fIconView->SetBitmap(NULL);
		fTitleView->SetText("");
		fPublisherView->SetText("");
		fRatingView->SetRating(-1.0f);
		fAvgRating->SetText("");
		fVoteInfo->SetText("");
	}

private:
	BitmapView*		fIconView;

	BStringView*	fTitleView;
	BStringView*	fPublisherView;

	RatingView*		fRatingView;
	BStringView*	fAvgRating;
	BStringView*	fVoteInfo;
};


// #pragma mark - PackageActionView


class PackageActionView : public BView {
public:
	PackageActionView(PackageManager* packageManager)
		:
		BView("about view", B_WILL_DRAW),
		fPackageManager(packageManager),
		fLayout(new BGroupLayout(B_HORIZONTAL))
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		
		SetLayout(fLayout);
	}
	
	virtual ~PackageActionView()
	{
		Clear();
	}
	
	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_PACKAGE_ACTION:
			{
				int32 index;
				if (message->FindInt32("index", &index) == B_OK) {
					const PackageActionRef& action
						= fPackageActions.ItemAt(index);
					if (action.Get() != NULL) {
						status_t result = action->Perform();
						if (result != B_OK) {
							fprintf(stderr, "Package action failed: %s '%s'\n",
								action->Label(),
								action->Package().Title().String());
						}
					}
				}
				break;
			}
			
			default:
				BView::MessageReceived(message);
				break;
		}
	}

	void SetPackage(const PackageInfo& package)
	{
		Clear();

		fPackageActions = fPackageManager->GetPackageActions(package);
		
		// Add Buttons in reverse action order
		for (int32 i = fPackageActions.CountItems() - 1; i >= 0; i--) {
			const PackageActionRef& action = fPackageActions.ItemAtFast(i);
			
			BMessage* message = new BMessage(MSG_PACKAGE_ACTION);
			message->AddInt32("index", i);
			
			BButton* button = new BButton(action->Label(), message);
			fLayout->AddView(button);
			button->SetTarget(this);
			
			fButtons.AddItem(button);
		}
	}

	void Clear()
	{
		for (int32 i = fButtons.CountItems() - 1; i >= 0; i--) {
			BButton* button = (BButton*)fButtons.ItemAtFast(i);
			button->RemoveSelf();
			delete button;
		}
		fButtons.MakeEmpty();
	}

private:
	PackageManager*		fPackageManager;
	
	BGroupLayout*		fLayout;
	PackageActionList	fPackageActions;
	BList				fButtons;
};


// #pragma mark - AboutView


class AboutView : public BView {
public:
	AboutView()
		:
		BView("about view", 0),
		fEmailIcon("text/x-email"),
		fWebsiteIcon("text/html")
	{
		SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			kContentTint));
		
		fDescriptionView = new BTextView("description view");
		fDescriptionView->SetViewColor(ViewColor());
		fDescriptionView->MakeEditable(false);
		const float textInset = be_plain_font->Size();
		fDescriptionView->SetInsets(textInset, textInset, textInset, 0.0f);
		
		BScrollView* scrollView = new CustomScrollView(
			"description scroll view", fDescriptionView);
		
		BFont smallFont;
		GetFont(&smallFont);
		smallFont.SetSize(std::max(9.0f, ceilf(smallFont.Size() * 0.85f)));
		
		fScreenshotView = new BitmapView("screenshot view");
		fScreenshotView->SetExplicitMinSize(BSize(64.0f, 64.0f));
		fScreenshotView->SetExplicitMaxSize(
			BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		fScreenshotView->SetExplicitAlignment(
			BAlignment(B_ALIGN_CENTER, B_ALIGN_TOP));
		
		fEmailIconView = new BitmapView("email icon view");
		fEmailLinkView = new BStringView("email link view", "");
		fEmailLinkView->SetFont(&smallFont);
		fEmailLinkView->SetHighColor(kLightBlack);

		fWebsiteIconView = new BitmapView("website icon view");
		fWebsiteLinkView = new BStringView("website link view", "");
		fWebsiteLinkView->SetFont(&smallFont);
		fWebsiteLinkView->SetHighColor(kLightBlack);
		
		BGroupView* leftGroup = new BGroupView(B_VERTICAL,
			B_USE_DEFAULT_SPACING);
		leftGroup->SetViewColor(ViewColor());
		
		BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0.0f)
//			.Add(BSpaceLayoutItem::CreateHorizontalStrut(32.0f))
			.AddGroup(leftGroup)
				.Add(fScreenshotView)
				.AddGroup(B_HORIZONTAL)
//					.AddGlue()
					.AddGrid(B_USE_HALF_ITEM_SPACING, B_USE_HALF_ITEM_SPACING)
						.Add(fEmailIconView, 0, 0)
						.Add(fEmailLinkView, 1, 0)
						.Add(fWebsiteIconView, 0, 1)
						.Add(fWebsiteLinkView, 1, 1)
					.End()
				.End()
				.SetInsets(B_USE_DEFAULT_SPACING)
			.End()
			.Add(scrollView, 1.0f)

			.SetExplicitMaxSize(BSize(B_SIZE_UNSET, B_SIZE_UNLIMITED))
			.SetInsets(0.0f, -1.0f, -1.0f, -1.0f)
		;
	}
	
	virtual ~AboutView()
	{
		Clear();
	}

	void SetPackage(const PackageInfo& package)
	{
		fDescriptionView->SetText(package.FullDescription());
		fEmailIconView->SetBitmap(fEmailIcon.Bitmap(SharedBitmap::SIZE_16));
		fEmailLinkView->SetText(package.Publisher().Email());
		fWebsiteIconView->SetBitmap(fWebsiteIcon.Bitmap(SharedBitmap::SIZE_16));
		fWebsiteLinkView->SetText(package.Publisher().Website());

		const BBitmap* screenshot = NULL;
		const BitmapList& screenShots = package.Screenshots();
		if (screenShots.CountItems() > 0) {
			const BitmapRef& bitmapRef = screenShots.ItemAtFast(0);
			if (bitmapRef.Get() != NULL)
				screenshot = bitmapRef->Bitmap(SharedBitmap::SIZE_ANY);
		}
		fScreenshotView->SetBitmap(screenshot);
	}

	void Clear()
	{
		fDescriptionView->SetText("");
		fEmailIconView->SetBitmap(NULL);
		fEmailLinkView->SetText("");
		fWebsiteIconView->SetBitmap(NULL);
		fWebsiteLinkView->SetText("");

		fScreenshotView->SetBitmap(NULL);
	}

private:
	BTextView*		fDescriptionView;

	BitmapView*		fScreenshotView;

	SharedBitmap	fEmailIcon;
	BitmapView*		fEmailIconView;
	BStringView*	fEmailLinkView;

	SharedBitmap	fWebsiteIcon;
	BitmapView*		fWebsiteIconView;
	BStringView*	fWebsiteLinkView;
};


// #pragma mark - UserRatingsView


class UserRatingsView : public BView {
public:
	UserRatingsView()
		:
		BView("package ratings view", 0),
		fLayout(new BGroupLayout(B_HORIZONTAL))
	{
		SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			kContentTint));
		
		SetLayout(fLayout);
		
		fTextView = new BTextView("ratings view");
		fTextView->SetViewColor(ViewColor());
		fTextView->MakeEditable(false);
		const float textInset = be_plain_font->Size();
		fTextView->SetInsets(textInset, textInset, textInset, 0.0f);
		
		BScrollView* scrollView = new CustomScrollView(
			"ratings scroll view", fTextView);
		
		BLayoutBuilder::Group<>(fLayout)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(32.0f))
			.Add(scrollView, 1.0f)
			.SetInsets(B_USE_DEFAULT_SPACING, -1.0f, -1.0f, -1.0f)
		;
	}
	
	virtual ~UserRatingsView()
	{
		Clear();
	}

	void SetPackage(const PackageInfo& package)
	{
		fTextView->SetText("");

		const UserRatingList& userRatings = package.UserRatings();
		
		// TODO: Sort by age or usefullness rating
		// TODO: Optionally hide ratings that are not in the system language
		
		for (int i = userRatings.CountItems() - 1; i >= 0; i--) {
			const UserRating& rating = userRatings.ItemAtFast(i);
			const BString& comment = rating.Comment();
			if (comment.Length() == 0)
				continue;
			
			int offset = fTextView->TextLength();
			if (offset > 0) {
				// Insert separator lines
				fTextView->Insert(offset, "\n\n", 2);
				offset += 2;
			}
			
			fTextView->Insert(offset, comment.String(), comment.Length());
		}
	}

	void Clear()
	{
		fTextView->SetText("");
	}

private:
	BGroupLayout*	fLayout;
	BTextView*		fTextView;
};


// #pragma mark - ChangelogView


class ChangelogView : public BView {
public:
	ChangelogView()
		:
		BView("package changelog view", B_WILL_DRAW),
		fLayout(new BGroupLayout(B_HORIZONTAL))
	{
		SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			kContentTint));
		
		SetLayout(fLayout);
		
		fTextView = new BTextView("changelog view");
		fTextView->SetViewColor(ViewColor());
		fTextView->MakeEditable(false);
		const float textInset = be_plain_font->Size();
		fTextView->SetInsets(textInset, textInset, textInset, 0.0f);
		
		BScrollView* scrollView = new CustomScrollView(
			"changelog scroll view", fTextView);
		
		BLayoutBuilder::Group<>(fLayout)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(32.0f))
			.Add(scrollView, 1.0f)
			.SetInsets(B_USE_DEFAULT_SPACING, -1.0f, -1.0f, -1.0f)
		;
	}
	
	virtual ~ChangelogView()
	{
		Clear();
	}

	virtual void Draw(BRect updateRect)
	{
	}

	void SetPackage(const PackageInfo& package)
	{
		fTextView->SetText(package.Changelog());
	}

	void Clear()
	{
		fTextView->SetText("");
	}

private:
	BGroupLayout*	fLayout;
	BTextView*		fTextView;
};


// #pragma mark - PagesView


class PagesView : public BTabView {
public:
	PagesView()
		:
		BTabView("pages view", B_WIDTH_FROM_WIDEST),
		fLayout(new BCardLayout())
	{
		SetBorder(B_NO_BORDER);
		
		fAboutView = new AboutView();
		fUserRatingsView = new UserRatingsView();
		fChangelogView = new ChangelogView();

		AddTab(fAboutView);
		AddTab(fUserRatingsView);
		AddTab(fChangelogView);
		
		TabAt(0)->SetLabel(B_TRANSLATE("About"));
		TabAt(1)->SetLabel(B_TRANSLATE("Ratings"));
		TabAt(2)->SetLabel(B_TRANSLATE("Changelog"));

		Select(0);
	}
	
	virtual ~PagesView()
	{
		Clear();
	}

	void SetPackage(const PackageInfo& package)
	{
		Select(0);
		fAboutView->SetPackage(package);
		fUserRatingsView->SetPackage(package);
		fChangelogView->SetPackage(package);
	}

	void Clear()
	{
		fAboutView->Clear();
		fUserRatingsView->Clear();
		fChangelogView->Clear();
	}

private:
	BCardLayout*		fLayout;
	
	AboutView*			fAboutView;
	UserRatingsView*	fUserRatingsView;
	ChangelogView*		fChangelogView;
};


// #pragma mark - PackageInfoView


PackageInfoView::PackageInfoView(PackageManager* packageManager)
	:
	BGroupView("package info view", B_VERTICAL)
{
	fTitleView = new TitleView();
	fPackageActionView = new PackageActionView(packageManager);
	fPagesView = new PagesView();

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(fTitleView)
			.Add(fPackageActionView)
			.SetInsets(
				B_USE_DEFAULT_SPACING, 0.0f,
				B_USE_DEFAULT_SPACING, 0.0f)
		.End()
		.Add(fPagesView)
	;

	Clear();
}


PackageInfoView::~PackageInfoView()
{
}


void
PackageInfoView::AttachedToWindow()
{
}


void
PackageInfoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
PackageInfoView::SetPackage(const PackageInfo& package)
{
	fTitleView->SetPackage(package);
	fPackageActionView->SetPackage(package);
	fPagesView->SetPackage(package);
	
	if (fPagesView->IsHidden(fPagesView))
		fPagesView->Show();
}


void
PackageInfoView::Clear()
{
	fTitleView->Clear();
	fPackageActionView->Clear();
	fPagesView->Clear();

	if (!fPagesView->IsHidden(fPagesView))
		fPagesView->Hide();
}

