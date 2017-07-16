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


UIDriver::UIDriver(BMessage* message, PrinterData *printerData,
	const PrinterCap *printerCap)
	:
	fMsg(message),
	fPrinterData(printerData),
	fPrinterCap(printerCap)
{
}


UIDriver::~UIDriver()
{
}


BMessage*
UIDriver::ConfigPage()
{
	BMessage *clonedMessage = new BMessage(*fMsg);
	JobData *jobData = new JobData(clonedMessage, fPrinterCap,
		JobData::kPageSettings);

	if (PageSetup(jobData, fPrinterData, fPrinterCap) < 0) {
		delete clonedMessage;
		clonedMessage = NULL;
	} else {
		clonedMessage->what = 'okok';
	}

	delete jobData;
	return clonedMessage;
}


BMessage*
UIDriver::ConfigJob()
{
	BMessage *clonedMessage = new BMessage(*fMsg);
	JobData *jobData = new JobData(clonedMessage, fPrinterCap,
		JobData::kJobSettings);

	if (JobSetup(jobData, fPrinterData, fPrinterCap) < 0) {
		delete clonedMessage;
		clonedMessage = NULL;
	} else {
		clonedMessage->what = 'okok';
	}

	delete jobData;
	return clonedMessage;
}


status_t
UIDriver::PageSetup(JobData* jobData, PrinterData* printerData,
	const PrinterCap* printerCap)
{
	PageSetupDlg *dialog = new PageSetupDlg(jobData, printerData, printerCap);
	return dialog->Go();
}


status_t
UIDriver::JobSetup(JobData *jobData, PrinterData *printerData,
	const PrinterCap *printerCap)
{
	JobSetupDlg *dialog = new JobSetupDlg(jobData, printerData, printerCap);
	return dialog->Go();
}
