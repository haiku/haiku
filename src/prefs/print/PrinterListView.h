/*
 *  Printers Preference Application.
 *  Copyright (C) 2001 OpenBeOS. All Rights Reserved.
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

class PrinterListView : public BListView
{
	typedef BListView Inherited;
public:
	PrinterListView(BRect frame);
	void AttachedToWindow();
};

class BBitmap;
class PrinterItem : public BListItem
{
public:
	PrinterItem(const BMessenger& thePrinter);

	void DrawItem(BView *owner, BRect bounds, bool complete);
	void Update(BView *owner, const BFont *font);
	
	void Remove(BListView* view);
	bool IsActivePrinter();
	
	const char* Name() const 
		{ return fName.String(); }
private:
	void GetStringProperty(const char* propName, BString& outString);

	BMessenger	fMessenger;
	BString		fComments;
	BString		fTransport;
	BString		fDriverName;
	BString		fName;
	static BBitmap* sIcon;
	static BBitmap* sSelectedIcon;
};

#endif
