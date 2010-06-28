/*
 * Copyright 2002-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#ifndef _CONFIG_WINDOW_H
#define _CONFIG_WINDOW_H


#include <InterfaceKit.h>
#include <Window.h>

#include "BeUtils.h"
#include "ObjectList.h"
#include "Printer.h"


enum config_setup_kind {
	kPageSetup,
	kJobSetup,
};


class ConfigWindow : public BWindow {
	enum {
		MSG_PAGE_SETUP       = 'cwps',
		MSG_JOB_SETUP        = 'cwjs',
		MSG_PRINTER_SELECTED = 'cwpr',
		MSG_OK               = 'cwok',
		MSG_CANCEL           = 'cwcl',
	};
	
public:
					ConfigWindow(config_setup_kind kind,
						Printer* defaultPrinter, BMessage* settings,
						AutoReply* sender);
					~ConfigWindow();
			void	 Go();
	
			void	MessageReceived(BMessage* m);
			void	AboutRequested();
			void	FrameMoved(BPoint p);

	static	BRect	GetWindowFrame();
	static	void	SetWindowFrame(BRect frame);

private:
			BPictureButton* AddPictureButton(BView* panel, BRect frame,
								const char* name, const char* on,
								const char* off, uint32 what);
			void	PrinterForMimeType();
			void	SetupPrintersMenu(BMenu* menu);
			void	UpdateAppSettings(const char* mime, const char* printer);
			void	UpdateSettings(bool read);
			void	UpdateUI();
			void	Setup(config_setup_kind);

			config_setup_kind fKind;
			Printer*		fDefaultPrinter;
			BMessage*		fSettings;
			AutoReply*		fSender;
			BString			fSenderMimeType;

			BString			fPrinterName;
			Printer*		fCurrentPrinter;
			BMessage		fPageSettings;
			BMessage		fJobSettings;

			sem_id			fFinished;

			BMenuField*		fPrinters;
			BPictureButton*	fPageSetup;
			BPictureButton*	fJobSetup;
			BButton*		fOk;
			BStringView*	fPageFormatText;
			BStringView*	fJobSetupText;
};


#endif
