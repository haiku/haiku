/*

BPrintJob code.

Main functionality is the creation of the spool file. The print_server
will take it from there.

Copyright (c) 2001 OpenBeOS. Written by I.R. Adema.

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

#include <Messenger.h>
#include <PrintJob.h>

#include <pr_server.h>

#include <stdlib.h>
#include <string.h>


static BMessenger *sPrintServer = NULL;


static void
EnsureValidMessenger()
{
	if (sPrintServer == NULL)
    	sPrintServer = new BMessenger;
    
    if (!sPrintServer->IsValid())
    	*sPrintServer = BMessenger(PSRV_SIGNATURE_TYPE);
}


BPrintJob::BPrintJob(const char *job_name)
	:
	print_job_name(NULL),
	page_number(0),
	spoolFile(NULL),
	pr_error(B_OK),
	setup_msg(NULL),
	default_setup_msg(NULL),
	m_curPageHeader(NULL)
{
	print_job_name = strdup(job_name);
}


BPrintJob::~BPrintJob()
{
	free(print_job_name);
}


void
BPrintJob::BeginJob()
{
}


void
BPrintJob::CommitJob()
{
}


status_t
BPrintJob::ConfigJob()
{
    return B_OK;
}


void
BPrintJob::CancelJob()
{
	// TODO: For real
	stop_the_show = 1;
}


status_t
BPrintJob::ConfigPage()
{
    return B_OK;
}


void
BPrintJob::SpoolPage()
{
}


bool
BPrintJob::CanContinue()
{
		// Check if our local error storage is still B_OK
    return pr_error == B_OK && !stop_the_show;
}


void
BPrintJob::DrawView(BView *view, BRect rect, BPoint where)
{
}


BMessage *
BPrintJob::Settings()
{
    BMessage *message = NULL;
    
    if (setup_msg != NULL)
    	message = new BMessage(*setup_msg);
    
    return message;
}


void
BPrintJob::SetSettings(BMessage *message)
{
	if (message != NULL) {
		HandlePageSetup(message);
		HandlePrintSetup(message);
		delete setup_msg;
		setup_msg = message;
	}
}


bool
BPrintJob::IsSettingsMessageValid(BMessage *message) const
{
	const char *messageName = NULL;
    char *printerName = GetCurrentPrinterName();
    
    bool result = false;
    
    // The passed message is valid if it contains the right printer name.
    if (message != NULL 
    	&& message->FindString("printer_name", &messageName) == B_OK
    	&& strcmp(printerName, messageName) == 0)
    	result = true;
    
    free(printerName);
    
    return result;
}


BRect
BPrintJob::PaperRect()
{
    if (default_setup_msg == NULL)
    	LoadDefaultSettings();
    	
    return paper_size;
}


BRect
BPrintJob::PrintableRect()
{
	if (default_setup_msg == NULL)
    	LoadDefaultSettings();
    	
    return usable_size;
}


void
BPrintJob::GetResolution(int32 *xdpi, int32 *ydpi)
{
	int32 xres = -1, yres = -1;
	
	const BMessage *message = NULL;
	
	if (setup_msg != NULL)
		message = setup_msg;
	else {
		if (default_setup_msg == NULL)
			LoadDefaultSettings();
		message = default_setup_msg;
	}
	
	if (message != NULL) {
		if (message->HasInt32(PSRV_FIELD_XRES))
			message->FindInt32(PSRV_FIELD_XRES, &xres);
		if (message->HasInt32(PSRV_FIELD_YRES))
			message->FindInt32(PSRV_FIELD_YRES, &yres);
	}
	
	if (xdpi != NULL)
		*xdpi = xres;
	if (ydpi != NULL)
		*ydpi = yres;
}


int32
BPrintJob::FirstPage()
{
    return first_page;
}


int32
BPrintJob::LastPage()
{
    return last_page;
}


int32
BPrintJob::PrinterType(void *) const
{
    EnsureValidMessenger();
    
    BMessage message(PSRV_GET_ACTIVE_PRINTER);
    BMessage reply;
    
    sPrintServer->SendMessage(&message, &reply);
    
    int32 type = B_COLOR_PRINTER;
    reply.FindInt32("color", &type);
    
    return type;
}


#if 0
#pragma mark ----- PRIVATE -----
#endif


void
BPrintJob::RecurseView(BView *view, BPoint origin,
                       BPicture *picture, BRect rect)
{
}


void
BPrintJob::MangleName(char *filename)
{
}


void
BPrintJob::HandlePageSetup(BMessage *setup)
{

}


bool
BPrintJob::HandlePrintSetup(BMessage *message)
{
    if (message->HasRect(PSRV_FIELD_PRINTABLE_RECT))
    	message->FindRect(PSRV_FIELD_PRINTABLE_RECT, &usable_size);
    if (message->HasRect(PSRV_FIELD_PAPER_RECT))
    	message->FindRect(PSRV_FIELD_PAPER_RECT, &paper_size);
    if (message->HasInt32(PSRV_FIELD_FIRST_PAGE))
    	message->FindInt32(PSRV_FIELD_FIRST_PAGE, &first_page);
    if (message->HasInt32(PSRV_FIELD_LAST_PAGE))
    	message->FindInt32(PSRV_FIELD_LAST_PAGE, &last_page);
    	
    return true;
}


void
BPrintJob::NewPage()
{
}


void
BPrintJob::EndLastPage()
{
}


void
BPrintJob::AddSetupSpec()
{
}


void
BPrintJob::AddPicture(BPicture *picture, BRect *rect, BPoint where)
{
}


char *
BPrintJob::GetCurrentPrinterName() const
{  
	EnsureValidMessenger();

    BMessage message(PSRV_GET_ACTIVE_PRINTER);
    BMessage reply;
    
    const char *printerName = NULL;
    
    if (sPrintServer->SendMessage(&message, &reply) == B_OK)
    	reply.FindString("printer_name", &printerName);
        
    return printerName != NULL ? strdup(printerName) : NULL;
}


void
BPrintJob::LoadDefaultSettings()
{
	EnsureValidMessenger();
	
	BMessage message(PSRV_GET_DEFAULT_SETTINGS);
    BMessage *reply = new BMessage;
    
    sPrintServer->SendMessage(&message, reply);
    
    if (reply->HasRect(PSRV_FIELD_PAPER_RECT))
    	reply->FindRect(PSRV_FIELD_PAPER_RECT, &paper_size);
    if (reply->HasRect(PSRV_FIELD_PRINTABLE_RECT))
    	reply->FindRect(PSRV_FIELD_PRINTABLE_RECT, &usable_size);
    
    // Should never happen, but anyway...
    if (default_setup_msg != NULL)
    	delete default_setup_msg;
    	
    default_setup_msg = reply;
}


void BPrintJob::_ReservedPrintJob1() {}
void BPrintJob::_ReservedPrintJob2() {}
void BPrintJob::_ReservedPrintJob3() {}
void BPrintJob::_ReservedPrintJob4() {}
