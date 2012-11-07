/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "BreakpointsView.h"

#include <new>

#include <Button.h>
#include <LayoutBuilder.h>

#include <AutoLocker.h>
#include <ObjectList.h>

#include "MessageCodes.h"
#include "Team.h"
#include "UserBreakpoint.h"


// #pragma mark - BreakpointsView


BreakpointsView::BreakpointsView(Team* team, Listener* listener)
	:
	BGroupView(B_HORIZONTAL, 4.0f),
	fTeam(team),
	fBreakpoint(NULL),
	fListView(NULL),
	fToggleBreakpointButton(NULL),
	fRemoveBreakpointButton(NULL),
	fListener(listener)
{
	SetName("Breakpoints");
}


BreakpointsView::~BreakpointsView()
{
	if (fListView != NULL)
		fListView->UnsetListener();
}


/*static*/ BreakpointsView*
BreakpointsView::Create(Team* team, Listener* listener)
{
	BreakpointsView* self = new BreakpointsView(team, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
BreakpointsView::UnsetListener()
{
	fListener = NULL;
}


void
BreakpointsView::SetBreakpoint(UserBreakpoint* breakpoint, Watchpoint* watchpoint)
{
	if (breakpoint == fBreakpoint && watchpoint == fWatchpoint)
		return;

	if (fWatchpoint != NULL)
		fWatchpoint->ReleaseReference();

	if (fBreakpoint != NULL)
		fBreakpoint->ReleaseReference();

	fWatchpoint = watchpoint;
	fBreakpoint = breakpoint;

	if (fBreakpoint != NULL)
		fBreakpoint->AcquireReference();
	else if (fWatchpoint != NULL)
		fWatchpoint->AcquireReference();

	fListView->SetBreakpoint(breakpoint, watchpoint);

	_UpdateButtons();
}


void
BreakpointsView::UserBreakpointChanged(UserBreakpoint* breakpoint)
{
	fListView->UserBreakpointChanged(breakpoint);

	_UpdateButtons();
}


void
BreakpointsView::WatchpointChanged(Watchpoint* watchpoint)
{
	fListView->WatchpointChanged(watchpoint);

	_UpdateButtons();
}


void
BreakpointsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_ENABLE_BREAKPOINT:
			if (fListener != NULL) {
				if (fBreakpoint != NULL) {
					fListener->SetBreakpointEnabledRequested(fBreakpoint,
						true);
				} else if (fWatchpoint != NULL) {
					fListener->SetWatchpointEnabledRequested(fWatchpoint,
						true);
				}
			}
			break;
		case MSG_DISABLE_BREAKPOINT:
			if (fListener != NULL) {
				if (fBreakpoint != NULL) {
					fListener->SetBreakpointEnabledRequested(fBreakpoint,
						false);
				} else if (fWatchpoint != NULL) {
					fListener->SetWatchpointEnabledRequested(fWatchpoint,
						false);
				}
			}
			break;
		case MSG_CLEAR_BREAKPOINT:
			if (fListener != NULL) {
				if (fBreakpoint != NULL) {
					fListener->ClearBreakpointRequested(fBreakpoint);
				} else if (fWatchpoint != NULL) {
					fListener->ClearWatchpointRequested(fWatchpoint);
				}
			}
			break;
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
BreakpointsView::AttachedToWindow()
{
	fToggleBreakpointButton->SetTarget(this);
	fRemoveBreakpointButton->SetTarget(this);
}


void
BreakpointsView::LoadSettings(const BMessage& settings)
{
	BMessage breakpointListSettings;
	if (settings.FindMessage("breakpointList", &breakpointListSettings)
		== B_OK)
		fListView->LoadSettings(breakpointListSettings);
}


status_t
BreakpointsView::SaveSettings(BMessage& settings)
{
	BMessage breakpointListSettings;
	if (fListView->SaveSettings(breakpointListSettings) != B_OK)
		return B_NO_MEMORY;

	if (settings.AddMessage("breakpointList", &breakpointListSettings) != B_OK)
		return B_NO_MEMORY;

	return B_OK;
}


void
BreakpointsView::BreakpointSelectionChanged(UserBreakpoint* breakpoint)
{
	if (fListener != NULL)
		fListener->BreakpointSelectionChanged(breakpoint);
}


void
BreakpointsView::WatchpointSelectionChanged(Watchpoint* watchpoint)
{
	if (fListener != NULL)
		fListener->WatchpointSelectionChanged(watchpoint);
}


void
BreakpointsView::_Init()
{
	BLayoutBuilder::Group<>(this)
		.Add(fListView = BreakpointListView::Create(fTeam, this))
		.AddGroup(B_VERTICAL, 4.0f)
			.Add(fToggleBreakpointButton = new BButton("Toggle"))
			.Add(fRemoveBreakpointButton = new BButton("Remove"))
			.AddGlue()
		.End();

	fToggleBreakpointButton->SetMessage(new BMessage(MSG_ENABLE_BREAKPOINT));
	fRemoveBreakpointButton->SetMessage(new BMessage(MSG_CLEAR_BREAKPOINT));

	_UpdateButtons();
}


void
BreakpointsView::_UpdateButtons()
{
	AutoLocker<Team> teamLocker(fTeam);

	bool enabled = false;
	bool valid = false;

	if (fBreakpoint != NULL && fBreakpoint->IsValid()) {
		valid = true;
		enabled = fBreakpoint->IsEnabled();
	} else if (fWatchpoint != NULL) {
		valid = true;
		enabled = fWatchpoint->IsEnabled();
	}

	if (valid) {
		if (enabled) {
			fToggleBreakpointButton->SetLabel("Disable");
			fToggleBreakpointButton->SetMessage(
				new BMessage(MSG_DISABLE_BREAKPOINT));
		} else {
			fToggleBreakpointButton->SetLabel("Enable");
			fToggleBreakpointButton->SetMessage(
				new BMessage(MSG_ENABLE_BREAKPOINT));
		}

		fToggleBreakpointButton->SetEnabled(true);
		fRemoveBreakpointButton->SetEnabled(true);
	} else {
		fToggleBreakpointButton->SetLabel("Enable");
		fToggleBreakpointButton->SetEnabled(false);
		fRemoveBreakpointButton->SetEnabled(false);
	}
}


// #pragma mark - Listener


BreakpointsView::Listener::~Listener()
{
}
