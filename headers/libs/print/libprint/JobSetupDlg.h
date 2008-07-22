/*
 * JobSetupDlg.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __JOBSETUPDLG_H
#define __JOBSETUPDLG_H

#include <View.h>
#include "DialogWindow.h"

#include "JobData.h"
#include "Halftone.h"
#include "JSDSlider.h"

class BTextControl;
class BTextView;
class BRadioButton;
class BCheckBox;
class BPopUpMenu;
class JobData;
class PrinterData;
class PrinterCap;
class HalftoneView;
class PagesView;

class JobSetupView : public BView {
public:
	JobSetupView(BRect frame, JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap);
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *msg);
	bool UpdateJobData(bool showPreview);

private:
	void UpdateButtonEnabledState();
	BRadioButton* AddPageSelectionItem(BView* parent, BRect rect, const char* name, const char* label, 
		JobData::PageSelection pageSelection);
	void AllowOnlyDigits(BTextView* textView, int maxDigits);
	JobData::Color getColor();
	Halftone::DitherType getDitherType();
	float getGamma();
	float getInkDensity();

	BTextControl     *fCopies;
	BTextControl     *fFromPage;
	BTextControl     *fToPage;
	JobData          *fJobData;
	PrinterData      *fPrinterData;
	const PrinterCap *fPrinterCap;
	BPopUpMenu       *fColorType;
	BPopUpMenu       *fDitherType;
	JSDSlider        *fGamma;
	JSDSlider        *fInkDensity;
	HalftoneView     *fHalftone;
	BRadioButton     *fAll;
	BCheckBox        *fCollate;
	BCheckBox        *fReverse;
	PagesView        *fPages;
	BPopUpMenu       *fPaperFeed;
	BCheckBox        *fDuplex;
	BPopUpMenu       *fNup;
	BRadioButton     *fAllPages;
	BRadioButton     *fOddNumberedPages;
	BRadioButton     *fEvenNumberedPages;	
};

class JobSetupDlg : public DialogWindow {
public:
	JobSetupDlg(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap);
	virtual	void MessageReceived(BMessage *message);

private:
	JobSetupView *fJobSetup;
};

#endif	/* __JOBSETUPDLG_H */
