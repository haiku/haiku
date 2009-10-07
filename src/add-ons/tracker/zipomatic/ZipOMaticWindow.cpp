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

#include <Alert.h>
#include <Application.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Path.h>
#include <Roster.h>
#include <SeparatorView.h>
#include <String.h>

#include "ZipOMatic.h"
#include "ZipOMaticActivity.h"
#include "ZipOMaticMisc.h"
#include "ZipperThread.h"


ZippoWindow::ZippoWindow(BRect frame, BMessage* refs)
	:
	BWindow(frame, "Zip-O-Matic", B_TITLED_WINDOW,
		B_NOT_RESIZABLE	| B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE),
	fThread(NULL),
	fWindowGotRefs(false),
	fZippingWasStopped(false),
	fFileCount(0),
	fWindowInvoker(new BInvoker(new BMessage(ZIPPO_QUIT_OR_CONTINUE), NULL,
		this))
{
	fActivityView = new Activity("activity");
	fActivityView->SetExplicitMinSize(BSize(171, 17));

	fArchiveNameView = new BStringView("archive_text", "");
	fArchiveNameView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));

	fZipOutputView = new BStringView("output_text", "Drop files to zip.");
	fZipOutputView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_UNSET));

	fStopButton = new BButton("stop", "Stop", new BMessage(B_QUIT_REQUESTED));
	fStopButton->SetEnabled(false);
	fStopButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT,
		B_ALIGN_VERTICAL_UNSET));

	BSeparatorView* separator = new BSeparatorView(B_HORIZONTAL);

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, 10)
			.Add(fActivityView)
			.Add(fArchiveNameView)
			.Add(fZipOutputView)
			.Add(separator)
			.Add(fStopButton)
			.SetInsets(14, 14, 14, 14)
			.End()
		.End();

	if (refs != NULL) {
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
			fActivityView->Stop();
			fStopButton->SetEnabled(false);
			fArchiveNameView->SetText(" ");
			if (fZippingWasStopped)
				fZipOutputView->SetText("Stopped");
			else
				fZipOutputView->SetText("Archive created OK");
				
			_CloseWindowOrKeepOpen();
			break;
											
		case ZIPPO_THREAD_EXIT_ERROR:
			// TODO: figure out why this case does not happen when it should
			fThread = NULL;
			fActivityView->Stop();
			fStopButton->SetEnabled(false);
			fArchiveNameView->SetText("");
			fZipOutputView->SetText("Error creating archive");
			break;
						
		case ZIPPO_TASK_DESCRIPTION:
		{
			BString string;
			if (message->FindString("archive_filename", &string) == B_OK)
				fArchiveNameView->SetText(string.String());
			break;
		}

		case ZIPPO_LINE_OF_STDOUT:
		{
			BString string;
			if (message->FindString("zip_output", &string) == B_OK) {
				if (string.FindFirst("Adding: ") == 0
					|| string.FindFirst("Updating: ") == 0) {

					// This is a workaround for the current window resizing
					// behavior as the window resizes for each line of output.
					// Instead of showing the true output of /bin/zip
					// we display a file count and whether the archive is
					// being created (added to) or if we're updating an 
					// already existing archive.

					fFileCount++;
					BString countString;
					countString << fFileCount;

					if (fFileCount == 1)
						countString << " file";
					else
						countString << " files";

					if (string.FindFirst("Adding: ") == 0)
						countString << " added.";

					if (string.FindFirst("Updating: ") == 0)
						countString << " updated.";

					fZipOutputView->SetText(countString.String());
				} else {
					fZipOutputView->SetText(string.String());
				}
			}
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
						
					fActivityView->Start();
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
		fActivityView->Pause();

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
	fStopButton->SetEnabled(true);
	fActivityView->Start();
	fFileCount = 0;

	fThread = new ZipperThread(message, this);
	fThread->Start();
	
	fZippingWasStopped = false;
}


void
ZippoWindow::StopZipping()
{
	fZippingWasStopped = true;

	fStopButton->SetEnabled(false);
	fActivityView->Stop();

	fThread->InterruptExternalZip();
	fThread->Quit();
	
	status_t status = B_OK;
	fThread->WaitForThread(&status);
	fThread = NULL;

	fArchiveNameView->SetText(" ");
	fZipOutputView->SetText("Stopped");

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

