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

class PrinterListView : public BListView, public FolderListener
{
	typedef BListView Inherited;
	
	BDirectory fNode;
	FolderWatcher* fFolder;
	
	void AddPrinter(BDirectory& printer);
	PrinterItem* FindItem(node_ref* node);
	
	void EntryCreated(node_ref* node, entry_ref* entry);
	void EntryRemoved(node_ref* node);
	void AttributeChanged(node_ref* node);

public:
	PrinterListView(BRect frame);
	void AttachedToWindow();
	bool QuitRequested();

	void BuildPrinterList();
	PrinterItem* SelectedItem();
		
	void UpdateItem(PrinterItem* item);
};

class BBitmap;
class PrintersWindow;
class PrinterItem : public BListItem
{
public:
	PrinterItem(PrintersWindow* window, const BDirectory& node/*const BMessenger& thePrinter*/);
	~PrinterItem();
	
	void DrawItem(BView *owner, BRect bounds, bool complete);
	void Update(BView *owner, const BFont *font);
	
	bool Remove(BListView* view);
	bool IsActivePrinter();
	bool HasPendingJobs();
	
	const char* Name() const 
		{ return fName.String(); }
		
	SpoolFolder* Folder();
	BDirectory* Node();
	void UpdatePendingJobs();
	
private:
	void GetStringProperty(const char* propName, BString& outString);

	SpoolFolder *fFolder;
	BDirectory  fNode;
	BString		fComments;
	BString		fTransport;
	BString		fDriverName;
	BString		fName;
	BString     fPendingJobs;
	static BBitmap* sIcon;
	static BBitmap* sSelectedIcon;
};

#endif
