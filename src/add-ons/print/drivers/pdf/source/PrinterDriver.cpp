/*

PDF Writer printer driver.

Copyright (c) 2001-2003 OpenBeOS. 

Authors: 
	Philippe Houdoin
	Simon Gauvin	
	Michael Pfeiffer
	
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/


#include <stdio.h>
#include <string.h>			// for memset()

#include <StorageKit.h>

#include "PrinterDriver.h"

#include "PageSetupWindow.h"
#include "JobSetupWindow.h"
#include "StatusWindow.h"
#include "PrinterSettings.h"
#include "Report.h"

// Private prototypes
// ------------------

#ifdef CODEWARRIOR
	#pragma mark [Constructor & destructor]
#endif

// Constructor & destructor
// ------------------------

// --------------------------------------------------
PrinterDriver::PrinterDriver()
	:	fJobFile(NULL),
		fPrinterNode(NULL),
		fJobMsg(NULL)
{
}


// --------------------------------------------------
PrinterDriver::~PrinterDriver() 
{
}

#ifdef CODEWARRIOR
	#pragma mark [Public methods]
#endif

#ifdef B_BEOS_VERSION_DANO
struct print_file_header {
       int32   version;
       int32   page_count;
       off_t   first_page;
       int32   _reserved_3_;
       int32   _reserved_4_;
       int32   _reserved_5_;
};
#endif

// Public methods
// --------------

status_t 
PrinterDriver::PrintJob
	(
	BFile 		*jobFile,		// spool file
	BNode 		*printerNode,	// printer node, used by OpenTransport() to find & load transport add-on
	BMessage 	*jobMsg			// job message
	)
{
	print_file_header	pfh;
	status_t			status;
	BMessage 			*msg;
	int32 				page;
	uint32				copy;
	uint32				copies;
	const int32         passes = 2;

	fJobFile		= jobFile;
	fPrinterNode	= printerNode;
	fJobMsg			= jobMsg;

	if (!fJobFile || !fPrinterNode) 
		return B_ERROR;

	if (fPrintTransport.Open(fPrinterNode) != B_OK) {
		return B_ERROR;
	}
	if (fPrintTransport.IsPrintToFileCanceled()) {
		return B_OK;
	}

	// read print file header	
	fJobFile->Seek(0, SEEK_SET);
	fJobFile->Read(&pfh, sizeof(pfh));
	
	// read job message
	fJobMsg = msg = new BMessage();
	msg->Unflatten(fJobFile);
	// We have to load the settings here for Dano/Zeta because they don't store 
	// all fields from the message returned by config_job in the job file!
	PrinterSettings::Read(printerNode, msg, PrinterSettings::kJobSettings);
	
	if (msg->HasInt32("copies")) {
		copies = msg->FindInt32("copies");
	} else {
		copies = 1;
	}
	
	// force creation of Report object
	Report::Instance();

	// show status window
	StatusWindow* statusWindow = new StatusWindow(passes, pfh.page_count, this);

	status = BeginJob();

	fPrinting = true;
	for (fPass = 0; fPass < passes && status == B_OK && fPrinting; fPass++) {
		for (copy = 0; copy < copies && status == B_OK && fPrinting; copy++) 
		{
			for (page = 1; page <= pfh.page_count && status == B_OK && fPrinting; page++) {
				statusWindow->NextPage();
				status = PrintPage(page, pfh.page_count);
			}
	
			// re-read job message for next page
			fJobFile->Seek(sizeof(pfh), SEEK_SET);
			msg->Unflatten(fJobFile);
		}
	}
	
	status_t s = EndJob();
	if (status == B_OK) status = s;

	delete fJobMsg;
	
	// close status window
	if (Report::Instance()->CountItems() != 0) {
		statusWindow->WaitForClose();
	}
	if (statusWindow->Lock()) {
		statusWindow->Quit();
	}

	// delete Report object
	Report::Instance()->Free();
	
	return status;
}

/**
 * This will stop the printing loop
 *
 * @param none
 * @return void
 */
void 
PrinterDriver::StopPrinting()
{
	fPrinting = false;
}


// --------------------------------------------------
status_t
PrinterDriver::BeginJob() 
{
	return B_OK;
}


// --------------------------------------------------
status_t 
PrinterDriver::PrintPage(int32 pageNumber, int32 pageCount) 
{
	char text[128];

	sprintf(text, "Faking print of page %ld/%ld...", pageNumber, pageCount);
	BAlert *alert = new BAlert("PrinterDriver::PrintPage()", text, "Hmm?");
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
	return B_OK;
}


// --------------------------------------------------
status_t
PrinterDriver::EndJob() 
{
	return B_OK;
}


// --------------------------------------------------
status_t 
PrinterDriver::PrinterSetup(char *printerName)
	// name of printer, to attach printer settings
{
	return B_OK;
}


// --------------------------------------------------
status_t 
PrinterDriver::PageSetup(BMessage *setupMsg, const char *printerName)
{
	PageSetupWindow *psw;
	
	psw = new PageSetupWindow(setupMsg, printerName);
	return psw->Go();
}


// --------------------------------------------------
status_t 
PrinterDriver::JobSetup(BMessage *jobMsg, const char *printerName)
{
	// set default value if property not set
	if (!jobMsg->HasInt32("copies"))
		jobMsg->AddInt32("copies", 1);

	if (!jobMsg->HasInt32("first_page"))
		jobMsg->AddInt32("first_page", 1);
		
	if (!jobMsg->HasInt32("last_page"))
		jobMsg->AddInt32("last_page", MAX_INT32);

	JobSetupWindow * jsw;

	jsw = new JobSetupWindow(jobMsg, printerName);
	return jsw->Go();
}

#ifdef CODEWARRIOR
	#pragma mark [Privates routines]
#endif

// Private routines
// ----------------
