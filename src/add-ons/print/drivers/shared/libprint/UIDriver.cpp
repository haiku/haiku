/*
 * UIDriver.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <Message.h>

#include "UIDriver.h"
#include "JobData.h"
#include "PrinterData.h"
#include "JobSetupDlg.h"
#include "PageSetupDlg.h"
#include "DbgMsg.h"

UIDriver::UIDriver(BMessage *msg, PrinterData *printer_data, const PrinterCap *printer_cap)
	: fMsg(msg), fPrinterData(printer_data), fPrinterCap(printer_cap)
{
}

UIDriver::~UIDriver()
{
}

BMessage *UIDriver::configPage()
{
	BMessage *clone_msg = new BMessage(*fMsg);
	JobData *job_data = new JobData(clone_msg, fPrinterCap);

	if (doPageSetup(job_data,fPrinterData, fPrinterCap) < 0) {
		delete clone_msg;
		clone_msg = NULL;
	} else {
		clone_msg->what = 'okok';
	}

	delete job_data;
	return clone_msg;
}

BMessage *UIDriver::configJob()
{
	BMessage *clone_msg = new BMessage(*fMsg);
	JobData *job_data = new JobData(clone_msg, fPrinterCap);

	if (doJobSetup(job_data, fPrinterData, fPrinterCap) < 0) {
		delete clone_msg;
		clone_msg = NULL;
	} else {
		clone_msg->what = 'okok';
	}

	delete job_data;
	return clone_msg;
}

long UIDriver::doPageSetup(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
{
	PageSetupDlg *dlg = new PageSetupDlg(job_data, printer_data, printer_cap);
	return dlg->Go();
}

long UIDriver::doJobSetup(JobData *job_data, PrinterData *printer_data, const PrinterCap *printer_cap)
{
	JobSetupDlg *dlg = new JobSetupDlg(job_data, printer_data, printer_cap);
	return dlg->Go();
}
