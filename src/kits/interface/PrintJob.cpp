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

#include "PrintJob.h"

// -----------------------------------------------------------------------------
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
	print_job_name = new char[strlen(job_name)+1];
	::strcpy(print_job_name, job_name);
}

// -----------------------------------------------------------------------------
BPrintJob::~BPrintJob()
{
	delete print_job_name;
}

// -----------------------------------------------------------------------------
void
BPrintJob::BeginJob()
{
}

// -----------------------------------------------------------------------------
void
BPrintJob::CommitJob()
{
}

// -----------------------------------------------------------------------------
status_t
BPrintJob::ConfigJob()
{
    return B_OK;
}

// -----------------------------------------------------------------------------
void
BPrintJob::CancelJob()
{
}

// -----------------------------------------------------------------------------
status_t
BPrintJob::ConfigPage()
{
    return B_OK;
}

// -----------------------------------------------------------------------------
void
BPrintJob::SpoolPage()
{
}

// -----------------------------------------------------------------------------
bool
BPrintJob::CanContinue()
{
		// Check if our local error storage is still B_OK
    return pr_error == B_OK && !stop_the_show;
}


// -----------------------------------------------------------------------------
void
BPrintJob::DrawView(BView *a_view,
                    BRect a_rect,
                    BPoint where)
{
}


// -----------------------------------------------------------------------------
BMessage *
BPrintJob::Settings()
{
    return NULL;
}

// -----------------------------------------------------------------------------
void
BPrintJob::SetSettings(BMessage *a_msg)
{
}

// -----------------------------------------------------------------------------
bool
BPrintJob::IsSettingsMessageValid(BMessage *a_msg) const
{
    return true;
}

// -----------------------------------------------------------------------------
BRect
BPrintJob::PaperRect()
{
    return paper_size;
}

// -----------------------------------------------------------------------------
BRect
BPrintJob::PrintableRect()
{
    return usable_size;
}

// -----------------------------------------------------------------------------
void
BPrintJob::GetResolution(int32 *xdpi,
                         int32 *ydpi)
{
	*xdpi = v_xres;
	*ydpi = v_yres;
}

// -----------------------------------------------------------------------------
int32
BPrintJob::FirstPage()
{
    return first_page;
}

// -----------------------------------------------------------------------------
int32
BPrintJob::LastPage()
{
    return last_page;
}

// -----------------------------------------------------------------------------
int32
BPrintJob::PrinterType(void *) const
{
    return B_COLOR_PRINTER;
}

#if 0
#pragma mark ----- PRIVATE -----
#endif

// -----------------------------------------------------------------------------
void
BPrintJob::RecurseView(BView *v,
                       BPoint origin,
                       BPicture *p,
                       BRect r)
{
}

// -----------------------------------------------------------------------------
void
BPrintJob::MangleName(char *filename)
{
}

// -----------------------------------------------------------------------------
void
BPrintJob::HandlePageSetup(BMessage *setup)
{
}

// -----------------------------------------------------------------------------
bool
BPrintJob::HandlePrintSetup(BMessage *setup)
{
    return true;
}

// -----------------------------------------------------------------------------
void
BPrintJob::NewPage()
{
}

// -----------------------------------------------------------------------------
void
BPrintJob::EndLastPage()
{
}

// -----------------------------------------------------------------------------
void
BPrintJob::AddSetupSpec()
{
}

// -----------------------------------------------------------------------------
void
BPrintJob::AddPicture(BPicture *picture,
                      BRect *rect,
                      BPoint where)
{
}

// -----------------------------------------------------------------------------
char *
BPrintJob::GetCurrentPrinterName() const
{
    return NULL;
}

// -----------------------------------------------------------------------------
void
BPrintJob::LoadDefaultSettings()
{
}



// -----------------------------------------------------------------------------
void
BPrintJob::_ReservedPrintJob1()
{
}

void
BPrintJob::_ReservedPrintJob2()
{
}

void
BPrintJob::_ReservedPrintJob3()
{
}

void
BPrintJob::_ReservedPrintJob4()
{
}
