/*
 * Copyright 2005-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Dr.H.Reh
 */

#include "StatusWindow.h"

#include <Button.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Message.h>
#include <View.h>
#include <StatusBar.h>
#include <stdio.h>
#include <StringView.h>


#define CANCEL_MSG 	'canM'
#define HIDE_MSG 	'hidM'


StatusWindow::StatusWindow(bool oddPages, bool evenPages, uint32 firstPage,
	uint32 numPages, uint32 numCopies, uint32 nup)
	:
	BWindow(BRect(200, 200, 250, 250),
		"Print Status",
		B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE
			| B_AUTO_UPDATE_SIZE_LIMITS)
{
	//	oddPages	- if true, only print odd numbered pages
	//	evenPages	- if true, only print even numbered pages
	//  firstPage	- number of first page
	//  numPages	- total number of pages (must be recalculate if odd/even is
	//                used)
	//  numCopies   - total number of document copies
	
	// the status bar
	fStatusBar = new BStatusBar("statusBar", "Page: ");
	
	// the cancel button
	fHideButton = new BButton("hideButton",	"Hide Status",
		new BMessage(HIDE_MSG));

	fCancelButton = new BButton("cancelButton", "Cancel",
		new BMessage(CANCEL_MSG));
	
	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 10)
		.Add(fStatusBar)
		.Add(fHideButton)
		.Add(fCancelButton)
		.SetInsets(10, 10, 10, 10)
	);


	fCancelled = false;
			
	// calculate the real number of pages
	fNops = numPages;
	
	bool evenFirstPage 	= (firstPage % 2) == 0;
	bool evenNumPages 	= (numPages  % 2) == 0;
	
	// recalculate page numbers if even or odd is used 
	if (oddPages || evenPages) { 		
		if  (evenNumPages) { 		
			fNops = numPages / 2;
		} else if (evenFirstPage) {
			if (oddPages)
				fNops = (numPages - 1) / 2;
			if (evenPages)
				fNops = (numPages + 1) / 2;
		} else {
			if (oddPages)  
				fNops = (numPages + 1) / 2;
			if (evenPages)
				fNops = (numPages - 1) / 2;
		}
	}	
	
	uint32 addPage = 0;
	if (fNops % nup > 0) 
		addPage = 1;
	fNops = (uint32)(fNops / (float)nup) + addPage;
		// recalculate page numbers nup-pages-up
			
	fStatusBar->SetMaxValue((float)fNops);
		// max value of status bar = real number of pages
	fDelta = 1.0/numCopies;
		// reduce step width of status bar
	fStatusDelta = fDelta;
	fDocCopies = numCopies;
	
	fDocumentCopy = fDocCopies > 1;	
	
	fDocCopies++;

	ResetStatusBar();
	Show();
}


StatusWindow::~StatusWindow(void)
{
}


void 
StatusWindow::ResetStatusBar(void)
{
	Lock();
		fStatusBar->Reset("Page:  ");
		InvalidateLayout(true);
	Unlock();
}


bool 
StatusWindow::UpdateStatusBar(uint32 page, uint32 copy)
{
	Lock();
		Activate(true);		
			// Frontmost Window
		char buffer[20];
		
		sprintf(buffer,"%d", (int)(page + 1));
		BString string1(buffer);
		
		sprintf(buffer,"%d",(int)fNops );						
		BString string2(buffer);
		string1.Append(BString(" / "));
		string1.Append(string2);
		
		BString string3 = BString("Remaining Document Copies:  ");	
		if (fDocumentCopy == true) {
			sprintf(buffer, "%d", (int)(fDocCopies));
			BString string4(buffer);
			string3.Append(string4);
		} else {
			string3 = BString("Remaining Page Copies:  ");	
			char buffer[20];
			sprintf(buffer,"%d",(int)(fCopies - copy) );						
			BString string4(buffer);
			string3.Append(string4);		
		}
		
		fStatusBar->Update(fStatusDelta * 100.0 / fNops,
			string1.String(), string3.String());
		if (fStatusBar->MaxValue() == fStatusBar->CurrentValue())
				fCancelButton->SetEnabled(false);

		InvalidateLayout(true);
	Unlock();
	return fCancelled;
}


void 
StatusWindow::SetPageCopies(uint32 copies)
{
	fCopies = copies;
	fStatusDelta = fDelta / (float)fCopies;
	fDocCopies--;
}


// Handling of user interface and other events
void 
StatusWindow::MessageReceived(BMessage *message)
{

	switch(message->what) {
		case CANCEL_MSG:	// 'CancelButton' is pressed...
			fCancelled = true;
			fCancelButton->SetEnabled(false);
			fCancelButton->SetLabel("Job cancelled");
			InvalidateLayout(true);
			break;

		case HIDE_MSG:	// 'HideButton' is pressed...
			Hide();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}

}
