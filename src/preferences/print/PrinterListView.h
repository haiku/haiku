/*****************************************************************************/
// Printers Preference Application.
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001-2003 OpenBeOS Project
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

#ifndef PRINTERSLISTVIEW_H
#define PRINTERSLISTVIEW_H

class PrinterListView;

#include <Messenger.h>
#include <ListView.h>
#include <String.h>
#include <Directory.h>
#include <Entry.h>

#include "FolderWatcher.h"

class SpoolFolder;
class PrinterItem;
class PrinterListView;

class PrinterListView : public BListView, public FolderListener {
	private:
		typedef BListView Inherited;
		
		void AddPrinter(BDirectory &printer);
		PrinterItem *FindItem(node_ref *node);
		
		void EntryCreated(node_ref *node, entry_ref *entry);
		void EntryRemoved(node_ref *node);
		void AttributeChanged(node_ref *node);
	
	public:
		PrinterListView(BRect frame);
		void AttachedToWindow();
		bool QuitRequested();
	
		void BuildPrinterList();
		PrinterItem *SelectedItem();
			
		void UpdateItem(PrinterItem *item);

	private:
		BDirectory fNode;
		FolderWatcher *fFolder;		
};

class BBitmap;
class PrintersWindow;
class PrinterItem : public BListItem {
	public:
		PrinterItem(PrintersWindow *window, const BDirectory &node);
		~PrinterItem();
		
		void DrawItem(BView *owner, BRect bounds, bool complete);
		void Update(BView *owner, const BFont *font);
		
		bool Remove(BListView *view);
		bool IsActivePrinter();
		bool HasPendingJobs();
		
		const char *Name() const { 
			return fName.String(); 
		}
			
		SpoolFolder *Folder();
		BDirectory *Node();
		void UpdatePendingJobs();
		
	private:
		void GetStringProperty(const char *propName, BString &outString);
	
		SpoolFolder *fFolder;
		BDirectory fNode;
		BString	fComments;
		BString	fTransport;
		BString	fDriverName;
		BString	fName;
		BString fPendingJobs;
		
		static BBitmap *sIcon;
		static BBitmap *sSelectedIcon;
};

#endif
