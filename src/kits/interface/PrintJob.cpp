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
BPrintJob::DrawView(BView *a_view,
                    BRect a_rect,
                    BPoint where)
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
BPrintJob::SetSettings(BMessage *a_msg)
{
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
    if (!paper_size.IsValid())
    	LoadDefaultSettings();
    	
    return paper_size;
}


BRect
BPrintJob::PrintableRect()
{
	if (!usable_size.IsValid())
    	LoadDefaultSettings();
    	
    return usable_size;
}

void
BPrintJob::GetResolution(int32 *xdpi, int32 *ydpi)
{
	*xdpi = v_xres;
	*ydpi = v_yres;
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
    return B_COLOR_PRINTER;
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
BPrintJob::HandlePrintSetup(BMessage *setup)
{
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
    BMessage reply;
    
    sPrintServer->SendMessage(&message, &reply);
    
    if (reply.HasRect("paper_rect"))
    	reply.FindRect("paper_rect", &paper_size);
    if (reply.HasRect("printable_rect"))
    	reply.FindRect("printable_rect", &usable_size);
}


void BPrintJob::_ReservedPrintJob1() {}
void BPrintJob::_ReservedPrintJob2() {}
void BPrintJob::_ReservedPrintJob3() {}
void BPrintJob::_ReservedPrintJob4() {}
