/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VARIABLES_VIEW_H
#define VARIABLES_VIEW_H


#include <GroupView.h>

#include "table/Table.h"


class StackFrame;
class Variable;


class VariablesView : public BGroupView, private TableListener {
public:
								VariablesView();
								~VariablesView();

	static	VariablesView*		Create();
									// throws

			void				SetStackFrame(StackFrame* stackFrame);

private:
			class VariableTableModel;

private:
			void				_Init();

private:
			StackFrame*			fStackFrame;
			Table*				fVariableTable;
			VariableTableModel*	fVariableTableModel;
};


#endif	// VARIABLES_VIEW_H
