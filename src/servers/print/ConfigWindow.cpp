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

// posix
#include <stdlib.h>
#include <string.h>

// BeOS
#include <Application.h>
#include <Autolock.h>
#include <Window.h>

ConfigWindow::ConfigWindow(config_setup_kind kind, Printer* defaultPrinter, BMessage* settings, AutoReply* sender)
	: BWindow(BRect(30, 30, 200, 125), "Printer Setup", 
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
	, fKind(kind)
	, fDefaultPrinter(defaultPrinter)
	, fSettings(settings)
	, fSender(sender)
	, fCurrentPrinter(NULL)
{
	MimeTypeForSender(settings, fSenderMimeType);
	PrinterForMimeType();

	BView* panel = new BBox(Bounds(), "top_panel", B_FOLLOW_ALL, 
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
					B_PLAIN_BORDER);

	AddChild(panel);
	
	float left = 10, top = 5;
	BRect r(left, top, 160, 15);

	BPopUpMenu* menu = new BPopUpMenu("PrinterMenu");
	SetupPrintersMenu(menu);

	fPrinters = new BMenuField(r, "Printer", "Printer", menu);
	fPrinters->SetDivider(60);
	panel->AddChild(fPrinters);
	top += fPrinters->Bounds().Height() + 5;
	
	r.OffsetTo(left, top);
	fPageSetup = new BButton(r, "Page Setup", "Page Setup", new BMessage(MSG_PAGE_SETUP));
	panel->AddChild(fPageSetup);
	fPageSetup->ResizeToPreferred();
	top += fPageSetup->Bounds().Height() + 5;
	
	fJobSetup = NULL;
	if (kind == kJobSetup) {
		r.OffsetTo(left, top);
		fJobSetup = new BButton(r, "Job Setup", "Job Setup", new BMessage(MSG_JOB_SETUP));
		panel->AddChild(fJobSetup);
		fJobSetup->SetEnabled(fKind == kJobSetup);	
		fJobSetup->ResizeToPreferred();
		top += fJobSetup->Bounds().Height() + 5;
	}
	
	ResizeTo(Bounds().Width(), top);
	
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
		case MSG_PAGE_SETUP: PageSetup(m);
			break;
		case MSG_JOB_SETUP: JobSetup(m);
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
		default:
			inherited::MessageReceived(m);
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
			if (fJobSettings.IsEmpty()) fJobSettings = fPageSettings;
			p->SetPageSettings(&fPageSettings);
			p->SetJobSettings(&fJobSettings);
		}
		if (fJobSetup) {
			fJobSetup->SetEnabled(fKind == kJobSetup && !fPageSettings.IsEmpty());
		}
	}
}

void ConfigWindow::PageSetup(BMessage* m) {
	if (fCurrentPrinter) {
		Hide();
		UpdateSettings(true);
		bool ok = fCurrentPrinter->ConfigurePage(fPageSettings) == B_OK;
		if (ok) UpdateSettings(false);
		if (ok && fKind == kPageSetup) {
			fSender->SetReply(&fJobSettings);
			Quit();
		} else {
			Show();
		}
	}
}

void ConfigWindow::JobSetup(BMessage* m) {
	if (fCurrentPrinter) {
		Hide();
		UpdateSettings(true);
		bool ok = fCurrentPrinter->ConfigureJob(fJobSettings) == B_OK;
		if (ok) UpdateSettings(false);
		if (ok && fKind == kJobSetup) {
			fSender->SetReply(&fJobSettings);
			Quit();
		} else {
			Show();
		}
	}
}
