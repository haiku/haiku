/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VARIABLES_VIEW_H
#define VARIABLES_VIEW_H


#include <GroupView.h>

#include "table/TreeTable.h"


class CpuState;
class SettingsMenu;
class StackFrame;
class Thread;
class TypeComponentPath;
class ValueNode;
class ValueNodeContainer;
class Variable;
class VariablesViewState;
class VariablesViewStateHistory;


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

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				DetachedFromWindow();

private:
	// TreeTableListener
	virtual	void				TreeTableNodeExpandedChanged(TreeTable* table,
									const TreeTablePath& path, bool expanded);

	virtual	void				TreeTableCellMouseDown(TreeTable* table,
									const TreeTablePath& path,
									int32 columnIndex, BPoint screenWhere,
									uint32 buttons);

private:
			class ContainerListener;
			class ModelNode;
			class VariableValueColumn;
			class VariableTableModel;
			class ContextMenu;
			class TableCellContextMenuTracker;

private:
			void				_Init();

			void				_RequestNodeValue(ModelNode* node);
			void				_FinishContextMenu(bool force);

			void				_SaveViewState() const;
			void				_RestoreViewState();
			status_t			_AddViewStateDescendentNodeInfos(
									VariablesViewState* viewState, void* parent,
									TreeTablePath& path) const;
			status_t			_ApplyViewStateDescendentNodeInfos(
									VariablesViewState* viewState, void* parent,
									TreeTablePath& path);

private:
			Thread*				fThread;
			StackFrame*			fStackFrame;
			TreeTable*			fVariableTable;
			VariableTableModel*	fVariableTableModel;
			ContainerListener*	fContainerListener;
			VariablesViewState*	fPreviousViewState;
			VariablesViewStateHistory* fViewStateHistory;
			TableCellContextMenuTracker* fTableCellContextMenuTracker;
			Listener*			fListener;
};


class VariablesView::Listener {
public:
	virtual						~Listener();

	virtual	void				ValueNodeValueRequested(CpuState* cpuState,
									ValueNodeContainer* container,
									ValueNode* valueNode) = 0;
};


#endif	// VARIABLES_VIEW_H
