/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TeamWindow.h"

#include <LayoutBuilder.h>
#include <Message.h>
#include <TabView.h>
#include <SplitView.h>
#include <TextView.h>

#include "ImageListView.h"
#include "Team.h"
#include "ThreadListView.h"


// #pragma mark - TeamWindow


TeamWindow::TeamWindow(::Team* team, Listener* listener)
	:
	BWindow(BRect(100, 100, 699, 499), "Team", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fTeam(team),
	fListener(listener),
	fTabView(NULL),
	fThreadListView(NULL),
	fImageListView(NULL)
{
	BString name = team->Name();
	if (team->ID() >= 0)
		name << " (" << team->ID() << ")";
	SetTitle(name.String());
}


TeamWindow::~TeamWindow()
{
}


/*static*/ TeamWindow*
TeamWindow::Create(::Team* team, Listener* listener)
{
	TeamWindow* self = new TeamWindow(team, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
TeamWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
TeamWindow::QuitRequested()
{
	return fListener->TeamWindowQuitRequested(this);
}


void
TeamWindow::_Init()
{
	BGroupLayout* rootLayout = new BGroupLayout(B_VERTICAL);
	SetLayout(rootLayout);

	BLayoutBuilder::Group<>(rootLayout)
		.AddSplit(B_VERTICAL, 3.0f)
			.Add(fTabView = new BTabView("tab view"), 0.4f)
			.AddSplit(B_HORIZONTAL, 3.0f)
				.Add(fImageListView = ImageListView::Create())
				.Add(new BTextView("source view"), 2.0f)
			.End()
		.End();
	
	fTabView->AddTab(fThreadListView = ThreadListView::Create());
//	fTabView->AddTab(fTeamsPage = new TeamsPage(this));
//	fTabView->AddTab(fThreadsPage = new ThreadsPage(this));

	fThreadListView->SetTeam(fTeam);
	fImageListView->SetTeam(fTeam);
}


// #pragma mark - Listener


TeamWindow::Listener::~Listener()
{
}


bool
TeamWindow::Listener::TeamWindowQuitRequested(TeamWindow* window)
{
	return true;
}
