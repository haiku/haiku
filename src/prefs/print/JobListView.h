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

#ifndef JOBLISTVIEW_H
#define JOBLISTVIEW_H

#include <Messenger.h>
#include <ListView.h>
#include <String.h>

#include "Jobs.h"

class JobItem;
class JobListView;
class SpoolFolder;

class JobListView : public BListView
{
	typedef BListView Inherited;
private:	
	JobItem* Find(Job* job);
	
public:
	JobListView(BRect frame);
	void AttachedToWindow();
	void SetSpoolFolder(SpoolFolder* folder);
	
	void AddJob(Job* job);
	void RemoveJob(Job* job);
	void UpdateJob(Job* job);

	JobItem* SelectedItem();
	
	void RestartJob();
	void CancelJob();
};

class BBitmap;
class JobItem : public BListItem
{
public:
	JobItem(Job* job);
	~JobItem();

	void Update();
	
	void Update(BView *owner, const BFont *font);
	void DrawItem(BView *owner, BRect bounds, bool complete);
	
	Job* GetJob() { return fJob; }

private:

	Job* fJob;
	BBitmap* fIcon;
	BString fName, fPages;
	BString fStatus, fSize;
};


#endif
