/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_TRACE_VIEW_H
#define STACK_TRACE_VIEW_H

#include <GroupView.h>

#include "table/Table.h"
#include "Team.h"


class StackFrame;


class StackTraceView : public BGroupView, private TableListener {
public:
	class Listener;

public:
								StackTraceView(Listener* listener);
								~StackTraceView();

	static	StackTraceView*		Create(Listener* listener);
									// throws

			void				UnsetListener();

			void				SetStackTrace(StackTrace* stackTrace);
			void				SetStackFrame(StackFrame* stackFrame);

			void				LoadSettings(const BMessage& settings);
			status_t			SaveSettings(BMessage& settings);

private:
			class FramesTableModel;

private:
	// TableListener
	virtual	void				TableSelectionChanged(Table* table);

			void				_Init();

private:
			StackTrace*			fStackTrace;
			Table*				fFramesTable;
			FramesTableModel*	fFramesTableModel;
			Listener*			fListener;
};


class StackTraceView::Listener {
public:
	virtual						~Listener();

	virtual	void				StackFrameSelectionChanged(
									StackFrame* frame) = 0;
};


#endif	// STACK_TRACE_VIEW_H
