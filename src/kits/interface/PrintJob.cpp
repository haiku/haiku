/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 				I.R. Adema
 				Stefano Ceccherini (burton666@libero.it)
 				Michael Pfeiffer
 */

// TODO refactor (avoid code duplications, decrease method sizes)

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Debug.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Messenger.h>
#include <NodeInfo.h>
#include <OS.h>
#include <Path.h>
#include <PrintJob.h>
#include <Roster.h>
#include <View.h>

#include <pr_server.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int kSemTimeOut = 50000;

static const char *kPrintServerNotRespondingText = "Print Server is not responding.";
static const char *kNoPagesToPrintText = "No Pages to print!";

// Summery of spool file format:
// See articel "How to Write a BeOS R5 Printer Driver" for description
// of spool file format: http://haiku-os.org/node/82

// print_file_header header
// BMessage          job_settings
// _page_header_     page_header
// followed by number_of_pictures:
//   BPoint   where
//   BRect    bounds
//   BPicture picture
// remaining pages start at page_header.next_page of previous page_header

struct _page_header_ {
	int32 number_of_pictures;
	off_t next_page;
	int32 reserved[10];
};


static status_t 
GetPrinterServerMessenger(BMessenger& messenger)
{
	messenger = BMessenger(PSRV_SIGNATURE_TYPE);
	return messenger.IsValid() ? B_OK : B_ERROR;
}


static void 
ShowError(const char *message)
{
	BAlert* alert = new BAlert("Error", message, "OK"); 
	alert->Go();
}


namespace BPrivate {
	
	class Configuration {
		public:
			Configuration(uint32 what, BMessage *input);
			~Configuration();
			
			status_t SendRequest(thread_func function);
	
			BMessage* Request();		
	
			void SetResult(BMessage* result);		
			BMessage* Result() const { return fResult; }		
		
		private:
			void RejectUserInput();
			void AllowUserInput();	
			void DeleteSemaphore();
		
			uint32 fWhat;	
			BMessage *fInput;
			BMessage *fRequest;
			BMessage *fResult;
			sem_id fThreadCompleted;
			BAlert *fHiddenApplicationModalWindow;
	};
	
	
	Configuration::Configuration(uint32 what, BMessage *input)
		: fWhat(what), 
		fInput(input),
		fRequest(NULL),
		fResult(NULL),
		fThreadCompleted(-1),
		fHiddenApplicationModalWindow(NULL)
	{
		RejectUserInput();
	}
	
	
	Configuration::~Configuration()
	{
		DeleteSemaphore();
			// in case SendRequest could not start the thread
		delete fRequest; fRequest = NULL;
		AllowUserInput();
	}
	
	
	void 
	Configuration::RejectUserInput()
	{
		BAlert* alert = new BAlert("bogus", "app_modal_dialog", "OK");
		fHiddenApplicationModalWindow = alert;
		alert->DefaultButton()->SetEnabled(false);
		alert->SetDefaultButton(NULL);
		alert->MoveTo(-65000, -65000);
		alert->Go(NULL);
	}
	
	
	void 
	Configuration::AllowUserInput()
	{
		fHiddenApplicationModalWindow->Lock();
		fHiddenApplicationModalWindow->Quit();
	}
	
	
	void
	Configuration::DeleteSemaphore()
	{
		if (fThreadCompleted >= B_OK) {
			sem_id id = fThreadCompleted;
			fThreadCompleted = -1;
			delete_sem(id);
		}
	}
	
	
	status_t
	Configuration::SendRequest(thread_func function)
	{
		fThreadCompleted = create_sem(0, "Configuration");	
		if (fThreadCompleted < B_OK) {
			return B_ERROR;
		}
		
		thread_id id = spawn_thread(function, "async_request", B_NORMAL_PRIORITY, this);
		if (id <= 0 || resume_thread(id) != B_OK) {
			return B_ERROR;
		}
		
		// Code copied from BAlert::Go()	
		BWindow* window = dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));
			// Get the originating window, if it exists
		
		// Heavily modified from TextEntryAlert code; the original didn't let the
		// blocked window ever draw.
		if (window != NULL) {
			status_t err;
			for (;;) {
				do {
					err = acquire_sem_etc(fThreadCompleted, 1, B_RELATIVE_TIMEOUT,
										  kSemTimeOut);
						// We've (probably) had our time slice taken away from us
				} while (err == B_INTERRUPTED);
	
				if (err == B_BAD_SEM_ID) {
					// Semaphore was finally nuked in SetResult(BMessage *)
					break;
				}
				window->UpdateIfNeeded();
			}
		} else {
			// No window to update, so just hang out until we're done.
			while (acquire_sem(fThreadCompleted) == B_INTERRUPTED);
		}
		
		status_t status;
		wait_for_thread(id, &status);
	
		return Result() != NULL ? B_OK : B_ERROR;
	}
	
	
	BMessage *
	Configuration::Request()
	{
		if (fRequest != NULL)
			return fRequest;
		
		if (fInput != NULL) {
			fRequest = new BMessage(*fInput);
			fRequest->what = fWhat;
		} else
			fRequest = new BMessage(fWhat);
		return fRequest;
	}
	
	
	void
	Configuration::SetResult(BMessage *result)
	{
		fResult = result;
		DeleteSemaphore();
			// terminate loop in thread spawned by SendRequest
	}

} // BPrivate


BPrintJob::BPrintJob(const char *job_name)
	:
	fPrintJobName(NULL),
	fPageNumber(0),
	fSpoolFile(NULL),
	fError(B_ERROR),
	fSetupMessage(NULL),
	fDefaultSetupMessage(NULL),
	fCurrentPageHeader(NULL)
{
	memset(&fCurrentHeader, 0, sizeof(fCurrentHeader));
	
	if (job_name != NULL) {
		fPrintJobName = strdup(job_name);
	}
	
	fCurrentPageHeader = new _page_header_;
	if (fCurrentPageHeader != NULL) {
		memset(fCurrentPageHeader, 0, sizeof(*fCurrentPageHeader));
	}
}


BPrintJob::~BPrintJob()
{
	CancelJob();
	
	if (fPrintJobName != NULL) {
		free(fPrintJobName);
		fPrintJobName = NULL;
	}
	
	delete fDefaultSetupMessage;
	fDefaultSetupMessage = NULL;
	
	delete fSetupMessage;
	fSetupMessage = NULL;
	
	delete fCurrentPageHeader;
	fCurrentPageHeader = NULL;
}

static
status_t ConfigPageThread(void *data)
{
	BPrivate::Configuration* configuration = static_cast<BPrivate::Configuration*>(data);
	
	BMessenger printServer;
	if (GetPrinterServerMessenger(printServer) != B_OK) {
		ShowError(kPrintServerNotRespondingText);
		configuration->SetResult(NULL);
		return B_ERROR;
	}

	BMessage *request = configuration->Request();
	if (request == NULL) {
		configuration->SetResult(NULL);
		return B_ERROR;
	}

	
	BMessage reply;
	if (printServer.SendMessage(request, &reply) != B_OK
		|| reply.what != 'okok') {
		configuration->SetResult(NULL);
		return B_ERROR;
	}
	
	configuration->SetResult(new BMessage(reply));
	return B_OK;
}


status_t
BPrintJob::ConfigPage()
{	
	BPrivate::Configuration configuration(PSRV_SHOW_PAGE_SETUP, fSetupMessage);
	status_t status = configuration.SendRequest(ConfigPageThread);
	if (status != B_OK)
		return status;
	delete fSetupMessage;
	fSetupMessage = configuration.Result();
	HandlePageSetup(fSetupMessage);
	return B_OK;
}


static status_t
ConfigJobThread(void *data)
{
	BPrivate::Configuration* configuration = static_cast<BPrivate::Configuration*>(data);
	
	BMessenger printServer;
	if (GetPrinterServerMessenger(printServer) != B_OK) {
		ShowError(kPrintServerNotRespondingText);
		configuration->SetResult(NULL);
		return B_ERROR;
	}

	BMessage *request = configuration->Request();
	if (request == NULL) {
		configuration->SetResult(NULL);
		return B_ERROR;
	}

	
	BMessage reply;
	if (printServer.SendMessage(request, &reply) != B_OK
		|| reply.what != 'okok') {
		configuration->SetResult(NULL);
		return B_ERROR;
	}
	
	configuration->SetResult(new BMessage(reply));
	return B_OK;
}

status_t
BPrintJob::ConfigJob()
{
	BPrivate::Configuration configuration(PSRV_SHOW_PRINT_SETUP, fSetupMessage);
	status_t status = configuration.SendRequest(ConfigJobThread);
	if (status != B_OK)
		return status;
	delete fSetupMessage;
	fSetupMessage = configuration.Result();	
	HandlePrintSetup(fSetupMessage);
	return B_OK;
}


void
BPrintJob::BeginJob()
{
	if (fSpoolFile != NULL) {
		// can not start a new job until it has been commited or cancelled
		return;
	}
	if (fCurrentPageHeader == NULL) {
		return;
	}
	
	if (fSetupMessage == NULL) {
		// TODO show alert, setup message is required
		return;
	}
	
	// create spool file
	BPath path;
	status_t status = find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	if (status != B_OK)
		return;
	
	char *printer = GetCurrentPrinterName();
	if (printer == NULL)
		return;
		
	path.Append(printer);
	free(printer);
	
	char mangledName[B_FILE_NAME_LENGTH];
	MangleName(mangledName);
	
	path.Append(mangledName);
	
	if (path.InitCheck() != B_OK)
		return;
	
	// TODO fSpoolFileName should store the name only (not path which can be 1024 bytes long)
	strncpy(fSpoolFileName, path.Path(), sizeof(fSpoolFileName));
	fSpoolFile = new BFile(fSpoolFileName, B_READ_WRITE | B_CREATE_FILE);

	if (fSpoolFile->InitCheck() != B_OK) {
		CancelJob();
		return;
	}		
	
	// add print_file_header 
	// page_count is updated in CommitJob()	
	fCurrentHeader.version = 1 << 16;
	fCurrentHeader.page_count = 0;
	
	if (fSpoolFile->Write(&fCurrentHeader, sizeof(fCurrentHeader)) != sizeof(fCurrentHeader)) {
		CancelJob();
		return;
	}
	
	// add printer settings message
	fSetupMessage->RemoveName(PSRV_FIELD_CURRENT_PRINTER);
	fSetupMessage->AddString(PSRV_FIELD_CURRENT_PRINTER, printer);
	AddSetupSpec();

	// prepare page header 
	// number_of_pictures is updated in DrawView()
	// next_page is updated in SpoolPage()	
	fCurrentPageHeaderOffset = fSpoolFile->Position();
	fCurrentPageHeader->number_of_pictures = 0;
	
	// state variables
	fAbort = 0;
	fPageNumber = 0;
	fError = B_OK;
	return;
}


void
BPrintJob::CommitJob()
{
	if (fSpoolFile == NULL) {
		return;
	}
	
	if (fPageNumber <= 0) {
		ShowError(kNoPagesToPrintText);
		CancelJob();
		return;
	}
	
	if (fCurrentPageHeader->number_of_pictures > 0) {
		SpoolPage();
	}
	
	// update spool file		
	EndLastPage();

	// set file attributes
 	app_info appInfo;
	be_app->GetAppInfo(&appInfo);
	const char* printerName = "";
	fSetupMessage->FindString(PSRV_FIELD_CURRENT_PRINTER, &printerName);
    
	BNodeInfo info(fSpoolFile);
	info.SetType(PSRV_SPOOL_FILETYPE);

	fSpoolFile->WriteAttr(PSRV_SPOOL_ATTR_PAGECOUNT, B_INT32_TYPE, 0, &fPageNumber, sizeof(int32));    
	fSpoolFile->WriteAttr(PSRV_SPOOL_ATTR_DESCRIPTION, B_STRING_TYPE, 0, fPrintJobName, strlen(fPrintJobName) + 1); 
	fSpoolFile->WriteAttr(PSRV_SPOOL_ATTR_PRINTER, B_STRING_TYPE, 0, printerName, strlen(printerName) + 1);    
	fSpoolFile->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0, PSRV_JOB_STATUS_WAITING, strlen(PSRV_JOB_STATUS_WAITING) + 1);    
	fSpoolFile->WriteAttr(PSRV_SPOOL_ATTR_MIMETYPE, B_STRING_TYPE, 0, appInfo.signature, strlen(appInfo.signature) + 1);

	delete fSpoolFile;
	fSpoolFile = NULL;
	fError = B_ERROR;

	// notify print server
	BMessenger printServer;
	if (GetPrinterServerMessenger(printServer) != B_OK) {
		return;
	}	

	BMessage request(PSRV_PRINT_SPOOLED_JOB);
	BMessage reply;
    	
	request.AddString("JobName", fPrintJobName);
	request.AddString("Spool File", fSpoolFileName);
	printServer.SendMessage(&request, &reply);
}

void
BPrintJob::CancelJob()
{
	if (fSpoolFile == NULL) {
		return;
	}
	
	fAbort = 1;
	BEntry(fSpoolFileName).Remove();
	delete fSpoolFile;
	fSpoolFile = NULL;
}


void
BPrintJob::SpoolPage()
{
	if (fSpoolFile == NULL) {
		return;
	}
	
	// update page header
	fCurrentPageHeader->next_page = fSpoolFile->Position();
	fSpoolFile->Seek(fCurrentPageHeaderOffset, SEEK_SET);
	fSpoolFile->Write(fCurrentPageHeader, sizeof(*fCurrentPageHeader));
	fSpoolFile->Seek(0, SEEK_END);
	
	fCurrentPageHeader->number_of_pictures = 0;
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
	
	if (view->LockLooper()) {
		BPicture picture;
		RecurseView(view, where, &picture, rect);
		AddPicture(&picture, &rect, where);		
		view->UnlockLooper();
	}
}


BMessage *
BPrintJob::Settings()
{
	if (fSetupMessage == NULL) {
		return NULL;
	}

	return new BMessage(*fSetupMessage);
}


void
BPrintJob::SetSettings(BMessage *message)
{
	if (message != NULL) {
		HandlePrintSetup(message);	
	} 
	delete fSetupMessage;
	fSetupMessage = message;
}


bool
BPrintJob::IsSettingsMessageValid(BMessage *message) const
{
	char *printerName = GetCurrentPrinterName();
	if (printerName == NULL) {
		return false;
	}
    
	const char *name = NULL;
	// The passed message is valid if it contains the right printer name.
	bool valid = message != NULL 
		&& message->FindString("printer_name", &name) == B_OK
		&& strcmp(printerName, name) == 0;
    
	free(printerName);
    
	return valid;
}

// Either SetSettings() or ConfigPage() has to be called prior
// to any of the getters otherwise they return undefined values.
BRect
BPrintJob::PaperRect()
{
	return fPaperSize;
}


BRect
BPrintJob::PrintableRect()
{
	return fUsableSize;
}


void
BPrintJob::GetResolution(int32 *xdpi, int32 *ydpi)
{
	if (xdpi != NULL) {
		*xdpi = fXResolution;
	}
	if (ydpi != NULL) {
		*ydpi = fYResolution;
	}
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
	BMessenger printServer;
	if (GetPrinterServerMessenger(printServer) != B_OK) {
		return B_COLOR_PRINTER; // default
	}    
	
	BMessage message(PSRV_GET_ACTIVE_PRINTER);
	BMessage reply;
    
	printServer.SendMessage(&message, &reply);
    
	int32 type;
	if (reply.FindInt32("color", &type) != B_OK) {
		return B_COLOR_PRINTER; // default
	}
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
	setup->FindRect(PSRV_FIELD_PRINTABLE_RECT, &fUsableSize);
	setup->FindRect(PSRV_FIELD_PAPER_RECT, &fPaperSize);
	
	// TODO verify data type (taken from libprint)
	int64 valueInt64;
	if (setup->FindInt64(PSRV_FIELD_XRES, &valueInt64) == B_OK) {
		fXResolution = (short)valueInt64;
	}
	if (setup->FindInt64(PSRV_FIELD_YRES, &valueInt64) == B_OK) {
		fYResolution = (short)valueInt64;
	}
}


bool
BPrintJob::HandlePrintSetup(BMessage *message)
{
	HandlePageSetup(message);

	bool valid = true;
	if (message->FindInt32(PSRV_FIELD_FIRST_PAGE, &fFirstPage) != B_OK) {
		valid = false;
	}
	if (message->FindInt32(PSRV_FIELD_LAST_PAGE, &fLastPage) != B_OK) {
		valid = false;
	}
    	
	return valid;
}


void
BPrintJob::NewPage()
{
	// write page header
	fCurrentPageHeaderOffset = fSpoolFile->Position();
	fSpoolFile->Write(fCurrentPageHeader, sizeof(*fCurrentPageHeader));
	fPageNumber ++;	
}


void
BPrintJob::EndLastPage()
{
	fSpoolFile->Seek(0, SEEK_SET);
	fCurrentHeader.page_count = fPageNumber;
	// TODO set first_page correctly
	// fCurrentHeader.first_page = 0;
	fSpoolFile->Write(&fCurrentHeader, sizeof(fCurrentHeader));}


void
BPrintJob::AddSetupSpec()
{
	fSetupMessage->Flatten(fSpoolFile);
}


void
BPrintJob::AddPicture(BPicture *picture, BRect *rect, BPoint where)
{
	ASSERT(picture != NULL);
	ASSERT(fSpoolFile != NULL);
	ASSERT(rect != NULL);

	if (fCurrentPageHeader->number_of_pictures == 0) {
		NewPage();	
	}
	fCurrentPageHeader->number_of_pictures ++;

	fSpoolFile->Write(&where, sizeof(where));
	fSpoolFile->Write(rect, sizeof(*rect));
	picture->Flatten(fSpoolFile);
}


// Returns a copy of the current printer name
// or NULL if it ccould not be obtained.
// Caller is responsible to free the string using free().
char *
BPrintJob::GetCurrentPrinterName() const
{  
	BMessenger printServer;
	if (GetPrinterServerMessenger(printServer)) {
		return NULL;
	}
	
	BMessage message(PSRV_GET_ACTIVE_PRINTER);
	BMessage reply;
    
	const char *printerName = NULL;
    
	if (printServer.SendMessage(&message, &reply) == B_OK) {
		reply.FindString("printer_name", &printerName);
	}
	if (printerName == NULL) {
		return NULL;
	}
	return strdup(printerName);
}


void
BPrintJob::LoadDefaultSettings()
{
	BMessenger printServer;
	if (GetPrinterServerMessenger(printServer) != B_OK) {
		return;
	}
	
	BMessage message(PSRV_GET_DEFAULT_SETTINGS);
	BMessage *reply = new BMessage;
    
	printServer.SendMessage(&message, reply);
    
	HandlePrintSetup(reply);
    
	delete fDefaultSetupMessage;  	
	fDefaultSetupMessage = reply;
}


void BPrintJob::_ReservedPrintJob1() {}
void BPrintJob::_ReservedPrintJob2() {}
void BPrintJob::_ReservedPrintJob3() {}
void BPrintJob::_ReservedPrintJob4() {}
