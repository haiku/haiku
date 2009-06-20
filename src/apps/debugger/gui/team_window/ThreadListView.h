/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_LIST_VIEW_H
#define THREAD_LIST_VIEW_H

#include <GroupView.h>

#include "table/Table.h"
#include "Team.h"


class Thread;


class ThreadListView : public BGroupView, private Team::Listener,
	private TableListener {
public:
	class Listener;

public:
								ThreadListView(Listener* listener);
								~ThreadListView();

	static	ThreadListView*		Create(Listener* listener);
									// throws

			void				UnsetListener();

			void				SetTeam(Team* team);

	virtual	void				MessageReceived(BMessage* message);

private:
			class ThreadsTableModel;

private:
	// Team::Listener
	virtual	void				ThreadAdded(const Team::ThreadEvent& event);
	virtual	void				ThreadRemoved(const Team::ThreadEvent& event);

	// TableListener
	virtual	void				TableSelectionChanged(Table* table);

			void				_Init();

private:
			Team*				fTeam;
			Table*				fThreadsTable;
			ThreadsTableModel*	fThreadsTableModel;
			Listener*			fListener;
};


class ThreadListView::Listener {
public:
	virtual						~Listener();

	virtual	void				ThreadSelectionChanged(Thread* thread) = 0;
};


#endif	// THREAD_LIST_VIEW_H
