/*
 * Copyright (c) 2001-2005, Haiku, Inc.
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

#include <stdio.h>

#define MOVE_INFO_VIEW 'mviv'

const char *CPUToString(cpu_type type);
const char *UptimeToString(void);

class AboutApp : public BApplication
{
public:
	AboutApp(void);
};

class AboutWindow : public BWindow
{
public:
			AboutWindow(void);
	bool	QuitRequested(void);
};

class AboutView : public BView
{
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

int main(void)
{
	AboutApp app;
	app.Run();
	return 0;
}

AboutApp::AboutApp(void)
 : BApplication("application/x-vnd.haiku-AboutHaiku")
{
	AboutWindow *win = new AboutWindow();
	win->Show();
}


AboutWindow::AboutWindow(void)
 : BWindow( BRect(0,0,500,300), "About Haiku", B_TITLED_WINDOW, 
 			B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	AboutView *view = new AboutView(Bounds());
	AddChild(view);
	
	MoveTo( (BScreen().Frame().Width() - Bounds().Width()) / 2,
			(BScreen().Frame().Height() - Bounds().Height()) / 2 );
}

bool
AboutWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


AboutView::AboutView(const BRect &r)
 : BView(r, "aboutview", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	fLogo=BTranslationUtils::GetBitmap("haikulogo.png");
	
	if(fLogo)
	{
		fDrawPoint.x=( 225-fLogo->Bounds().Width() )/2;
		fDrawPoint.y=0;
	}
	
	// Begin Construction of System Information controls
	
	font_height height;
	float labelsize, textsize;
	char string[255];
	system_info sysinfo;
	
	get_system_info(&sysinfo);
	
	be_plain_font->GetHeight(&height);
	textsize = height.ascent + height.descent + height.leading;
	
	be_bold_font->GetHeight(&height);
	labelsize = height.ascent + height.descent + height.leading;
	
	BRect r(0,0,225,Bounds().bottom);
	if(fLogo)
		r.OffsetBy(0,fLogo->Bounds().Height());
	
	fInfoView = new BView(r, "infoview", B_FOLLOW_NONE, B_WILL_DRAW);
	fInfoView->SetViewColor(235,235,235);
	AddChild(fInfoView);
	
	// Add all the various labels for system infomation
	
	BStringView *strview;
	
	// OS Version
	r.Set(5,5,225,labelsize+5);
	strview = new BStringView(r, "oslabel","Version:");
	strview->SetFont(be_bold_font);
	fInfoView->AddChild(strview);

	r.OffsetBy(0,labelsize);
	r.bottom=r.top + textsize;
	
	// the version is stored in the BEOS:APP_VERSION attribute of libbe.so
	BFile file("/boot/beos/system/lib/libbe.so",B_READ_ONLY);
	BAppFileInfo afInfo;
	version_info vInfo;
	
	if( file.InitCheck()==B_OK && 
		afInfo.SetTo(&file)==B_OK && 
		afInfo.GetVersionInfo(&vInfo, B_APP_VERSION_KIND)==B_OK )
	{
		strcpy(string,vInfo.short_info);
	}
	else
		strcpy(string,"Unknown");
	
	strview = new BStringView(r, "ostext",string);
	fInfoView->AddChild(strview);
	
	// CPU count, type and clock speed
	r.OffsetBy(0,textsize*1.5);
	r.bottom=r.top + labelsize;
	
	if(sysinfo.cpu_count > 1)
		sprintf(string,"%ld Processors:",sysinfo.cpu_count);
	else
		strcpy(string,"Processor:");
	
	strview = new BStringView(r, "cpulabel","Processor:");
	strview->SetFont(be_bold_font);
	fInfoView->AddChild(strview);
	
	r.OffsetBy(0,labelsize);
	r.bottom=r.top + textsize;
	strview = new BStringView(r, "cputext",CPUToString(sysinfo.cpu_type));
	fInfoView->AddChild(strview);

	r.OffsetBy(0,labelsize);
	r.bottom=r.top + textsize;
	
	if(sysinfo.cpu_clock_speed < 1000000000)
		sprintf(string,"%.2f Mhz",sysinfo.cpu_clock_speed / 1000000.0f);
	else
		sprintf(string,"%.2f Ghz",sysinfo.cpu_clock_speed / 1000000000.0f);
	
	strview = new BStringView(r, "mhztext",string);
	fInfoView->AddChild(strview);
	
	// RAM
	r.OffsetBy(0,textsize*1.5);
	r.bottom=r.top + labelsize;
	strview = new BStringView(r, "ramlabel","Memory:");
	strview->SetFont(be_bold_font);
	fInfoView->AddChild(strview);

	r.OffsetBy(0,labelsize);
	r.bottom=r.top + textsize;
	
	sprintf(string,"%ld MB total", sysinfo.max_pages / 256);
	
	strview = new BStringView(r, "ramtext",string);
	fInfoView->AddChild(strview);
	
	// Kernel build time/date
	r.OffsetBy(0,textsize*1.5);
	r.bottom=r.top + labelsize;
	strview = new BStringView(r, "kernellabel","Kernel:");
	strview->SetFont(be_bold_font);
	fInfoView->AddChild(strview);

	r.OffsetBy(0,labelsize);
	r.bottom=r.top + textsize;
	
	sprintf(string,"%s %s",sysinfo.kernel_build_date, sysinfo.kernel_build_time);
	
	strview = new BStringView(r, "kerneltext",string);
	fInfoView->AddChild(strview);
	
	// Uptime
	r.OffsetBy(0,textsize*1.5);
	r.bottom=r.top + labelsize;
	strview = new BStringView(r, "uptimelabel","Time Running:");
	strview->SetFont(be_bold_font);
	fInfoView->AddChild(strview);

	r.OffsetBy(0,labelsize);
	r.bottom=r.top + textsize;

	fUptimeView = new BStringView(r, "uptimetext","");
	fInfoView->AddChild(fUptimeView);
	fUptimeView->SetText(UptimeToString());

	// Begin construction of the credits view
	r=Bounds();
	r.left += fInfoView->Bounds().right;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	
	fCreditsView = new BTextView(r, "credits", r.OffsetToCopy(0,0).InsetByCopy(5,5),B_FOLLOW_NONE);
	BScrollView *creditsScroller = new BScrollView("creditsScroller",fCreditsView,0,0,
													false,true, B_PLAIN_BORDER);
	AddChild(creditsScroller);
	
	fCreditsView->SetStylable(true);
	fCreditsView->MakeEditable(false);
	
	BFont font(be_bold_font);
	font.SetSize(be_bold_font->Size()+2);
	
	fCreditsView->SetFontAndColor(&font);
	fCreditsView->Insert("Haiku\n");
	
	font.SetSize(be_bold_font->Size()+1);
	font.SetFace(B_BOLD_FACE | B_ITALIC_FACE);
	
	fCreditsView->SetFontAndColor(be_plain_font);
	fCreditsView->Insert("Copyright (C) 2001-2005 Haiku, Inc.\n\n");
	
	fCreditsView->SetFontAndColor(&font);
	fCreditsView->Insert("Team Leads:\n");
	
	fCreditsView->SetFontAndColor(be_plain_font);
	fCreditsView->Insert(
	"Bruno G. Albuquerque\n"
	"DarkWyrm\n"
	"Axel Dörfler\n"
	"Philippe Houdoin\n"
	"Kurtis Kopf\n"
	"Marcus Overhagen\n"
	"Sikosis\n"
	"Ingo Weinhold\n"
	"Michael Wilber\n"
	"\n"
					);
	
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
	"Jeremy Rand\n"
	"David Reid"
	"Daniel Reinhold\n"
	"Thomas Roell\n"
	"François Revol\n"
	"Zousar Shaker\n"
	"Daniel Switkin\n"
	"Atsushi Takamatsu\n"
	"Jason Vandermark\n"
	"Sandor Vroemisse\n"
	"Nathan Whitehorn\n"
	"Ulrich Wimboeck\n"
	"Gabe Yoder\n"
				"\n");
	
	fCreditsView->SetFontAndColor(&font);
	fCreditsView->Insert("Special Thanks To:\n");
	
	fCreditsView->SetFontAndColor(be_plain_font);
	fCreditsView->Insert("Michael Phipps\n\n");
	
/*
	// TODO: Add these (somehow)
	Brian Paul (Mesa)
	Silicon Graphics Inc. (GLU)
	Mark Kilgard & Be Inc. & Jam Hamby (GLUT)
*/
	
	font.SetSize(be_bold_font->Size()+2);
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
	fCreditsView->Insert("Copyright (c) 1997-2005 PDFlib GmbH and Thomas Merz. "
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
	BRect r(92,26,105,31);
	if(r.Contains(pt))
		printf("Easter Egg\n");
}

void
AboutView::Draw(BRect update)
{
	if(fLogo)
		DrawBitmap(fLogo,fDrawPoint);
	
}

void
AboutView::Pulse(void)
{
	fUptimeView->SetText(UptimeToString());
}

void
AboutView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case MOVE_INFO_VIEW:
		{
			fInfoView->MoveBy(0,-5);
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}

const char *CPUToString(cpu_type type)
{
	switch(type)
	{
		case B_CPU_PPC_601: return "PowerPC 601";
		case B_CPU_PPC_603: return "PowerPC 603";
		case B_CPU_PPC_603e: return "PowerPC 603e";
		case B_CPU_PPC_604: return "PowerPC 604";
		case B_CPU_PPC_604e: return "PowerPC 604e";
		case B_CPU_PPC_750: return "PowerPC 750";
		case B_CPU_PPC_686: return "PowerPC 686";
		case B_CPU_AMD_29K: return "AMD 29000";
//		case B_CPU_X86: return "Generic x86";
		
		case B_CPU_MC6502: return "MC 6502";
		case B_CPU_MIPS: return "MIPS";
		case B_CPU_HPPA: return "HPPA";
		case B_CPU_SH: return "SH";
		
		case B_CPU_Z80: return "Z80";
		case B_CPU_ALPHA: return "Alpha";
		case B_CPU_M68K: return "Motorola 68000";
		case B_CPU_ARM: return "ARM";
		case B_CPU_SPARC: return "SPARC";
		
//		case B_CPU_INTEL_X86: return "Generic Intel x86";
		case B_CPU_INTEL_PENTIUM: return "Intel Pentium";
		case B_CPU_INTEL_PENTIUM75: return "Intel Pentium";
		case B_CPU_INTEL_PENTIUM_486_OVERDRIVE: return "Intel 486 Overdrive";
		case B_CPU_INTEL_PENTIUM_MMX: return "Intel Pentium MMX";
		case B_CPU_INTEL_PENTIUM_MMX_MODEL_8: return "Intel Pentium MMX, Model 8";
		case B_CPU_INTEL_PENTIUM75_486_OVERDRIVE: return "Intel 486 Overdrive";
		case B_CPU_INTEL_PENTIUM_PRO: return "Intel Pentium Pro";
		case B_CPU_INTEL_PENTIUM_II: return "Intel Pentium II";
		case B_CPU_INTEL_PENTIUM_II_MODEL_5: return "Intel Pentium II, Model 5";
		case B_CPU_INTEL_CELERON: return "Intel Celeron";
		case B_CPU_INTEL_PENTIUM_III: return "Intel Pentium III";
		case B_CPU_INTEL_PENTIUM_III_MODEL_8: return "Intel Pentium III, Model 8";
		case B_CPU_INTEL_PENTIUM_M: return "Intel Pentium M";
		case B_CPU_INTEL_PENTIUM_III_XEON: return "Intel Pentium III Xeon";
		case B_CPU_INTEL_PENTIUM_III_MODEL_11: return "Intel Pentium III, Model 11";
		case B_CPU_INTEL_PENTIUM_M_MODEL_13: return "Intel Pentium M, Model 13";
		case B_CPU_INTEL_PENTIUM_IV: return "Intel Pentium IV";
		case B_CPU_INTEL_PENTIUM_IV_MODEL_1: return "Intel Pentium IV, Model 1";
		case B_CPU_INTEL_PENTIUM_IV_MODEL_2: return "Intel Pentium IV, Model 2";
		case B_CPU_INTEL_PENTIUM_IV_MODEL_3: return "Intel Pentium IV, Model 3";
		case B_CPU_INTEL_PENTIUM_IV_MODEL_4: return "Intel Pentium IV, Model 4";
		
		
/*		case B_CPU_AMD_X86: return "Generic AMD x86";
		case B_CPU_AMD_K5_MODEL0: return "AMD K5, Model 0";
		case B_CPU_AMD_K5_MODEL1: return "AMD K5, Model 1";
		case B_CPU_AMD_K5_MODEL2: return "AMD K5, Model 2";
		case B_CPU_AMD_K5_MODEL3: return "AMD K5, Model 3";
		case B_CPU_AMD_K6_MODEL6: return "AMD K6, Model 6";
		case B_CPU_AMD_K6_MODEL7: return "AMD K6, Model 7";
*/
		case B_CPU_AMD_K6_2: return "AMD K6 2";
		case B_CPU_AMD_K6_III: return "AMD K6 3";
		
		case B_CPU_AMD_ATHLON_MODEL_1: return "AMD Athlon, Model 1";
		case B_CPU_AMD_ATHLON_MODEL_2: return "AMD Athlon, Model 2";
		case B_CPU_AMD_DURON: return "AMD Duron";
		
		case B_CPU_AMD_ATHLON_THUNDERBIRD: return "AMD Athlon (Thunderbird)";
		case B_CPU_AMD_ATHLON_XP: return "AMD Athlon XP";
		case B_CPU_AMD_ATHLON_XP_MODEL_7: return "AMD Athlon XP, Model 7";
		case B_CPU_AMD_ATHLON_XP_MODEL_8: return "AMD Athlon XP, Model 8";
		case B_CPU_AMD_ATHLON_XP_MODEL_10: return "AMD Athlon XP, Model 10";
		
		case B_CPU_AMD_ATHLON_64_MODEL_4: return "AMD Athlon64, Model 4";
		case B_CPU_AMD_ATHLON_64_MODEL_5: return "AMD Athlon64, Model 5";
		case B_CPU_AMD_ATHLON_64_MODEL_7: return "AMD Athlon64, Model 7";
		case B_CPU_AMD_ATHLON_64_MODEL_8: return "AMD Athlon64, Model 8";
		case B_CPU_AMD_ATHLON_64_MODEL_11: return "AMD Athlon64, Model 11";
		case B_CPU_AMD_ATHLON_64_MODEL_12: return "AMD Athlon64, Model 12";
		case B_CPU_AMD_ATHLON_64_MODEL_14: return "AMD Athlon64, Model 14";
		case B_CPU_AMD_ATHLON_64_MODEL_15: return "AMD Athlon64, Model 15";
		
		
//		case B_CPU_CYRIX_X86: return "Generic Cyrix x86";
		case B_CPU_CYRIX_GXm: return "Cyrix GXm";
		case B_CPU_CYRIX_6x86MX: return "Cyrix 6x86 MX";
//		case B_CPU_IDT_X86: return "Generic IDT x86";
		case B_CPU_IDT_WINCHIP_C6: return "IDT Winchip C6";
		case B_CPU_IDT_WINCHIP_2: return "IDT Winchip 2";
		case B_CPU_IDT_WINCHIP_3: return "IDT Winchip 3";
		case B_CPU_VIA_EDEN: return "VIA Eden";
		case B_CPU_VIA_EDEN_EZRA_T: return "Via Eden (Ezra)";
		
//		case B_CPU_RISE_X86: return "Rise x86";
		case B_CPU_RISE_mP6: return "Rise mP6";
		
		case B_CPU_TRANSMETA_x86: return "Transmeta x86";
		case B_CPU_TRANSMETA_CRUSOE: return "Transmeta Crusoe";
		
		case B_CPU_NATIONAL_x86: return "National Semiconductor x86";
		case B_CPU_NATIONAL_GEODE_GX1: return "National Semiconductor GX1";
		case B_CPU_NATIONAL_GEODE_GX2: return "National Semiconductor GX2";
		default:
			break;
	}
	return "Unrecognized CPU";
}

const char *UptimeToString(void)
{
	char string[255];
	int64 days, hours, minutes, seconds, remainder;
	int64 systime = system_time();
	
	if(systime > 86400000000LL)
	{            
		days = systime / 86400000000LL;
		remainder = systime % 86400000000LL;
		
		hours = remainder / 3600000000LL;
		remainder = remainder % 3600000000LL;
		
		minutes = remainder / 60000000;
		remainder = remainder % 60000000;
		
		seconds = remainder / 1000000;
		sprintf(string,"%lld days, %lld hours, %lld minutes, %lld seconds",days,
				hours, minutes, seconds);
	}
	else
	if(systime > 3600000000LL)
	{
		hours = systime / 3600000000LL;
		remainder = systime % 3600000000LL;
		
		minutes = remainder / 60000000;
		remainder = remainder % 60000000;
		
		seconds = remainder / 1000000;
		sprintf(string,"%lld hours, %lld minutes, %lld seconds",hours, minutes, seconds);
	}
	else
	if(systime > 60000000)
	{
		minutes = systime / 60000000;
		remainder = systime % 60000000;
		
		seconds = remainder / 1000000;
		sprintf(string,"%lld minutes, %lld seconds",minutes, seconds);
	}
	else
	{
		seconds = systime / 1000000;
		sprintf(string,"%lld seconds",seconds);
	}
	return string;
}
