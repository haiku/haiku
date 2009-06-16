/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_LIST_VIEW_H
#define THREAD_LIST_VIEW_H

#include <GroupView.h>

#include "table/Table.h"
#include "Team.h"


class ThreadListView : public BGroupView, private Team::Listener,
	private TableListener {
public:
								ThreadListView();
								~ThreadListView();

	static	ThreadListView*		Create();
									// throws

			void				SetTeam(Team* team);

	virtual	void				MessageReceived(BMessage* message);

private:
			class ThreadsTableModel;

private:
	// Team::Listener
	virtual	void				ThreadAdded(Team* team, Thread* thread);
	virtual	void				ThreadRemoved(Team* team, Thread* thread);

	// TableListener
	virtual	void				TableRowInvoked(Table* table, int32 rowIndex);

			void				_Init();

private:
			Team*				fTeam;
			Table*				fThreadsTable;
			ThreadsTableModel*	fThreadsTableModel;
};


#endif	// THREAD_LIST_VIEW_H
