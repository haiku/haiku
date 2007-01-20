/*
 * Copyright 2005-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Dr.H.Reh
 */

#include "StatusWindow.h"
#include <Message.h>
#include <View.h>
#include <StatusBar.h>
#include <Button.h>
#include <stdio.h>
#include <StringView.h>


#define CANCEL_MSG 	'canM'
#define HIDE_MSG 	'hidM'


StatusWindow::StatusWindow(bool oddPages, bool evenPages, uint32 firstPage, uint32 numPages, uint32 numCopies, uint32 nup)
	: BWindow (BRect(200, 200, 650, 270), "Print Status", B_DOCUMENT_WINDOW, B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE)
{
	//	oddPages	- if true, only print odd umbered pages		
	//	evenPages	- if true, only print even umbered pages	
	//  firstPage	- number of first page
	//  numPages	- total number of pages (must be recalculate if odd/even is used)
	//  numCopies - total number of document copies	
	
	BRect frame = Frame();
	// the status view
	frame.OffsetTo(B_ORIGIN);
	fStatusView = new BView(frame,"Status View",B_FOLLOW_ALL, B_WILL_DRAW);
	fStatusView->SetViewColor(216,216,216);
	AddChild(fStatusView);			
	
	// the status bar
	fStatusBar = new BStatusBar(BRect(10, 15, 245, 50),"Status Bar","Page: ");
	fStatusView->AddChild(fStatusBar);
	
	// the cancel button
	fHideButton = new BButton(BRect(260, 25, 330,50),"Hide Button","Hide Status", new BMessage(HIDE_MSG));
	fHideButton->ResizeToPreferred();
	fStatusView->AddChild(fHideButton);

	fCancelButton = new BButton(BRect(260, 25, 330,50),"Cancel Button","Cancel", new BMessage(CANCEL_MSG));
	fCancelButton->ResizeToPreferred();
	fCancelButton->MoveBy(90,0);
	
	fStatusView->AddChild(fCancelButton);
	
	fCancelBar = false;		
			
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
	fNops = (uint32) ( fNops/(float)nup ) +  addPage;
		// recalculate page numbers nup-pages-up
			
	fStatusBar->SetMaxValue ((float)fNops);
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
		
		fStatusBar->Update(fStatusDelta*100.0/fNops, string1.String(), string3.String());
		if ( (fStatusBar->MaxValue()) == (fStatusBar->CurrentValue()) )
				fCancelButton->SetEnabled(false);
	Unlock();
	return fCancelBar;
}

void 
StatusWindow::SetPageCopies(uint32 copies)
{
	fCopies = copies;
	fStatusDelta = fDelta/(float)fCopies;
	fDocCopies--;
}


// Handling of user interface and other events
void 
StatusWindow::MessageReceived(BMessage *message)
{

	switch(message->what) {
		case CANCEL_MSG:	// 'CancelButton' is pressed...
			{
				fCancelBar = true;		
				fCancelButton->SetEnabled(false);
				fCancelButton->SetLabel("Job cancelled");
			}
			break;

		case HIDE_MSG:	// 'HideButton' is pressed...
			{
				Hide();
			}
			break;

		default:
			inherited::MessageReceived(message);
			break;
	}

}
