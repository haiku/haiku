/*
 * Copyright (c) 2005-2008, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		René Gollent
 */

#include <ctype.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <fs_attr.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <OS.h>
#include <Path.h>
#include <Query.h>
#include <Resources.h>
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>
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

#include "HyperTextActions.h"
#include "HyperTextView.h"
#include "Utilities.h"


#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

#define SCROLL_CREDITS_VIEW 'mviv'
#define READ_APP_QUERY_ENT 'raqe'


static const char *UptimeToString(char string[], size_t size);
static const char *MemUsageToString(char string[], size_t size,
	system_info *info);

static const rgb_color kDarkGrey = { 100, 100, 100, 255 };
static const rgb_color kHaikuGreen = { 42, 131, 36, 255 };
static const rgb_color kHaikuOrange = { 255, 69, 0, 255 };
static const rgb_color kHaikuYellow = { 255, 176, 0, 255 };
static const rgb_color kLinkBlue = { 80, 80, 200, 255 };


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

		virtual void AttachedToWindow();
		virtual void Pulse();

		virtual void FrameResized(float width, float height);
		virtual void Draw(BRect update);
		virtual void MessageReceived(BMessage *msg);
		virtual void MouseDown(BPoint pt);

		void	AddCopyrightEntry(const char *name, const char *text,
					const Licenses& licenses, const char *url);
		void	AddCopyrightEntry(const char *name, const char *text,
					const char *url = NULL);
		void	AddCopyrightEntry(const BMessage& packageDescription);
		void	PickRandomHaiku();

	private:
		void	_AddCopyrightsFromAttribute();

		BStringView		*fMemView;
		BStringView		*fUptimeView;
		BView			*fInfoView;
		HyperTextView	*fCreditsView;

		BBitmap			*fLogo;

		BPoint			fDrawPoint;

		bigtime_t		fLastActionTime;
		BMessageRunner	*fScrollRunner;
		BQuery			fAppsQuery;
};


//	#pragma mark -


AboutApp::AboutApp(void)
	: BApplication("application/x-vnd.Haiku-About")
{
	AboutWindow *window = new AboutWindow();
	window->Show();
}


//	#pragma mark -


AboutWindow::AboutWindow()
	: BWindow(BRect(0, 0, 500, 300), "About This System", B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	AboutView *view = new AboutView(Bounds());
	AddChild(view);

	// start reading from the app query
	BMessage msg(READ_APP_QUERY_ENT);
	BMessenger msgr(view);
	msgr.SendMessage(&msg);

	MoveTo((BScreen().Frame().Width() - Bounds().Width()) / 2,
		(BScreen().Frame().Height() - Bounds().Height()) / 2 );
}


bool
AboutWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


AboutView::AboutView(const BRect &rect)
	: BView(rect, "aboutview", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED),
	fLastActionTime(system_time()),
	fScrollRunner(NULL)
{
	fLogo = BTranslationUtils::GetBitmap(B_PNG_FORMAT, "haikulogo.png");
	if (fLogo) {
		fDrawPoint.x = (225-fLogo->Bounds().Width()) / 2;
		fDrawPoint.y = 0;
	}

	// Begin Construction of System Information controls

	font_height height;
	float labelHeight, textHeight;

	system_info systemInfo;
	get_system_info(&systemInfo);

	be_plain_font->GetHeight(&height);
	textHeight = height.ascent + height.descent + height.leading;

	be_bold_font->GetHeight(&height);
	labelHeight = height.ascent + height.descent + height.leading;

	BRect r(0, 0, 225, Bounds().bottom);
	if (fLogo)
		r.OffsetBy(0, fLogo->Bounds().Height());

	fInfoView = new BView(r, "infoview", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,
		B_WILL_DRAW);
	fInfoView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fInfoView->SetLowColor(fInfoView->ViewColor());
	fInfoView->SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	AddChild(fInfoView);

	// Add all the various labels for system infomation

	BStringView *stringView;

	// OS Version
	r.Set(5, 5, 224, labelHeight + 5);
	stringView = new BStringView(r, "oslabel", "Version:");
	stringView->SetFont(be_bold_font);
	fInfoView->AddChild(stringView);
	stringView->ResizeToPreferred();

	// we update "labelHeight" to the actual height of the string view
	labelHeight = stringView->Bounds().Height();

	r.OffsetBy(0, labelHeight);
	r.bottom = r.top + textHeight;

	char string[1024];
	strcpy(string, "Unknown");

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
			strcpy(string, versionInfo.short_info);
	}

	// Add revision from uname() info
	utsname unameInfo;
	if (uname(&unameInfo) == 0) {
		long revision;
		if (sscanf(unameInfo.version, "r%ld", &revision) == 1) {
			char version[16];
			snprintf(version, sizeof(version), "%ld", revision);
			strlcat(string, " (Revision ", sizeof(string));
			strlcat(string, version, sizeof(string));
			strlcat(string, ")", sizeof(string));
		}
	}

	stringView = new BStringView(r, "ostext", string);
	fInfoView->AddChild(stringView);
	stringView->ResizeToPreferred();

	// GCC version
#if __GNUC__ != 2
	r.OffsetBy(0, textHeight);
	r.bottom = r.top + textHeight;

	snprintf(string, sizeof(string), "GCC %d", __GNUC__);

	stringView = new BStringView(r, "gcctext", string);
	fInfoView->AddChild(stringView);
	stringView->ResizeToPreferred();
#endif

	// CPU count, type and clock speed
	r.OffsetBy(0, textHeight * 1.5);
	r.bottom = r.top + labelHeight;

	if (systemInfo.cpu_count > 1)
		sprintf(string, "%ld Processors:", systemInfo.cpu_count);
	else
		strcpy(string, "Processor:");

	stringView = new BStringView(r, "cpulabel", string);
	stringView->SetFont(be_bold_font);
	fInfoView->AddChild(stringView);
	stringView->ResizeToPreferred();

	BString cpuType;
	cpuType << get_cpu_vendor_string(systemInfo.cpu_type)
		<< " " << get_cpu_model_string(&systemInfo);

	r.OffsetBy(0, labelHeight);
	r.bottom = r.top + textHeight;
	stringView = new BStringView(r, "cputext", cpuType.String());
	fInfoView->AddChild(stringView);
	stringView->ResizeToPreferred();

	r.OffsetBy(0, textHeight);
	r.bottom = r.top + textHeight;

	int32 clockSpeed = get_rounded_cpu_speed();
	if (clockSpeed < 1000)
		sprintf(string,"%ld MHz", clockSpeed);
	else
		sprintf(string,"%.2f GHz", clockSpeed / 1000.0f);

	stringView = new BStringView(r, "mhztext", string);
	fInfoView->AddChild(stringView);
	stringView->ResizeToPreferred();

	// RAM
	r.OffsetBy(0, textHeight * 1.5);
	r.bottom = r.top + labelHeight;
	stringView = new BStringView(r, "ramlabel", "Memory:");
	stringView->SetFont(be_bold_font);
	fInfoView->AddChild(stringView);
	stringView->ResizeToPreferred();

	r.OffsetBy(0, labelHeight);
	r.bottom = r.top + textHeight;

	fMemView = new BStringView(r, "ramtext", "");
	fInfoView->AddChild(fMemView);
	fMemView->SetText(MemUsageToString(string, sizeof(string), &systemInfo));

	// Kernel build time/date
	r.OffsetBy(0, textHeight * 1.5);
	r.bottom = r.top + labelHeight;
	stringView = new BStringView(r, "kernellabel", "Kernel:");
	stringView->SetFont(be_bold_font);
	fInfoView->AddChild(stringView);
	stringView->ResizeToPreferred();

	r.OffsetBy(0, labelHeight);
	r.bottom = r.top + textHeight;

	snprintf(string, sizeof(string), "%s %s",
		systemInfo.kernel_build_date, systemInfo.kernel_build_time);

	stringView = new BStringView(r, "kerneltext", string);
	fInfoView->AddChild(stringView);
	stringView->ResizeToPreferred();

	// Uptime
	r.OffsetBy(0, textHeight * 1.5);
	r.bottom = r.top + labelHeight;
	stringView = new BStringView(r, "uptimelabel", "Time Running:");
	stringView->SetFont(be_bold_font);
	fInfoView->AddChild(stringView);
	stringView->ResizeToPreferred();

	r.OffsetBy(0, labelHeight);
	r.bottom = r.top + textHeight;

	fUptimeView = new BStringView(r, "uptimetext", "");
	fInfoView->AddChild(fUptimeView);
	// string width changes, so we don't do ResizeToPreferred()

	fUptimeView->SetText(UptimeToString(string, sizeof(string)));

	// Begin construction of the credits view
	r = Bounds();
	r.left += fInfoView->Bounds().right + 1;
	r.right -= B_V_SCROLL_BAR_WIDTH;

	fCreditsView = new HyperTextView(r, "credits",
		r.OffsetToCopy(0, 0).InsetByCopy(5, 5), B_FOLLOW_ALL);
	fCreditsView->SetFlags(fCreditsView->Flags() | B_FRAME_EVENTS );
	fCreditsView->SetStylable(true);
	fCreditsView->MakeEditable(false);
	fCreditsView->MakeSelectable(false);
	fCreditsView->SetWordWrap(true);

	BScrollView *creditsScroller = new BScrollView("creditsScroller",
		fCreditsView, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS, false, true,
		B_PLAIN_BORDER);
	AddChild(creditsScroller);

	// Haiku copyright
	BFont font(be_bold_font);
	font.SetSize(font.Size() + 4);

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuGreen);
	fCreditsView->Insert("Haiku\n");

	time_t time = ::time(NULL);
	struct tm* tm = localtime(&time);
	int32 year = tm->tm_year + 1900;
	if (year < 2008)
		year = 2008;
	snprintf(string, sizeof(string),
		"Copyright " B_UTF8_COPYRIGHT " 2001-%ld The Haiku project. ", year);

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(string);

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert("The copyright to the Haiku code is property of "
		"Haiku, Inc. or of the respective authors where expressly noted "
		"in the source."
		"\n\n");


	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kLinkBlue);
	fCreditsView->InsertHyperText("http://www.haiku-os.org",
		new URLAction("http://www.haiku-os.org"));
	fCreditsView->Insert("\n\n");

	font.SetSize(be_bold_font->Size());
	font.SetFace(B_BOLD_FACE | B_ITALIC_FACE);

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuOrange);
	fCreditsView->Insert("Current Maintainers:\n");

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(
		"Ithamar R. Adema\n"
		"Bruno G. Albuquerque\n"
		"Stephan Aßmus\n"
		"Salvatore Benedetto\n"
		"Stefano Ceccherini\n"
		"Rudolf Cornelissen\n"
		"Alexandre Deckner\n"
		"Oliver Ruiz Dorantes\n"
		"Axel Dörfler\n"
		"Jérôme Duval\n"
		"René Gollent\n"
		"Karsten Heimrich\n"
		"Philippe Houdoin\n"
		"Maurice Kalinowski\n"
		"Euan Kirkhope\n"
		"Ryan Leavengood\n"
		"Michael Lotz\n"
		"David McPaul\n"
		"Fredrik Modéen\n"
		"Marcus Overhagen\n"
		"Michael Pfeiffer\n"
		"François Revol\n"
		"Andrej Spielmann\n"
		"Oliver Tappe\n"
		"Gerasim Troeglazov\n"
		"Ingo Weinhold\n"
		"Siarzhuk Zharski\n"
		"\n");

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuOrange);
	fCreditsView->Insert("Past Maintainers:\n");

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(
		"Andrew Bachmann\n"
		"Tyler Dauwalder\n"
		"Daniel Furrer\n"
		"Andre Alves Garzia\n"
		"Erik Jaesler\n"
		"Marcin Konicki\n"
		"Waldemar Kornewald\n"
		"Thomas Kurschel\n"
		"Frans Van Nispen\n"
		"Adi Oanca\n"
		"Michael Phipps\n"
		"Niels Sascha Reedijk\n"
		"David Reid\n"
		"Hugo Santos\n"
		"Alexander G. M. Smith\n"
		"Jonas Sundström\n"
		"Bryan Varner\n"
		"Nathan Whitehorn\n"
		"Michael Wilber\n"
		"Jonathan Yoder\n"
		"Gabe Yoder\n"
		"\n");

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuOrange);
	fCreditsView->Insert("Website, Marketing & Documentation:\n");

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(
		"Phil Greenway\n"
		"Gavin James\n"
		"Urias McCullough\n"
		"Niels Sascha Reedijk\n"
		"Jonathan Yoder\n"
		"\n");

	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuOrange);
	fCreditsView->Insert("Contributors:\n");

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(
		"Andrea Anzani\n"
		"Andre Braga\n"
		"Bruce Cameron\n"
		"Greg Crain\n"
		"David Dengg\n"
		"John Drinkwater\n"
		"Cian Duffy\n"
		"Christian Fasshauer\n"
		"Andreas Färber\n"
		"Marc Flerackers\n"
		"Matthijs Hollemans\n"
		"Mathew Hounsell\n"
		"Morgan Howe\n"
		"Carwyn Jones\n"
		"Vasilis Kaoutsis\n"
		"James Kim\n"
		"Shintaro Kinugawa\n"
		"Jan Klötzke\n"
		"Kurtis Kopf\n"
		"Tomáš Kučera\n"
		"Luboš Kulič\n"
		"Elad Lahav\n"
		"Anthony Lee\n"
		"Santiago Lema\n"
		"Raynald Lesieur\n"
		"Oscar Lesta\n"
		"Jerome Leveque\n"
		"Christof Lutteroth\n"
		"Graham MacDonald\n"
		"Jan Matějek\n"
		"Brian Matzon\n"
		"Christopher ML Zumwalt May\n"
		"Andrew McCall\n"
		"Scott McCreary\n"
		"Michele (zuMi)\n"
		"Marius Middelthon\n"
		"Marco Minutoli\n"
		"Misza\n"
		"MrSiggler\n"
		"Alan Murta\n"
		"Pahtz\n"
		"Michael Paine\n"
		"Adrian Panasiuk\n"
		"Francesco Piccinno\n"
		"David Powell\n"
		"Jeremy Rand\n"
		"Hartmut Reh\n"
		"Daniel Reinhold\n"
		"Samuel Rodriguez Perez\n"
		"Thomas Roell\n"
		"Rafael Romo\n"
		"Philippe Saint-Pierre\n"
		"Ralf Schülke\n"
		"Reznikov Sergei\n"
		"Zousar Shaker\n"
		"Daniel Switkin\n"
		"Atsushi Takamatsu\n"
		"James Urquhart\n"
		"Jason Vandermark\n"
		"Sandor Vroemisse\n"
		"Denis Washington\n"
		"Ulrich Wimboeck\n"
		"James Woodcock\n"
		"Artur Wyszynski\n"
		"Gerald Zajac\n"
		"Clemens Zeidler\n"
		"Łukasz Zemczak\n"
		"JiSheng Zhang\n"
		"Zhao Shuai\n"
		"\n" B_UTF8_ELLIPSIS " and probably some more we forgot to mention "
		"(sorry!)"
		"\n\n");


	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuOrange);
	fCreditsView->Insert("Special Thanks To:\n");

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert("Travis Geiselbrecht (and his NewOS kernel)\n");
	fCreditsView->Insert("Michael Phipps (project founder)\n\n");

	// copyrights for various projects we use

	font.SetSize(be_bold_font->Size() + 4);
	font.SetFace(B_BOLD_FACE);
	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuGreen);
	fCreditsView->Insert("\nCopyrights\n\n");

	fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert("[Click a license name to read the respective "
		"license.]\n\n");

	// Haiku license
	fCreditsView->Insert("The code that is unique to Haiku, especially the "
		"kernel and all code that applications may link against, is "
		"distributed under the terms of the ");
	fCreditsView->InsertHyperText("MIT license",
		new OpenFileAction("/etc/licenses/MIT"));
	fCreditsView->Insert(". Some system libraries contain third party code "
		"distributed under the LGPL license. You can find the copyrights "
		"to third party code below.\n\n");


	// GNU copyrights
	AddCopyrightEntry("The GNU Project",
		"Contains software from the GNU Project, "
		"released under the GPL and LGPL licences:\n"
		"GNU C Library, "
		"GNU coretools, diffutils, findutils, "
		"sharutils, gawk, bison, m4, make, "
		"gdb, wget, ncurses, termcap, "
		"Bourne Again Shell.\n"
		"Copyright " B_UTF8_COPYRIGHT " The Free Software Foundation.",
		Licenses("GNU LGPL v2.1", "GNU GPL v2", "GNU GPL v3", NULL),
		"http://www.gnu.org");

	// FreeBSD copyrights
	AddCopyrightEntry("The FreeBSD Project",
		"Contains software from the FreeBSD Project, "
		"released under the BSD licence:\n"
		"cal, ftpd, ping, telnet, "
		"telnetd, traceroute\n"
		"Copyright " B_UTF8_COPYRIGHT " 1994-2008 The FreeBSD Project.  "
		"All rights reserved.",
		"http://www.freebsd.org");

	// NetBSD copyrights
	AddCopyrightEntry("The NetBSD Project",
		"Contains software developed by the NetBSD, "
		"Foundation, Inc. and its contributors:\n"
		"ftp, tput\n"
		"Copyright " B_UTF8_COPYRIGHT " 1996-2008 The NetBSD Foundation, Inc.  "
		"All rights reserved.",
		"http://www.netbsd.org");

	// FFMpeg copyrights
	AddCopyrightEntry("FFMpeg libavcodec",
		"Copyright " B_UTF8_COPYRIGHT " 2000-2007 Fabrice Bellard, et al.",
		"http://www.ffmpeg.org");

	// AGG copyrights
	AddCopyrightEntry("AntiGrain Geometry",
		"Copyright " B_UTF8_COPYRIGHT " 2002-2006 Maxim Shemanarev (McSeem).",
		"http://www.antigrain.com");

	// PDFLib copyrights
	AddCopyrightEntry("PDFLib",
		"Copyright " B_UTF8_COPYRIGHT " 1997-2006 PDFlib GmbH and Thomas Merz. "
		"All rights reserved.\n"
		"PDFlib and PDFlib logo are registered trademarks of PDFlib GmbH.",
		"http://www.pdflib.com");

	// FreeType copyrights
	AddCopyrightEntry("FreeType2",
		"Portions of this software are copyright " B_UTF8_COPYRIGHT " 1996-2006 "
		"The FreeType Project.  All rights reserved.",
		"http://www.freetype.org");

	// Mesa3D (http://www.mesa3d.org) copyrights
	AddCopyrightEntry("Mesa",
		"Copyright " B_UTF8_COPYRIGHT " 1999-2006 Brian Paul. "
		"Mesa3D project.  All rights reserved.",
		"http://www.mesa3d.org");

	// SGI's GLU implementation copyrights
	AddCopyrightEntry("GLU",
		"Copyright " B_UTF8_COPYRIGHT " 1991-2000 Silicon Graphics, Inc. "
		"SGI's Software FreeB license.  All rights reserved.");

	// GLUT implementation copyrights
	AddCopyrightEntry("GLUT",
		"Copyright " B_UTF8_COPYRIGHT " 1994-1997 Mark Kilgard. "
		"All rights reserved.\n"
		"Copyright " B_UTF8_COPYRIGHT " 1997 Be Inc.\n"
		"Copyright " B_UTF8_COPYRIGHT " 1999 Jake Hamby.");

	// OpenGroup & DEC (BRegion backend) copyright
	AddCopyrightEntry("BRegion backend (XFree86)",
		"Copyright " B_UTF8_COPYRIGHT " 1987, 1988, 1998  The Open Group.\n"
		"Copyright " B_UTF8_COPYRIGHT " 1987, 1988 Digital Equipment "
		"Corporation, Maynard, Massachusetts.\n"
		"All rights reserved.");

	// Konatu font
	AddCopyrightEntry("Konatu font",
		"Copyright " B_UTF8_COPYRIGHT " 2002- MASUDA mitiya.\n"
		"MIT license. All rights reserved.");

	// expat copyrights
	AddCopyrightEntry("expat",
		"Copyright " B_UTF8_COPYRIGHT " 1998, 1999, 2000 Thai Open Source "
		"Software Center Ltd and Clark Cooper.\n"
		"Copyright " B_UTF8_COPYRIGHT " 2001, 2002, 2003 Expat maintainers.");

	// zlib copyrights
	AddCopyrightEntry("zlib",
		"Copyright " B_UTF8_COPYRIGHT " 1995-2004 Jean-loup Gailly and Mark "
		"Adler.");

	// zip copyrights
	AddCopyrightEntry("Info-ZIP",
		"Copyright " B_UTF8_COPYRIGHT " 1990-2002 Info-ZIP. All rights reserved.");

	// bzip2 copyrights
	AddCopyrightEntry("bzip2",
		"Copyright " B_UTF8_COPYRIGHT " 1996-2005 Julian R Seward. All rights "
		"reserved.");

	// VIM copyrights
	AddCopyrightEntry("Vi IMproved",
		"Copyright " B_UTF8_COPYRIGHT " Bram Moolenaar et al.");

	// lp_solve copyrights
	AddCopyrightEntry("lp_solve",
		"Copyright " B_UTF8_COPYRIGHT
			" Michel Berkelaar, Kjell Eikland, Peter Notebaert",
		"http://lpsolve.sourceforge.net/");
			// license: LGPL

	// OpenEXR copyrights
	AddCopyrightEntry("OpenEXR",
		"Copyright " B_UTF8_COPYRIGHT " 2002-2005 Industrial Light & Magic, "
		"a division of Lucas Digital Ltd. LLC.");

	// Bullet copyrights
	AddCopyrightEntry("Bullet",
		"Copyright " B_UTF8_COPYRIGHT " 2003-2008 Erwin Coumans",
		"http://www.bulletphysics.com");

	// atftp copyrights
	AddCopyrightEntry("atftp",
		"Copyright " B_UTF8_COPYRIGHT " 2000 Jean-Pierre Lefebvre and Remi "
		"Lefebvre");

	// Netcat copyrights
	AddCopyrightEntry("Netcat",
		"Copyright " B_UTF8_COPYRIGHT " 1996 Hobbit");

	// acpica copyrights
	AddCopyrightEntry("acpica",
		"Copyright " B_UTF8_COPYRIGHT " 1999-2006 Intel Corp.");

	// unrar copyrights
	AddCopyrightEntry("unrar",
		"Copyright " B_UTF8_COPYRIGHT " 2002-2008 Alexander L. Roshal. "
		"All rights reserved.",
		"http://www.rarlab.com");

// p7zip copyrights
//	AddCopyrightEntry("p7zip",
//		"Copyright " B_UTF8_COPYRIGHT " 2008 Igor Pavlov. "
//		"All rights reserved.");

	// libpng copyrights
	AddCopyrightEntry("libpng",
		"Copyright " B_UTF8_COPYRIGHT " 2004, 2006-2008 Glenn "
		"Randers-Pehrson.");

	// libprint copyrights
	AddCopyrightEntry("libprint",
		"Copyright " B_UTF8_COPYRIGHT " 1999-2000 Y.Takagi. All rights "
		"reserved.");

	// cortex copyrights
	AddCopyrightEntry("Cortex",
		"Copyright " B_UTF8_COPYRIGHT " 1999-2000 Eric Moon.");

	// FluidSynth copyrights
	AddCopyrightEntry("FluidSynth",
		"Copyright " B_UTF8_COPYRIGHT " 2003 Peter Hanappe and others.");

	// CannaIM copyrights
	AddCopyrightEntry("CannaIM",
		"Copyright " B_UTF8_COPYRIGHT " 1999 Masao Kawamura.");

	// libxml2, libxslt, libexslt copyrights
	AddCopyrightEntry("libxml2, libxslt",
		"Copyright " B_UTF8_COPYRIGHT " 1998-2003 Daniel Veillard. "
		"All rights reserved.");

	AddCopyrightEntry("libexslt",
		"Copyright " B_UTF8_COPYRIGHT " 2001-2002 Thomas Broyer, Charlie "
		"Bozeman and Daniel Veillard.  All rights reserved.");

	// Xiph.org Foundation copyrights
	AddCopyrightEntry("Xiph.org Foundation",
		"libvorbis, libogg, libtheora, libspeex"
		"Copyright " B_UTF8_COPYRIGHT " 1994-2008 Xiph.Org. "
		"All rights reserved.",
		"http://www.xiph.org");

	// The Tcpdump Group
	AddCopyrightEntry("The Tcpdump Group",
		"tcpdump, libpcap",
		"http://www.tcpdump.org");

	// Matroska
	AddCopyrightEntry("libmatroska",
		"Copyright " B_UTF8_COPYRIGHT " 2002-2003 Steve Lhomme. "
		"All rights reserved.",
		"http://www.matroska.org");

	// BColorQuantizer (originally CQuantizer code)
	AddCopyrightEntry("CQuantizer",
		"Copyright " B_UTF8_COPYRIGHT " 1996-1997 Jeff Prosise. "
		"All rights reserved.");

	// MAPM (Mike's Arbitrary Precision Math Library) used by DeskCalc
	AddCopyrightEntry("MAPM",
		"Copyright " B_UTF8_COPYRIGHT "1999-2007 Michael C. Ring. "
		"All rights reserved.",
		"http://tc.umn.edu/~ringx004");

// OpenSound
//	AddCopyrightEntry("OpenSound",
//		"Copyright " B_UTF8_COPYRIGHT " 1996-2008 4Front Technologies ",
//		"http://www.opensound.com");
// BSD license

#ifdef __INTEL__
	// Udis86 copyrights
	AddCopyrightEntry("Udis86",
		"Copyright " B_UTF8_COPYRIGHT " 2002, 2003, 2004 Vivek Mohan. "
		"All rights reserved.",
		"http://udis86.sourceforge.net");
#endif

	_AddCopyrightsFromAttribute();

	// Build a list of installed applications and show their
	// long version info. Well-behaved apps usually give
	// copyright info there.

	font.SetSize(be_bold_font->Size() + 4);
	font.SetFace(B_BOLD_FACE);
	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kHaikuGreen);
	fCreditsView->Insert("\nInstalled applications\n\n");

	BVolume bootVolume;
	BVolumeRoster().GetBootVolume(&bootVolume);
	fAppsQuery.SetVolume(&bootVolume);
	if (fAppsQuery.SetPredicate(
			"((BEOS:APP_SIG==\"**\")&&(name!=\"*.so\")&&(name!=\"*.rsrc\")&&"
			"(BEOS:TYPE==\"application/x-vnd.Be-elfexecutable\"))") >= B_OK) {
		fAppsQuery.Fetch();
	}
}


AboutView::~AboutView(void)
{
	delete fScrollRunner;
}


void
AboutView::AttachedToWindow(void)
{
	BView::AttachedToWindow();
	Window()->SetPulseRate(500000);
	SetEventMask(B_POINTER_EVENTS);
}


void
AboutView::MouseDown(BPoint pt)
{
	BRect r(92, 26, 105, 31);
	if (r.Contains(pt)) {
		printf("Easter Egg\n");
		PickRandomHaiku();
	}

	if (Bounds().Contains(pt)) {
		fLastActionTime = system_time();
		delete fScrollRunner;
		fScrollRunner = NULL;
	}
}


void
AboutView::FrameResized(float width, float height)
{
	BRect r = fCreditsView->Bounds();
	r.OffsetTo(B_ORIGIN);
	r.InsetBy(3, 3);
	fCreditsView->SetTextRect(r);
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
	char string[255];
	system_info info;
	get_system_info(&info);
	fUptimeView->SetText(UptimeToString(string, sizeof(string)));
	fMemView->SetText(MemUsageToString(string, sizeof(string), &info));

	if (fScrollRunner == NULL && (system_time() > fLastActionTime + 10000000)) {
		BMessage message(SCROLL_CREDITS_VIEW);
		//fScrollRunner = new BMessageRunner(this, &message, 300000, -1);
	}
}


void
AboutView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case SCROLL_CREDITS_VIEW:
		{
			BScrollBar *scrollBar = fCreditsView->ScrollBar(B_VERTICAL);
			if (scrollBar == NULL)
				break;
			float max, min;
			scrollBar->GetRange(&min, &max);
			if (scrollBar->Value() < max)
				fCreditsView->ScrollBy(0, 5);

			break;
		}

		case READ_APP_QUERY_ENT:
		{
			BEntry ent;
			if (fAppsQuery.GetNextEntry(&ent) < B_OK) {
				fAppsQuery.Clear();
				fCreditsView->MakeSelectable(true);
				break;
			}
			BFile file;
			BPath path;
			if (ent.Exists() &&
				ent.GetPath(&path) >= B_OK &&
				file.SetTo(&ent, B_READ_ONLY) >= B_OK) {
				/* filter only apps */
				if (strncmp(path.Path(), "/boot/apps", 10) == 0) {
					BAppFileInfo appFileInfo(&file);
					uint32 flags;
					version_info version;
					if (appFileInfo.InitCheck() >= B_OK &&
						appFileInfo.GetAppFlags(&flags) >= B_OK &&
						appFileInfo.GetVersionInfo(&version,
							B_APP_VERSION_KIND) >= B_OK) {
						//printf("AppFileInfo for %s :\n", path.Path());
						//printf("flags: %08x\n", flags);
						BString name;
						BString info;
						name << path.Leaf();
						if (strlen(version.short_info) &&
							strcmp(version.short_info, path.Leaf()))
							name << " (" << version.short_info << ")";
						/*
						info << "\tVersion: ";
						info << version.major << ".";
						info << version.middle << ".";
						info << version.minor;
						char varieties[] = "dabgmf";
						if (version.variety > B_FINAL_VERSION)
							info << "?";
						else
							info << varieties[version.variety];
						info << version.internal;
						info << "\n";
						*/
						info << version.long_info;
						AddCopyrightEntry(name.String(), info.String());

					}
				}
			}
			// note for self: read next entry :)
			BMessage m(READ_APP_QUERY_ENT);
			BMessenger(this).SendMessage(&m);
			break;
		}

		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
AboutView::AddCopyrightEntry(const char *name, const char *text,
	const char *url)
{
	AddCopyrightEntry(name, text, NULL, url);
}


void
AboutView::AddCopyrightEntry(const char *name, const char *text,
	const Licenses& licenses, const char *url)
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

	if (licenses.CountLicenses() > 0) {
		if (licenses.CountLicenses() > 1)
			fCreditsView->Insert("Licenses: ");
		else
			fCreditsView->Insert("License: ");

		for (int32 i = 0; i < licenses.CountLicenses(); i++) {
			const char* license = licenses.LicenseAt(i);
			BString licensePath("/etc/licenses/");
			licensePath += license;

			if (i > 0)
				fCreditsView->Insert(", ");

			fCreditsView->InsertHyperText(license,
				new OpenFileAction(licensePath));
		}

		fCreditsView->Insert("\n");
	}

	if (url) {
		fCreditsView->SetFontAndColor(be_plain_font, B_FONT_ALL, &kLinkBlue);
		fCreditsView->InsertHyperText(url, new URLAction(url));
		fCreditsView->Insert("\n");
	}
	fCreditsView->Insert("\n");
}


void
AboutView::AddCopyrightEntry(const BMessage& packageDescription)
{
	const char* package;
	const char* copyright;
	const char* url;

	// package and copyright are mandatory
	if (packageDescription.FindString("Package", &package) != B_OK
		|| packageDescription.FindString("Copyright", &copyright) != B_OK) {
		return;
	}

	// URL is optional
	if (packageDescription.FindString("URL", &url) != B_OK)
		url = NULL;

	BString copyrightLine("Copyright " B_UTF8_COPYRIGHT " ");
	copyrightLine += copyright;

	AddCopyrightEntry(package, copyrightLine.String(), packageDescription, url);
}


void
AboutView::PickRandomHaiku()
{
	BFile fortunes(
#ifdef __HAIKU__
		"/etc/fortunes/Haiku",
#else
		"data/etc/fortunes/Haiku",
#endif
		B_READ_ONLY);
	struct stat st;
	if (fortunes.InitCheck() < B_OK)
		return;
	if (fortunes.GetStat(&st) < B_OK)
		return;
	char *buff = (char *)malloc((size_t)st.st_size + 1);
	if (!buff)
		return;
	buff[(size_t)st.st_size] = '\0';
	BList haikuList;
	if (fortunes.Read(buff, (size_t)st.st_size) == (ssize_t)st.st_size) {
		char *p = buff;
		while (p && *p) {
			char *e = strchr(p, '%');
			BString *s = new BString(p, e ? (e - p) : -1);
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
	BString *s = (BString *)haikuList.ItemAt(rand() % haikuList.CountItems());
	BFont font(be_bold_font);
	font.SetSize(be_bold_font->Size());
	font.SetFace(B_BOLD_FACE | B_ITALIC_FACE);
	fCreditsView->SelectAll();
	fCreditsView->Delete();
	fCreditsView->SetFontAndColor(&font, B_FONT_ALL, &kDarkGrey);
	fCreditsView->Insert(s->String());
	fCreditsView->Insert("\n");
	while ((s = (BString *)haikuList.RemoveItem((int32)0))) {
		delete s;
	}
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
	int attrFD = fs_open_attr(appFD, "COPYRIGHTS", B_STRING_TYPE, O_RDONLY);
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
			AddCopyrightEntry(package);
			package.MakeEmpty();
		}
	}

	// flush current package
	AddCopyrightEntry(package);
#endif
}


//	#pragma mark -


static const char *
MemUsageToString(char string[], size_t size, system_info *info)
{
	snprintf(string, size, "%d MB total, %d MB used (%d%%)",
			int(info->max_pages / 256.0f + 0.5f),
			int(info->used_pages / 256.0f + 0.5f),
			int(100 * info->used_pages / info->max_pages));

	return string;
}


static const char *
UptimeToString(char string[], size_t size)
{
	int64 days, hours, minutes, seconds, remainder;
	int64 systime = system_time();

	days = systime / 86400000000LL;
	remainder = systime % 86400000000LL;

	hours = remainder / 3600000000LL;
	remainder = remainder % 3600000000LL;

	minutes = remainder / 60000000;
	remainder = remainder % 60000000;

	seconds = remainder / 1000000;

	char *str = string;
	if (days) {
		str += snprintf(str, size, "%lld day%s",days, days > 1 ? "s" : "");
	}
	if (hours) {
		str += snprintf(str, size - strlen(string), "%s%lld hour%s",
				str != string ? ", " : "",
				hours, hours > 1 ? "s" : "");
	}
	if (minutes) {
		str += snprintf(str, size - strlen(string), "%s%lld minute%s",
				str != string ? ", " : "",
				minutes, minutes > 1 ? "s" : "");
	}

	if (seconds || str == string) {
		// Haiku would be well-known to boot very fast.
		// Let's be ready to handle below minute uptime, zero second included ;-)
		str += snprintf(str, size - strlen(string), "%s%lld second%s",
				str != string ? ", " : "",
				seconds, seconds > 1 ? "s" : "");
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

