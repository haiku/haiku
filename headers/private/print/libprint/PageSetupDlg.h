/*
 * PageSetupDlg.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PAGESETUPDLG_H
#define __PAGESETUPDLG_H

#include <View.h>
#include <Window.h>

class BRadioButton;
class BPopUpMenu;
class JobData;
class PrinterData;
class PrinterCap;

class PageSetupView : public BView {
public:
	PageSetupView(BRect frame, JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap);
	~PageSetupView();
	virtual void AttachedToWindow();
	bool UpdateJobData();

private:
	JobData          *fJobData;
	PrinterData      *fPrinterData;
	const PrinterCap *fPrinterCap;
	BRadioButton     *fPortrait;
	BPopUpMenu       *fPaper;
	BPopUpMenu       *fResolution;
};

class PageSetupDlg : public BWindow {
public:
	PageSetupDlg(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap);
	~PageSetupDlg();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *message);
	int Go();

private:
	int  fResult;
	long fSemaphore;
	BMessageFilter *fFilter;
};

#endif	/* __PAGESETUPDLG_H */
