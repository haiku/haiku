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
	: __msg(msg), __printer_data(printer_data), __printer_cap(printer_cap)
{
}

UIDriver::~UIDriver()
{
}

BMessage *UIDriver::configPage()
{
	BMessage *clone_msg = new BMessage(*__msg);
	JobData *job_data = new JobData(clone_msg, __printer_cap);

	if (doPageSetup(job_data,__printer_data, __printer_cap) < 0) {
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
	BMessage *clone_msg = new BMessage(*__msg);
	JobData *job_data = new JobData(clone_msg, __printer_cap);

	if (doJobSetup(job_data, __printer_data, __printer_cap) < 0) {
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
