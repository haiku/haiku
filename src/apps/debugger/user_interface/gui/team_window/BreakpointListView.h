/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef BREAKPOINT_LIST_VIEW_H
#define BREAKPOINT_LIST_VIEW_H


#include <GroupView.h>

#include "table/Table.h"


class Team;
class UserBreakpoint;
class Watchpoint;


enum breakpoint_proxy_type {
	BREAKPOINT_PROXY_TYPE_BREAKPOINT = 0,
	BREAKPOINT_PROXY_TYPE_WATCHPOINT = 1
};


class BreakpointProxy : public BReferenceable {
public:
								BreakpointProxy(UserBreakpoint* breakpoint,
									Watchpoint* watchpoint);
								~BreakpointProxy();

			breakpoint_proxy_type Type() const;

			UserBreakpoint*		GetBreakpoint() const { return fBreakpoint; }
			Watchpoint*			GetWatchpoint() const { return fWatchpoint; }

private:
			UserBreakpoint*		fBreakpoint;
			Watchpoint*			fWatchpoint;
};

typedef BObjectList<BreakpointProxy> BreakpointProxyList;


class BreakpointListView : public BGroupView, private TableListener {
public:
	class Listener;

public:
								BreakpointListView(Team* team,
									Listener* listener);
								~BreakpointListView();

	static	BreakpointListView*	Create(Team* team, Listener* listener,
									BView* filterTarget);
									// throws

			void				UnsetListener();

			void				UserBreakpointChanged(
									UserBreakpoint* breakpoint);
			void				WatchpointChanged(
									Watchpoint* breakpoint);

			void				LoadSettings(const BMessage& settings);
			status_t			SaveSettings(BMessage& settings);

private:
			class BreakpointsTableModel;
			class ListInputFilter;

private:
	// TableListener
	virtual	void				TableSelectionChanged(Table* table);

			void				_Init(BView* filterTarget);

private:
			Team*				fTeam;
			Table*				fBreakpointsTable;
			BreakpointsTableModel* fBreakpointsTableModel;
			Listener*			fListener;
};


class BreakpointListView::Listener {
public:
	virtual						~Listener();

	virtual	void				BreakpointSelectionChanged(
									BreakpointProxyList& breakpoints) = 0;
};


#endif	// BREAKPOINT_LIST_VIEW_H
