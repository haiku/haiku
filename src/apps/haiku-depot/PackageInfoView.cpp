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
#include <Message.h>
#include <RadioButton.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageInfoView"


static const rgb_color kLightBlack = (rgb_color){ 60, 60, 60, 255 };


class BitmapView : public BView {
public:
	BitmapView(const char* name)
		:
		BView(name, B_WILL_DRAW),
		fBitmap(NULL)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetDrawingMode(B_OP_OVER);
	}
	
	virtual ~BitmapView()
	{
	}

	virtual void Draw(BRect updateRect)
	{
		BRect bounds(Bounds());
		FillRect(updateRect, B_SOLID_LOW);
		
		if (fBitmap == NULL)
			return;

		DrawBitmap(fBitmap, fBitmap->Bounds(), bounds);
	}

	virtual BSize MinSize()
	{
		BSize size(0.0f, 0.0f);
		
		if (fBitmap != NULL) {
			BRect bounds = fBitmap->Bounds();
			size.width = bounds.Width();
			size.height = bounds.Height();
		}
		
		return size;
	}

	virtual BSize MaxSize()
	{
		return MinSize();
	}
	
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
	MSG_SHOW_ABOUT_PAGE			= 'shap',
	MSG_SHOW_RATINGS_PAGE		= 'shrp',
	MSG_SHOW_CHANGELOG_PAGE		= 'shcp',
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

		fAboutPageButton = new BRadioButton("about page button",
			B_TRANSLATE("About"), new BMessage(MSG_SHOW_ABOUT_PAGE));
		fAboutPageButton->SetValue(B_CONTROL_ON);

		fRatingsPageButton = new BRadioButton("ratings page button",
			B_TRANSLATE("Ratings"), new BMessage(MSG_SHOW_RATINGS_PAGE));

		fChangelogPageButton = new BRadioButton("changelog page button",
			B_TRANSLATE("Changelog"), new BMessage(MSG_SHOW_CHANGELOG_PAGE));

		BLayoutBuilder::Group<>(this)
			.Add(fIconView)
			.AddGroup(B_VERTICAL, 1.0f)
				.Add(fTitleView)
				.Add(fPublisherView)
//				.SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET))
			.End()
			.AddGlue(0.2f)
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
				.Add(fRatingView)
				.Add(fAvgRating)
				.Add(fVoteInfo)
			.End()
			.AddGlue(3.0f)
			.Add(fAboutPageButton)
			.Add(fRatingsPageButton)
			.Add(fChangelogPageButton)
		;
	
		Clear();
	}
	
	virtual ~TitleView()
	{
	}

	void SetTarget(BHandler* handler)
	{
		fAboutPageButton->SetTarget(handler);
		fRatingsPageButton->SetTarget(handler);
		fChangelogPageButton->SetTarget(handler);
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

		if (fAboutPageButton->IsHidden(fAboutPageButton))
			fAboutPageButton->Show();
		if (fRatingsPageButton->IsHidden(fRatingsPageButton))
			fRatingsPageButton->Show();
		if (fChangelogPageButton->IsHidden(fChangelogPageButton))
			fChangelogPageButton->Show();

		fAboutPageButton->SetValue(B_CONTROL_ON);

		InvalidateLayout();
		Invalidate();
	}

	void Clear()
	{
		fTitleView->SetText("");
		fPublisherView->SetText("");
		fRatingView->SetRating(-1.0f);
		fAvgRating->SetText("");
		fVoteInfo->SetText("");
		
		if (!fAboutPageButton->IsHidden(fAboutPageButton))
			fAboutPageButton->Hide();
		if (!fRatingsPageButton->IsHidden(fRatingsPageButton))
			fRatingsPageButton->Hide();
		if (!fChangelogPageButton->IsHidden(fChangelogPageButton))
			fChangelogPageButton->Hide();
	}

private:
	BitmapView*		fIconView;

	BStringView*	fTitleView;
	BStringView*	fPublisherView;

	RatingView*		fRatingView;
	BStringView*	fAvgRating;
	BStringView*	fVoteInfo;

	BRadioButton*	fAboutPageButton;
	BRadioButton*	fRatingsPageButton;
	BRadioButton*	fChangelogPageButton;
};


// #pragma mark - AboutView


class AboutView : public BView {
public:
	AboutView()
		:
		BView("about view", B_WILL_DRAW),
		fLayout(new BGroupLayout(B_HORIZONTAL)),
		fEmailIcon("text/x-email"),
		fWebsiteIcon("text/html")
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		
		SetLayout(fLayout);
		
		fDescriptionView = new BTextView("description view");
		fDescriptionView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fDescriptionView->MakeEditable(false);
		
		BFont smallFont;
		GetFont(&smallFont);
		smallFont.SetSize(std::max(9.0f, ceilf(smallFont.Size() * 0.85f)));
		
		fEmailIconView = new BitmapView("email icon view");
		fEmailLinkView = new BStringView("email link view", "");
		fEmailLinkView->SetFont(&smallFont);
		fEmailLinkView->SetHighColor(kLightBlack);

		fWebsiteIconView = new BitmapView("website icon view");
		fWebsiteLinkView = new BStringView("website link view", "");
		fWebsiteLinkView->SetFont(&smallFont);
		fWebsiteLinkView->SetHighColor(kLightBlack);
		
		BLayoutBuilder::Group<>(fLayout)
			.Add(fDescriptionView, 1.0f)
			.AddGroup(B_VERTICAL, 0.0f)
				.AddGlue()
				.AddGroup(B_HORIZONTAL)
//					.AddGlue()
					.AddGrid(B_USE_HALF_ITEM_SPACING, B_USE_HALF_ITEM_SPACING)
						.Add(fEmailIconView, 0, 0)
						.Add(fEmailLinkView, 1, 0)
						.Add(fWebsiteIconView, 0, 1)
						.Add(fWebsiteLinkView, 1, 1)
					.End()
				.End()
			.End()
			.SetExplicitMaxSize(BSize(B_SIZE_UNSET, B_SIZE_UNLIMITED))
		;
	}
	
	virtual ~AboutView()
	{
		Clear();
	}

	virtual void Draw(BRect updateRect)
	{
	}

	void SetPackage(const PackageInfo& package)
	{
		fDescriptionView->SetText(package.FullDescription());
		fEmailIconView->SetBitmap(fEmailIcon.Bitmap(SharedBitmap::SIZE_16));
		fEmailLinkView->SetText(package.Publisher().Email());
		fWebsiteIconView->SetBitmap(fWebsiteIcon.Bitmap(SharedBitmap::SIZE_16));
		fWebsiteLinkView->SetText(package.Publisher().Website());
	}

	void Clear()
	{
		fDescriptionView->SetText("");
		fEmailIconView->SetBitmap(NULL);
		fEmailLinkView->SetText("");
		fWebsiteIconView->SetBitmap(NULL);
		fWebsiteLinkView->SetText("");
	}

private:
	BGroupLayout*	fLayout;
	BTextView*		fDescriptionView;

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
		BView("package ratings view", B_WILL_DRAW),
		fLayout(new BGroupLayout(B_HORIZONTAL))
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		
		SetLayout(fLayout);
		
		fTextView = new BTextView("ratings view");
		fTextView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fTextView->MakeEditable(false);
		
		BLayoutBuilder::Group<>(fLayout)
			.Add(fTextView, 1.0f)
		;
	}
	
	virtual ~UserRatingsView()
	{
		Clear();
	}

	virtual void Draw(BRect updateRect)
	{
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
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		
		SetLayout(fLayout);
		
		fTextView = new BTextView("changelog view");
		fTextView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fTextView->MakeEditable(false);
		
		BLayoutBuilder::Group<>(fLayout)
			.Add(fTextView, 1.0f)
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


class PagesView : public BView {
public:
	PagesView()
		:
		BView("pages view", 0),
		fLayout(new BCardLayout())
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetLayout(fLayout);
		
		fAboutView = new AboutView();
		fUserRatingsView = new UserRatingsView();
		fChangelogView = new ChangelogView();
		
		fLayout->AddView(fAboutView);
		fLayout->AddView(fUserRatingsView);
		fLayout->AddView(fChangelogView);
		
		fLayout->SetVisibleItem(0L);
	}
	
	virtual ~PagesView()
	{
		Clear();
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_SHOW_ABOUT_PAGE:
				fLayout->SetVisibleItem(0L);
				break;
			case MSG_SHOW_RATINGS_PAGE:
				fLayout->SetVisibleItem(1L);
				break;
			case MSG_SHOW_CHANGELOG_PAGE:
				fLayout->SetVisibleItem(2L);
				break;

			default:
				BView::MessageReceived(message);
				break;
		}
	}

	void SetPackage(const PackageInfo& package)
	{
		fLayout->SetVisibleItem(0L);
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


PackageInfoView::PackageInfoView()
	:
	BGroupView("package info view", B_VERTICAL)
{
	fTitleView = new TitleView();
	fPagesView = new PagesView();

	BLayoutBuilder::Group<>(this)
		.Add(fTitleView, 0.0f)
		.AddGroup(B_HORIZONTAL)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(32.0f))
			.Add(fPagesView)
		.End()
	;
}


PackageInfoView::~PackageInfoView()
{
}


void
PackageInfoView::AttachedToWindow()
{
	fTitleView->SetTarget(fPagesView);
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
	fPackageInfo = package;
	fTitleView->SetPackage(fPackageInfo);
	fPagesView->SetPackage(fPackageInfo);
}


void
PackageInfoView::Clear()
{
	fPackageInfo = PackageInfo();
	fTitleView->Clear();
	fPagesView->Clear();
}

