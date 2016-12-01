/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "BreakpointsView.h"

#include <new>

#include <Button.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>

#include <AutoLocker.h>
#include <ObjectList.h>

#include "AppMessageCodes.h"
#include "MessageCodes.h"
#include "Team.h"
#include "UserBreakpoint.h"


// #pragma mark - BreakpointsView


BreakpointsView::BreakpointsView(Team* team, Listener* listener)
	:
	BGroupView(B_HORIZONTAL, 4.0f),
	fTeam(team),
	fListView(NULL),
	fToggleBreakpointButton(NULL),
	fEditBreakpointButton(NULL),
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
		case MSG_DISABLE_BREAKPOINT:
		case MSG_CLEAR_BREAKPOINT:
			_HandleBreakpointAction(message->what);
			break;

		case MSG_SHOW_BREAKPOINT_EDIT_WINDOW:
			message->AddPointer("breakpoint",
				fSelectedBreakpoints.ItemAt(0)->GetBreakpoint());
			Window()->PostMessage(message);
			break;

		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
BreakpointsView::AttachedToWindow()
{
	fEditBreakpointButton->SetTarget(this);
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
BreakpointsView::BreakpointSelectionChanged(BreakpointProxyList& proxies)
{
	if (fListener != NULL)
		fListener->BreakpointSelectionChanged(proxies);

	_SetSelection(proxies);
}


void
BreakpointsView::_Init()
{
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(fListView = BreakpointListView::Create(fTeam, this, this))
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.SetInsets(B_USE_SMALL_SPACING)
			.AddGlue()
			.Add(fRemoveBreakpointButton = new BButton("Remove"))
			.Add(fEditBreakpointButton = new BButton("Edit" B_UTF8_ELLIPSIS))
			.Add(fToggleBreakpointButton = new BButton("Toggle"))
		.End();

	fToggleBreakpointButton->SetMessage(new BMessage(MSG_ENABLE_BREAKPOINT));
	fRemoveBreakpointButton->SetMessage(new BMessage(MSG_CLEAR_BREAKPOINT));
	fEditBreakpointButton->SetMessage(
		new BMessage(MSG_SHOW_BREAKPOINT_EDIT_WINDOW));

	_UpdateButtons();
}


void
BreakpointsView::_UpdateButtons()
{
	AutoLocker<Team> teamLocker(fTeam);

	bool hasEnabled = false;
	bool hasDisabled = false;
	bool valid = false;

	for (int32 i = 0; i < fSelectedBreakpoints.CountItems(); i++) {
		BreakpointProxy* proxy = fSelectedBreakpoints.ItemAt(i);
		switch (proxy->Type()) {
			case BREAKPOINT_PROXY_TYPE_BREAKPOINT:
			{
				UserBreakpoint* breakpoint = proxy->GetBreakpoint();
				if (breakpoint->IsValid()) {
					valid = true;
					if (breakpoint->IsEnabled())
						hasEnabled = true;
					else
						hasDisabled = true;

				}
				break;
			}
			case BREAKPOINT_PROXY_TYPE_WATCHPOINT:
			{
				Watchpoint* watchpoint = proxy->GetWatchpoint();
				valid = true;
				if (watchpoint->IsEnabled())
					hasEnabled = true;
				else
					hasDisabled = true;
				break;
			}
			default:
				break;
		}
	}

	if (valid) {
		// only allow condition editing if we have a single
		// actual breakpoint selected.
		// TODO: allow using this to modify watchpoints as
		// well.
		if (fSelectedBreakpoints.CountItems() == 1
			&& fSelectedBreakpoints.ItemAt(0)->Type()
				== BREAKPOINT_PROXY_TYPE_BREAKPOINT) {
			fEditBreakpointButton->SetEnabled(true);
		} else
			fEditBreakpointButton->SetEnabled(false);

		// if we have at least one disabled breakpoint in the
		// selection, we leave the button as an Enable button
		if (hasEnabled && !hasDisabled) {
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
		fEditBreakpointButton->SetEnabled(false);
		fRemoveBreakpointButton->SetEnabled(false);
	}
}


void
BreakpointsView::_SetSelection(BreakpointProxyList& proxies)
{
	for (int32 i = 0; i < fSelectedBreakpoints.CountItems(); i++)
		fSelectedBreakpoints.ItemAt(i)->ReleaseReference();

	fSelectedBreakpoints.MakeEmpty();

	for (int32 i = 0; i < proxies.CountItems(); i++) {
		BreakpointProxy* proxy = proxies.ItemAt(i);
		if (!fSelectedBreakpoints.AddItem(proxy))
			return;
		proxy->AcquireReference();
	}

	_UpdateButtons();
}


void
BreakpointsView::_HandleBreakpointAction(uint32 action)
{
	if (fListener == NULL)
		return;

	for (int32 i = 0; i < fSelectedBreakpoints.CountItems(); i++) {
		BreakpointProxy* proxy = fSelectedBreakpoints.ItemAt(i);
		if (proxy->Type() == BREAKPOINT_PROXY_TYPE_BREAKPOINT) {
			UserBreakpoint* breakpoint = proxy->GetBreakpoint();
			if (action == MSG_ENABLE_BREAKPOINT && !breakpoint->IsEnabled())
				fListener->SetBreakpointEnabledRequested(breakpoint, true);
			else if (action == MSG_DISABLE_BREAKPOINT
				&& breakpoint->IsEnabled()) {
				fListener->SetBreakpointEnabledRequested(breakpoint, false);
			} else if (action == MSG_CLEAR_BREAKPOINT)
				fListener->ClearBreakpointRequested(breakpoint);
		} else {
			Watchpoint* watchpoint = proxy->GetWatchpoint();
			if (action == MSG_ENABLE_BREAKPOINT && !watchpoint->IsEnabled())
				fListener->SetWatchpointEnabledRequested(watchpoint, true);
			else if (action == MSG_DISABLE_BREAKPOINT
				&& watchpoint->IsEnabled()) {
				fListener->SetWatchpointEnabledRequested(watchpoint, false);
			} else if (action == MSG_CLEAR_BREAKPOINT)
				fListener->ClearWatchpointRequested(watchpoint);
		}
	}
}


// #pragma mark - Listener


BreakpointsView::Listener::~Listener()
{
}
