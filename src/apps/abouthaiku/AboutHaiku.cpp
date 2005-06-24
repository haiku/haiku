/*
 * Copyright (c) 2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author: DarkWyrm <bpmagic@columbus.rr.com>
 */


#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <File.h>
#include <Font.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <OS.h>
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>

#include <cpu_type.h>

#include <stdio.h>


#define MOVE_INFO_VIEW 'mviv'


const char *UptimeToString(void);


class AboutApp : public BApplication {
	public:
		AboutApp(void);
};

class AboutWindow : public BWindow {
	public:
				AboutWindow(void);
		bool	QuitRequested(void);
};

class AboutView : public BView {
	public:
				AboutView(const BRect &r);
				~AboutView(void);
		void	AttachedToWindow(void);
		void	Pulse(void);
		void	Draw(BRect update);
		void	MessageReceived(BMessage *msg);
		void	MouseDown(BPoint pt);

	private:
		BStringView		*fUptimeView;
		BView			*fInfoView;
		BTextView		*fCreditsView;
		
		BBitmap			*fLogo;
		
		BPoint			fDrawPoint;
};


//	#pragma mark -


AboutApp::AboutApp(void)
	: BApplication("application/x-vnd.haiku-AboutHaiku")
{
	AboutWindow *window = new AboutWindow();
	window->Show();
}


//	#pragma mark -


AboutWindow::AboutWindow()
	: BWindow(BRect(0, 0, 500, 300), "About Haiku", B_TITLED_WINDOW, 
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	AddChild(new AboutView(Bounds()));

	MoveTo((BScreen().Frame().Width() - Bounds().Width()) / 2,
		(BScreen().Frame().Height() - Bounds().Height()) / 2 );
}


bool
AboutWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


AboutView::AboutView(const BRect &r)
	: BView(r, "aboutview", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	fLogo = BTranslationUtils::GetBitmap('PNG ', "haikulogo.png");
	if (fLogo) {
		fDrawPoint.x = (225-fLogo->Bounds().Width()) / 2;
		fDrawPoint.y = 0;
	}

	// Begin Construction of System Information controls

	font_height height;
	float labelHeight, textHeight;
	char string[255];

	system_info systemInfo;
	get_system_info(&systemInfo);

	be_plain_font->GetHeight(&height);
	textHeight = height.ascent + height.descent + height.leading;

	be_bold_font->GetHeight(&height);
	labelHeight = height.ascent + height.descent + height.leading;

	BRect r(0, 0, 225, Bounds().bottom);
	if (fLogo)
		r.OffsetBy(0, fLogo->Bounds().Height());

	fInfoView = new BView(r, "infoview", B_FOLLOW_NONE, B_WILL_DRAW);
	fInfoView->SetViewColor(235, 235, 235);
	AddChild(fInfoView);

	// Add all the various labels for system infomation

	BStringView *stringView;

	// OS Version
	r.Set(5, 5, 225, labelHeight + 5);
	stringView = new BStringView(r, "oslabel", "Version:");
	stringView->SetFont(be_bold_font);
	fInfoView->AddChild(stringView);

	r.OffsetBy(0, labelHeight);
	r.bottom = r.top + textHeight;

	// the version is stored in the BEOS:APP_VERSION attribute of libbe.so
	BFile file("/boot/beos/system/lib/libbe.so", B_READ_ONLY);
	if (file.InitCheck() == B_OK) {
		BAppFileInfo appFileInfo;
		version_info versionInfo;

		if (appFileInfo.SetTo(&file) == B_OK
			&& appFileInfo.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND) == B_OK)
			strcpy(string, versionInfo.short_info);
		else
			strcpy(string, "Unknown");
	}

	stringView = new BStringView(r, "ostext", string);
	fInfoView->AddChild(stringView);

	// CPU count, type and clock speed
	r.OffsetBy(0, textHeight * 1.5);
	r.bottom = r.top + labelHeight;

	if (systemInfo.cpu_count > 1)
		sprintf(string, "%ld Processors:", systemInfo.cpu_count);
	else
		strcpy(string, "Processor:");

	stringView = new BStringView(r, "cpulabel", "Processor:");
	stringView->SetFont(be_bold_font);
	fInfoView->AddChild(stringView);

	BString cpuType;
	cpuType << get_cpu_vendor_string(systemInfo.cpu_type) 
		<< " " << get_cpu_model_string(systemInfo.cpu_type);

	r.OffsetBy(0, labelHeight);
	r.bottom = r.top + textHeight;
	stringView = new BStringView(r, "cputext", cpuType.String());
	fInfoView->AddChild(stringView);

	r.OffsetBy(0, labelHeight);
	r.bottom = r.top + textHeight;

	if (systemInfo.cpu_clock_speed < 1000000000)
		sprintf(string,"%.2f Mhz", systemInfo.cpu_clock_speed / 1000000.0f);
	else
		sprintf(string,"%.2f Ghz", systemInfo.cpu_clock_speed / 1000000000.0f);

	stringView = new BStringView(r, "mhztext", string);
	fInfoView->AddChild(stringView);

	// RAM
	r.OffsetBy(0, textHeight * 1.5);
	r.bottom = r.top + labelHeight;
	stringView = new BStringView(r, "ramlabel", "Memory:");
	stringView->SetFont(be_bold_font);
	fInfoView->AddChild(stringView);

	r.OffsetBy(0, labelHeight);
	r.bottom = r.top + textHeight;

	sprintf(string, "%ld MB total", systemInfo.max_pages / 256);

	stringView = new BStringView(r, "ramtext", string);
	fInfoView->AddChild(stringView);

	// Kernel build time/date
	r.OffsetBy(0, textHeight * 1.5);
	r.bottom = r.top + labelHeight;
	stringView = new BStringView(r, "kernellabel", "Kernel:");
	stringView->SetFont(be_bold_font);
	fInfoView->AddChild(stringView);

	r.OffsetBy(0, labelHeight);
	r.bottom = r.top + textHeight;

	sprintf(string, "%s %s", systemInfo.kernel_build_date, systemInfo.kernel_build_time);

	stringView = new BStringView(r, "kerneltext", string);
	fInfoView->AddChild(stringView);

	// Uptime
	r.OffsetBy(0, textHeight * 1.5);
	r.bottom = r.top + labelHeight;
	stringView = new BStringView(r, "uptimelabel", "Time Running:");
	stringView->SetFont(be_bold_font);
	fInfoView->AddChild(stringView);

	r.OffsetBy(0, labelHeight);
	r.bottom = r.top + textHeight;

	fUptimeView = new BStringView(r, "uptimetext", "");
	fInfoView->AddChild(fUptimeView);
	fUptimeView->SetText(UptimeToString());

	// Begin construction of the credits view
	r = Bounds();
	r.left += fInfoView->Bounds().right;
	r.right -= B_V_SCROLL_BAR_WIDTH;

	fCreditsView = new BTextView(r, "credits",
		r.OffsetToCopy(0, 0).InsetByCopy(5, 5), B_FOLLOW_NONE);
	BScrollView *creditsScroller = new BScrollView("creditsScroller",
		fCreditsView, 0, 0, false, true, B_PLAIN_BORDER);
	AddChild(creditsScroller);

	fCreditsView->SetStylable(true);
	fCreditsView->MakeEditable(false);

	BFont font(be_bold_font);
	font.SetSize(font.Size() + 2);

	fCreditsView->SetFontAndColor(&font);
	fCreditsView->Insert("Haiku\n");

	font.SetSize(be_bold_font->Size() + 1);
	font.SetFace(B_BOLD_FACE | B_ITALIC_FACE);

	fCreditsView->SetFontAndColor(be_plain_font);
	fCreditsView->Insert("Copyright " B_UTF8_COPYRIGHT "2001-2005 Haiku, Inc.\n\n");

	fCreditsView->SetFontAndColor(&font);
	fCreditsView->Insert("Team Leads:\n");

	fCreditsView->SetFontAndColor(be_plain_font);
	fCreditsView->Insert(
		"Bruno G. Albuquerque\n"
		"DarkWyrm\n"
		"Axel Dörfler\n"
		"Phil Greenway\n"
		"Philippe Houdoin\n"
		"Kurtis Kopf\n"
		"Marcus Overhagen\n"
		"Michael Pfeiffer\n"
		"Ingo Weinhold\n"
		"Michael Wilber\n"
		"\n");

	fCreditsView->SetFontAndColor(&font);
	fCreditsView->Insert("Developers:\n");

	fCreditsView->SetFontAndColor(be_plain_font);
	fCreditsView->Insert(
		"Stephan Aßmus\n"
		"Andrew Bachmann\n"
		"Stefano Ceccherini\n"
		"Rudolf Cornelissen\n"
		"Jérôme Duval\n"
		"Matthijs Hollemans\n"
		"Thomas Kurschel\n"
		"Adi Oanca\n"
		"Niels Reedijk\n"
		"\n");

	fCreditsView->SetFontAndColor(&font);
	fCreditsView->Insert("Contributors:\n");

	fCreditsView->SetFontAndColor(be_plain_font);
	fCreditsView->Insert(
		"Bruce Cameron\n"
		"Tyler Dauwalder\n"
		"Oliver Ruiz Dorantes\n"
		"Marc Flerackers\n"
		"Erik Jaesler\n"
		"Elad Lahav\n"
		"Santiago Lema\n"
		"Jerome Leveque\n"
		"Michael Lotz\n"
		"Graham MacDonald\n"
		"Brian Matzon\n"
		"Christopher ML Zumwalt May\n"
		"Andrew McCall\n"
		"David McPaul\n"
		"Misza\n"
		"MrSiggler\n"
		"Alan Murta\n"
		"Frans Van Nispen\n"
		"Pahtz\n"
		"Michael Phipps\n"
		"Jeremy Rand\n"
		"David Reid"
		"Daniel Reinhold\n"
		"François Revol\n"
		"Thomas Roell\n"
		"Rafael Romo\n"
		"Zousar Shaker\n"
		"Daniel Switkin\n"
		"Atsushi Takamatsu\n"
		"Jason Vandermark\n"
		"Sandor Vroemisse\n"
		"Nathan Whitehorn\n"
		"Ulrich Wimboeck\n"
		"Gabe Yoder\n"
		//"(and probably some more we forgot to mention (sorry!)...)\n"
		"\n");

	fCreditsView->SetFontAndColor(&font);
	fCreditsView->Insert("Special Thanks To:\n");

	fCreditsView->SetFontAndColor(be_plain_font);
	fCreditsView->Insert("Michael Phipps (project founder)\n\n");

/*
	// TODO: Add these (somehow)
	Brian Paul (Mesa)
	Silicon Graphics Inc. (GLU)
	Mark Kilgard & Be Inc. & Jam Hamby (GLUT)
*/

	font.SetSize(be_bold_font->Size() + 2);
	font.SetFace(B_BOLD_FACE);
	fCreditsView->SetFontAndColor(&font);
	fCreditsView->Insert("\nCopyrights\n\n");

	font.SetSize(be_bold_font->Size()+1);
	font.SetFace(B_ITALIC_FACE);

	fCreditsView->SetFontAndColor(&font);
	fCreditsView->Insert("AntiGrain Geometry\n");
	fCreditsView->SetFontAndColor(be_plain_font);
	fCreditsView->Insert("Copyright (C) 2002-2005 Maxim Shemanarev (McSeem)\n\n");

	font.SetFace(B_ITALIC_FACE);
	fCreditsView->SetFontAndColor(&font);
	fCreditsView->Insert("PDFLib\n");
	fCreditsView->SetFontAndColor(be_plain_font);
	fCreditsView->Insert(
		"Copyright (c) 1997-2005 PDFlib GmbH and Thomas Merz. "
		"All rights reserved.\n"
		"PDFlib and the PDFlib logo are registered trademarks of PDFlib GmbH.\n\n");

	font.SetFace(B_ITALIC_FACE);
	fCreditsView->SetFontAndColor(&font);
	fCreditsView->Insert("FreeType2\n");
	fCreditsView->SetFontAndColor(be_plain_font);
	fCreditsView->Insert("Portions of this software are copyright (C) 1996-2002 The FreeType"
		" Project (www.freetype.org).  All rights reserved.\n\n");
}


AboutView::~AboutView(void)
{
}


void
AboutView::AttachedToWindow(void)
{
	Window()->SetPulseRate(500000);
}


void
AboutView::MouseDown(BPoint pt)
{
	BRect r(92, 26, 105, 31);
	if (r.Contains(pt))
		printf("Easter Egg\n");
}


void
AboutView::Draw(BRect update)
{
	if (fLogo)
		DrawBitmap(fLogo, fDrawPoint);
}


void
AboutView::Pulse(void)
{
	fUptimeView->SetText(UptimeToString());
}


void
AboutView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MOVE_INFO_VIEW:
			fInfoView->MoveBy(0, -5);
			break;

		default:
			BView::MessageReceived(msg);
	}
}


//	#pragma mark -


const char *
UptimeToString(void)
{
	char string[255];
	int64 days, hours, minutes, seconds, remainder;
	int64 systime = system_time();

	if (systime > 86400000000LL) {
		days = systime / 86400000000LL;
		remainder = systime % 86400000000LL;

		hours = remainder / 3600000000LL;
		remainder = remainder % 3600000000LL;

		minutes = remainder / 60000000;
		remainder = remainder % 60000000;

		seconds = remainder / 1000000;
		sprintf(string, "%lld days, %lld hours, %lld minutes, %lld seconds",
			days, hours, minutes, seconds);
	} else if (systime > 3600000000LL) {
		hours = systime / 3600000000LL;
		remainder = systime % 3600000000LL;

		minutes = remainder / 60000000;
		remainder = remainder % 60000000;

		seconds = remainder / 1000000;
		sprintf(string, "%lld hours, %lld minutes, %lld seconds",
			hours, minutes, seconds);
	} else if (systime > 60000000) {
		minutes = systime / 60000000;
		remainder = systime % 60000000;

		seconds = remainder / 1000000;
		sprintf(string, "%lld minutes, %lld seconds", minutes, seconds);
	} else {
		seconds = systime / 1000000;
		sprintf(string, "%lld seconds", seconds);
	}

	return string;
}


int
main()
{
	AboutApp app;
	app.Run();
	return 0;
}

