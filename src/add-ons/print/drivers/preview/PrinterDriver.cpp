/*
 * Copyright 2003-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin
 *		Michael Pfeiffer
 *		Dr. Hartmut Reh
 */

#include "PrinterDriver.h"

#include "JobSetupWindow.h"
#include "PageSetupWindow.h"
#include "PrintJobReader.h"


#include <File.h>
#include <Node.h>
#include <Message.h>
#include <PrintJob.h>


#include <stdio.h>


PrinterDriver::PrinterDriver(BNode* printerNode)
	: fPrinting(false)
	, fJobFile(NULL)
	, fPrinterNode(printerNode)
{
}


PrinterDriver::~PrinterDriver()
{
}


status_t
PrinterDriver::PrintJob(BFile *spoolFile, BMessage *jobMsg)
{
	if (!spoolFile || !fPrinterNode)
		return B_ERROR;

	fJobFile = spoolFile;
	print_file_header pfh;

	// read print file header
	fJobFile->Seek(0, SEEK_SET);
	fJobFile->Read(&pfh, sizeof(pfh));

	// read job message
	BMessage msg;
	msg.Unflatten(fJobFile);

	BeginJob();
	fPrinting = true;

	printf("PrinterDriver::PrintJob, print massage read from spool file\n");
	jobMsg->PrintToStream();
	
	printf("\nPrinterDriver::PrintJob, print massage passed to print job\n");
	msg.PrintToStream();
	printf("\n");

	for (int32 page = 1; page <= pfh.page_count; ++page) {
		printf("PrinterDriver::PrintPage(): Faking print of page %" B_PRId32
			"/ %" B_PRId32 "\n", page, pfh.page_count);
	}

	fJobFile->Seek(0, SEEK_SET);
	PrintJobReader reader(fJobFile);

	status_t status = reader.InitCheck();
	printf("\nPrintJobReader::InitCheck(): %s\n", status == B_OK ? "B_OK" : "B_ERROR");
	printf("PrintJobReader::NumberOfPages(): %" B_PRId32 "\n",
		reader.NumberOfPages());
	printf("PrintJobReader::FirstPage(): %" B_PRId32 "\n", reader.FirstPage());
	printf("PrintJobReader::LastPage(): %" B_PRId32 "\n", reader.LastPage());

	BRect rect = reader.PaperRect();
	printf("PrintJobReader::PaperRect(): BRect(l:%.1f, t:%.1f, r:%.1f, b:%.1f)\n",
		rect.left, rect.top, rect.right, rect.bottom);

	rect = reader.PrintableRect();
	printf("PrintJobReader::PrintableRect(): BRect(l:%.1f, t:%.1f, r:%.1f, b:%.1f)\n",
		rect.left, rect.top, rect.right, rect.bottom);

	int32 xdpi, ydpi;
	reader.GetResolution(&xdpi, &ydpi);
	printf("PrintJobReader::GetResolution(): xdpi:%" B_PRId32 ", ydpi:%"
		B_PRId32 "\n", xdpi, ydpi);
	printf("PrintJobReader::GetScale(): %.1f\n", reader.GetScale());

	fPrinting = false;
	EndJob();

	return status;
}


void
PrinterDriver::StopPrinting()
{
	fPrinting = false;
}


status_t
PrinterDriver::BeginJob()
{
	return B_OK;
}


status_t
PrinterDriver::PrintPage(int32 pageNumber, int32 pageCount)
{
	return B_OK;
}


status_t
PrinterDriver::EndJob()
{
	return B_OK;
}


BlockingWindow*
PrinterDriver::NewPageSetupWindow(BMessage *setupMsg, const char *printerName)
{
	return new PageSetupWindow(setupMsg, printerName);
}


BlockingWindow*
PrinterDriver::NewJobSetupWindow(BMessage *jobMsg, const char *printerName)
{
	return new JobSetupWindow(jobMsg, printerName);
}


status_t
PrinterDriver::PageSetup(BMessage *setupMsg, const char *printerName)
{
	BlockingWindow* w = NewPageSetupWindow(setupMsg, printerName);
	return w->Go();
}


status_t
PrinterDriver::JobSetup(BMessage *jobMsg, const char *printerName)
{
	// set default value if property not set
	if (!jobMsg->HasInt32("copies"))
		jobMsg->AddInt32("copies", 1);

	if (!jobMsg->HasInt32("first_page"))
		jobMsg->AddInt32("first_page", 1);

	if (!jobMsg->HasInt32("last_page"))
		jobMsg->AddInt32("last_page", INT32_MAX);

	BlockingWindow* w = NewJobSetupWindow(jobMsg, printerName);
	return w->Go();
}


/* copied from PDFlib.h: */
#define letter_width	 (float) 612.0
#define letter_height	 (float) 792.0


BMessage*
PrinterDriver::GetDefaultSettings()
{
	BMessage* msg = new BMessage();
	BRect paperRect(0, 0, letter_width, letter_height);
	BRect printableRect(paperRect);
	printableRect.InsetBy(10, 10);
	msg->AddRect("paper_rect", paperRect);
	msg->AddRect("printable_rect", printableRect);
	msg->AddInt32("orientation", 0);
	msg->AddInt32("xres", 300);
	msg->AddInt32("yres", 300);
	return msg;
}
