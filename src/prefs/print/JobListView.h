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
