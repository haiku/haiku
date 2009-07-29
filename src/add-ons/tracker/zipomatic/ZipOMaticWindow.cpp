/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "ZipOMaticWindow.h"

#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <InterfaceKit.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include "ZipOMatic.h"
#include "ZipOMaticActivity.h"
#include "ZipOMaticMisc.h"
#include "ZipOMaticView.h"
#include "ZipperThread.h"


ZippoWindow::ZippoWindow(BMessage* message)
	:
	BWindow(BRect(200, 200, 430, 310), "Zip-O-Matic", B_TITLED_WINDOW,
		B_NOT_V_RESIZABLE),
	fView(NULL),
	fSettings(),
	fThread(NULL),
	fWindowGotRefs(false),
	fZippingWasStopped(false),
	fWindowInvoker(new BInvoker(new BMessage('alrt'), NULL, this))
{
	status_t status = B_OK;

	status = fSettings.SetTo("ZipOMatic.msg");
	if (status != B_OK)	
		ErrorMessage("fSettings.SetTo()", status);

	status = fSettings.InitCheck();
	if (status != B_OK)
		ErrorMessage("fSettings.InitCheck()", status);
	
	fView = new ZippoView(Bounds());
	AddChild(fView);
	
	SetSizeLimits(Bounds().Width(), 15000, Bounds().Height(),
		Bounds().Height());

	_ReadSettings();

	if (message != NULL)
	{
		fWindowGotRefs = true;
		_StartZipping(message);
	}
}


ZippoWindow::~ZippoWindow()	
{
	delete fWindowInvoker;
}


void 
ZippoWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_REFS_RECEIVED:
			_StartZipping(message);
			break;

		case B_SIMPLE_DATA:
			if (IsZipping()) {	
				message->what = B_REFS_RECEIVED;	
				be_app_messenger.SendMessage(message);
			} else {
				_StartZipping(message);
			}
			break;
		
		case 'exit':
			// thread has finished - (finished, quit, killed, we don't know)
			fThread = NULL;
			fView->fActivityView->Stop();
			fView->fStopButton->SetEnabled(false);
			fView->fArchiveNameView->SetText(" ");
			if (fZippingWasStopped)
				fView->fZipOutputView->SetText("Stopped");
			else
				fView->fZipOutputView->SetText("Archive created OK");
				
			_CloseWindowOrKeepOpen();
			break;
											
		case 'exrr':	// thread has finished - badly
			fThread = NULL;
			fView->fActivityView->Stop();
			fView->fStopButton->SetEnabled(false);
			fView->fArchiveNameView->SetText("");
			fView->fZipOutputView->SetText("Error creating archive");
			break;
						
		case 'strt':
		{
			BString string;
			if (message->FindString("archive_filename", &string) == B_OK)
				fView->fArchiveNameView->SetText(string.String());
			break;
		}

		case 'outp':
		{
			BString string;
			if (message->FindString("zip_output", &string) == B_OK)
				fView->fZipOutputView->SetText(string.String());
			break;
		}

		case 'alrt':
		{
			int32 which_button = -1;
			if (message->FindInt32("which", &which_button) == B_OK) {
				if (which_button == 0) {
					_StopZipping();
				} else {
					if (fThread != NULL)
						fThread->ResumeExternalZip();
						
					fView->fActivityView->Start();
				}
			}
			break;
		}
		
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
ZippoWindow::QuitRequested()
{
	if (fThread == NULL) {
		_WriteSettings();
		be_app_messenger.SendMessage(ZIPPO_WINDOW_QUIT);
		return true;
	} else {
		if (fThread != NULL)
			fThread->SuspendExternalZip();
	
		fView->fActivityView->Pause();
	
		BAlert* alert = new BAlert("Stop or Continue",
			"Are you sure you want to stop creating this archive?", "Stop",
			"Continue", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go(fWindowInvoker);
		return false;
	}
}


status_t
ZippoWindow::_ReadSettings()
{
	status_t status = B_OK;

	status = fSettings.InitCheck();
	if (status != B_OK)
		ErrorMessage("fSettings.InitCheck()", status);

	status = fSettings.ReadSettings();
	if (status != B_OK)
		ErrorMessage("fSettings.ReadSettings()", status);

	BRect windowRect;
	
	status = fSettings.FindRect("windowRect", &windowRect);
	if (status != B_OK)
	{
		ErrorMessage("fSettings.FindRect(windowRect)", status);
		return status;
	}
	
	ResizeTo(windowRect.Width(), windowRect.Height());
	MoveTo(windowRect.LeftTop());
	
	return B_OK;
}


status_t
ZippoWindow::_WriteSettings()
{
	status_t status = B_OK;

	status = fSettings.InitCheck();
	if (status != B_OK)
		ErrorMessage("fSettings.InitCheck()", status);

	status = fSettings.MakeEmpty();
	if (status != B_OK)
		ErrorMessage("fSettings.MakeEmpty()", status);

	status = fSettings.AddRect("windowRect", Frame());
	if (status != B_OK)
	{
		ErrorMessage("fSettings.AddRect(windowRect)", status);
		return status;
	}
	
	status = fSettings.WriteSettings();
	if (status != B_OK)
	{
		ErrorMessage("fSettings.WriteSettings()", status);
		return status;
	}
	
	return B_OK;
}


void
ZippoWindow::_StartZipping(BMessage* message)
{
	fView->fStopButton->SetEnabled(true);
	fView->fActivityView->Start();

	fThread = new ZipperThread(message, this);
	fThread->Start();
	
	fZippingWasStopped = false;
}


void
ZippoWindow::_StopZipping()
{
	fZippingWasStopped = true;

	fView->fStopButton->SetEnabled(false);
	fView->fActivityView->Stop();

	fThread->InterruptExternalZip();
	fThread->Quit();
	
	status_t status = B_OK;
	fThread->WaitForThread(&status);
	fThread = NULL;

	fView->fArchiveNameView->SetText(" ");
	fView->fZipOutputView->SetText("Stopped");

	_CloseWindowOrKeepOpen();
}


bool
ZippoWindow::IsZipping()
{
	if (fThread == NULL)
		return false;
	else
		return true;
}


void
ZippoWindow::_CloseWindowOrKeepOpen()
{
	if (fWindowGotRefs)
		PostMessage(B_QUIT_REQUESTED);
}


void
ZippoWindow::Zoom(BPoint origin, float width, float height)
{
	if (IsZipping()) {
		float archiveNameWidth =
			fView->StringWidth(fView->fArchiveNameView->Text());
		float zipOutputWidth  = 
			fView->StringWidth(fView->fZipOutputView->Text());
	
		if (zipOutputWidth > archiveNameWidth)
			ResizeTo(zipOutputWidth, Bounds().Height());
		else
			ResizeTo(archiveNameWidth, Bounds().Height());
		
	} else {
		ResizeTo(230,110);
	}
}

