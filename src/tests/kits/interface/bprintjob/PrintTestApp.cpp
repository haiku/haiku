#include "PrintTestApp.hpp"
#include "PrintTestWindow.hpp"

#include <PrintJob.h>

#define your_document_last_page		100

int main()
{
	new PrintTestApp;
		be_app->Run();
	delete be_app;

	return 0;
}

PrintTestApp::PrintTestApp()
	: Inherited(PRINTTEST_SIGNATURE)
{
}

void PrintTestApp::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case 'PStp':
			DoTestPageSetup();
			break;

		case 'Prnt':
			Print();
			break;
	}
}

void PrintTestApp::ReadyToRun()
{
	(new PrintTestWindow)->Show();
}


status_t PrintTestApp::DoTestPageSetup()
{
	BPrintJob job("document's name"); 
	status_t result = job.ConfigPage();
	if (result == B_OK) {
		// Get the user Settings 
		fSetupData = job.Settings();

		// Use the new settings for your internal use 
		fPaperRect = job.PaperRect(); 
		fPrintableRect = job.PrintableRect(); 
	}
	
	return result;
}

status_t PrintTestApp::Print()
{ 
	status_t result = B_OK; 

	// If we have no setup message, we should call ConfigPage() 
	// You must use the same instance of the BPrintJob object 
	// (you can't call the previous "PageSetup()" function, since it 
	// creates its own BPrintJob object).

	if (!fSetupData) {
		result = DoTestPageSetup();
	}

	BPrintJob job("document's name"); 
	if (result == B_OK) { 
		// Setup the driver with user settings 
		job.SetSettings(fSetupData); 

		result = job.ConfigJob(); 
		if (result == B_OK) { 
			// WARNING: here, setup CANNOT be NULL. 
			if (fSetupData == NULL) {
				// something's wrong, handle the error and bail out
			}
			delete fSetupData;

			// Get the user Settings
			fSetupData = job.Settings(); 

			// Use the new settings for your internal use 
			// They may have changed since the last time you read it 
			fPaperRect = job.PaperRect(); 
			fPrintableRect = job.PrintableRect(); 

			// Now you have to calculate the number of pages 
			// (note: page are zero-based) 
			int32 firstPage = job.FirstPage(); 
			int32 lastPage = job.LastPage(); 

			// Verify the range is correct 
			// 0 ... LONG_MAX -> Print all the document 
			// n ... LONG_MAX -> Print from page n to the end 
			// n ... m -> Print from page n to page m 

			if (lastPage > your_document_last_page) 
				lastPage = your_document_last_page; 

			int32 nbPages = lastPage - firstPage + 1; 

			// Verify the range is correct 
			if (nbPages <= 0) 
				return B_ERROR; 

			// Now you can print the page 
			job.BeginJob(); 

			// Print all pages 
			bool can_continue = job.CanContinue(); 

			for (int i=firstPage ; can_continue && i<=lastPage ; i++) { 
				// Draw all the needed views 
//IRA			job.DrawView(someView, viewRect, pointOnPage); 
//IRA			job.DrawView(anotherView, anotherRect, differentPoint); 

/*
				// If the document have a lot of pages, you can update a BStatusBar, here 
				// or else what you want... 
				update_status_bar(i-firstPage, nbPages); 
*/

				// Spool the page 
				job.SpoolPage(); 

/*
				// Cancel the job if needed. 
				// You can for exemple verify if the user pressed the ESC key 
				// or (SHIFT + '.')... 
				if (user_has_canceled) 
				{ 
					// tell the print_server to cancel the printjob 
					job.CancelJob(); 
					can_continue = false; 
					break; 
				}
*/
				// Verify that there was no error (disk full for example) 
				can_continue = job.CanContinue(); 
			}

			// Now, you just have to commit the job! 
			if (can_continue) 
				job.CommitJob(); 
			else 
				result = B_ERROR; 
		}
	} 

	return result; 
}
