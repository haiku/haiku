/*
 *  Printers Preference Application.
 *  Copyright (C) 2001, 2002 OpenBeOS. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PRINTERSWINDOW_H
#define PRINTERSWINDOW_H

class PrintersWindow;
class PrinterListView;
class JobListView;
class Job;
class SpoolFolder;
class PrinterItem;

#include <Window.h>

class PrintersWindow : public BWindow
{
	typedef BWindow Inherited;
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
	
	PrinterListView*	fPrinterListView;
	BButton*	fMakeDefault;
	BButton*	fRemove;

	JobListView*	fJobListView;
	BButton*	fRestart;
	BButton*    fCancel;
	
	BBox*		fJobsBox;

	PrinterItem* fSelectedPrinter;
};

#endif
