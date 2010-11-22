/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#ifndef _PRINTERS_WINDOW_H
#define _PRINTERS_WINDOW_H


#include <Box.h>
#include <Window.h>


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

#endif	// _PRINTERS_WINDOW_H
