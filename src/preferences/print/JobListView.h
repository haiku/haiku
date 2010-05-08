/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#ifndef _JOB_LISTVIEW_H
#define _JOB_LISTVIEW_H


#include <Bitmap.h>
#include <ListItem.h>
#include <ListView.h>
#include <String.h>


class Job;
class JobItem;
class SpoolFolder;


class JobListView : public BListView {
	typedef BListView Inherited;
public:
								JobListView(BRect frame);
								~JobListView();

			void				AttachedToWindow();
			void				SetSpoolFolder(SpoolFolder* folder);

			void				AddJob(Job* job);
			void				RemoveJob(Job* job);
			void				UpdateJob(Job* job);

			JobItem* 			SelectedItem() const;

			void				RestartJob();
			void				CancelJob();

private:
			JobItem* 			FindJob(Job* job) const;
};


class JobItem : public BListItem {
public:
								JobItem(Job* job);
								~JobItem();

			void				Update();

			void				Update(BView *owner, const BFont *font);
			void				DrawItem(BView *owner, BRect bounds,
									bool complete);

			Job* 				GetJob() const { return fJob; }

private:
			Job*				fJob;
			BBitmap*			fIcon;
			BString				fName;
			BString				fPages;
			BString				fStatus;
			BString				fSize;
};

#endif // _JOB_LISTVIEW_H
