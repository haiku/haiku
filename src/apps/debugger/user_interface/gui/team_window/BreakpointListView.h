/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BREAKPOINT_LIST_VIEW_H
#define BREAKPOINT_LIST_VIEW_H


#include <GroupView.h>

#include "table/Table.h"


class Team;
class UserBreakpoint;


class BreakpointListView : public BGroupView, private TableListener {
public:
	class Listener;

public:
								BreakpointListView(Team* team,
									Listener* listener);
								~BreakpointListView();

	static	BreakpointListView*	Create(Team* team, Listener* listener);
									// throws

			void				UnsetListener();

			void				SetBreakpoint(UserBreakpoint* breakpoint);
			void				UserBreakpointChanged(
									UserBreakpoint* breakpoint);

			void				LoadSettings(const BMessage& settings);
			status_t			SaveSettings(BMessage& settings);

private:
			class BreakpointsTableModel;

private:
	// TableListener
	virtual	void				TableSelectionChanged(Table* table);

			void				_Init();

private:
			Team*				fTeam;
			UserBreakpoint*		fBreakpoint;
			Table*				fBreakpointsTable;
			BreakpointsTableModel* fBreakpointsTableModel;
			Listener*			fListener;
};


class BreakpointListView::Listener {
public:
	virtual						~Listener();

	virtual	void				BreakpointSelectionChanged(
									UserBreakpoint* breakpoint) = 0;
};


#endif	// BREAKPOINT_LIST_VIEW_H
