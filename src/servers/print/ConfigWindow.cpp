/*****************************************************************************/
// ConfigWindow
//
// Author
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/


#include "Printer.h"
#include "PrintServerApp.h"
#include "ConfigWindow.h"
#include "BeUtils.h"

// posix
#include <stdlib.h>
#include <string.h>

// BeOS
#include <Application.h>
#include <Autolock.h>
#include <Window.h>

ConfigWindow::ConfigWindow(config_setup_kind kind, Printer* defaultPrinter, BMessage* settings, AutoReply* sender)
	: BWindow(ConfigWindow::GetWindowFrame(), "Page Setup", 
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
	, fKind(kind)
	, fDefaultPrinter(defaultPrinter)
	, fSettings(settings)
	, fSender(sender)
	, fCurrentPrinter(NULL)
{
	MimeTypeForSender(settings, fSenderMimeType);
	PrinterForMimeType();

	if (kind == kJobSetup) SetTitle("Print Setup");

	BView* panel = new BBox(Bounds(), "top_panel", B_FOLLOW_ALL, 
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
					B_PLAIN_BORDER);

	AddChild(panel);
	
	float left = 10, top = 5, width;
	BRect r(left, top, 160, 15);

		// print selection popup menu
	BPopUpMenu* menu = new BPopUpMenu("Select a Printer");
	SetupPrintersMenu(menu);

	fPrinters = new BMenuField(r, "Printer", "Printer", menu);
	fPrinters->SetDivider(40);
	panel->AddChild(fPrinters);
	top += fPrinters->Bounds().Height() + 10;
	width = fPrinters->Bounds().Width();
	
		// page format button
	r.OffsetTo(left, top);
	fPageSetup = new BButton(r, "Page Format", "Page Format", new BMessage(MSG_PAGE_SETUP));
	panel->AddChild(fPageSetup);
	fPageSetup->ResizeToPreferred();
	top += fPageSetup->Bounds().Height() + 5;
	if (fPageSetup->Bounds().Width() > width) width = fPageSetup->Bounds().Width();
	
		// page selection button
	fJobSetup = NULL;
	if (kind == kJobSetup) {
		r.OffsetTo(left, top);
		fJobSetup = new BButton(r, "Page Selection", "Page Selection", new BMessage(MSG_JOB_SETUP));
		panel->AddChild(fJobSetup);
		fJobSetup->ResizeToPreferred();
		top += fJobSetup->Bounds().Height() + 5;
		if (fJobSetup->Bounds().Width() > width) width = fJobSetup->Bounds().Width();
	}
	top += 5;
	
		// separator line
	BRect line(Bounds());
	line.OffsetTo(0, top);
	line.bottom = line.top+1;
	AddChild(new BBox(line, "line", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP));
	top += 10;
	
		// Cancel button
	r.OffsetTo(left, top);
	BButton* cancel = new BButton(r, "Cancel", "Cancel", new BMessage(B_QUIT_REQUESTED));
	panel->AddChild(cancel);
	cancel->ResizeToPreferred();
	left = cancel->Frame().right + 10;
	
		// OK button
	r.OffsetTo(left, top);
	fOk = new BButton(r, "OK", "OK", new BMessage(MSG_OK));
	panel->AddChild(fOk);
	fOk->ResizeToPreferred();
	top += fOk->Bounds().Height() + 10;

		// resize buttons to equal width	
	float height = fOk->Bounds().Height();
	fPageSetup->ResizeTo(width, height);
	if (fJobSetup) fJobSetup->ResizeTo(width, height);
	
		// resize window	
	ResizeTo(fOk->Frame().right + 10, top);
	
	AddShortcut('i', 0, new BMessage(B_ABOUT_REQUESTED));
	
	SetDefaultButton(fOk);
	
	fPrinters->MakeFocus(true);
	
	UpdateSettings(true);
}

ConfigWindow::~ConfigWindow() {
	if (fCurrentPrinter) fCurrentPrinter->Release();
	release_sem(fFinished);
}

void ConfigWindow::Go() {
	sem_id sid = create_sem(0, "finished");
	if (sid >= 0) {
		fFinished = sid;
		Show();
		acquire_sem(sid);
		delete_sem(sid);
	} else {
		Quit();
	}
}

void ConfigWindow::MessageReceived(BMessage* m) {
	switch (m->what) {
		case MSG_PAGE_SETUP: Setup(kPageSetup);
			break;
		case MSG_JOB_SETUP: Setup(kJobSetup);
			break;
		case MSG_PRINTER_SELECTED: {
				BString printer;
				if (m->FindString("name", &printer) == B_OK) {
					UpdateAppSettings(fSenderMimeType.String(), printer.String());
					PrinterForMimeType();
					UpdateSettings(true);
				}
			}
			break;
		case MSG_OK:
			UpdateSettings(false);
			fSender->SetReply(fKind == kPageSetup ? &fPageSettings : &fJobSettings);
			Quit();
			break;
		case B_ABOUT_REQUESTED: AboutRequested();
			break;
		default:
			inherited::MessageReceived(m);
	}
}

static const char* 
kAbout =
"Printer Server\n"
"© 2001, 2002 OpenBeOS\n"
"\n"
"\tIthamar R. Adema - Initial Implementation\n"
"\tMichael Pfeiffer - Release 1 and beyond\n"
;

void
ConfigWindow::AboutRequested()
{
	BAlert *about = new BAlert("About Printer Server", kAbout, "Cool");
	BTextView *v = about->TextView();
	if (v) {
		rgb_color red = {255, 0, 51, 255};
		rgb_color blue = {0, 102, 255, 255};

		v->SetStylable(true);
		char *text = (char*)v->Text();
		char *s = text;
		// set all Be in blue and red
		while ((s = strstr(s, "Be")) != NULL) {
			int32 i = s - text;
			v->SetFontAndColor(i, i+1, NULL, 0, &blue);
			v->SetFontAndColor(i+1, i+2, NULL, 0, &red);
			s += 2;
		}
		// first text line 
		s = strchr(text, '\n');
		BFont font;
		v->GetFontAndColor(0, &font);
		font.SetSize(12);
		v->SetFontAndColor(0, s-text+1, &font, B_FONT_SIZE);
	};
	about->Go();
}


void ConfigWindow::FrameMoved(BPoint p) {
	BRect frame = GetWindowFrame();
	frame.OffsetTo(p);
	SetWindowFrame(frame);
}

BRect ConfigWindow::GetWindowFrame() {
	BAutolock lock(gLock);
	if (lock.IsLocked()) {
		return Settings::GetSettings()->ConfigWindowFrame();
	}
	return BRect(30, 30, 300, 300);
}

void ConfigWindow::SetWindowFrame(BRect r) {
	BAutolock lock(gLock);
	if (lock.IsLocked()) {
		Settings::GetSettings()->SetConfigWindowFrame(r);
	}
}

void ConfigWindow::PrinterForMimeType() {
	BAutolock lock(gLock);
	if (fCurrentPrinter) {
		fCurrentPrinter->Release(); fCurrentPrinter = NULL;
	}
	if (lock.IsLocked()) {
		Settings* s = Settings::GetSettings();
		AppSettings* app = s->FindAppSettings(fSenderMimeType.String());
		if (app) {
			fPrinterName = app->GetPrinter();
		} else {
			fPrinterName = fDefaultPrinter ? fDefaultPrinter->Name() : "";
		}
		fCurrentPrinter = Printer::Find(fPrinterName);
		if (fCurrentPrinter) fCurrentPrinter->Acquire();
	}
}

void ConfigWindow::SetupPrintersMenu(BMenu* menu) {
		// clear menu
	for (int i = menu->CountItems() - 1; i >= 0; i --) {
		delete menu->RemoveItem(i);
	}
		// fill menu with printer names
	BAutolock lock(gLock);
	if (lock.IsLocked()) {
		BString n;
		Printer* p;
		BMessage* m;
		BMenuItem* item;
		for (int i = 0; i < Printer::CountPrinters(); i ++) {
			Printer::At(i)->GetName(n);
			m = new BMessage(MSG_PRINTER_SELECTED);
			m->AddString("name", n.String());
			menu->AddItem(item = new BMenuItem(n.String(), m));
			if (n == fPrinterName) item->SetMarked(true);
		}
	}
}

void ConfigWindow::UpdateAppSettings(const char* mime, const char* printer) {
	BAutolock lock(gLock);
	if (lock.IsLocked()) {
		Settings* s = Settings::GetSettings();
		AppSettings* app = s->FindAppSettings(mime);
		if (app) app->SetPrinter(printer);
		else s->AddAppSettings(new AppSettings(mime, printer));
	}
}

void ConfigWindow::UpdateSettings(bool read) {
	BAutolock lock(gLock);
	if (lock.IsLocked()) {
		Settings* s = Settings::GetSettings();
		PrinterSettings* p = s->FindPrinterSettings(fPrinterName.String());
		if (p == NULL) {
			p = new PrinterSettings(fPrinterName.String());
			s->AddPrinterSettings(p);
		}
		ASSERT(p != NULL);
		if (read) {
			fPageSettings = *p->GetPageSettings();
			fJobSettings = *p->GetJobSettings();
		} else {
			p->SetPageSettings(&fPageSettings);
			p->SetJobSettings(&fJobSettings);
		}
	}
	UpdateUI();
}

void ConfigWindow::UpdateUI() {
	if (fCurrentPrinter == NULL) {
		fPageSetup->SetEnabled(false);
		if (fJobSetup) fJobSetup->SetEnabled(false);
		fOk->SetEnabled(false);
	} else {	
		fPageSetup->SetEnabled(true);
	
		if (fJobSetup) {
			fJobSetup->SetEnabled(fKind == kJobSetup && !fPageSettings.IsEmpty());
		}
		fOk->SetEnabled(fKind == kJobSetup && !fJobSettings.IsEmpty() ||
			fKind == kPageSetup && !fPageSettings.IsEmpty());
	}
}

void ConfigWindow::Setup(config_setup_kind kind) {
	if (fCurrentPrinter) {
		Hide();
		if (kind == kPageSetup) {
			BMessage settings = fPageSettings;			
			if (fCurrentPrinter->ConfigurePage(settings) == B_OK) {
				fPageSettings = settings;
				if (!fJobSettings.IsEmpty()) AddFields(&fJobSettings, &fPageSettings);
			}
		} else {
			BMessage settings;			
			if (fJobSettings.IsEmpty()) settings = fPageSettings;
			else settings = fJobSettings;
			
			if (fCurrentPrinter->ConfigureJob(settings) == B_OK)
				fJobSettings = settings;
		}
		UpdateUI();
		Show();
	}
}
