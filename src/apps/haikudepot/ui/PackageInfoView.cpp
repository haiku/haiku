/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2018-2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfoView.h"

#include <algorithm>

#include <Alert.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <ColumnListView.h>
#include <Font.h>
#include <GridView.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <LocaleRoster.h>
#include <Message.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TabView.h>
#include <Url.h>

#include <package/hpkg/PackageReader.h>
#include <package/hpkg/NoErrorOutput.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageEntry.h>

#include "BitmapView.h"
#include "LinkView.h"
#include "LinkedBitmapView.h"
#include "LocaleUtils.h"
#include "Logger.h"
#include "MarkupTextView.h"
#include "MessagePackageListener.h"
#include "PackageContentsView.h"
#include "ProcessCoordinatorFactory.h"
#include "PackageInfo.h"
#include "PackageManager.h"
#include "RatingView.h"
#include "ScrollableGroupView.h"
#include "TextView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageInfoView"


enum {
	TAB_ABOUT		= 0,
	TAB_RATINGS		= 1,
	TAB_CHANGELOG	= 2,
	TAB_CONTENTS	= 3
};


static const float kContentTint = (B_NO_TINT + B_LIGHTEN_1_TINT) / 2.0f;


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


class RatingsScrollView : public CustomScrollView {
public:
	RatingsScrollView(const char* name, BView* target)
		:
		CustomScrollView(name, target)
	{
	}

	virtual void DoLayout()
	{
		CustomScrollView::DoLayout();

		BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
		BView* target = Target();
		if (target != NULL && scrollBar != NULL) {
			// Set the scroll steps
			BView* item = target->ChildAt(0);
			if (item != NULL) {
				scrollBar->SetSteps(item->MinSize().height + 1,
					item->MinSize().height + 1);
			}
		}
	}
};


// #pragma mark - rating stats


class DiagramBarView : public BView {
public:
	DiagramBarView()
		:
		BView("diagram bar view", B_WILL_DRAW),
		fValue(0.0f)
	{
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR, kContentTint);
		SetHighUIColor(B_CONTROL_MARK_COLOR);
	}

	virtual ~DiagramBarView()
	{
	}

	virtual void AttachedToWindow()
	{
	}

	virtual void Draw(BRect updateRect)
	{
		FillRect(updateRect, B_SOLID_LOW);

		if (fValue <= 0.0f)
			return;

		BRect rect(Bounds());
		rect.right = ceilf(rect.left + fValue * rect.Width());

		FillRect(rect, B_SOLID_HIGH);
	}

	virtual BSize MinSize()
	{
		return BSize(64, 10);
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual BSize MaxSize()
	{
		return BSize(64, 10);
	}

	void SetValue(float value)
	{
		if (fValue != value) {
			fValue = value;
			Invalidate();
		}
	}

private:
	float			fValue;
};


// #pragma mark - TitleView


enum {
	MSG_MOUSE_ENTERED_RATING	= 'menr',
	MSG_MOUSE_EXITED_RATING		= 'mexr',
};


class TransitReportingButton : public BButton {
public:
	TransitReportingButton(const char* name, const char* label,
			BMessage* message)
		:
		BButton(name, label, message),
		fTransitMessage(NULL)
	{
	}

	virtual ~TransitReportingButton()
	{
		SetTransitMessage(NULL);
	}

	virtual void MouseMoved(BPoint point, uint32 transit,
		const BMessage* dragMessage)
	{
		BButton::MouseMoved(point, transit, dragMessage);

		if (fTransitMessage != NULL && transit == B_EXITED_VIEW)
			Invoke(fTransitMessage);
	}

	void SetTransitMessage(BMessage* message)
	{
		if (fTransitMessage != message) {
			delete fTransitMessage;
			fTransitMessage = message;
		}
	}

private:
	BMessage*	fTransitMessage;
};


class TransitReportingRatingView : public RatingView, public BInvoker {
public:
	TransitReportingRatingView(BMessage* transitMessage)
		:
		RatingView("package rating view"),
		fTransitMessage(transitMessage)
	{
	}

	virtual ~TransitReportingRatingView()
	{
		delete fTransitMessage;
	}

	virtual void MouseMoved(BPoint point, uint32 transit,
		const BMessage* dragMessage)
	{
		RatingView::MouseMoved(point, transit, dragMessage);

		if (fTransitMessage != NULL && transit == B_ENTERED_VIEW)
			Invoke(fTransitMessage);
	}

private:
	BMessage*	fTransitMessage;
};


class TitleView : public BGroupView {
public:
	TitleView(PackageIconRepository& packageIconRepository)
		:
		BGroupView("title view", B_HORIZONTAL),
		fPackageIconRepository(packageIconRepository)
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
		fPublisherView->SetHighUIColor(B_PANEL_TEXT_COLOR, B_LIGHTEN_1_TINT);

		// slightly bigger font
		GetFont(&font);
		font.SetSize(ceilf(font.Size() * 1.2f));

		// Version info
		fVersionInfo = new BStringView("package version info", "");
		fVersionInfo->SetFont(&font);
		fVersionInfo->SetHighUIColor(B_PANEL_TEXT_COLOR, B_LIGHTEN_1_TINT);

		// Rating view
		fRatingView = new TransitReportingRatingView(
			new BMessage(MSG_MOUSE_ENTERED_RATING));

		fAvgRating = new BStringView("package average rating", "");
		fAvgRating->SetFont(&font);
		fAvgRating->SetHighUIColor(B_PANEL_TEXT_COLOR, B_LIGHTEN_1_TINT);

		fVoteInfo = new BStringView("package vote info", "");
		// small font
		GetFont(&font);
		font.SetSize(std::max(9.0f, floorf(font.Size() * 0.85f)));
		fVoteInfo->SetFont(&font);
		fVoteInfo->SetHighUIColor(B_PANEL_TEXT_COLOR, B_LIGHTEN_1_TINT);

		// Rate button
		fRateButton = new TransitReportingButton("rate",
			B_TRANSLATE("Rate package" B_UTF8_ELLIPSIS),
			new BMessage(MSG_RATE_PACKAGE));
		fRateButton->SetTransitMessage(new BMessage(MSG_MOUSE_EXITED_RATING));
		fRateButton->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
			B_ALIGN_VERTICAL_CENTER));

		// Rating group
		BView* ratingStack = new BView("rating stack", 0);
		fRatingLayout = new BCardLayout();
		ratingStack->SetLayout(fRatingLayout);
		ratingStack->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
		ratingStack->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

		BGroupView* ratingGroup = new BGroupView(B_HORIZONTAL,
			B_USE_SMALL_SPACING);
		BLayoutBuilder::Group<>(ratingGroup)
			.Add(fRatingView)
			.Add(fAvgRating)
			.Add(fVoteInfo)
		;

		ratingStack->AddChild(ratingGroup);
		ratingStack->AddChild(fRateButton);
		fRatingLayout->SetVisibleItem((int32)0);

		BLayoutBuilder::Group<>(this)
			.Add(fIconView)
			.AddGroup(B_VERTICAL, 1.0f, 2.2f)
				.Add(fTitleView)
				.Add(fPublisherView)
				.SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET))
			.End()
			.AddGlue(0.1f)
			.Add(ratingStack, 0.8f)
			.AddGlue(0.2f)
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING, 2.0f)
				.Add(fVersionInfo)
				.AddGlue()
				.SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET))
			.End()
		;

		Clear();
	}

	virtual ~TitleView()
	{
	}

	virtual void AttachedToWindow()
	{
		fRateButton->SetTarget(this);
		fRatingView->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_RATE_PACKAGE:
				// Forward to window (The button has us as target so
				// we receive the message below.)
				Window()->PostMessage(MSG_RATE_PACKAGE);
				break;

			case MSG_MOUSE_ENTERED_RATING:
				fRatingLayout->SetVisibleItem(1);
				break;

			case MSG_MOUSE_EXITED_RATING:
				fRatingLayout->SetVisibleItem((int32)0);
				break;
		}
	}

	void SetPackage(const PackageInfoRef package)
	{
		BitmapRef bitmap;
		status_t iconResult = fPackageIconRepository.GetIcon(
			package->Name(), BITMAP_SIZE_64, bitmap);

		if (iconResult == B_OK)
			fIconView->SetBitmap(bitmap, BITMAP_SIZE_32);
		else
			fIconView->UnsetBitmap();

		fTitleView->SetText(package->Title());

		BString publisher = package->Publisher().Name();
		if (publisher.CountChars() > 45) {
			fPublisherView->SetToolTip(publisher);
			fPublisherView->SetText(publisher.TruncateChars(45)
				.Append(B_UTF8_ELLIPSIS));
		} else
			fPublisherView->SetText(publisher);

		fVersionInfo->SetText(package->Version().ToString());

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
			fVoteInfo->SetText(B_TRANSLATE("n/a"));
		}

		InvalidateLayout();
		Invalidate();
	}

	void Clear()
	{
		fIconView->UnsetBitmap();
		fTitleView->SetText("");
		fPublisherView->SetText("");
		fVersionInfo->SetText("");
		fRatingView->SetRating(-1.0f);
		fAvgRating->SetText("");
		fVoteInfo->SetText("");
	}

private:
	PackageIconRepository&			fPackageIconRepository;

	BitmapView*						fIconView;

	BStringView*					fTitleView;
	BStringView*					fPublisherView;

	BStringView*					fVersionInfo;

	BCardLayout*					fRatingLayout;

	TransitReportingRatingView*		fRatingView;
	BStringView*					fAvgRating;
	BStringView*					fVoteInfo;

	TransitReportingButton*			fRateButton;
};


// #pragma mark - PackageActionView


class PackageActionView : public BView {
public:
	PackageActionView(ProcessCoordinatorConsumer* processCoordinatorConsumer,
			Model* model)
		:
		BView("about view", B_WILL_DRAW),
		fModel(model),
		fLayout(new BGroupLayout(B_HORIZONTAL)),
		fProcessCoordinatorConsumer(processCoordinatorConsumer),
		fStatusLabel(NULL),
		fStatusBar(NULL)
	{
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		SetLayout(fLayout);
		fLayout->AddItem(BSpaceLayoutItem::CreateGlue());
	}

	virtual ~PackageActionView()
	{
		Clear();
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_PKG_INSTALL:
			case MSG_PKG_UNINSTALL:
			case MSG_PKG_OPEN:
				_RunPackageAction(message);
				break;
			default:
				BView::MessageReceived(message);
				break;
		}
	}

	void SetPackage(const PackageInfoRef package)
	{
		if (package->State() == DOWNLOADING) {
			AdoptDownloadProgress(package);
		} else {
			AdoptActions(package);
		}
	}

	void AdoptActions(const PackageInfoRef package)
	{
		PackageManager manager(
			BPackageKit::B_PACKAGE_INSTALLATION_LOCATION_HOME);

		// TODO: if the given package is either a system package
		// or a system dependency, show a message indicating that status
		// so the user knows why no actions are presented
		std::vector<PackageActionRef> actions;
		VectorCollector<PackageActionRef> actionsCollector(actions);
		manager.CollectPackageActions(package, actionsCollector);

		if (_IsClearNeededToAdoptActions(actions)) {
			Clear();
			_CreateAllNewButtonsForAdoptActions(actions);
		} else {
			_UpdateExistingButtonsForAdoptActions(actions);
		}
	}

	void AdoptDownloadProgress(const PackageInfoRef package)
	{
		if (fButtons.CountItems() > 0)
			Clear();

		if (fStatusBar == NULL) {
			fStatusLabel = new BStringView("progress label",
				B_TRANSLATE("Downloading:"));
			fLayout->AddView(fStatusLabel);

			fStatusBar = new BStatusBar("progress");
			fStatusBar->SetMaxValue(100.0);
			fStatusBar->SetExplicitMinSize(
				BSize(StringWidth("XXX") * 5, B_SIZE_UNSET));

			fLayout->AddView(fStatusBar);
		}

		fStatusBar->SetTo(package->DownloadProgress() * 100.0);
	}

	void Clear()
	{
		for (int32 i = fButtons.CountItems() - 1; i >= 0; i--) {
			BButton* button = (BButton*)fButtons.ItemAtFast(i);
			button->RemoveSelf();
			delete button;
		}
		fButtons.MakeEmpty();

		if (fStatusBar != NULL) {
			fStatusBar->RemoveSelf();
			delete fStatusBar;
			fStatusBar = NULL;
		}
		if (fStatusLabel != NULL) {
			fStatusLabel->RemoveSelf();
			delete fStatusLabel;
			fStatusLabel = NULL;
		}
	}

private:
	bool _IsClearNeededToAdoptActions(std::vector<PackageActionRef> actions)
	{
		if (fStatusBar != NULL)
			return true;
		if (fButtons.CountItems() != static_cast<int32>(actions.size()))
			return true;
		return false;
	}

	void _UpdateExistingButtonsForAdoptActions(
		std::vector<PackageActionRef> actions)
	{
		int32 index = 0;
		for (int32 i = actions.size() - 1; i >= 0; i--) {
			const PackageActionRef& action = actions[i];
			BMessage* message = new BMessage(action->Message());
			BButton* button = (BButton*)fButtons.ItemAtFast(index++);
			button->SetLabel(action->Title());
			button->SetMessage(message);
		}
	}

	void _CreateAllNewButtonsForAdoptActions(
		std::vector<PackageActionRef> actions)
	{
		for (int32 i = actions.size() - 1; i >= 0; i--) {
			const PackageActionRef& action = actions[i];
			BMessage* message = new BMessage(action->Message());
			BButton* button = new BButton(action->Title(), message);
			fLayout->AddView(button);
			button->SetTarget(this);

			fButtons.AddItem(button);
		}
	}

	bool _MatchesPackageActionMessage(BButton *button, BMessage* message)
	{
		if (button == NULL)
			return false;
		BMessage* buttonMessage = button->Message();
		if (buttonMessage == NULL)
			return false;
		return buttonMessage == message;
	}

	/*!	Since the action has been fired; it should not be possible
		to run it again because this may make no sense.  For this
		reason, disable the corresponding button.
	*/

	void _DisableButtonForPackageActionMessage(BMessage* message)
	{
		for (int32 i = 0; i < fButtons.CountItems(); i++) {
			BButton* button = static_cast<BButton*>(fButtons.ItemAt(i));
			if (_MatchesPackageActionMessage(button, message))
				button->SetEnabled(false);
		}
	}

	void _RunPackageAction(BMessage* message)
	{
		ProcessCoordinator *processCoordinator =
			ProcessCoordinatorFactory::CreatePackageActionCoordinator(
				fModel, message);
		fProcessCoordinatorConsumer->Consume(processCoordinator);
		_DisableButtonForPackageActionMessage(message);
	}

private:
	Model*				fModel;
	BGroupLayout*		fLayout;
	ProcessCoordinatorConsumer*
						fProcessCoordinatorConsumer;
	BList				fButtons;

	BStringView*		fStatusLabel;
	BStatusBar*			fStatusBar;
};


// #pragma mark - AboutView


enum {
	MSG_EMAIL_PUBLISHER				= 'emlp',
	MSG_VISIT_PUBLISHER_WEBSITE		= 'vpws',
};


class AboutView : public BView {
public:
	AboutView()
		:
		BView("about view", 0),
		fEmailIcon("text/x-email"),
		fWebsiteIcon("text/html")
	{
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR, kContentTint);

		fDescriptionView = new MarkupTextView("description view");
		fDescriptionView->SetViewUIColor(ViewUIColor(), kContentTint);
		fDescriptionView->SetInsets(be_plain_font->Size());

		BScrollView* scrollView = new CustomScrollView(
			"description scroll view", fDescriptionView);

		BFont smallFont;
		GetFont(&smallFont);
		smallFont.SetSize(std::max(9.0f, ceilf(smallFont.Size() * 0.85f)));

		// TODO: Clicking the screen shot view should open ShowImage with the
		// the screen shot. This could be done by writing the screen shot to
		// a temporary folder, launching ShowImage to display it, and writing
		// all other screenshots associated with the package to the same folder
		// so the user can use the ShowImage navigation to view the other
		// screenshots.
		fScreenshotView = new LinkedBitmapView("screenshot view",
			new BMessage(MSG_SHOW_SCREENSHOT));
		fScreenshotView->SetExplicitMinSize(BSize(64.0f, 64.0f));
		fScreenshotView->SetExplicitMaxSize(
			BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		fScreenshotView->SetExplicitAlignment(
			BAlignment(B_ALIGN_CENTER, B_ALIGN_TOP));

		fEmailIconView = new BitmapView("email icon view");
		fEmailLinkView = new LinkView("email link view", "",
			new BMessage(MSG_EMAIL_PUBLISHER));
		fEmailLinkView->SetFont(&smallFont);

		fWebsiteIconView = new BitmapView("website icon view");
		fWebsiteLinkView = new LinkView("website link view", "",
			new BMessage(MSG_VISIT_PUBLISHER_WEBSITE));
		fWebsiteLinkView->SetFont(&smallFont);

		BGroupView* leftGroup = new BGroupView(B_VERTICAL,
			B_USE_DEFAULT_SPACING);

		fScreenshotView->SetViewUIColor(ViewUIColor(), kContentTint);
		fEmailLinkView->SetViewUIColor(ViewUIColor(), kContentTint);
		fWebsiteLinkView->SetViewUIColor(ViewUIColor(), kContentTint);

		BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0.0f)
			.AddGroup(leftGroup, 1.0f)
				.Add(fScreenshotView)
				.AddGroup(B_HORIZONTAL)
					.AddGrid(B_USE_HALF_ITEM_SPACING, B_USE_HALF_ITEM_SPACING)
						.Add(fEmailIconView, 0, 0)
						.Add(fEmailLinkView, 1, 0)
						.Add(fWebsiteIconView, 0, 1)
						.Add(fWebsiteLinkView, 1, 1)
					.End()
				.End()
				.SetInsets(B_USE_DEFAULT_SPACING)
				.SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET))
			.End()
			.Add(scrollView, 2.0f)

			.SetExplicitMaxSize(BSize(B_SIZE_UNSET, B_SIZE_UNLIMITED))
			.SetInsets(0.0f, -1.0f, -1.0f, -1.0f)
		;
	}

	virtual ~AboutView()
	{
		Clear();
	}

	virtual void AttachedToWindow()
	{
		fScreenshotView->SetTarget(this);
		fEmailLinkView->SetTarget(this);
		fWebsiteLinkView->SetTarget(this);
	}

	virtual void AllAttached()
	{
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR, kContentTint);

		for (int32 index = 0; index < CountChildren(); ++index)
			ChildAt(index)->AdoptParentColors();
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_SHOW_SCREENSHOT:
			{
				// Forward to window for now
				Window()->PostMessage(message, Window());
				break;
			}

			case MSG_EMAIL_PUBLISHER:
			{
				// TODO: Implement. If memory serves, there is a
				// standard command line interface which mail apps should
				// support, i.e. to open a compose window with the TO: field
				// already set.
				break;
			}

			case MSG_VISIT_PUBLISHER_WEBSITE:
			{
				BUrl url(fWebsiteLinkView->Text());
				url.OpenWithPreferredApplication();
				break;
			}

			default:
				BView::MessageReceived(message);
				break;
		}
	}

	void SetPackage(const PackageInfoRef package)
	{
		fDescriptionView->SetText(package->ShortDescription(),
			package->FullDescription());

		fEmailIconView->SetBitmap(&fEmailIcon, BITMAP_SIZE_16);
		_SetContactInfo(fEmailLinkView, package->Publisher().Email());
		fWebsiteIconView->SetBitmap(&fWebsiteIcon, BITMAP_SIZE_16);
		_SetContactInfo(fWebsiteLinkView, package->Publisher().Website());

		int32 countScreenshots = package->CountScreenshots();
		bool hasScreenshot = false;
		if (countScreenshots > 0) {
			const BitmapRef& bitmapRef = package->ScreenshotAtIndex(0);
			if (bitmapRef.IsSet()) {
				HDDEBUG("did find screenshot for package [%s]",
					package->Name().String());
				hasScreenshot = true;
				fScreenshotView->SetBitmap(bitmapRef);
			}
		}
		else {
			HDTRACE("did not find screenshots for package [%s]",
				package->Name().String());
		}

		if (!hasScreenshot)
			fScreenshotView->UnsetBitmap();

		fScreenshotView->SetEnabled(hasScreenshot);
	}

	void Clear()
	{
		fDescriptionView->SetText("");
		fEmailIconView->UnsetBitmap();
		fEmailLinkView->SetText("");
		fWebsiteIconView->UnsetBitmap();
		fWebsiteLinkView->SetText("");

		fScreenshotView->UnsetBitmap();
		fScreenshotView->SetEnabled(false);
	}

private:
	void _SetContactInfo(LinkView* view, const BString& string)
	{
		if (string.Length() > 0) {
			view->SetText(string);
			view->SetEnabled(true);
		} else {
			view->SetText(B_TRANSLATE("<no info>"));
			view->SetEnabled(false);
		}
	}

private:
	MarkupTextView*		fDescriptionView;

	LinkedBitmapView*	fScreenshotView;

	SharedBitmap		fEmailIcon;
	BitmapView*			fEmailIconView;
	LinkView*			fEmailLinkView;

	SharedBitmap		fWebsiteIcon;
	BitmapView*			fWebsiteIconView;
	LinkView*			fWebsiteLinkView;
};


// #pragma mark - UserRatingsView


class RatingItemView : public BGroupView {
public:
	RatingItemView(const UserRatingRef rating)
		:
		BGroupView(B_HORIZONTAL, 0.0f)
	{
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR, kContentTint);

		BGroupLayout* verticalGroup = new BGroupLayout(B_VERTICAL, 0.0f);
		GroupLayout()->AddItem(verticalGroup);

		{
			BStringView* userNicknameView = new BStringView("user-nickname",
				rating->User().NickName());
			userNicknameView->SetFont(be_bold_font);
			verticalGroup->AddView(userNicknameView);
		}

		BGroupLayout* ratingGroup =
			new BGroupLayout(B_HORIZONTAL, B_USE_DEFAULT_SPACING);
		verticalGroup->AddItem(ratingGroup);

		if (rating->Rating() >= 0) {
			RatingView* ratingView = new RatingView("package rating view");
			ratingView->SetRating(rating->Rating());
			ratingGroup->AddView(ratingView);
		}

		{
			BString createTimestampPresentation =
				LocaleUtils::TimestampToDateTimeString(
					rating->CreateTimestamp());

			BString ratingContextDescription(
				B_TRANSLATE("%hd.timestamp% (version %hd.version%)"));
			ratingContextDescription.ReplaceAll("%hd.timestamp%",
				createTimestampPresentation);
			ratingContextDescription.ReplaceAll("%hd.version%",
				rating->PackageVersion());

			BStringView* ratingContextView = new BStringView("rating-context",
				ratingContextDescription);
			BFont versionFont(be_plain_font);
			ratingContextView->SetFont(&versionFont);
			ratingGroup->AddView(ratingContextView);
		}

		ratingGroup->AddItem(BSpaceLayoutItem::CreateGlue());

		if (rating->Comment() > 0) {
			TextView* textView = new TextView("rating-text");
			ParagraphStyle paragraphStyle(textView->ParagraphStyle());
			paragraphStyle.SetJustify(true);
			textView->SetParagraphStyle(paragraphStyle);
			textView->SetText(rating->Comment());
			verticalGroup->AddItem(BSpaceLayoutItem::CreateVerticalStrut(8.0f));
			verticalGroup->AddView(textView);
			verticalGroup->AddItem(BSpaceLayoutItem::CreateVerticalStrut(8.0f));
		}

		verticalGroup->SetInsets(B_USE_DEFAULT_SPACING);

		SetFlags(Flags() | B_WILL_DRAW);
	}

	void AllAttached()
	{
		for (int32 index = 0; index < CountChildren(); ++index)
			ChildAt(index)->AdoptParentColors();
	}

	void Draw(BRect rect)
	{
		rgb_color color = mix_color(ViewColor(), ui_color(B_PANEL_TEXT_COLOR), 64);
		SetHighColor(color);
		StrokeLine(Bounds().LeftBottom(), Bounds().RightBottom());
	}

};


class RatingSummaryView : public BGridView {
public:
	RatingSummaryView()
		:
		BGridView("rating summary view", B_USE_HALF_ITEM_SPACING, 0.0f)
	{
		float tint = kContentTint - 0.1;
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR, tint);

		BLayoutBuilder::Grid<> layoutBuilder(this);

		BFont smallFont;
		GetFont(&smallFont);
		smallFont.SetSize(std::max(9.0f, floorf(smallFont.Size() * 0.85f)));

		for (int32 i = 0; i < 5; i++) {
			BString label;
			label.SetToFormat("%" B_PRId32, 5 - i);
			fLabelViews[i] = new BStringView("", label);
			fLabelViews[i]->SetFont(&smallFont);
			fLabelViews[i]->SetViewUIColor(ViewUIColor(), tint);
			layoutBuilder.Add(fLabelViews[i], 0, i);

			fDiagramBarViews[i] = new DiagramBarView();
			layoutBuilder.Add(fDiagramBarViews[i], 1, i);

			fCountViews[i] = new BStringView("", "");
			fCountViews[i]->SetFont(&smallFont);
			fCountViews[i]->SetViewUIColor(ViewUIColor(), tint);
			fCountViews[i]->SetAlignment(B_ALIGN_RIGHT);
			layoutBuilder.Add(fCountViews[i], 2, i);
		}

		layoutBuilder.SetInsets(5);
	}

	void SetToSummary(const RatingSummary& summary) {
		for (int32 i = 0; i < 5; i++) {
			int32 count = summary.ratingCountByStar[4 - i];

			BString label;
			label.SetToFormat("%" B_PRId32, count);
			fCountViews[i]->SetText(label);

			if (summary.ratingCount > 0) {
				fDiagramBarViews[i]->SetValue(
					(float)count / summary.ratingCount);
			} else
				fDiagramBarViews[i]->SetValue(0.0f);
		}
	}

	void Clear() {
		for (int32 i = 0; i < 5; i++) {
			fCountViews[i]->SetText("");
			fDiagramBarViews[i]->SetValue(0.0f);
		}
	}

private:
	BStringView*	fLabelViews[5];
	DiagramBarView*	fDiagramBarViews[5];
	BStringView*	fCountViews[5];
};


class UserRatingsView : public BGroupView {
public:
	UserRatingsView()
		:
		BGroupView("package ratings view", B_HORIZONTAL)
	{
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR, kContentTint);

		fRatingSummaryView = new RatingSummaryView();

		ScrollableGroupView* ratingsContainerView = new ScrollableGroupView();
		ratingsContainerView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR,
												kContentTint);
		fRatingContainerLayout = ratingsContainerView->GroupLayout();

		BScrollView* scrollView = new RatingsScrollView(
			"ratings scroll view", ratingsContainerView);
		scrollView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
			B_SIZE_UNLIMITED));

		BLayoutBuilder::Group<>(this)
			.AddGroup(B_VERTICAL)
				.Add(fRatingSummaryView, 0.0f)
				.AddGlue()
				.SetInsets(0.0f, B_USE_DEFAULT_SPACING, 0.0f, 0.0f)
			.End()
			.AddStrut(64.0)
			.Add(scrollView, 1.0f)
			.SetInsets(B_USE_DEFAULT_SPACING, -1.0f, -1.0f, -1.0f)
		;
	}

	virtual ~UserRatingsView()
	{
		Clear();
	}

	void SetPackage(const PackageInfoRef package)
	{
		ClearRatings();

		// TODO: Re-use rating summary already used for TitleView...
		fRatingSummaryView->SetToSummary(package->CalculateRatingSummary());

		int count = package->CountUserRatings();
		if (count == 0) {
			BStringView* noRatingsView = new BStringView("no ratings",
				B_TRANSLATE("No user ratings available."));
			noRatingsView->SetViewUIColor(ViewUIColor(), kContentTint);
			noRatingsView->SetAlignment(B_ALIGN_CENTER);
			noRatingsView->SetHighColor(disable_color(ui_color(B_PANEL_TEXT_COLOR),
				ViewColor()));
			noRatingsView->SetExplicitMaxSize(
				BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
			fRatingContainerLayout->AddView(0, noRatingsView);
			return;
		}

		for (int i = count - 1; i >= 0; i--) {
			UserRatingRef rating = package->UserRatingAtIndex(i);
				// was previously filtering comments just for the current
				// user's language, but as there are not so many comments at
				// the moment, just show all of them for now.
			RatingItemView* view = new RatingItemView(rating);
			fRatingContainerLayout->AddView(0, view);
		}
	}

	void Clear()
	{
		fRatingSummaryView->Clear();
		ClearRatings();
	}

	void ClearRatings()
	{
		for (int32 i = fRatingContainerLayout->CountItems() - 1;
				BLayoutItem* item = fRatingContainerLayout->ItemAt(i); i--) {
			BView* view = dynamic_cast<RatingItemView*>(item->View());
			if (view == NULL)
				view = dynamic_cast<BStringView*>(item->View());
			if (view != NULL) {
				view->RemoveSelf();
				delete view;
			}
		}
	}

private:
	BGroupLayout*			fRatingContainerLayout;
	RatingSummaryView*		fRatingSummaryView;
};


// #pragma mark - ContentsView


class ContentsView : public BGroupView {
public:
	ContentsView()
		:
		BGroupView("package contents view", B_HORIZONTAL)
	{
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR, kContentTint);

		fPackageContents = new PackageContentsView("contents_list");
		AddChild(fPackageContents);

	}

	virtual ~ContentsView()
	{
	}

	virtual void Draw(BRect updateRect)
	{
	}

	void SetPackage(const PackageInfoRef package)
	{
		fPackageContents->SetPackage(package);
	}

	void Clear()
	{
		fPackageContents->Clear();
	}

private:
	PackageContentsView*	fPackageContents;
};


// #pragma mark - ChangelogView


class ChangelogView : public BGroupView {
public:
	ChangelogView()
		:
		BGroupView("package changelog view", B_HORIZONTAL)
	{
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR, kContentTint);

		fTextView = new MarkupTextView("changelog view");
		fTextView->SetLowUIColor(ViewUIColor());
		fTextView->SetInsets(be_plain_font->Size());

		BScrollView* scrollView = new CustomScrollView(
			"changelog scroll view", fTextView);

		BLayoutBuilder::Group<>(this)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(32.0f))
			.Add(scrollView, 1.0f)
			.SetInsets(B_USE_DEFAULT_SPACING, -1.0f, -1.0f, -1.0f)
		;
	}

	virtual ~ChangelogView()
	{
	}

	virtual void Draw(BRect updateRect)
	{
	}

	void SetPackage(const PackageInfoRef package)
	{
		const BString& changelog = package->Changelog();
		if (changelog.Length() > 0)
			fTextView->SetText(changelog);
		else
			fTextView->SetDisabledText(B_TRANSLATE("No changelog available."));
	}

	void Clear()
	{
		fTextView->SetText("");
	}

private:
	MarkupTextView*		fTextView;
};


// #pragma mark - PagesView


class PagesView : public BTabView {
public:
	PagesView()
		:
		BTabView("pages view", B_WIDTH_FROM_WIDEST)
	{
		SetBorder(B_NO_BORDER);

		fAboutView = new AboutView();
		fUserRatingsView = new UserRatingsView();
		fChangelogView = new ChangelogView();
		fContentsView = new ContentsView();

		AddTab(fAboutView);
		AddTab(fUserRatingsView);
		AddTab(fChangelogView);
		AddTab(fContentsView);

		TabAt(TAB_ABOUT)->SetLabel(B_TRANSLATE("About"));
		TabAt(TAB_RATINGS)->SetLabel(B_TRANSLATE("Ratings"));
		TabAt(TAB_CHANGELOG)->SetLabel(B_TRANSLATE("Changelog"));
		TabAt(TAB_CONTENTS)->SetLabel(B_TRANSLATE("Contents"));

		Select(TAB_ABOUT);
	}

	virtual ~PagesView()
	{
		Clear();
	}

	void SetPackage(const PackageInfoRef package, bool switchToDefaultTab)
	{
		if (switchToDefaultTab)
			Select(TAB_ABOUT);

		TabAt(TAB_CHANGELOG)->SetEnabled(
			package.IsSet() && package->HasChangelog());
		TabAt(TAB_CONTENTS)->SetEnabled(
			package.IsSet()
				&& (package->State() == ACTIVATED || package->IsLocalFile()));
		Invalidate(TabFrame(TAB_CHANGELOG));
		Invalidate(TabFrame(TAB_CONTENTS));

		fAboutView->SetPackage(package);
		fUserRatingsView->SetPackage(package);
		fChangelogView->SetPackage(package);
		fContentsView->SetPackage(package);
	}

	void Clear()
	{
		fAboutView->Clear();
		fUserRatingsView->Clear();
		fChangelogView->Clear();
		fContentsView->Clear();
	}

private:
	AboutView*			fAboutView;
	UserRatingsView*	fUserRatingsView;
	ChangelogView*		fChangelogView;
	ContentsView* 		fContentsView;
};


// #pragma mark - PackageInfoView


PackageInfoView::PackageInfoView(Model* model,
		ProcessCoordinatorConsumer* processCoordinatorConsumer)
	:
	BView("package info view", 0),
	fModel(model),
	fPackageListener(new(std::nothrow) OnePackageMessagePackageListener(this))
{
	fCardLayout = new BCardLayout();
	SetLayout(fCardLayout);

	BGroupView* noPackageCard = new BGroupView("no package card", B_VERTICAL);
	AddChild(noPackageCard);

	BStringView* noPackageView = new BStringView("no package view",
		B_TRANSLATE("Click a package to view information"));
	noPackageView->SetHighUIColor(B_PANEL_TEXT_COLOR, B_LIGHTEN_1_TINT);
	noPackageView->SetExplicitAlignment(BAlignment(
		B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));

	BLayoutBuilder::Group<>(noPackageCard)
		.Add(noPackageView)
		.SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED))
	;

	BGroupView* packageCard = new BGroupView("package card", B_VERTICAL);
	AddChild(packageCard);

	fCardLayout->SetVisibleItem((int32)0);

	fTitleView = new TitleView(fModel->GetPackageIconRepository());
	fPackageActionView = new PackageActionView(processCoordinatorConsumer,
		model);
	fPackageActionView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNSET));
	fPagesView = new PagesView();

	BLayoutBuilder::Group<>(packageCard)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(fTitleView, 6.0f)
			.Add(fPackageActionView, 1.0f)
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
	fPackageListener->SetPackage(PackageInfoRef(NULL));
	delete fPackageListener;
}


void
PackageInfoView::AttachedToWindow()
{
}


void
PackageInfoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_UPDATE_PACKAGE:
		{
			if (!fPackageListener->Package().IsSet())
				break;

			BString name;
			uint32 changes;
			if (message->FindString("name", &name) != B_OK
				|| message->FindUInt32("changes", &changes) != B_OK) {
				break;
			}

			const PackageInfoRef& package = fPackageListener->Package();
			if (package->Name() != name)
				break;

			BAutolock _(fModel->Lock());

			if ((changes & PKG_CHANGED_SUMMARY) != 0
				|| (changes & PKG_CHANGED_DESCRIPTION) != 0
				|| (changes & PKG_CHANGED_SCREENSHOTS) != 0
				|| (changes & PKG_CHANGED_TITLE) != 0
				|| (changes & PKG_CHANGED_RATINGS) != 0
				|| (changes & PKG_CHANGED_STATE) != 0
				|| (changes & PKG_CHANGED_CHANGELOG) != 0) {
				fPagesView->SetPackage(package, false);
			}

			if ((changes & PKG_CHANGED_TITLE) != 0
				|| (changes & PKG_CHANGED_RATINGS) != 0) {
				fTitleView->SetPackage(package);
			}

			if ((changes & PKG_CHANGED_STATE) != 0)
				fPackageActionView->SetPackage(package);

			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
PackageInfoView::SetPackage(const PackageInfoRef& packageRef)
{
	BAutolock _(fModel->Lock());

	if (!packageRef.IsSet()) {
		Clear();
		return;
	}

	bool switchToDefaultTab = true;
	if (fPackage == packageRef) {
		// When asked to display the already showing package ref,
		// don't switch to the default tab.
		switchToDefaultTab = false;
	} else if (fPackage.IsSet() && packageRef.IsSet()
		&& fPackage->Name() == packageRef->Name()) {
		// When asked to display a different PackageInfo instance,
		// but it has the same package title as the already showing
		// instance, this probably means there was a repository
		// refresh and we are in fact still requested to show the
		// same package as before the refresh.
		switchToDefaultTab = false;
	}

	fTitleView->SetPackage(packageRef);
	fPackageActionView->SetPackage(packageRef);
	fPagesView->SetPackage(packageRef, switchToDefaultTab);

	fCardLayout->SetVisibleItem(1);

	fPackageListener->SetPackage(packageRef);

	// Set the fPackage reference last, so we keep a reference to the
	// previous package before switching all the views to the new package.
	// Otherwise the PackageInfo instance may go away because we had the
	// last reference. And some of the views, the PackageActionView for
	// example, keeps references to stuff from the previous package and
	// access it while switching to the new package.
	fPackage = packageRef;
}


void
PackageInfoView::Clear()
{
	BAutolock _(fModel->Lock());

	fTitleView->Clear();
	fPackageActionView->Clear();
	fPagesView->Clear();

	fCardLayout->SetVisibleItem((int32)0);

	fPackageListener->SetPackage(PackageInfoRef(NULL));

	fPackage.Unset();
}
