/*
 * JobSetupDlg.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __JOBSETUPDLG_H
#define __JOBSETUPDLG_H

#include <View.h>
#include <Window.h>

#include "JobData.h"
#include "Halftone.h"

class BTextControl;
class BRadioButton;
class BCheckBox;
class BPopUpMenu;
class BSlider;
class JobData;
class PrinterData;
class PrinterCap;
class HalftoneView;

class JobSetupView : public BView {
public:
	JobSetupView(BRect frame, JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap);
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *msg);
	bool UpdateJobData();

public:
	BTextControl *copies;
	BTextControl *from_page;
	BTextControl *to_page;

private:
	JobData::Color getColor();
	Halftone::DitherType getDitherType();
	float getGamma();
	float getInkDensity();

	JobData          *fJobData;
	PrinterData      *fPrinterData;
	const PrinterCap *fPrinterCap;
	BPopUpMenu       *fColorType;
	BPopUpMenu       *fDitherType;
	BSlider          *fGamma;
	BSlider          *fInkDensity;
	HalftoneView     *fHalftone;
	BRadioButton     *fAll;
	BCheckBox        *fCollate;
	BCheckBox        *fReverse;
	BPopUpMenu       *fPaperFeed;
	BCheckBox        *fDuplex;
	BPopUpMenu       *fNup;
};

class JobSetupDlg : public BWindow {
public:
	JobSetupDlg(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap);
	~JobSetupDlg();
	virtual bool QuitRequested();
	virtual	void MessageReceived(BMessage *message);
	int Go();

private:
	int  fResult;
	long fSemaphore;
	BMessageFilter *fFilter;
};

#endif	/* __JOBSETUPDLG_H */
