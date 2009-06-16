/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TeamWindow.h"

#include <GroupLayoutBuilder.h>
#include <Message.h>
#include <TabView.h>

#include "Team.h"
#include "ThreadListView.h"


// #pragma mark - TeamWindow


TeamWindow::TeamWindow(::Team* team, Listener* listener)
	:
	BWindow(BRect(100, 100, 399, 299), _GetWindowTitle(team).String(),
		B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fTeam(team),
	fListener(listener),
	fTabView(NULL),
	fThreadListView(NULL)
{
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


/*static*/ BString
TeamWindow::_GetWindowTitle(::Team* team)
{
	BString name = team->Name();
	if (team->ID() >= 0)
		name << " (" << team->ID() << ")";
	return name;
}


void
TeamWindow::_Init()
{
	BGroupLayout* rootLayout = new BGroupLayout(B_VERTICAL);
	SetLayout(rootLayout);

	fTabView = new BTabView("tab view");

	BGroupLayoutBuilder(rootLayout)
		.Add(fTabView);

	fTabView->AddTab(fThreadListView = ThreadListView::Create());
//	fTabView->AddTab(fTeamsPage = new TeamsPage(this));
//	fTabView->AddTab(fThreadsPage = new ThreadsPage(this));

	fThreadListView->SetTeam(fTeam);
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
