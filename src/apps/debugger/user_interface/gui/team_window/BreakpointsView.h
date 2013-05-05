/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BREAKPOINTS_VIEW_H
#define BREAKPOINTS_VIEW_H


#include <GroupView.h>

#include "BreakpointListView.h"


class BButton;


class BreakpointsView : public BGroupView,
	private BreakpointListView::Listener {
public:
	class Listener;

public:
								BreakpointsView(Team* team, Listener* listener);
								~BreakpointsView();

	static	BreakpointsView*	Create(Team* team, Listener* listener);
									// throws

			void				UnsetListener();

			void				SetBreakpoint(UserBreakpoint* breakpoint,
									Watchpoint* watchpoint);

			void				UserBreakpointChanged(
									UserBreakpoint* breakpoint);
			void				WatchpointChanged(
									Watchpoint* watchpoint);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

			void				LoadSettings(const BMessage& settings);
			status_t			SaveSettings(BMessage& settings);

private:
	// BreakpointListView::Listener
	virtual	void				BreakpointSelectionChanged(
									UserBreakpoint* breakpoint);
	virtual	void				WatchpointSelectionChanged(
									Watchpoint* watchpoint);

			void				_Init();

			void				_UpdateButtons();

private:
			Team*				fTeam;
			UserBreakpoint*		fBreakpoint;
			Watchpoint*			fWatchpoint;
			BreakpointListView*	fListView;
			BButton*			fToggleBreakpointButton;
			BButton*			fRemoveBreakpointButton;
			Listener*			fListener;
};



class BreakpointsView::Listener {
public:
	virtual						~Listener();

	virtual	void				BreakpointSelectionChanged(
									UserBreakpoint* breakpoint) = 0;
	virtual	void				SetBreakpointEnabledRequested(
									UserBreakpoint* breakpoint,
									bool enabled) = 0;
	virtual	void				ClearBreakpointRequested(
									UserBreakpoint* breakpoint) = 0;

	virtual	void				WatchpointSelectionChanged(
									Watchpoint* Watchpoint) = 0;
	virtual	void				SetWatchpointEnabledRequested(
									Watchpoint* breakpoint,
									bool enabled) = 0;
	virtual	void				ClearWatchpointRequested(
									Watchpoint* watchpoint) = 0;
};


#endif	// BREAKPOINT_LIST_VIEW_H
