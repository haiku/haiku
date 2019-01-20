/*
 * Copyright 2005-2018, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		René Gollent
 *		Wim van der Meer <WPJvanderMeer@gmail.com>
 */


#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <string>

#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <DateTimeFormat.h>
#include <DurationFormat.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <fs_attr.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <ObjectList.h>
#include <OS.h>
#include <Path.h>
#include <PathFinder.h>
#include <Resources.h>
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>
#include <StringFormat.h>
#include <StringList.h>
#include <StringView.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>
#include <View.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#include <AppMisc.h>
#include <AutoDeleter.h>
#include <cpu_type.h>
#include <parsedate.h>
#include <system_revision.h>

#include <Catalog.h>
#include <Language.h>
#include <Locale.h>
#include <LocaleRoster.h>

#include "HyperTextActions.h"
#include "HyperTextView.h"
#include "Utilities.h"

#include "Credits.h"

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

#define SCROLL_CREDITS_VIEW 'mviv'

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AboutWindow"



static const char* UptimeToString(char string[], size_t size);
static const char* MemSizeToString(char string[], size_t size,
	system_info* info);
static const char* MemUsageToString(char string[], size_t size,
	system_info* info);


static const rgb_color kDarkGrey = { 100, 100, 100, 255 };
static const rgb_color kHaikuGreen = { 42, 131, 36, 255 };
static const rgb_color kHaikuOrange = { 255, 69, 0, 255 };
static const rgb_color kHaikuYellow = { 255, 176, 0, 255 };
static const rgb_color kLinkBlue = { 80, 80, 200, 255 };
static const rgb_color kBeOSBlue = { 0, 0, 200, 255 };
static const rgb_color kBeOSRed = { 200, 0, 0, 255 };

static const char* kBerkeley = B_TRANSLATE_MARK("Berkeley");
static const char* kBSDTwoClause = B_TRANSLATE_MARK("BSD (2-clause)");
static const char* kBSDThreeClause = B_TRANSLATE_MARK("BSD (3-clause)");
static const char* kBSDFourClause = B_TRANSLATE_MARK("BSD (4-clause)");
static const char* kGPLv2 = B_TRANSLATE_MARK("GNU GPL v2");
static const char* kGPLv3 = B_TRANSLATE_MARK("GNU GPL v3");
static const char* kLGPLv2 = B_TRANSLATE_MARK("GNU LGPL v2");
static const char* kLGPLv21 = B_TRANSLATE_MARK("GNU LGPL v2.1");
#if 0
static const char* kPublicDomain = B_TRANSLATE_MARK("Public Domain");
#endif
#ifdef __INTEL__
static const char* kIntel2xxxFirmware = B_TRANSLATE_MARK("Intel (2xxx firmware)");
static const char* kIntelFirmware = B_TRANSLATE_MARK("Intel (firmware)");
static const char* kMarvellFirmware = B_TRANSLATE_MARK("Marvell (firmware)");
static const char* kRalinkFirmware = B_TRANSLATE_MARK("Ralink (firmware)");
#endif


static int
TranslationComparator(const void* left, const void* right)
{
	const Translation* leftTranslation = *(const Translation**)left;
	const Translation* rightTranslation = *(const Translation**)right;

	BLanguage* language;
	BString leftName;
	if (BLocaleRoster::Default()->GetLanguage(leftTranslation->languageCode,
			&language) == B_OK) {
		language->GetName(leftName);
		delete language;
	} else
		leftName = leftTranslation->languageCode;

	BString rightName;
	if (BLocaleRoster::Default()->GetLanguage(rightTranslation->languageCode,
			&language) == B_OK) {
		language->GetName(rightName);
		delete language;
	} else
		rightName = rightTranslation->languageCode;

	BCollator collator;
	BLocale::Default()->GetCollator(&collator);
	return collator.Compare(leftName.String(), rightName.String());
}


class AboutApp : public BApplication {
public:
							AboutApp();
			void			MessageReceived(BMessage* message);
};


class AboutView;

class AboutWindow : public BWindow {
public:
							AboutWindow();

	virtual	bool			QuitRequested();

			AboutView*		fAboutView;
};


class LogoView : public BView {
public:
							LogoView();
	virtual					~LogoView();

	virtual	BSize			MinSize();
	virtual	BSize			MaxSize();

	virtual void			Draw(BRect updateRect);

private:
			BBitmap*		fLogo;
};


class CropView : public BView {
public:
							CropView(BView* target, int32 left, int32 top,
								int32 right, int32 bottom);
	virtual					~CropView();

	virtual	BSize			MinSize();
	virtual	BSize			MaxSize();

	virtual void			DoLayout();

private:
			BView*			fTarget;
			int32			fCropLeft;
			int32			fCropTop;
			int32			fCropRight;
			int32			fCropBottom;
};


class AboutView : public BView {
public:
							AboutView();
							~AboutView();

	virtual void			AttachedToWindow();
	virtual	void			AllAttached();
	virtual void			Pulse();

	virtual void			MessageReceived(BMessage* msg);
	virtual void			MouseDown(BPoint point);

			void			AddCopyrightEntry(const char* name,
								const char* text,
								const StringVector& licenses,
								const StringVector& sources,
								const char* url);
			void			AddCopyrightEntry(const char* name,
								const char* text, const char* url = NULL);
			void			PickRandomHaiku();


			void			_AdjustTextColors();
private:
	typedef std::map<std::string, PackageCredit*> PackageCreditMap;

private:
			BView*			_CreateLabel(const char* name, const char* label);
			BView*			_CreateCreditsView();
			status_t		_GetLicensePath(const char* license,
								BPath& path);
			void			_AddCopyrightsFromAttribute();
			void			_AddPackageCredit(const PackageCredit& package);
			void			_AddPackageCreditEntries();

			BStringView*	fMemView;
			BStringView*	fUptimeView;
			BView*			fInfoView;
			HyperTextView*	fCreditsView;

			BObjectList<BView> fTextViews;
			BObjectList<BView> fSubTextViews;

			BBitmap*		fLogo;

			bigtime_t		fLastActionTime;
			BMessageRunner*	fScrollRunner;
			PackageCreditMap fPackageCredits;
};


//	#pragma mark -


AboutApp::AboutApp()
	: BApplication("application/x-vnd.Haiku-About")
{
	B_TRANSLATE_MARK_SYSTEM_NAME_VOID("AboutSystem");

	AboutWindow *window = new(std::nothrow) AboutWindow();
	if (window)
		window->Show();
}


void
AboutApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SILENT_RELAUNCH:
			WindowAt(0)->Activate();
			break;
	}

	BApplication::MessageReceived(message);
}


//	#pragma mark -


AboutWindow::AboutWindow()
	: BWindow(BRect(0, 0, 500, 300), B_TRANSLATE("About this system"),
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	fAboutView = new AboutView();
	AddChild(fAboutView);

	// Make sure we take the minimal window size into account when centering
	BSize size = GetLayout()->MinSize();
	ResizeTo(max_c(size.width, Bounds().Width()),
		max_c(size.height, Bounds().Height()));

	CenterOnScreen();
}


bool
AboutWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark - LogoView


LogoView::LogoView()
	: BView("logo", B_WILL_DRAW)
{
	fLogo = BTranslationUtils::GetBitmap(B_PNG_FORMAT, "logo.png");
	SetViewColor(255, 255, 255);
}


LogoView::~LogoView()
{
	delete fLogo;
}


BSize
LogoView::MinSize()
{
	if (fLogo == NULL)
		return BSize(0, 0);

	return BSize(fLogo->Bounds().Width(), fLogo->Bounds().Height());
}


BSize
LogoView::MaxSize()
{
	if (fLogo == NULL)
		return BSize(0, 0);

	return BSize(B_SIZE_UNLIMITED, fLogo->Bounds().Height());
}


void
LogoView::Draw(BRect updateRect)
{
	if (fLogo != NULL) {
		DrawBitmap(fLogo,
			BPoint((Bounds().Width() - fLogo->Bounds().Width()) / 2, 0));
	}
}


//	#pragma mark - CropView


CropView::CropView(BView* target, int32 left, int32 top, int32 right,
		int32 bottom)
	: BView("crop view", 0),
	fTarget(target),
	fCropLeft(left),
	fCropTop(top),
	fCropRight(right),
	fCropBottom(bottom)
{
	AddChild(target);
}


CropView::~CropView()
{
}


BSize
CropView::MinSize()
{
	if (fTarget == NULL)
		return BSize();

	BSize size = fTarget->MinSize();
	if (size.width != B_SIZE_UNSET)
		size.width -= fCropLeft + fCropRight;
	if (size.height != B_SIZE_UNSET)
		size.height -= fCropTop + fCropBottom;

	return size;
}


BSize
CropView::MaxSize()
{
	if (fTarget == NULL)
		return BSize();

	BSize size = fTarget->MaxSize();
	if (size.width != B_SIZE_UNSET)
		size.width -= fCropLeft + fCropRight;
	if (size.height != B_SIZE_UNSET)
		size.height -= fCropTop + fCropBottom;

	return size;
}


void
CropView::DoLayout()
{
	BView::DoLayout();

	if (fTarget == NULL)
		return;

	fTarget->MoveTo(-fCropLeft, -fCropTop);
	fTarget->ResizeTo(Bounds().Width() + fCropLeft + fCropRight,
		Bounds().Height() + fCropTop + fCropBottom);
}


//	#pragma mark - AboutView

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AboutView"

AboutView::AboutView()
	: BView("aboutview", B_WILL_DRAW | B_PULSE_NEEDED),
	fLastActionTime(system_time()),
	fScrollRunner(NULL)
{
	// Begin Construction of System Information controls
	system_info systemInfo;
	get_system_info(&systemInfo);

	// Create all the various labels for system infomation

	// OS Version

	char string[1024];
	strlcpy(string, B_TRANSLATE("Unknown"), sizeof(string));

	// the version is stored in the BEOS:APP_VERSION attribute of libbe.so
	BPath path;
	if (find_directory(B_BEOS_LIB_DIRECTORY, &path) == B_OK) {
		path.Append("libbe.so");

		BAppFileInfo appFileInfo;
		version_info versionInfo;
		BFile file;
		if (file.SetTo(path.Path(), B_READ_ONLY) == B_OK
			&& appFileInfo.SetTo(&file) == B_OK
			&& appFileInfo.GetVersionInfo(&versionInfo,
				B_APP_VERSION_KIND) == B_OK
			&& versionInfo.short_info[0] != '\0')
			strlcpy(string, versionInfo.short_info, sizeof(string));
	}

	// Add system revision
	const char* haikuRevision = __get_haiku_revision();
	if (haikuRevision != NULL) {
		strlcat(string, " (", sizeof(string));
		strlcat(string, B_TRANSLATE("Revision"), sizeof(string));
		strlcat(string, " ", sizeof(string));
		strlcat(string, haikuRevision, sizeof(string));
		strlcat(string, ")", sizeof(string));
	}

	BStringView* versionView = new BStringView("ostext", string);
	fSubTextViews.AddItem(versionView);
	versionView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));

	BStringView* abiView = new BStringView("abitext", B_HAIKU_ABI_NAME);
	fSubTextViews.AddItem(abiView);
	abiView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));

	// CPU count, type and clock speed
	static BStringFormat format(B_TRANSLATE_COMMENT(
		"{0, plural, one{Processor:} other{# Processors:}}",
		"\"Processor:\" or \"2 Processors:\""));

	BString processorLabel;
	format.Format(processorLabel, systemInfo.cpu_count);

	uint32 topologyNodeCount = 0;
	cpu_topology_node_info* topology = NULL;
	get_cpu_topology_info(NULL, &topologyNodeCount);
	if (topologyNodeCount != 0)
		topology = new cpu_topology_node_info[topologyNodeCount];
	get_cpu_topology_info(topology, &topologyNodeCount);

	enum cpu_platform platform = B_CPU_UNKNOWN;
	enum cpu_vendor cpuVendor = B_CPU_VENDOR_UNKNOWN;
	uint32 cpuModel = 0;
	for (uint32 i = 0; i < topologyNodeCount; i++) {
		switch (topology[i].type) {
			case B_TOPOLOGY_ROOT:
				platform = topology[i].data.root.platform;
				break;

			case B_TOPOLOGY_PACKAGE:
				cpuVendor = topology[i].data.package.vendor;
				break;

			case B_TOPOLOGY_CORE:
				cpuModel = topology[i].data.core.model;
				break;

			default:
				break;
		}
	}

	delete[] topology;

	BString cpuType;
	cpuType << get_cpu_vendor_string(cpuVendor)
		<< " " << get_cpu_model_string(platform, cpuVendor, cpuModel);

	BStringView* cpuView = new BStringView("cputext", cpuType.String());
	fSubTextViews.AddItem(cpuView);
	cpuView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));

	int32 clockSpeed = get_rounded_cpu_speed();
	if (clockSpeed < 1000)
		snprintf(string, sizeof(string), B_TRANSLATE("%ld MHz"), clockSpeed);
	else
		snprintf(string, sizeof(string), B_TRANSLATE("%.2f GHz"),
			clockSpeed / 1000.0f);

	BStringView* frequencyView = new BStringView("frequencytext", string);
	fSubTextViews.AddItem(frequencyView);
	frequencyView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));

	// RAM
	BStringView *memSizeView = new BStringView("ramsizetext",
		MemSizeToString(string, sizeof(string), &systemInfo));
	fSubTextViews.AddItem(memSizeView);
	memSizeView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));

	fMemView = new BStringView("ramtext",
		MemUsageToString(string, sizeof(string), &systemInfo));
	fSubTextViews.AddItem(fMemView);
	fMemView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));

	// Kernel build time/date
	BString kernelTimeDate;
	kernelTimeDate << systemInfo.kernel_build_date
		<< " " << systemInfo.kernel_build_time;
	BString buildTimeDate;

	time_t buildTimeDateStamp = parsedate(kernelTimeDate, -1);
	if (buildTimeDateStamp > 0) {
		if (BDateTimeFormat().Format(buildTimeDate, buildTimeDateStamp,
			B_LONG_DATE_FORMAT, B_MEDIUM_TIME_FORMAT) != B_OK)
			buildTimeDate.SetTo(kernelTimeDate);
	} else
		buildTimeDate.SetTo(kernelTimeDate);

	BStringView* kernelView = new BStringView("kerneltext", buildTimeDate);
	fSubTextViews.AddItem(kernelView);
	kernelView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));

	// Uptime
	fUptimeView = new BStringView("uptimetext", "...");
	fSubTextViews.AddItem(fUptimeView);
	fUptimeView->SetText(UptimeToString(string, sizeof(string)));

	const float offset = 5;

	SetLayout(new BGroupLayout(B_HORIZONTAL, 0));
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BLayoutBuilder::Group<>((BGroupLayout*)GetLayout())
		.AddGroup(B_VERTICAL, 0)
			.Add(new LogoView())
			.AddGroup(B_VERTICAL, 0)
				.Add(_CreateLabel("oslabel", B_TRANSLATE("Version:")))
				.Add(versionView)
				.Add(abiView)
				.AddStrut(offset)
				.Add(_CreateLabel("cpulabel", processorLabel.String()))
				.Add(cpuView)
				.Add(frequencyView)
				.AddStrut(offset)
				.Add(_CreateLabel("memlabel", B_TRANSLATE("Memory:")))
				.Add(memSizeView)
				.Add(fMemView)
				.AddStrut(offset)
				.Add(_CreateLabel("kernellabel", B_TRANSLATE("Kernel:")))
				.Add(kernelView)
				.AddStrut(offset)
				.Add(_CreateLabel("uptimelabel",
					B_TRANSLATE("Time running:")))
				.Add(fUptimeView)
				.SetInsets(5, 5, 5, 5)
			.End()
			// TODO: investigate: adding this causes the time to be cut
			//.AddGlue()
		.End()
		.Add(_CreateCreditsView());

	float min = fMemView->MinSize().width * 1.1f;
	fCreditsView->SetExplicitMinSize(BSize(min, min));
}


AboutView::~AboutView()
{
	for (PackageCreditMap::iterator it = fPackageCredits.begin();
		it != fPackageCredits.end(); it++) {

		delete it->second;
	}

	delete fScrollRunner;
}


void
AboutView::AttachedToWindow()
{
	BView::AttachedToWindow();
	Window()->SetPulseRate(500000);
	SetEventMask(B_POINTER_EVENTS);
	DoLayout();
}


void
AboutView::AllAttached()
{
	_AdjustTextColors();
}


void
AboutView::MouseDown(BPoint point)
{
	BRect r(92, 26, 105, 31);
	if (r.Contains(point))
		BMessenger(this).SendMessage('eegg');

	if (Bounds().Contains(point)) {
		fLastActionTime = system_time();
		delete fScrollRunner;
		fScrollRunner = NULL;
	}
}


void
AboutView::Pulse()
{
	char string[255];
	system_info info;
	get_system_info(&info);
	fUptimeView->SetText(UptimeToString(string, sizeof(string)));
	fMemView->SetText(MemUsageToString(string, sizeof(string), &info));

	if (fScrollRunner == NULL
		&& system_time() > fLastActionTime + 10000000) {
		BMessage message(SCROLL_CREDITS_VIEW);
		//fScrollRunner = new BMessageRunner(this, &message, 25000, -1);
	}
}


void
AboutView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_COLORS_UPDATED:
		{
			if (msg->HasColor(ui_color_name(B_PANEL_TEXT_COLOR)))
				_AdjustTextColors();

			break;
		}
		case SCROLL_CREDITS_VIEW:
		{
			BScrollBar* scrollBar =
				fCreditsView->ScrollBar(B_VERTICAL);
			if (scrollBar == NULL)
				break;
			float max, min;
			scrollBar->GetRange(&min, &max);
			if (scrollBar->Value() < max)
				fCreditsView->ScrollBy(0, 1);

			break;
		}

		case 'eegg':
		{
			printf("Easter egg\n");
			PickRandomHaiku();
			break;
		}

		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
AboutView::AddCopyrightEntry(const char* name, const char* text,
	const char* url)
{
	AddCopyrightEntry(name, text, StringVector(), StringVector(), url);
}


void
AboutView::AddCopyrightEntry(const char* name, const char* text,
	const StringVector& licenses, const StringVector& sources,
	const char* url)
{
	BFont font(be_bold_font);
	//font.SetSize(be_bold_font->Size());
	font.SetFace(B_BOLD_FACE | B_ITALIC_FACE);

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuYellow);
	fCreditsView->Insert(name);
	fCreditsView->Insert("\n");
	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(text);
	fCreditsView->Insert("\n");

	if (licenses.CountStrings() > 0) {
		if (licenses.CountStrings() > 1)
			fCreditsView->Insert(B_TRANSLATE("Licenses: "));
		else
			fCreditsView->Insert(B_TRANSLATE("License: "));

		for (int32 i = 0; i < licenses.CountStrings(); i++) {
			const char* license = licenses.StringAt(i);

			if (i > 0)
				fCreditsView->Insert(", ");

			BString licenseName;
			BString licenseURL;
			parse_named_url(license, licenseName, licenseURL);

			BPath licensePath;
			if (_GetLicensePath(licenseURL, licensePath) == B_OK) {
				fCreditsView->InsertHyperText(B_TRANSLATE_NOCOLLECT(licenseName),
					new OpenFileAction(licensePath.Path()));
			} else
				fCreditsView->Insert(licenseName);
		}

		fCreditsView->Insert("\n");
	}

	if (sources.CountStrings() > 0) {
		fCreditsView->Insert(B_TRANSLATE("Source Code: "));

		for (int32 i = 0; i < sources.CountStrings(); i++) {
			const char* source = sources.StringAt(i);

			if (i > 0)
				fCreditsView->Insert(", ");

			BString urlName;
			BString urlAddress;
			parse_named_url(source, urlName, urlAddress);

			fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL,
				&kLinkBlue);
			fCreditsView->InsertHyperText(urlName,
				new URLAction(urlAddress));
		}

		fCreditsView->Insert("\n");
	}

	if (url) {
		BString urlName;
		BString urlAddress;
		parse_named_url(url, urlName, urlAddress);

		fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL,
			&kLinkBlue);
		fCreditsView->InsertHyperText(urlName,
			new URLAction(urlAddress));
		fCreditsView->Insert("\n");
	}
	fCreditsView->Insert("\n");
}


void
AboutView::PickRandomHaiku()
{
	BPath path;
	if (find_directory(B_SYSTEM_DATA_DIRECTORY, &path) != B_OK)
		path = "/system/data";
	path.Append("fortunes");
	path.Append("Haiku");

	BFile fortunes(path.Path(), B_READ_ONLY);
	struct stat st;
	if (fortunes.InitCheck() < B_OK)
		return;
	if (fortunes.GetStat(&st) < B_OK)
		return;

	char* buff = (char*)malloc((size_t)st.st_size + 1);
	if (!buff)
		return;
	buff[(size_t)st.st_size] = '\0';
	BList haikuList;
	if (fortunes.Read(buff, (size_t)st.st_size) == (ssize_t)st.st_size) {
		char* p = buff;
		while (p && *p) {
			char* e = strchr(p, '%');
			BString* s = new BString(p, e ? (e - p) : -1);
			haikuList.AddItem(s);
			p = e;
			if (p && (*p == '%'))
				p++;
			if (p && (*p == '\n'))
				p++;
		}
	}
	free(buff);
	if (haikuList.CountItems() < 1)
		return;

	BString* s = (BString*)haikuList.ItemAt(rand() % haikuList.CountItems());
	BFont font(be_bold_font);
	font.SetSize(be_bold_font->Size());
	font.SetFace(B_BOLD_FACE | B_ITALIC_FACE);
	fCreditsView->SelectAll();
	fCreditsView->Delete();
	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(s->String());
	fCreditsView->Insert("\n");
	while ((s = (BString*)haikuList.RemoveItem((int32)0))) {
		delete s;
	}
}


void
AboutView::_AdjustTextColors()
{
	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	rgb_color color = mix_color(ViewColor(), textColor, 192);

	BView* view = NULL;
	for (int32 index = 0; index < fSubTextViews.CountItems(); ++index) {
		view = fSubTextViews.ItemAt(index);
		view->SetHighColor(color);
		view->Invalidate();
	}

	// Labels
	for (int32 index = 0; index < fTextViews.CountItems(); ++index) {
		view = fTextViews.ItemAt(index);
		view->SetHighColor(textColor);
		view->Invalidate();
	}
}


BView*
AboutView::_CreateLabel(const char* name, const char* label)
{
	BStringView* labelView = new BStringView(name, label);
	labelView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));
	labelView->SetFont(be_bold_font);
	fTextViews.AddItem(labelView);
	return labelView;
}


BView*
AboutView::_CreateCreditsView()
{
	// Begin construction of the credits view
	fCreditsView = new HyperTextView("credits");
	fCreditsView->SetFlags(fCreditsView->Flags() | B_FRAME_EVENTS);
	fCreditsView->SetStylable(true);
	fCreditsView->MakeEditable(false);
	fCreditsView->SetWordWrap(true);
	fCreditsView->SetInsets(5, 5, 5, 5);
	fCreditsView->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);

	BScrollView* creditsScroller = new BScrollView("creditsScroller",
		fCreditsView, B_WILL_DRAW | B_FRAME_EVENTS, false, true,
		B_PLAIN_BORDER);

	// Haiku copyright
	BFont font(be_bold_font);
	font.SetSize(font.Size() + 4);

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuGreen);
	fCreditsView->Insert("Haiku\n");

	char string[1024];
	time_t time = ::time(NULL);
	struct tm* tm = localtime(&time);
	int32 year = tm->tm_year + 1900;
	if (year < 2008)
		year = 2008;
	snprintf(string, sizeof(string),
		COPYRIGHT_STRING "2001-%" B_PRId32 " The Haiku project. ", year);

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(string);

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(B_TRANSLATE("The copyright to the Haiku code is "
		"property of Haiku, Inc. or of the respective authors where expressly "
		"noted in the source. Haiku" B_UTF8_REGISTERED
		" and the HAIKU logo" B_UTF8_REGISTERED
		" are registered trademarks of Haiku, Inc."
		"\n\n"));

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kLinkBlue);
	fCreditsView->InsertHyperText("https://www.haiku-os.org",
		new URLAction("https://www.haiku-os.org"));
	fCreditsView->Insert("\n\n");

	font.SetSize(be_bold_font->Size());
	font.SetFace(B_BOLD_FACE | B_ITALIC_FACE);

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuOrange);
	fCreditsView->Insert(B_TRANSLATE("Current maintainers:\n"));

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(kCurrentMaintainers);

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuOrange);
	fCreditsView->Insert(B_TRANSLATE("Past maintainers:\n"));

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(kPastMaintainers);

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuOrange);
	fCreditsView->Insert(B_TRANSLATE("Website & marketing:\n"));

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(kWebsiteTeam);

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuOrange);
	fCreditsView->Insert(B_TRANSLATE("Past website & marketing:\n"));

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(kPastWebsiteTeam);

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuOrange);
	fCreditsView->Insert(B_TRANSLATE("Contributors:\n"));

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(kContributors);
	fCreditsView->Insert(
		B_TRANSLATE("\n" B_UTF8_ELLIPSIS
			"and probably some more we forgot to mention (sorry!)"
			"\n\n"));

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuOrange);
	fCreditsView->Insert(B_TRANSLATE("Translations:\n"));

	BLanguage* lang;
	BString langName;

	BList sortedTranslations;
	for (uint32 i = 0; i < kNumberOfTranslations; i ++) {
		const Translation* translation = &kTranslations[i];
		sortedTranslations.AddItem((void*)translation);
	}
	sortedTranslations.SortItems(TranslationComparator);

	for (uint32 i = 0; i < kNumberOfTranslations; i ++) {
		const Translation& translation
			= *(const Translation*)sortedTranslations.ItemAt(i);

		langName.Truncate(0);
		if (BLocaleRoster::Default()->GetLanguage(translation.languageCode,
				&lang) == B_OK) {
			lang->GetName(langName);
			delete lang;
		} else {
			// We failed to get the localized readable name,
			// go with what we have.
			langName.Append(translation.languageCode);
		}

		fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuGreen);
		fCreditsView->Insert("\n");
		fCreditsView->Insert(langName);
		fCreditsView->Insert("\n");
		fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
		fCreditsView->Insert(translation.names);
	}

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuOrange);
	fCreditsView->Insert(B_TRANSLATE("\n\nSpecial thanks to:\n"));

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	BString beosCredits(B_TRANSLATE(
		"Be Inc. and its developer team, for having created BeOS!\n\n"));
	int32 beosOffset = beosCredits.FindFirst("BeOS");
	fCreditsView->Insert(beosCredits.String(),
		(beosOffset < 0) ? beosCredits.Length() : beosOffset);
	if (beosOffset > -1) {
		fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kBeOSBlue);
		fCreditsView->Insert("B");
		fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kBeOSRed);
		fCreditsView->Insert("e");
		fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
		beosCredits.Remove(0, beosOffset + 2);
		fCreditsView->Insert(beosCredits.String(), beosCredits.Length());
	}
	fCreditsView->Insert(
		B_TRANSLATE("Travis Geiselbrecht (and his NewOS kernel)\n"));
	fCreditsView->Insert(
		B_TRANSLATE("Michael Phipps (project founder)\n\n"));
	fCreditsView->Insert(
		B_TRANSLATE("The HaikuPorts team\n"));
	fCreditsView->Insert(
		B_TRANSLATE("The Haikuware team and their bounty program\n"));
	fCreditsView->Insert(
		B_TRANSLATE("The BeGeistert team\n"));
	fCreditsView->Insert(
		B_TRANSLATE("Google and their Google Summer of Code and Google Code In "
			"programs\n"));
	fCreditsView->Insert(
		B_TRANSLATE("The University of Auckland and Christof Lutteroth\n\n"));
	fCreditsView->Insert(
		B_TRANSLATE(B_UTF8_ELLIPSIS "and the many people making donations!\n\n"));

	// copyrights for various projects we use

	BPath mitPath;
	_GetLicensePath("MIT", mitPath);
	BPath lgplPath;
	_GetLicensePath("GNU LGPL v2.1", lgplPath);

	font.SetSize(be_bold_font->Size() + 4);
	font.SetFace(B_BOLD_FACE);
	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuGreen);
	fCreditsView->Insert(B_TRANSLATE("\nCopyrights\n\n"));

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(B_TRANSLATE("[Click a license name to read the "
		"respective license.]\n\n"));

	// Haiku license
	BString haikuLicense = B_TRANSLATE_COMMENT("The code that is unique to "
		"Haiku, especially the kernel and all code that applications may link "
		"against, is distributed under the terms of the <MIT license>. "
		"Some system libraries contain third party code distributed under the "
		"<LGPL license>. You can find the copyrights to third party code below."
		"\n\n", "<MIT license> and <LGPL license> aren't variables and can be "
		"translated. However, please, don't remove < and > as they're needed "
		"as placeholders for proper hypertext functionality.");
	int32 licensePart1 = haikuLicense.FindFirst("<");
	int32 licensePart2 = haikuLicense.FindFirst(">");
	int32 licensePart3 = haikuLicense.FindLast("<");
	int32 licensePart4 = haikuLicense.FindLast(">");
	BString part;
	haikuLicense.CopyInto(part, 0, licensePart1);
	fCreditsView->Insert(part);

	part.Truncate(0);
	haikuLicense.CopyInto(part, licensePart1 + 1, licensePart2 - 1
		- licensePart1);
	fCreditsView->InsertHyperText(part, new OpenFileAction(mitPath.Path()));

	part.Truncate(0);
	haikuLicense.CopyInto(part, licensePart2 + 1, licensePart3 - 1
		- licensePart2);
	fCreditsView->Insert(part);

	part.Truncate(0);
	haikuLicense.CopyInto(part, licensePart3 + 1, licensePart4 - 1
		- licensePart3);
	fCreditsView->InsertHyperText(part, new OpenFileAction(lgplPath.Path()));

	part.Truncate(0);
	haikuLicense.CopyInto(part, licensePart4 + 1, haikuLicense.Length() - 1
		- licensePart4);
	fCreditsView->Insert(part);

	// GNU copyrights
	AddCopyrightEntry("The GNU Project",
		B_TRANSLATE("Contains software from the GNU Project, "
		"released under the GPL and LGPL licenses:\n"
		"GNU C Library, "
		"GNU coretools, diffutils, findutils, "
		"sharutils, gawk, bison, m4, make, "
		"wget, ncurses, termcap, "
		"Bourne Again Shell.\n"
		COPYRIGHT_STRING "The Free Software Foundation."),
		StringVector(kLGPLv21, kGPLv2, kGPLv3, NULL),
		StringVector(),
		"https://www.gnu.org");

	// FreeBSD copyrights
	AddCopyrightEntry("The FreeBSD Project",
		B_TRANSLATE("Contains software from the FreeBSD Project, "
		"released under the BSD license:\n"
		"ftpd, ping, telnet, telnetd, traceroute\n"
		COPYRIGHT_STRING "1994-2008 The FreeBSD Project. "
		"All rights reserved."),
		StringVector(kBSDTwoClause, kBSDThreeClause, kBSDFourClause,
			NULL),
		StringVector(),
		"https://www.freebsd.org");

	// NetBSD copyrights
	AddCopyrightEntry("The NetBSD Project",
		B_TRANSLATE("Contains software developed by the NetBSD "
		"Foundation, Inc. and its contributors:\n"
		"ftp, tput\n"
		COPYRIGHT_STRING "1996-2008 The NetBSD Foundation, Inc. "
		"All rights reserved."),
		StringVector(kBerkeley, kBSDFourClause, NULL),
		StringVector(),
		"https://www.netbsd.org");

	// FFmpeg copyrights
	_AddPackageCredit(PackageCredit("FFmpeg")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "2000-2014 Fabrice "
			"Bellard, et al."))
		.SetLicenses(kLGPLv21, kLGPLv2, NULL)
		.SetURL("https://www.ffmpeg.org"));

	// AGG copyrights
	_AddPackageCredit(PackageCredit("AntiGrain Geometry")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "2002-2006 Maxim "
			"Shemanarev (McSeem)."))
		.SetLicenses("Anti-Grain Geometry", kBSDThreeClause, NULL)
		.SetURL("http://www.antigrain.com"));

	// FreeType copyrights
	_AddPackageCredit(PackageCredit("FreeType2")
		.SetCopyrights(B_TRANSLATE(COPYRIGHT_STRING "1996-2002, 2006 "
			"David Turner, Robert Wilhelm and Werner Lemberg."),
			COPYRIGHT_STRING "2014 The FreeType Project. "
			"All rights reserved.",
			NULL)
		.SetLicense("FreeType")
		.SetURL("http://www.freetype.org"));

	// Mesa3D (http://www.mesa3d.org) copyrights
	_AddPackageCredit(PackageCredit("Mesa")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "1999-2006 Brian Paul. "
			"Mesa3D Project. All rights reserved."))
		.SetLicense("MIT")
		.SetURL("http://www.mesa3d.org"));

	// SGI's GLU implementation copyrights
	_AddPackageCredit(PackageCredit("GLU")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "1991-2000 "
			"Silicon Graphics, Inc. All rights reserved."))
		.SetLicense("SGI Free B")
		.SetURL("http://www.sgi.com/products/software/opengl"));

	// GLUT implementation copyrights
	_AddPackageCredit(PackageCredit("GLUT")
		.SetCopyrights(B_TRANSLATE(COPYRIGHT_STRING "1994-1997 Mark Kilgard. "
			"All rights reserved."),
			COPYRIGHT_STRING "1997 Be Inc.",
			COPYRIGHT_STRING "1999 Jake Hamby.",
			NULL)
		.SetLicense("MIT")
		.SetURL("http://www.opengl.org/resources/libraries/glut"));

	// OpenGroup & DEC (BRegion backend) copyright
	_AddPackageCredit(PackageCredit("BRegion backend (XFree86)")
		.SetCopyrights(COPYRIGHT_STRING "1987-1988, 1998 The Open Group.",
			B_TRANSLATE(COPYRIGHT_STRING "1987-1988 Digital Equipment "
			"Corporation, Maynard, Massachusetts.\n"
			"All rights reserved."),
			NULL)
		.SetLicenses("OpenGroup", "DEC", NULL)
		.SetURL("https://xfree86.org"));

	// Bitstream Charter font
	_AddPackageCredit(PackageCredit("Bitstream Charter font")
		.SetCopyrights(COPYRIGHT_STRING "1989-1992 Bitstream Inc.,"
			"Cambridge, MA.",
			B_TRANSLATE("BITSTREAM CHARTER is a registered trademark of "
				"Bitstream Inc."),
			NULL)
		.SetLicense("Bitstream Charter")
		.SetURL("http://www.bitstream.com/"));

	// Noto fonts copyright
	_AddPackageCredit(PackageCredit("Noto fonts")
		.SetCopyrights(B_TRANSLATE(COPYRIGHT_STRING
			"2012-2016 Google Internationalization team."),
			NULL)
		.SetLicense("SIL Open Font Licence v1.1")
		.SetURL("http://www.google.com/get/noto/"));

	// expat copyrights
	_AddPackageCredit(PackageCredit("expat")
		.SetCopyrights(B_TRANSLATE(COPYRIGHT_STRING "1998-2000 Thai "
			"Open Source Software Center Ltd and Clark Cooper."),
			B_TRANSLATE(COPYRIGHT_STRING "2001-2003 Expat maintainers."),
			NULL)
		.SetLicense("Expat")
		.SetURL("http://expat.sourceforge.net"));

	// zlib copyrights
	_AddPackageCredit(PackageCredit("zlib")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "1995-2004 Jean-loup "
			"Gailly and Mark Adler."))
		.SetLicense("Zlib")
		.SetURL("http://www.zlib.net"));

	// zip copyrights
	_AddPackageCredit(PackageCredit("Info-ZIP")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "1990-2002 Info-ZIP. "
			"All rights reserved."))
		.SetLicense("Info-ZIP")
		.SetURL("http://www.info-zip.org"));

	// bzip2 copyrights
	_AddPackageCredit(PackageCredit("bzip2")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "1996-2005 Julian R "
			"Seward. All rights reserved."))
		.SetLicense(kBSDFourClause)
		.SetURL("http://bzip.org"));

	// OpenEXR copyrights
	_AddPackageCredit(PackageCredit("OpenEXR")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "2002-2014 Industrial "
			"Light & Magic, a division of Lucas Digital Ltd. LLC."))
		.SetLicense(kBSDThreeClause)
		.SetURL("http://www.openexr.com"));

	// acpica copyrights
	_AddPackageCredit(PackageCredit("ACPI Component Architecture (ACPICA)")
		.SetCopyright(COPYRIGHT_STRING "1999-2018 Intel Corp.")
		.SetLicense("Intel (ACPICA)")
		.SetURL("https://www.acpica.org"));

	// libpng copyrights
	_AddPackageCredit(PackageCredit("libpng")
		.SetCopyright(COPYRIGHT_STRING "1995-2017 libpng authors")
		.SetLicense("LibPNG")
		.SetURL("http://www.libpng.org"));

	// libjpeg copyrights
	_AddPackageCredit(PackageCredit("libjpeg")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "1994-2009, Thomas G. "
			"Lane, Guido Vollbeding. This software is based in part on the "
			"work of the Independent JPEG Group."))
		.SetLicense("LibJPEG")
		.SetURL("http://www.ijg.org"));

	// libprint copyrights
	_AddPackageCredit(PackageCredit("libprint")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "1999-2000 Y.Takagi. "
			"All rights reserved.")));
			// TODO: License!

	// cortex copyrights
	_AddPackageCredit(PackageCredit("Cortex")
		.SetCopyright(COPYRIGHT_STRING "1999-2000 Eric Moon.")
		.SetLicense(kBSDThreeClause)
		.SetURL("http://cortex.sourceforge.net/documentation"));

	// FluidSynth copyrights
	_AddPackageCredit(PackageCredit("FluidSynth")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "2003 Peter Hanappe "
			"and others."))
		.SetLicense(kLGPLv2)
		.SetURL("http://www.fluidsynth.org"));

	// Xiph.org Foundation copyrights
	_AddPackageCredit(PackageCredit("Xiph.org Foundation")
		.SetCopyrights("libvorbis, libogg, libtheora, libspeex",
			B_TRANSLATE(COPYRIGHT_STRING "1994-2008 Xiph.Org. "
			"All rights reserved."), NULL)
		.SetLicense(kBSDThreeClause)
		.SetURL("http://www.xiph.org"));

	// Matroska
	_AddPackageCredit(PackageCredit("libmatroska")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "2002-2003 Steve Lhomme. "
			"All rights reserved."))
		.SetLicense(kLGPLv21)
		.SetURL("http://www.matroska.org"));

	// BColorQuantizer (originally CQuantizer code)
	_AddPackageCredit(PackageCredit("CQuantizer")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "1996-1997 Jeff Prosise. "
			"All rights reserved."))
		.SetLicense("CQuantizer")
		.SetURL("http://www.xdp.it"));

	// MAPM (Mike's Arbitrary Precision Math Library) used by DeskCalc
	_AddPackageCredit(PackageCredit("MAPM")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "1999-2007 Michael C. "
			"Ring. All rights reserved."))
		.SetLicense("MAPM")
		.SetURL("http://tc.umn.edu/~ringx004"));

	// MkDepend 1.7 copyright (Makefile dependency generator)
	_AddPackageCredit(PackageCredit("MkDepend")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "1995-2001 Lars Düning. "
			"All rights reserved."))
		.SetLicense("MIT")
		.SetURL("http://bearnip.com/lars/be"));

	// libhttpd copyright (used as Poorman backend)
	_AddPackageCredit(PackageCredit("libhttpd")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "1995, 1998-2001 "
			"Jef Poskanzer. All rights reserved."))
		.SetLicense(kBSDTwoClause)
		.SetURL("http://www.acme.com/software/thttpd/"));

#ifdef __INTEL__
	// Udis86 copyrights
	_AddPackageCredit(PackageCredit("Udis86")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "2002-2004 "
			"Vivek Mohan. All rights reserved."))
		.SetLicense(kBSDTwoClause)
		.SetURL("http://udis86.sourceforge.net"));

	// Intel PRO/Wireless 2100 & 2200BG firmwares
	_AddPackageCredit(PackageCredit("Intel PRO/Wireless 2100 & 2200BG firmwares")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "2003-2006 "
			"Intel Corporation. All rights reserved."))
		.SetLicense(kIntel2xxxFirmware)
		.SetURL("http://www.intellinuxwireless.org/"));

	// Intel wireless firmwares
	_AddPackageCredit(
		PackageCredit("Intel PRO/Wireless network adapter firmwares")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "2006-2015 "
			"Intel Corporation. All rights reserved."))
		.SetLicense(kIntelFirmware)
		.SetURL("http://www.intellinuxwireless.org/"));

	// Marvell 88w8363
	_AddPackageCredit(PackageCredit("Marvell 88w8363")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "2007-2009 "
			"Marvell Semiconductor, Inc. All rights reserved."))
		.SetLicense(kMarvellFirmware)
		.SetURL("http://www.marvell.com/"));

	// Ralink Firmware RT2501/RT2561/RT2661
	_AddPackageCredit(PackageCredit("Ralink Firmware RT2501/RT2561/RT2661")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "2007 "
			"Ralink Technology Corporation. All rights reserved."))
		.SetLicense(kRalinkFirmware)
		.SetURL("http://www.ralinktech.com/"));
#endif

	// Gutenprint
	_AddPackageCredit(PackageCredit("Gutenprint")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING
			"1999-2010 by the authors of Gutenprint. All rights reserved."))
		.SetLicense(kGPLv2)
		.SetURL("http://gutenprint.sourceforge.net/"));

	// libwebp
	_AddPackageCredit(PackageCredit("libwebp")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING
			"2010-2011 Google Inc. All rights reserved."))
		.SetLicense(kBSDThreeClause)
		.SetURL("http://www.webmproject.org/code/#libwebp_webp_image_library"));

	// GTF
	_AddPackageCredit(PackageCredit("GTF")
		.SetCopyright(B_TRANSLATE("2001 by Andy Ritger based on the "
			"Generalized Timing Formula"))
		.SetLicense(kBSDThreeClause)
		.SetURL("http://gtf.sourceforge.net/"));

	// libqrencode
	_AddPackageCredit(PackageCredit("libqrencode")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "2006-2012 Kentaro Fukuchi"))
		.SetLicense(kLGPLv21)
		.SetURL("http://fukuchi.org/works/qrencode/"));

	// scrypt
	_AddPackageCredit(PackageCredit("scrypt")
		.SetCopyright(B_TRANSLATE(COPYRIGHT_STRING "2009 Colin Percival"))
		.SetLicense(kBSDTwoClause)
		.SetURL("https://tarsnap.com/scrypt.html"));

	_AddCopyrightsFromAttribute();
	_AddPackageCreditEntries();

	return new CropView(creditsScroller, 0, 1, 1, 1);
}


status_t
AboutView::_GetLicensePath(const char* license, BPath& path)
{
	BPathFinder pathFinder;
	BStringList paths;
	struct stat st;

	status_t error = pathFinder.FindPaths(B_FIND_PATH_DATA_DIRECTORY,
		"licenses", paths);

	for (int i = 0; i < paths.CountStrings(); ++i) {
		if (error == B_OK && path.SetTo(paths.StringAt(i)) == B_OK
			&& path.Append(license) == B_OK
			&& lstat(path.Path(), &st) == 0) {
			return B_OK;
		}
	}

	path.Unset();
	return B_ENTRY_NOT_FOUND;
}


void
AboutView::_AddCopyrightsFromAttribute()
{
#ifdef __HAIKU__
	// open the app executable file
	char appPath[B_PATH_NAME_LENGTH];
	int appFD;
	if (BPrivate::get_app_path(appPath) != B_OK
		|| (appFD = open(appPath, O_RDONLY)) < 0) {
		return;
	}

	// open the attribute
	int attrFD = fs_fopen_attr(appFD, "COPYRIGHTS", B_STRING_TYPE, O_RDONLY);
	close(appFD);
	if (attrFD < 0)
		return;

	// attach it to a FILE
	FILE* attrFile = fdopen(attrFD, "r");
	if (attrFile == NULL) {
		close(attrFD);
		return;
	}
	CObjectDeleter<FILE, int> _(attrFile, fclose);

	// read and parse the copyrights
	BMessage package;
	BString fieldName;
	BString fieldValue;
	char lineBuffer[LINE_MAX];
	while (char* line = fgets(lineBuffer, sizeof(lineBuffer), attrFile)) {
		// chop off line break
		size_t lineLen = strlen(line);
		if (lineLen > 0 && line[lineLen - 1] == '\n')
			line[--lineLen] = '\0';

		// flush previous field, if a new field begins, otherwise append
		if (lineLen == 0 || !isspace(line[0])) {
			// new field -- flush the previous one
			if (fieldName.Length() > 0) {
				fieldValue = trim_string(fieldValue.String(),
					fieldValue.Length());
				package.AddString(fieldName.String(), fieldValue);
				fieldName = "";
			}
		} else if (fieldName.Length() > 0) {
			// append to current field
			fieldValue += line;
			continue;
		} else {
			// bogus line -- ignore
			continue;
		}

		if (lineLen == 0)
			continue;

		// parse new field
		char* colon = strchr(line, ':');
		if (colon == NULL) {
			// bogus line -- ignore
			continue;
		}

		fieldName.SetTo(line, colon - line);
		fieldName = trim_string(line, colon - line);
		if (fieldName.Length() == 0) {
			// invalid field name
			continue;
		}

		fieldValue = colon + 1;

		if (fieldName == "Package") {
			// flush the current package
			_AddPackageCredit(PackageCredit(package));
			package.MakeEmpty();
		}
	}

	// flush current package
	_AddPackageCredit(PackageCredit(package));
#endif
}


void
AboutView::_AddPackageCreditEntries()
{
	// sort the packages case-insensitively
	PackageCredit* packages[fPackageCredits.size()];
	int32 count = 0;
	for (PackageCreditMap::iterator it = fPackageCredits.begin();
			it != fPackageCredits.end(); ++it) {
		packages[count++] = it->second;
	}

	if (count > 1) {
		std::sort(packages, packages + count,
			&PackageCredit::NameLessInsensitive);
	}

	// add the credits
	for (int32 i = 0; i < count; i++) {
		PackageCredit* package = packages[i];

		BString text(package->CopyrightAt(0));
		int32 count = package->CountCopyrights();
		for (int32 i = 1; i < count; i++)
			text << "\n" << package->CopyrightAt(i);

		AddCopyrightEntry(package->PackageName(), text.String(),
			package->Licenses(), package->Sources(), package->URL());
	}
}


void
AboutView::_AddPackageCredit(const PackageCredit& package)
{
	if (!package.IsValid())
		return;

	PackageCreditMap::iterator it = fPackageCredits.find(package.PackageName());
	if (it != fPackageCredits.end()) {
		// If the new package credit isn't "better" than the old one, ignore it.
		PackageCredit* oldPackage = it->second;
		if (!package.IsBetterThan(*oldPackage))
			return;

		// replace the old credit
		fPackageCredits.erase(it);
		delete oldPackage;
	}

	fPackageCredits[package.PackageName()] = new PackageCredit(package);
}


//	#pragma mark -


static const char*
MemSizeToString(char string[], size_t size, system_info* info)
{
	int inaccessibleMemory = int(info->ignored_pages
		* (B_PAGE_SIZE / 1048576.0f) + 0.5f);
	if (inaccessibleMemory > 0) {
		BString message(B_TRANSLATE("%total MiB total, %inaccessible MiB "
			"inaccessible"));

		snprintf(string, size, "%d", int((info->max_pages
			+ info->ignored_pages) * (B_PAGE_SIZE / 1048576.0f) + 0.5f));
		message.ReplaceFirst("%total", string);

		snprintf(string, size, "%d", inaccessibleMemory);
		message.ReplaceFirst("%inaccessible", string);
		strlcpy(string, message.String(), size);
	} else {
		snprintf(string, size, B_TRANSLATE("%d MiB total"),
			int(info->max_pages * (B_PAGE_SIZE / 1048576.0f) + 0.5f));
	}

	return string;
}


static const char*
MemUsageToString(char string[], size_t size, system_info* info)
{
	snprintf(string, size, B_TRANSLATE("%d MiB used (%d%%)"),
		int(info->used_pages * (B_PAGE_SIZE / 1048576.0f) + 0.5f),
		int(100 * info->used_pages / info->max_pages));

	return string;
}


static const char*
UptimeToString(char string[], size_t size)
{
	BDurationFormat formatter;
	BString str;

	bigtime_t uptime = system_time();
	bigtime_t now = (bigtime_t)time(NULL) * 1000000;
	formatter.Format(str, now - uptime, now);
	str.CopyInto(string, 0, size);
	string[std::min((size_t)str.Length(), size)] = '\0';

	return string;
}


int
main()
{
	AboutApp app;
	app.Run();
	return 0;
}

