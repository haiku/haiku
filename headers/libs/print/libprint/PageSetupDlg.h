/*
 * PageSetupDlg.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __PAGESETUPDLG_H
#define __PAGESETUPDLG_H

#include <View.h>
#include "DialogWindow.h"
#include "JobData.h"

class BRadioButton;
class BPopUpMenu;
class JobData;
class PaperCap;
class PrinterData;
class PrinterCap;
class MarginView;

class PageSetupView : public BView {
public:
	PageSetupView(JobData *job_data, PrinterData *printer_data,
		const PrinterCap *printer_cap);
	~PageSetupView();
	virtual void AttachedToWindow();
	bool UpdateJobData();
	void MessageReceived(BMessage *msg);

private:
	void AddOrientationItem(const char *name, JobData::Orientation orientation);
	JobData::Orientation GetOrientation();
	PaperCap *GetPaperCap();

	JobData          *fJobData;
	PrinterData      *fPrinterData;
	const PrinterCap *fPrinterCap;
	BPopUpMenu       *fPaper;
	BPopUpMenu       *fOrientation;
	BPopUpMenu       *fResolution;
	BTextControl     *fScaling;
	MarginView       *fMarginView;
};

class PageSetupDlg : public DialogWindow {
public:
	PageSetupDlg(JobData *job_data, PrinterData *printer_data,
		const PrinterCap *printer_cap);
	virtual void MessageReceived(BMessage *message);

private:
	BMessageFilter* fFilter;
	PageSetupView* fPageSetupView;
};

#endif	/* __PAGESETUPDLG_H */
