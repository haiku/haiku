/*****************************************************************************/
// Printers Preference Application.
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001-2007 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/
#ifndef PRINTERS_WINDOW_H
#define PRINTERS_WINDOW_H


#include <Window.h>

class BBox;
class PrintersWindow;
class PrinterListView;
class JobListView;
class Job;
class SpoolFolder;
class PrinterItem;


class PrintersWindow : public BWindow {
public:
	PrintersWindow(BRect frame);
	
	void MessageReceived(BMessage* msg);
	bool QuitRequested();
	
	void AddJob(SpoolFolder* folder, Job* job);
	void RemoveJob(SpoolFolder* folder, Job* job);
	void UpdateJob(SpoolFolder* folder, Job* job);
	
private:
	void BuildGUI();
	bool IsSelected(PrinterItem* printer);
	void UpdatePrinterButtons();
	void UpdateJobButtons();

	typedef BWindow Inherited;
	
	PrinterListView*	fPrinterListView;
	BButton*	fMakeDefault;
	BButton*	fRemove;

	JobListView*	fJobListView;
	BButton*	fRestart;
	BButton*    fCancel;
	
	BBox*		fJobsBox;

	PrinterItem* fSelectedPrinter;
	
	bool fAddingPrinter;
};

#endif	// PRINTERS_WINDOW_H
