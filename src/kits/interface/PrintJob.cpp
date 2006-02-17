/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 				I.R. Adema
 				Stefano Ceccherini (burton666@libero.it)
 */

#include <Alert.h>
#include <Application.h>
#include <Debug.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Messenger.h>
#include <Path.h>
#include <PrintJob.h>
#include <Roster.h>
#include <View.h>

#include <pr_server.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static BMessenger *sPrintServer = NULL;


static bool
EnsureValidMessenger()
{
	if (sPrintServer == NULL)
    		sPrintServer = new BMessenger;
    
	if (!sPrintServer->IsValid())
    		*sPrintServer = BMessenger(PSRV_SIGNATURE_TYPE);

	return sPrintServer->IsValid();
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
	memset(&current_header, 0, sizeof(current_header));
	if (job_name != NULL)
		print_job_name = strdup(job_name);
}


BPrintJob::~BPrintJob()
{
	free(print_job_name);
}


void
BPrintJob::BeginJob()
{
	// TODO: this should be improved:
	// - Take system printers into account
	// - handle the case where the path doesn't exist
	// - more
	BPath path;
	status_t status = find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	if (status < B_OK)
		return;
	
	char *printer = GetCurrentPrinterName();
	if (printer == NULL)
		return;
		
	path.Append(printer);
	free(printer);
	
	char mangledName[B_FILE_NAME_LENGTH];
	MangleName(mangledName);
	
	path.Append(mangledName);
	
	if (path.InitCheck() < B_OK)
		return;
	
	stop_the_show = 0;
	page_number = 0;
	
	strncpy(spool_file_name, path.Path(), sizeof(spool_file_name));
	spoolFile = new BFile(spool_file_name, B_READ_WRITE|B_CREATE_FILE);
	
	AddSetupSpec();
}


void
BPrintJob::CommitJob()
{
	if (page_number <= 0) {
		BAlert *alert = new BAlert("Alert", "No Pages to print!", "Okay");
		alert->Go();
		CancelJob();
		return;
	}
		
	if (!EnsureValidMessenger())
		return;

	BMessage *message = new BMessage(PSRV_GET_ACTIVE_PRINTER);
	BMessage *reply = new BMessage;
    
	const char *printerName = NULL;
	if (sPrintServer->SendMessage(message, reply) < B_OK ||
    		reply->FindString("printer_name", &printerName) < B_OK) {
		// TODO: Show an alert
    		delete message;
   		delete reply;
    		return;
	}	
    
	delete message;
	delete reply;
    
 	app_info appInfo;
	be_app->GetAppInfo(&appInfo);
    
	spoolFile->WriteAttr("_spool/Page Count", B_INT32_TYPE, 0, &page_number, sizeof(int32));    
	spoolFile->WriteAttr("_spool/Description", B_STRING_TYPE, 0, print_job_name, strlen(print_job_name) + 1); 
	spoolFile->WriteAttr("_spool/Printer", B_STRING_TYPE, 0, printerName, strlen(printerName) + 1);    
	spoolFile->WriteAttr("_spool/Status", B_STRING_TYPE, 0, "waiting", strlen("waiting") + 1);    
	spoolFile->WriteAttr("_spool/MimeType", B_STRING_TYPE, 0, appInfo.signature, strlen(appInfo.signature) + 1);
	
	message = new BMessage(PSRV_PRINT_SPOOLED_JOB);
	reply = new BMessage;
	
	message->AddString("JobName", print_job_name);
	message->AddString("Spool File", spool_file_name);
	sPrintServer->SendMessage(message, reply);
		
	delete message;
	delete reply;
}


status_t
BPrintJob::ConfigJob()
{
	// TODO: Implement
	return B_OK;
}


void
BPrintJob::CancelJob()
{
	stop_the_show = 1;
	BEntry(spool_file_name).Remove();
	delete spoolFile;
}


status_t
BPrintJob::ConfigPage()
{
	return B_OK;
}


void
BPrintJob::SpoolPage()
{
	NewPage();
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
	if (view == NULL)
		return;
	
	// TODO: Finish me	
	if (view->LockLooper()) {
		BPicture *picture = new BPicture;
		RecurseView(view, where, picture, rect);
		AddPicture(picture, &rect, where);
		
		view->UnlockLooper();
		delete picture;
	}
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

	if (!EnsureValidMessenger())
		return B_COLOR_PRINTER; // default
    
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
	ASSERT(picture != NULL);
	
	view->AppendToPicture(picture);
	view->f_is_printing = true;
	view->Draw(rect);
	view->f_is_printing = false;
	view->EndPicture();
	
	// TODO: call recursively on every view's children.
}


void
BPrintJob::MangleName(char *filename)
{
	char sysTime[10];
	snprintf(sysTime, sizeof(sysTime), "@%lld", system_time() / 1000);
	strncpy(filename, print_job_name, B_FILE_NAME_LENGTH - sizeof(sysTime));
	strcat(filename, sysTime);
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
	// TODO: this function could be removed, and its functionality moved
	// to SpoolPage()
	// TODO: Implement for real
	page_number++;
}


void
BPrintJob::EndLastPage()
{
}


void
BPrintJob::AddSetupSpec()
{
	if (setup_msg != NULL && spoolFile != NULL)
		setup_msg->Flatten(spoolFile);
}


void
BPrintJob::AddPicture(BPicture *picture, BRect *rect, BPoint where)
{
	ASSERT(picture != NULL);
	ASSERT(spoolFile != NULL);
	ASSERT(rect != NULL);
	
	spoolFile->Write(&where, sizeof(where));
	spoolFile->Write(rect, sizeof(*rect));
	picture->Flatten(spoolFile);
}


char *
BPrintJob::GetCurrentPrinterName() const
{  
	if (!EnsureValidMessenger())
		return NULL;

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
	if (!EnsureValidMessenger())
		return;

	
	BMessage message(PSRV_GET_DEFAULT_SETTINGS);
	BMessage *reply = new BMessage;
    
	sPrintServer->SendMessage(&message, reply);
    
	if (reply->HasRect(PSRV_FIELD_PAPER_RECT))
 		reply->FindRect(PSRV_FIELD_PAPER_RECT, &paper_size);
	if (reply->HasRect(PSRV_FIELD_PRINTABLE_RECT))
		reply->FindRect(PSRV_FIELD_PRINTABLE_RECT, &usable_size);
    
	delete default_setup_msg;  	
	default_setup_msg = reply;
}


void BPrintJob::_ReservedPrintJob1() {}
void BPrintJob::_ReservedPrintJob2() {}
void BPrintJob::_ReservedPrintJob3() {}
void BPrintJob::_ReservedPrintJob4() {}
