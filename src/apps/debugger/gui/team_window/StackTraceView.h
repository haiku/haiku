/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_TRACE_VIEW_H
#define STACK_TRACE_VIEW_H

#include <GroupView.h>

#include "table/Table.h"
#include "Team.h"


class StackTraceView : public BGroupView, private TableListener {
public:
								StackTraceView();
								~StackTraceView();

	static	StackTraceView*		Create();
									// throws

			void				SetStackTrace(StackTrace* stackTrace);

private:
			class FramesTableModel;

private:
	// TableListener
	virtual	void				TableRowInvoked(Table* table, int32 rowIndex);

			void				_Init();

private:
			StackTrace*			fStackTrace;
			Table*				fFramesTable;
			FramesTableModel*	fFramesTableModel;
};


#endif	// STACK_TRACE_VIEW_H
