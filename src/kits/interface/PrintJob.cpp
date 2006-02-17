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


// TODO: No clue at the moment
// We'll need to find out examining a r5 spool file
// Maybe the Print kit team found out already ?
struct _page_header_ {
	char padding[52];
};


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
	fPrintJobName(NULL),
	fPageNumber(0),
	fSpoolFile(NULL),
	fError(B_OK),
	fSetupMessage(NULL),
	fDefaultSetupMessage(NULL),
	fCurrentPageHeader(NULL)
{
	memset(&fCurrentHeader, 0, sizeof(fCurrentHeader));
	if (job_name != NULL)
		fPrintJobName = strdup(job_name);
	fCurrentPageHeader = new _page_header_;
}


BPrintJob::~BPrintJob()
{
	free(fPrintJobName);
	delete fCurrentPageHeader;
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
	
	fAbort = 0;
	fPageNumber = 0;
	
	strncpy(fSpoolFileName, path.Path(), sizeof(fSpoolFileName));
	fSpoolFile = new BFile(fSpoolFileName, B_READ_WRITE|B_CREATE_FILE);
	
	AddSetupSpec();

	fCurrentPageHeaderOffset = fSpoolFile->Position();
	fSpoolFile->Write(fCurrentPageHeader, sizeof(_page_header_));
}


void
BPrintJob::CommitJob()
{
	if (fPageNumber <= 0) {
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
    
	fSpoolFile->WriteAttr("_spool/Page Count", B_INT32_TYPE, 0, &fPageNumber, sizeof(int32));    
	fSpoolFile->WriteAttr("_spool/Description", B_STRING_TYPE, 0, fPrintJobName, strlen(fPrintJobName) + 1); 
	fSpoolFile->WriteAttr("_spool/Printer", B_STRING_TYPE, 0, printerName, strlen(printerName) + 1);    
	fSpoolFile->WriteAttr("_spool/Status", B_STRING_TYPE, 0, "waiting", strlen("waiting") + 1);    
	fSpoolFile->WriteAttr("_spool/MimeType", B_STRING_TYPE, 0, appInfo.signature, strlen(appInfo.signature) + 1);
	
	message = new BMessage(PSRV_PRINT_SPOOLED_JOB);
	reply = new BMessage;
	
	message->AddString("JobName", fPrintJobName);
	message->AddString("Spool File", fSpoolFileName);
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
	fAbort = 1;
	BEntry(fSpoolFileName).Remove();
	delete fSpoolFile;
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
	return fError == B_OK && !fAbort;
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
    
	if (fSetupMessage != NULL)
		message = new BMessage(*fSetupMessage);
    
	return message;
}


void
BPrintJob::SetSettings(BMessage *message)
{
	if (message != NULL) {
		HandlePageSetup(message);
		HandlePrintSetup(message);
		delete fSetupMessage;
		fSetupMessage = message;
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
	if (fDefaultSetupMessage == NULL)
		LoadDefaultSettings();
    	
	return fPaperSize;
}


BRect
BPrintJob::PrintableRect()
{
	if (fDefaultSetupMessage == NULL)
		LoadDefaultSettings();
    	
	return fUsableSize;
}


void
BPrintJob::GetResolution(int32 *xdpi, int32 *ydpi)
{
	int32 xres = -1, yres = -1;
	
	const BMessage *message = NULL;
	
	if (fSetupMessage != NULL)
		message = fSetupMessage;
	else {
		if (fDefaultSetupMessage == NULL)
			LoadDefaultSettings();
		message = fDefaultSetupMessage;
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
    return fFirstPage;
}


int32
BPrintJob::LastPage()
{
    return fLastPage;
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
	
	BView *child = view->ChildAt(0);
	while (child != NULL) {
		// TODO: origin and rect should probably
		// be converted for children views in some way
		RecurseView(child, origin, picture, rect);
		child = child->NextSibling();
	}
}


void
BPrintJob::MangleName(char *filename)
{
	char sysTime[10];
	snprintf(sysTime, sizeof(sysTime), "@%lld", system_time() / 1000);
	strncpy(filename, fPrintJobName, B_FILE_NAME_LENGTH - sizeof(sysTime));
	strcat(filename, sysTime);
}


void
BPrintJob::HandlePageSetup(BMessage *setup)
{
	if (fSetupMessage != NULL && setup != fSetupMessage && setup != NULL)
		delete fSetupMessage;

	if (setup->HasRect(PSRV_FIELD_PRINTABLE_RECT))
		setup->FindRect(PSRV_FIELD_PRINTABLE_RECT, &fUsableSize);
	if (setup->HasRect(PSRV_FIELD_PAPER_RECT))
		setup->FindRect(PSRV_FIELD_PAPER_RECT, &fPaperSize);
	
	fSetupMessage = setup;
}


bool
BPrintJob::HandlePrintSetup(BMessage *message)
{
	if (message->HasRect(PSRV_FIELD_PRINTABLE_RECT))
		message->FindRect(PSRV_FIELD_PRINTABLE_RECT, &fUsableSize);
	if (message->HasRect(PSRV_FIELD_PAPER_RECT))
		message->FindRect(PSRV_FIELD_PAPER_RECT, &fPaperSize);
	if (message->HasInt32(PSRV_FIELD_FIRST_PAGE))
		message->FindInt32(PSRV_FIELD_FIRST_PAGE, &fFirstPage);
	if (message->HasInt32(PSRV_FIELD_LAST_PAGE))
		message->FindInt32(PSRV_FIELD_LAST_PAGE, &fLastPage);
    	
	return true;
}


void
BPrintJob::NewPage()
{
	// TODO: this function could be removed, and its functionality moved
	// to SpoolPage()
	// TODO: Implement for real
	fPageNumber++;
}


void
BPrintJob::EndLastPage()
{
	if (fSpoolFile == NULL)
		return;

	fCurrentHeader.page_count++;
	fSpoolFile->Seek(fCurrentPageHeaderOffset, SEEK_SET);
	fSpoolFile->Write(fCurrentPageHeader, sizeof(_page_header_));
	fSpoolFile->Seek(0, SEEK_END);
}


void
BPrintJob::AddSetupSpec()
{
	if (fSetupMessage != NULL && fSpoolFile != NULL)
		fSetupMessage->Flatten(fSpoolFile);
}


void
BPrintJob::AddPicture(BPicture *picture, BRect *rect, BPoint where)
{
	ASSERT(picture != NULL);
	ASSERT(fSpoolFile != NULL);
	ASSERT(rect != NULL);
	
	fSpoolFile->Write(&where, sizeof(where));
	fSpoolFile->Write(rect, sizeof(*rect));
	picture->Flatten(fSpoolFile);
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
 		reply->FindRect(PSRV_FIELD_PAPER_RECT, &fPaperSize);
	if (reply->HasRect(PSRV_FIELD_PRINTABLE_RECT))
		reply->FindRect(PSRV_FIELD_PRINTABLE_RECT, &fUsableSize);
    
	delete fDefaultSetupMessage;  	
	fDefaultSetupMessage = reply;
}


void BPrintJob::_ReservedPrintJob1() {}
void BPrintJob::_ReservedPrintJob2() {}
void BPrintJob::_ReservedPrintJob3() {}
void BPrintJob::_ReservedPrintJob4() {}
