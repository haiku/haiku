/*
 * Copyright 2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill
 */
#ifndef REPOSITORIES_VIEW_H
#define REPOSITORIES_VIEW_H


#include <ColumnListView.h>
#include <GroupView.h>
#include <String.h>
#include <StringView.h>
#include <View.h>

#include "RepositoriesSettings.h"
#include "RepoRow.h"
#include "TaskLooper.h"


class RepositoriesListView : public BColumnListView {
public:
							RepositoriesListView(const char* name);
	virtual void			KeyDown(const char* bytes, int32 numBytes);
};


class RepositoriesView : public BGroupView {
public:
							RepositoriesView();
							~RepositoriesView();
	virtual void			AllAttached();
	virtual void			AttachedToWindow();
	virtual void			MessageReceived(BMessage*);
	void					AddManualRepository(BString url);
	bool					IsTaskRunning() { return fRunningTaskCount > 0; }

private:
	// Message helpers
	void					_AddSelectedRowsToQueue();
	void					_TaskStarted(RepoRow* rowItem, int16 count);
	void					_TaskCompleted(RepoRow* rowItem, int16 count,
								BString& newName);
	void					_TaskCanceled(RepoRow* rowItem, int16 count);
	void					_ShowCompletedStatusIfDone();
	void					_UpdateFromRepoConfig(RepoRow* rowItem);

	// GUI functions
	status_t				_EmptyList();
	void					_InitList();
	void					_RefreshList();
	void					_UpdateListFromRoster();
	void					_SaveList();
	RepoRow*				_AddRepo(BString name, BString url, bool enabled);
	void					_FindSiblings();
	void					_UpdateButtons();
	void					_UpdateStatusView();
	
	RepositoriesSettings	fSettings;
	RepositoriesListView*	fListView;
	BView*					fStatusContainerView;
	BStringView*			fListStatusView;
	TaskLooper*				fTaskLooper;
	bool					fShowCompletedStatus;
	int						fRunningTaskCount, fLastCompletedTimerId;
	BButton*				fAddButton;
	BButton*				fRemoveButton;
	BButton*				fEnableButton;
	BButton*				fDisableButton;
};


#endif
