/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VARIABLES_VIEW_H
#define VARIABLES_VIEW_H


#include <GroupView.h>

#include "table/TreeTable.h"


class StackFrame;
class Thread;
class TypeComponentPath;
class Variable;


class VariablesView : public BGroupView, private TreeTableListener {
public:
	class Listener;

public:
								VariablesView(Listener* listener);
								~VariablesView();

	static	VariablesView*		Create(Listener* listener);
									// throws

			void				SetStackFrame(Thread* thread,
									StackFrame* stackFrame);
			void				StackFrameValueRetrieved(StackFrame* stackFrame,
									Variable* variable,
									TypeComponentPath* path);

private:
			class ValueNode;
			class VariableValueColumn;
			class VariableTableModel;

private:
			void				_Init();

			void				_RequestVariableValue(Variable* variable);

private:
			Thread*				fThread;
			StackFrame*			fStackFrame;
			TreeTable*			fVariableTable;
			VariableTableModel*	fVariableTableModel;
			Listener*			fListener;
};


class VariablesView::Listener {
public:
	virtual						~Listener();

	virtual	void				StackFrameValueRequested(Thread* thread,
									StackFrame* stackFrame, Variable* variable,
									TypeComponentPath* path) = 0;
									// called with team locked
};


#endif	// VARIABLES_VIEW_H
