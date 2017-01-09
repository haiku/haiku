/*
 * Copyright 2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill
 */
#ifndef REPOSITORIES_WINDOW_H
#define REPOSITORIES_WINDOW_H


#include <Node.h>
#include <Window.h>

#include "AddRepoWindow.h"
#include "RepositoriesSettings.h"
#include "RepositoriesView.h"


class RepositoriesWindow : public BWindow {
public:
							RepositoriesWindow();
							~RepositoriesWindow();
	virtual	bool			QuitRequested();
	virtual void			MessageReceived(BMessage*);

private:
	void					_StartWatching();
	void					_StopWatching();
	
	RepositoriesSettings	fSettings;
	RepositoriesView*		fView;
	AddRepoWindow*			fAddWindow;
	BMessenger				fMessenger;
	node_ref				fPackageNodeRef;
		// node_ref to watch for changes to package-repositories directory
	status_t				fPackageNodeStatus;
	bool					fWatchingPackageNode;
		// true when package-repositories directory is being watched
};


#endif
