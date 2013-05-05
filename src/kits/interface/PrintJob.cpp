/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		I.R. Adema
 *		Stefano Ceccherini (burton666@libero.it)
 *		Michael Pfeiffer
 *		julun <host.haiku@gmx.de>
 */


#include <PrintJob.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#include <Region.h>
#include <Roster.h>
#include <SystemCatalog.h>
#include <View.h>

#include <pr_server.h>
#include <ViewPrivate.h>

using BPrivate::gSystemCatalog;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrintJob"

#undef B_TRANSLATE
#define B_TRANSLATE(str) \
	gSystemCatalog.GetString(B_TRANSLATE_MARK(str), "PrintJob")


/*!	Summary of spool file:

		|-----------------------------------|
		|         print_file_header         |
		|-----------------------------------|
		|    BMessage print_job_settings    |
		|-----------------------------------|
		|                                   |
		| ********** (first page) ********* |
		| *                               * |
		| *         _page_header_         * |
		| * ----------------------------- * |
		| * |---------------------------| * |
		| * |       BPoint where        | * |
		| * |       BRect bounds        | * |
		| * |       BPicture pic        | * |
		| * |---------------------------| * |
		| * |---------------------------| * |
		| * |       BPoint where        | * |
		| * |       BRect bounds        | * |
		| * |       BPicture pic        | * |
		| * |---------------------------| * |
		| ********************************* |
		|                                   |
		| ********* (second page) ********* |
		| *                               * |
		| *         _page_header_         * |
		| * ----------------------------- * |
		| * |---------------------------| * |
		| * |       BPoint where        | * |
		| * |       BRect bounds        | * |
		| * |       BPicture pic        | * |
		| * |---------------------------| * |
		| ********************************* |
		|-----------------------------------|

	BeOS R5 print_file_header.version is 1 << 16
	BeOS R5 print_file_header.first_page is -1

	each page can consist of a collection of picture structures
	remaining pages start at _page_header_.next_page of previous _page_header_

	See also: "How to Write a BeOS R5 Printer Driver" for description of spool
	file format: http://haiku-os.org/documents/dev/how_to_write_a_printer_driver
*/


struct _page_header_ {
	int32 number_of_pictures;
	off_t next_page;
	int32 reserved[10];
};


static void
ShowError(const char* message)
{
	BAlert* alert = new BAlert(B_TRANSLATE("Error"), message, B_TRANSLATE("OK"));
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


// #pragma mark -- PrintServerMessenger


namespace BPrivate {


class PrintServerMessenger {
public:
							PrintServerMessenger(uint32 what, BMessage* input);
							~PrintServerMessenger();

			BMessage*		Request();
			status_t		SendRequest();

			void			SetResult(BMessage* result);
			BMessage*		Result() const { return fResult; }

	static	status_t		GetPrintServerMessenger(BMessenger& messenger);

private:
			void			RejectUserInput();
			void			AllowUserInput();
			void			DeleteSemaphore();
	static	status_t		MessengerThread(void* data);

			uint32			fWhat;
			BMessage*		fInput;
			BMessage*		fRequest;
			BMessage*		fResult;
			sem_id			fThreadCompleted;
			BAlert*			fHiddenApplicationModalWindow;
};


}	// namespace BPrivate


using namespace BPrivate;


// #pragma mark -- BPrintJob


BPrintJob::BPrintJob(const char* jobName)
	:
	fPrintJobName(NULL),
	fSpoolFile(NULL),
	fError(B_NO_INIT),
	fSetupMessage(NULL),
	fDefaultSetupMessage(NULL),
	fAbort(0),
	fCurrentPageHeader(NULL)
{
	memset(&fSpoolFileHeader, 0, sizeof(print_file_header));

	if (jobName != NULL && jobName[0])
		fPrintJobName = strdup(jobName);

	fCurrentPageHeader = new _page_header_;
	if (fCurrentPageHeader != NULL)
		memset(fCurrentPageHeader, 0, sizeof(_page_header_));
}


BPrintJob::~BPrintJob()
{
	CancelJob();

	free(fPrintJobName);
	delete fSetupMessage;
	delete fDefaultSetupMessage;
	delete fCurrentPageHeader;
}


status_t
BPrintJob::ConfigPage()
{
	PrintServerMessenger messenger(PSRV_SHOW_PAGE_SETUP, fSetupMessage);
	status_t status = messenger.SendRequest();
	if (status != B_OK)
		return status;

	delete fSetupMessage;
	fSetupMessage = messenger.Result();
	_HandlePageSetup(fSetupMessage);

	return B_OK;
}


status_t
BPrintJob::ConfigJob()
{
	PrintServerMessenger messenger(PSRV_SHOW_PRINT_SETUP, fSetupMessage);
	status_t status = messenger.SendRequest();
	if (status != B_OK)
		return status;

	delete fSetupMessage;
	fSetupMessage = messenger.Result();
	if (!_HandlePrintSetup(fSetupMessage))
		return B_ERROR;

	fError = B_OK;
	return B_OK;
}


void
BPrintJob::BeginJob()
{
	fError = B_ERROR;

	// can not start a new job until it has been commited or cancelled
	if (fSpoolFile != NULL || fCurrentPageHeader == NULL)
		return;

	// TODO show alert, setup message is required
	if (fSetupMessage == NULL)
		return;

	// create spool file
	BPath path;
	status_t status = find_directory(B_USER_PRINTERS_DIRECTORY, &path);
	if (status != B_OK)
		return;

	char *printer = _GetCurrentPrinterName();
	if (printer == NULL)
		return;

	path.Append(printer);
	free(printer);

	char mangledName[B_FILE_NAME_LENGTH];
	_GetMangledName(mangledName, B_FILE_NAME_LENGTH);

	path.Append(mangledName);
	if (path.InitCheck() != B_OK)
		return;

	// TODO: fSpoolFileName should store the name only (not path which can be
	// 1024 bytes long)
	strlcpy(fSpoolFileName, path.Path(), sizeof(fSpoolFileName));
	fSpoolFile = new BFile(fSpoolFileName, B_READ_WRITE | B_CREATE_FILE);

	if (fSpoolFile->InitCheck() != B_OK) {
		CancelJob();
		return;
	}

	// add print_file_header
	// page_count is updated in CommitJob()
	// on BeOS R5 the offset to the first page was always -1
	fSpoolFileHeader.version = 1 << 16;
	fSpoolFileHeader.page_count = 0;
	fSpoolFileHeader.first_page = (off_t)-1;

	if (fSpoolFile->Write(&fSpoolFileHeader, sizeof(print_file_header))
			!= sizeof(print_file_header)) {
		CancelJob();
		return;
	}

	// add printer settings message
	if (!fSetupMessage->HasString(PSRV_FIELD_CURRENT_PRINTER))
		fSetupMessage->AddString(PSRV_FIELD_CURRENT_PRINTER, printer);

	_AddSetupSpec();
	_NewPage();

	// state variables
	fAbort = 0;
	fError = B_OK;
}


void
BPrintJob::CommitJob()
{
	if (fSpoolFile == NULL)
		return;

	if (fSpoolFileHeader.page_count == 0) {
		ShowError(B_TRANSLATE("No Pages to print!"));
		CancelJob();
		return;
	}

	// update spool file
	_EndLastPage();

	// write spool file header
	fSpoolFile->Seek(0, SEEK_SET);
	fSpoolFile->Write(&fSpoolFileHeader, sizeof(print_file_header));

	// set file attributes
	app_info appInfo;
	be_app->GetAppInfo(&appInfo);
	const char* printerName = "";
	fSetupMessage->FindString(PSRV_FIELD_CURRENT_PRINTER, &printerName);

	BNodeInfo info(fSpoolFile);
	info.SetType(PSRV_SPOOL_FILETYPE);

	fSpoolFile->WriteAttr(PSRV_SPOOL_ATTR_PAGECOUNT, B_INT32_TYPE, 0,
		&fSpoolFileHeader.page_count, sizeof(int32));
	fSpoolFile->WriteAttr(PSRV_SPOOL_ATTR_DESCRIPTION, B_STRING_TYPE, 0,
		fPrintJobName, strlen(fPrintJobName) + 1);
	fSpoolFile->WriteAttr(PSRV_SPOOL_ATTR_PRINTER, B_STRING_TYPE, 0,
		printerName, strlen(printerName) + 1);
	fSpoolFile->WriteAttr(PSRV_SPOOL_ATTR_STATUS, B_STRING_TYPE, 0,
		PSRV_JOB_STATUS_WAITING, strlen(PSRV_JOB_STATUS_WAITING) + 1);
	fSpoolFile->WriteAttr(PSRV_SPOOL_ATTR_MIMETYPE, B_STRING_TYPE, 0,
		appInfo.signature, strlen(appInfo.signature) + 1);

	delete fSpoolFile;
	fSpoolFile = NULL;
	fError = B_ERROR;

	// notify print server
	BMessenger printServer;
	if (PrintServerMessenger::GetPrintServerMessenger(printServer) != B_OK)
		return;

	BMessage request(PSRV_PRINT_SPOOLED_JOB);
	request.AddString("JobName", fPrintJobName);
	request.AddString("Spool File", fSpoolFileName);

	BMessage reply;
	printServer.SendMessage(&request, &reply);
}


void
BPrintJob::CancelJob()
{
	if (fSpoolFile == NULL)
		return;

	fAbort = 1;
	BEntry(fSpoolFileName).Remove();
	delete fSpoolFile;
	fSpoolFile = NULL;
}


void
BPrintJob::SpoolPage()
{
	if (fSpoolFile == NULL)
		return;

	if (fCurrentPageHeader->number_of_pictures == 0)
		return;

	fSpoolFileHeader.page_count++;
	fSpoolFile->Seek(0, SEEK_END);
	if (fCurrentPageHeaderOffset) {
		// update last written page_header
		fCurrentPageHeader->next_page = fSpoolFile->Position();
		fSpoolFile->Seek(fCurrentPageHeaderOffset, SEEK_SET);
		fSpoolFile->Write(fCurrentPageHeader, sizeof(_page_header_));
		fSpoolFile->Seek(fCurrentPageHeader->next_page, SEEK_SET);
	}

	_NewPage();
}


bool
BPrintJob::CanContinue()
{
	// Check if our local error storage is still B_OK
	return fError == B_OK && !fAbort;
}


void
BPrintJob::DrawView(BView* view, BRect rect, BPoint where)
{
	if (fSpoolFile == NULL)
		return;

	if (view == NULL)
		return;

	if (view->LockLooper()) {
		BPicture picture;
		_RecurseView(view, B_ORIGIN - rect.LeftTop(), &picture, rect);
		_AddPicture(picture, rect, where);
		view->UnlockLooper();
	}
}


BMessage*
BPrintJob::Settings()
{
	if (fSetupMessage == NULL)
		return NULL;

	return new BMessage(*fSetupMessage);
}


void
BPrintJob::SetSettings(BMessage* message)
{
	if (message != NULL)
		_HandlePrintSetup(message);

	delete fSetupMessage;
	fSetupMessage = message;
}


bool
BPrintJob::IsSettingsMessageValid(BMessage* message) const
{
	char* printerName = _GetCurrentPrinterName();
	if (printerName == NULL)
		return false;

	const char* name = NULL;
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
	if (fDefaultSetupMessage == NULL)
		_LoadDefaultSettings();

	return fPaperSize;
}


BRect
BPrintJob::PrintableRect()
{
	if (fDefaultSetupMessage == NULL)
		_LoadDefaultSettings();

	return fUsableSize;
}


void
BPrintJob::GetResolution(int32* xdpi, int32* ydpi)
{
	if (fDefaultSetupMessage == NULL)
		_LoadDefaultSettings();

	if (xdpi != NULL)
		*xdpi = fXResolution;

	if (ydpi != NULL)
		*ydpi = fYResolution;
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
BPrintJob::PrinterType(void*) const
{
	BMessenger printServer;
	if (PrintServerMessenger::GetPrintServerMessenger(printServer) != B_OK)
		return B_COLOR_PRINTER; // default

	BMessage reply;
	BMessage message(PSRV_GET_ACTIVE_PRINTER);
	printServer.SendMessage(&message, &reply);

	int32 type;
	if (reply.FindInt32("color", &type) != B_OK)
		return B_COLOR_PRINTER; // default

	return type;
}


// #pragma mark - private


void
BPrintJob::_RecurseView(BView* view, BPoint origin, BPicture* picture,
	BRect rect)
{
	ASSERT(picture != NULL);

	BRegion region;
	region.Set(BRect(rect.left, rect.top, rect.right, rect.bottom));
	view->fState->print_rect = rect;

	view->AppendToPicture(picture);
	view->PushState();
	view->SetOrigin(origin);
	view->ConstrainClippingRegion(&region);

	if (view->ViewColor() != B_TRANSPARENT_COLOR) {
		rgb_color highColor = view->HighColor();
		view->SetHighColor(view->ViewColor());
		view->FillRect(rect);
		view->SetHighColor(highColor);
	}

	if ((view->Flags() & B_WILL_DRAW) != 0) {
		view->fIsPrinting = true;
		view->Draw(rect);
		view->fIsPrinting = false;
	}

	view->PopState();
	view->EndPicture();

	BView* child = view->ChildAt(0);
	while (child != NULL) {
		if (!child->IsHidden()) {
			BPoint leftTop(view->Bounds().LeftTop() + child->Frame().LeftTop());
			BRect printRect(rect.OffsetToCopy(rect.LeftTop() - leftTop)
				& child->Bounds());
			if (printRect.IsValid())
				_RecurseView(child, origin + leftTop, picture, printRect);
		}
		child = child->NextSibling();
	}

	if ((view->Flags() & B_DRAW_ON_CHILDREN) != 0) {
		view->AppendToPicture(picture);
		view->PushState();
		view->SetOrigin(origin);
		view->ConstrainClippingRegion(&region);
		view->fIsPrinting = true;
		view->DrawAfterChildren(rect);
		view->fIsPrinting = false;
		view->PopState();
		view->EndPicture();
	}
}


void
BPrintJob::_GetMangledName(char* buffer, size_t bufferSize) const
{
	snprintf(buffer, bufferSize, "%s@%" B_PRId64, fPrintJobName,
		system_time() / 1000);
}


void
BPrintJob::_HandlePageSetup(BMessage* setup)
{
	setup->FindRect(PSRV_FIELD_PRINTABLE_RECT, &fUsableSize);
	setup->FindRect(PSRV_FIELD_PAPER_RECT, &fPaperSize);

	// TODO verify data type (taken from libprint)
	int64 valueInt64;
	if (setup->FindInt64(PSRV_FIELD_XRES, &valueInt64) == B_OK)
		fXResolution = (short)valueInt64;

	if (setup->FindInt64(PSRV_FIELD_YRES, &valueInt64) == B_OK)
		fYResolution = (short)valueInt64;
}


bool
BPrintJob::_HandlePrintSetup(BMessage* message)
{
	_HandlePageSetup(message);

	bool valid = true;
	if (message->FindInt32(PSRV_FIELD_FIRST_PAGE, &fFirstPage) != B_OK)
		valid = false;

	if (message->FindInt32(PSRV_FIELD_LAST_PAGE, &fLastPage) != B_OK)
		valid = false;

	return valid;
}


void
BPrintJob::_NewPage()
{
	// init, write new page_header
	fCurrentPageHeader->next_page = 0;
	fCurrentPageHeader->number_of_pictures = 0;
	fCurrentPageHeaderOffset = fSpoolFile->Position();
	fSpoolFile->Write(fCurrentPageHeader, sizeof(_page_header_));
}


void
BPrintJob::_EndLastPage()
{
	if (!fSpoolFile)
		return;

	if (fCurrentPageHeader->number_of_pictures == 0)
		return;

	fSpoolFileHeader.page_count++;
	fSpoolFile->Seek(0, SEEK_END);
	if (fCurrentPageHeaderOffset) {
		fCurrentPageHeader->next_page = 0;
		fSpoolFile->Seek(fCurrentPageHeaderOffset, SEEK_SET);
		fSpoolFile->Write(fCurrentPageHeader, sizeof(_page_header_));
		fSpoolFile->Seek(0, SEEK_END);
	}
}


void
BPrintJob::_AddSetupSpec()
{
	fSetupMessage->Flatten(fSpoolFile);
}


void
BPrintJob::_AddPicture(BPicture& picture, BRect& rect, BPoint& where)
{
	ASSERT(fSpoolFile != NULL);

	fCurrentPageHeader->number_of_pictures++;
	fSpoolFile->Write(&where, sizeof(BRect));
	fSpoolFile->Write(&rect, sizeof(BPoint));
	picture.Flatten(fSpoolFile);
}


/*!	Returns a copy of the applications default printer name or NULL if it
	could not be obtained. Caller is responsible to free the string using
	free().
*/
char*
BPrintJob::_GetCurrentPrinterName() const
{
	BMessenger printServer;
	if (PrintServerMessenger::GetPrintServerMessenger(printServer) != B_OK)
		return NULL;

	const char* printerName = NULL;

	BMessage reply;
	BMessage message(PSRV_GET_ACTIVE_PRINTER);
	if (printServer.SendMessage(&message, &reply) == B_OK)
		reply.FindString("printer_name", &printerName);

	if (printerName == NULL)
		return NULL;

	return strdup(printerName);
}


void
BPrintJob::_LoadDefaultSettings()
{
	BMessenger printServer;
	if (PrintServerMessenger::GetPrintServerMessenger(printServer) != B_OK)
		return;

	BMessage message(PSRV_GET_DEFAULT_SETTINGS);
	BMessage* reply = new BMessage;

	printServer.SendMessage(&message, reply);

	// Only override our settings if we don't have any settings yet
	if (fSetupMessage == NULL)
		_HandlePrintSetup(reply);

	delete fDefaultSetupMessage;
	fDefaultSetupMessage = reply;
}


void BPrintJob::_ReservedPrintJob1() {}
void BPrintJob::_ReservedPrintJob2() {}
void BPrintJob::_ReservedPrintJob3() {}
void BPrintJob::_ReservedPrintJob4() {}


// #pragma mark -- PrintServerMessenger


namespace BPrivate {


PrintServerMessenger::PrintServerMessenger(uint32 what, BMessage *input)
	:
	fWhat(what),
	fInput(input),
	fRequest(NULL),
	fResult(NULL),
	fThreadCompleted(-1),
	fHiddenApplicationModalWindow(NULL)
{
	RejectUserInput();
}


PrintServerMessenger::~PrintServerMessenger()
{
	DeleteSemaphore();
		// in case SendRequest could not start the thread
	delete fRequest; fRequest = NULL;
	AllowUserInput();
}


void
PrintServerMessenger::RejectUserInput()
{
	fHiddenApplicationModalWindow = new BAlert("bogus", "app_modal", "OK");
	fHiddenApplicationModalWindow->DefaultButton()->SetEnabled(false);
	fHiddenApplicationModalWindow->SetDefaultButton(NULL);
	fHiddenApplicationModalWindow->SetFlags(fHiddenApplicationModalWindow->Flags() | B_CLOSE_ON_ESCAPE);
	fHiddenApplicationModalWindow->MoveTo(-65000, -65000);
	fHiddenApplicationModalWindow->Go(NULL);
}


void
PrintServerMessenger::AllowUserInput()
{
	fHiddenApplicationModalWindow->Lock();
	fHiddenApplicationModalWindow->Quit();
}


void
PrintServerMessenger::DeleteSemaphore()
{
	if (fThreadCompleted >= B_OK) {
		sem_id id = fThreadCompleted;
		fThreadCompleted = -1;
		delete_sem(id);
	}
}


status_t
PrintServerMessenger::SendRequest()
{
	fThreadCompleted = create_sem(0, "print_server_messenger_sem");
	if (fThreadCompleted < B_OK)
		return B_ERROR;

	thread_id id = spawn_thread(MessengerThread, "async_request",
		B_NORMAL_PRIORITY, this);
	if (id <= 0 || resume_thread(id) != B_OK)
		return B_ERROR;

	// Get the originating window, if it exists
	BWindow* window = dynamic_cast<BWindow*>(
		BLooper::LooperForThread(find_thread(NULL)));
	if (window != NULL) {
		status_t err;
		while (true) {
			do {
				err = acquire_sem_etc(fThreadCompleted, 1, B_RELATIVE_TIMEOUT,
					50000);
			// We've (probably) had our time slice taken away from us
			} while (err == B_INTERRUPTED);

			// Semaphore was finally nuked in SetResult(BMessage *)
			if (err == B_BAD_SEM_ID)
				break;
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


BMessage*
PrintServerMessenger::Request()
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
PrintServerMessenger::SetResult(BMessage* result)
{
	fResult = result;
	DeleteSemaphore();
	// terminate loop in thread spawned by SendRequest
}


status_t
PrintServerMessenger::GetPrintServerMessenger(BMessenger& messenger)
{
	messenger = BMessenger(PSRV_SIGNATURE_TYPE);
	return messenger.IsValid() ? B_OK : B_ERROR;
}


status_t
PrintServerMessenger::MessengerThread(void* data)
{
	PrintServerMessenger* messenger = static_cast<PrintServerMessenger*>(data);

	BMessenger printServer;
	if (messenger->GetPrintServerMessenger(printServer) != B_OK) {
		ShowError(B_TRANSLATE("Print Server is not responding."));
		messenger->SetResult(NULL);
		return B_ERROR;
	}

	BMessage* request = messenger->Request();
	if (request == NULL) {
		messenger->SetResult(NULL);
		return B_ERROR;
	}


	BMessage reply;
	if (printServer.SendMessage(request, &reply) != B_OK
		|| reply.what != 'okok' ) {
		messenger->SetResult(NULL);
		return B_ERROR;
	}

	messenger->SetResult(new BMessage(reply));
	return B_OK;
}


}	// namespace BPrivate
