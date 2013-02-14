/*
 * Copyright 2007-2012 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood <leavengood@gmail.com>
 *		John Scipione <jscipione@gmail.com>
 */


#include <AboutWindow.h>

#include <stdarg.h>
#include <time.h>

#include <Alert.h>
#include <AppFileInfo.h>
#include <Bitmap.h>
#include <File.h>
#include <Font.h>
#include <GroupLayoutBuilder.h>
#include <GroupView.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Point.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <Size.h>
#include <String.h>
#include <StringView.h>
#include <SystemCatalog.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>


static const float kStripeWidth = 30.0;

using BPrivate::gSystemCatalog;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AboutWindow"


class StripeView : public BView {
 public:
							StripeView(BBitmap* icon);
	virtual					~StripeView();

	virtual	void			Draw(BRect updateRect);

			BBitmap*		Icon() const { return fIcon; };
			void			SetIcon(BBitmap* icon);

 private:
			BBitmap*		fIcon;
};


class AboutView : public BGroupView {
 public:
							AboutView(const char* name,
								const char* signature);
	virtual					~AboutView();

			BTextView*		InfoView() const { return fInfoView; };

			BBitmap*		Icon();
			status_t		SetIcon(BBitmap* icon);

			const char*		Name();
			status_t		SetName(const char* name);

			const char*		Version();
			status_t		SetVersion(const char* version);

 protected:
			const char*		_GetVersionFromSignature(const char* signature);
			BBitmap*		_GetIconFromSignature(const char* signature);

 private:
			BStringView*	fNameView;
			BStringView*	fVersionView;
			BTextView*		fInfoView;
			StripeView*		fStripeView;
};


//	#pragma mark -


StripeView::StripeView(BBitmap* icon)
	:
	BView("StripeView", B_WILL_DRAW),
	fIcon(icon)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	float width = 0.0f;
	if (icon != NULL)
		width += icon->Bounds().Width() + 32.0f;

	SetExplicitMinSize(BSize(width, B_SIZE_UNSET));
	SetExplicitPreferredSize(BSize(width, B_SIZE_UNLIMITED));
}


StripeView::~StripeView()
{
}


void
StripeView::Draw(BRect updateRect)
{
	if (fIcon == NULL)
		return;

	SetHighColor(ViewColor());
	FillRect(updateRect);

	BRect stripeRect = Bounds();
	stripeRect.right = kStripeWidth;
	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRect(stripeRect);

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmapAsync(fIcon, BPoint(15.0f, 10.0f));
}


void
StripeView::SetIcon(BBitmap* icon)
{
	if (fIcon != NULL)
		delete fIcon;

	fIcon = icon;

	float width = 0.0f;
	if (icon != NULL)
		width += icon->Bounds().Width() + 32.0f;

	SetExplicitMinSize(BSize(width, B_SIZE_UNSET));
	SetExplicitPreferredSize(BSize(width, B_SIZE_UNLIMITED));
};


//	#pragma mark - AboutView


AboutView::AboutView(const char* appName, const char* signature)
	:
	BGroupView("AboutView", B_VERTICAL)
{
	fNameView = new BStringView("name", appName);
	BFont font;
	fNameView->GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetSize(font.Size() * 2.0);
	fNameView->SetFont(&font, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE
		| B_FONT_FLAGS);

	fVersionView = new BStringView("version",
		_GetVersionFromSignature(signature));

	fInfoView = new BTextView("info", B_WILL_DRAW);
	fInfoView->SetExplicitMinSize(BSize(210.0, 160.0));
	fInfoView->MakeEditable(false);
	fInfoView->SetWordWrap(true);
	fInfoView->SetInsets(5.0, 5.0, 5.0, 5.0);
	fInfoView->SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
	fInfoView->SetHighColor(ui_color(B_DOCUMENT_TEXT_COLOR));

	BScrollView* infoViewScroller = new BScrollView(
		"infoViewScroller", fInfoView, B_WILL_DRAW | B_FRAME_EVENTS,
		false, true, B_PLAIN_BORDER);

	fStripeView = new StripeView(_GetIconFromSignature(signature));

	GroupLayout()->SetSpacing(0);
	BLayoutBuilder::Group<>(this)
		.AddGroup(B_HORIZONTAL, 0)
			.Add(fStripeView)
			.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
				.SetInsets(0, B_USE_DEFAULT_SPACING,
					B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
				.Add(fNameView)
				.Add(fVersionView)
				.Add(infoViewScroller)
				.End()
			.AddGlue()
			.End();
}


AboutView::~AboutView()
{
}


//	#pragma mark - AboutView protected methods


const char*
AboutView::_GetVersionFromSignature(const char* signature)
{
	if (signature == NULL)
		return NULL;

	entry_ref ref;
	if (be_roster->FindApp(signature, &ref) != B_OK)
		return NULL;

	BFile file(&ref, B_READ_ONLY);
	BAppFileInfo appMime(&file);
	if (appMime.InitCheck() != B_OK)
		return NULL;

	version_info versionInfo;
	if (appMime.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND) == B_OK) {
		if (versionInfo.major == 0 && versionInfo.middle == 0
			&& versionInfo.minor == 0) {
			return NULL;
		}

		const char* version = B_TRANSLATE_MARK("Version");
		version = gSystemCatalog.GetString(version, "AboutWindow");
		BString appVersion(version);
		appVersion << " " << versionInfo.major << "." << versionInfo.middle;
		if (versionInfo.minor > 0)
			appVersion << "." << versionInfo.minor;

		// Add the version variety
		const char* variety = NULL;
		switch (versionInfo.variety) {
			case B_DEVELOPMENT_VERSION:
				variety = B_TRANSLATE_MARK("development");
				break;
			case B_ALPHA_VERSION:
				variety = B_TRANSLATE_MARK("alpha");
				break;
			case B_BETA_VERSION:
				variety = B_TRANSLATE_MARK("beta");
				break;
			case B_GAMMA_VERSION:
				variety = B_TRANSLATE_MARK("gamma");
				break;
			case B_GOLDEN_MASTER_VERSION:
				variety = B_TRANSLATE_MARK("gold master");
				break;
		}

		if (variety != NULL) {
			variety = gSystemCatalog.GetString(variety, "AboutWindow");
			appVersion << "-" << variety;
		}

		return appVersion.String();
	}

	return NULL;
}


BBitmap*
AboutView::_GetIconFromSignature(const char* signature)
{
	if (signature == NULL)
		return NULL;

	entry_ref ref;
	if (be_roster->FindApp(signature, &ref) != B_OK)
		return NULL;

	BFile file(&ref, B_READ_ONLY);
	BAppFileInfo appMime(&file);
	if (appMime.InitCheck() != B_OK)
		return NULL;

	BBitmap* icon = new BBitmap(BRect(0.0, 0.0, 127.0, 127.0), B_RGBA32);
	if (appMime.GetIcon(icon, (icon_size)128) == B_OK)
		return icon;

	delete icon;
	return NULL;
}


//	#pragma mark - AboutView public methods


const char*
AboutView::Name()
{
	return fNameView->Text();
}


status_t
AboutView::SetName(const char* name)
{
	fNameView->SetText(name);

	return B_OK;
}


const char*
AboutView::Version()
{
	return fVersionView->Text();
}


status_t
AboutView::SetVersion(const char* version)
{
	fVersionView->SetText(version);

	return B_OK;
}


BBitmap*
AboutView::Icon()
{
	if (fStripeView == NULL)
		return NULL;

	return fStripeView->Icon();
}


status_t
AboutView::SetIcon(BBitmap* icon)
{
	if (fStripeView == NULL)
		return B_NO_INIT;

	fStripeView->SetIcon(icon);

	return B_OK;
}


//	#pragma mark - BAboutWindow


BAboutWindow::BAboutWindow(const char* appName, const char* signature)
	:	BWindow(BRect(0.0, 0.0, 200.0, 200.0), appName, B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	const char* about = B_TRANSLATE_MARK("About");
	about = gSystemCatalog.GetString(about, "AboutWindow");

	BString title(about);
	title << " " << appName;
	SetTitle(title.String());

	fAboutView = new AboutView(appName, signature);
	AddChild(fAboutView);

	MoveTo(AboutPosition(Frame().Width(), Frame().Height()));
}


BAboutWindow::~BAboutWindow()
{
	fAboutView->RemoveSelf();
	delete fAboutView;
	fAboutView = NULL;
}


//	#pragma mark - BAboutWindow virtual methods


bool
BAboutWindow::QuitRequested()
{
	Hide();

	return false;
}


void
BAboutWindow::Show()
{
	if (IsHidden()) {
		// move to current workspace
		SetWorkspaces(B_CURRENT_WORKSPACE);
	}

	BWindow::Show();
}


//	#pragma mark - BAboutWindow public methods


BPoint
BAboutWindow::AboutPosition(float width, float height)
{
	BPoint result(100, 100);

	BWindow* window =
		dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));

	BScreen screen(window);
 	BRect screenFrame(0, 0, 640, 480);
 	if (screen.IsValid())
 		screenFrame = screen.Frame();

	// Horizontally, we're smack in the middle
	result.x = screenFrame.left + (screenFrame.Width() / 2.0) - (width / 2.0);

	// This is probably sooo wrong, but it looks right on 1024 x 768
	result.y = screenFrame.top + (screenFrame.Height() / 4.0)
		- ceil(height / 3.0);

	return result;
}


void
BAboutWindow::AddDescription(const char* description)
{
	if (description == NULL)
		return;

	const char* appDesc = B_TRANSLATE_MARK(description);
	appDesc = gSystemCatalog.GetString(appDesc, "AboutWindow");

	BString desc("");
	if (fAboutView->InfoView()->TextLength() > 0)
		desc << "\n\n";

	desc << appDesc;

	fAboutView->InfoView()->Insert(desc.String());
}


void
BAboutWindow::AddCopyright(int32 firstCopyrightYear,
	const char* copyrightHolder, const char** extraCopyrights)
{
	BString copyright(B_UTF8_COPYRIGHT " %years% %holder%");

	// Get current year
	time_t tp;
	time(&tp);
	char currentYear[5];
	strftime(currentYear, 5, "%Y", localtime(&tp));
	BString copyrightYears;
	copyrightYears << firstCopyrightYear;
	if (copyrightYears != currentYear)
		copyrightYears << "-" << currentYear;

	BString text("");
	if (fAboutView->InfoView()->TextLength() > 0)
		text << "\n\n";

	text << copyright;

	// Fill out the copyright year placeholder
	text.ReplaceAll("%years%", copyrightYears.String());

	// Fill in the copyright holder placeholder
	text.ReplaceAll("%holder%", copyrightHolder);

	// Add extra copyright strings
	if (extraCopyrights != NULL) {
		// Add optional extra copyright information
		for (int32 i = 0; extraCopyrights[i]; i++)
			text << "\n" << B_UTF8_COPYRIGHT << " " << extraCopyrights[i];
	}

	const char* allRightsReserved = B_TRANSLATE_MARK("All Rights Reserved.");
	allRightsReserved = gSystemCatalog.GetString(allRightsReserved,
		"AboutWindow");
	text << "\n    " << allRightsReserved;

	fAboutView->InfoView()->Insert(text.String());
}


void
BAboutWindow::AddAuthors(const char** authors)
{
	if (authors == NULL)
		return;

	const char* writtenBy = B_TRANSLATE_MARK("Written by:");
	writtenBy = gSystemCatalog.GetString(writtenBy, "AboutWindow");

	BString text("");
	if (fAboutView->InfoView()->TextLength() > 0)
		text << "\n\n";

	text << writtenBy;
	text << "\n";
	for (int32 i = 0; authors[i]; i++)
		text << "    " << authors[i] << "\n";

	fAboutView->InfoView()->Insert(text.String());
}


void
BAboutWindow::AddSpecialThanks(const char** thanks)
{
	if (thanks == NULL)
		return;

	const char* specialThanks = B_TRANSLATE_MARK("Special Thanks:");
	specialThanks = gSystemCatalog.GetString(specialThanks, "AboutWindow");

	BString text("");
	if (fAboutView->InfoView()->TextLength() > 0)
		text << "\n\n";

	text << specialThanks << "\n";
	for (int32 i = 0; thanks[i]; i++)
		text << "    " << thanks[i] << "\n";

	fAboutView->InfoView()->Insert(text.String());
}


void
BAboutWindow::AddVersionHistory(const char** history)
{
	if (history == NULL)
		return;

	const char* versionHistory = B_TRANSLATE_MARK("Version history:");
	versionHistory = gSystemCatalog.GetString(versionHistory, "AboutWindow");

	BString text("");
	if (fAboutView->InfoView()->TextLength() > 0)
		text << "\n\n";

	text << versionHistory << "\n";
	for (int32 i = 0; history[i]; i++)
		text << "    " << history[i] << "\n";

	fAboutView->InfoView()->Insert(text.String());
}


void
BAboutWindow::AddExtraInfo(const char* extraInfo)
{
	if (extraInfo == NULL)
		return;

	const char* appExtraInfo = B_TRANSLATE_MARK(extraInfo);
	appExtraInfo = gSystemCatalog.GetString(extraInfo, "AboutWindow");

	BString extra("");
	if (fAboutView->InfoView()->TextLength() > 0)
		extra << "\n\n";

	extra << appExtraInfo;

	fAboutView->InfoView()->Insert(extra.String());
}


const char*
BAboutWindow::Name()
{
	return fAboutView->Name();
}


void
BAboutWindow::SetName(const char* name)
{
	fAboutView->SetName(name);
}


const char*
BAboutWindow::Version()
{
	return fAboutView->Version();
}


void
BAboutWindow::SetVersion(const char* version)
{
	fAboutView->SetVersion(version);
}


BBitmap*
BAboutWindow::Icon()
{
	return fAboutView->Icon();
}


void
BAboutWindow::SetIcon(BBitmap* icon)
{
	fAboutView->SetIcon(icon);
}
