/*
 * JobSetupDlg.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __JOBSETUPDLG_H
#define __JOBSETUPDLG_H

#include <View.h>
#include <Window.h>

class BTextControl;
class BRadioButton;
class BCheckBox;
class BPopUpMenu;
class JobData;
class PrinterData;
class PrinterCap;

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
	JobData          *__job_data;
	PrinterData      *__printer_data;
	const PrinterCap *__printer_cap;
	BTextControl     *__gamma;
	BRadioButton     *__all;
	BCheckBox        *__collate;
	BCheckBox        *__reverse;
	BPopUpMenu       *__surface_type;
	BPopUpMenu       *__paper_feed;
	BCheckBox        *__duplex;
	BPopUpMenu       *__nup;
};

class JobSetupDlg : public BWindow {
public:
	JobSetupDlg(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap);
	~JobSetupDlg();
	virtual bool QuitRequested();
	virtual	void MessageReceived(BMessage *message);
	int Go();

private:
	int  __result;
	long __semaphore;
	BMessageFilter *__filter;
};

#endif	/* __JOBSETUPDLG_H */
