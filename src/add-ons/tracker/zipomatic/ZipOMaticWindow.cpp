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


ZippoWindow::ZippoWindow(BRect frame, BMessage* refs)
	:
	BWindow(frame, "Zip-O-Matic", B_TITLED_WINDOW, B_NOT_V_RESIZABLE),
	fView(NULL),
	fThread(NULL),
	fWindowGotRefs(false),
	fZippingWasStopped(false),
	fWindowInvoker(new BInvoker(new BMessage(ZIPPO_QUIT_OR_CONTINUE), NULL,
		this))
{
	fView = new ZippoView(Bounds());
	AddChild(fView);
	
	SetSizeLimits(Bounds().Width(), 15000, Bounds().Height(),
		Bounds().Height());

	if (refs != NULL)
	{
		fWindowGotRefs = true;
		_StartZipping(refs);
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
		
		case ZIPPO_THREAD_EXIT:
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
											
		case ZIPPO_THREAD_EXIT_ERROR:
			// TODO: figure out why this case does not happen when it should
			fThread = NULL;
			fView->fActivityView->Stop();
			fView->fStopButton->SetEnabled(false);
			fView->fArchiveNameView->SetText("");
			fView->fZipOutputView->SetText("Error creating archive");
			break;
						
		case ZIPPO_TASK_DESCRIPTION:
		{
			BString string;
			if (message->FindString("archive_filename", &string) == B_OK)
				fView->fArchiveNameView->SetText(string.String());
			break;
		}

		case ZIPPO_LINE_OF_STDOUT:
		{
			BString string;
			if (message->FindString("zip_output", &string) == B_OK)
				fView->fZipOutputView->SetText(string.String());
			break;
		}

		case ZIPPO_QUIT_OR_CONTINUE:
		{
			int32 which_button = -1;
			if (message->FindInt32("which", &which_button) == B_OK) {
				if (which_button == 0) {
					StopZipping();
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
	if (!IsZipping()) {
		BMessage message(ZIPPO_WINDOW_QUIT);
		message.AddRect("frame", Frame());
		be_app_messenger.SendMessage(&message);
		return true;
	} else {
		fThread->SuspendExternalZip();
		fView->fActivityView->Pause();

		BAlert* alert = new BAlert("Stop or Continue",
			"Are you sure you want to stop creating this archive?", "Stop",
			"Continue", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go(fWindowInvoker);
		
		return false;
	}
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
ZippoWindow::StopZipping()
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

