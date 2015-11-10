/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef VARIABLES_VIEW_H
#define VARIABLES_VIEW_H


#include <GroupView.h>
#include <util/OpenHashTable.h>

#include "table/TreeTable.h"

#include "ExpressionInfo.h"


class ActionMenuItem;
class CpuState;
class SettingsMenu;
class StackFrame;
class Thread;
class Type;
class TypeComponentPath;
class ValueLocation;
class ValueNode;
class ValueNodeChild;
class ValueNodeContainer;
class ValueNodeManager;
class Value;
class Variable;
class VariableEditWindow;
class VariablesViewState;
class VariablesViewStateHistory;


class VariablesView : public BGroupView, private TreeTableListener,
	private ExpressionInfo::Listener {
public:
	class Listener;

public:
								VariablesView(Listener* listener);
								~VariablesView();

	static	VariablesView*		Create(Listener* listener,
									ValueNodeManager* manager);
									// throws

			void				SetStackFrame(Thread* thread,
									StackFrame* stackFrame);

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				DetachedFromWindow();

			void				LoadSettings(const BMessage& settings);
			status_t			SaveSettings(BMessage& settings);

			void				SetStackFrameClearPending();

private:
	// TreeTableListener
	virtual	void				TreeTableNodeExpandedChanged(TreeTable* table,
									const TreeTablePath& path, bool expanded);
	virtual	void				TreeTableNodeInvoked(TreeTable* table,
									const TreeTablePath& path);
	virtual	void				TreeTableCellMouseDown(TreeTable* table,
									const TreeTablePath& path,
									int32 columnIndex, BPoint screenWhere,
									uint32 buttons);

	// ExpressionInfo::Listener
	virtual	void				ExpressionEvaluated(ExpressionInfo* info,
									status_t result, ExpressionResult* value);

private:
			class ContainerListener;
			class ExpressionVariableID;
			class ModelNode;
			class VariableValueColumn;
			class VariableTableModel;
			class ContextMenu;
			class TableCellContextMenuTracker;
			class VariablesExpressionInfo;
			typedef BObjectList<ActionMenuItem> ContextActionList;
			typedef BObjectList<ExpressionInfo> ExpressionInfoList;
			typedef BObjectList<ValueNodeChild> ExpressionChildList;

			struct FunctionKey;
			struct ExpressionInfoEntry;
			struct ExpressionInfoEntryHashDefinition;

			typedef BOpenHashTable<ExpressionInfoEntryHashDefinition>
				ExpressionInfoTable;

private:
			void				_Init(ValueNodeManager* manager);

			void				_RequestNodeValue(ModelNode* node);
			status_t			_GetContextActionsForNode(ModelNode* node,
									ContextActionList*& _preActions,
									ContextActionList*& _postActions);
			status_t			_AddContextAction(const char* action,
									uint32 what, ContextActionList* actions,
									BMessage*& _message);
			void				_FinishContextMenu(bool force);
			void				_SaveViewState(bool updateValues) const;
			void				_RestoreViewState();
			status_t			_AddViewStateDescendentNodeInfos(
									VariablesViewState* viewState,
									void* parent,
									TreeTablePath& path,
									bool updateValues) const;
			status_t			_ApplyViewStateDescendentNodeInfos(
									VariablesViewState* viewState,
									void* parent,
									TreeTablePath& path);
			void				_CopyVariableValueToClipboard();

			status_t			_AddExpression(const char* expression,
									bool persistentExpression,
									ExpressionInfo*& _info);
			void				_RemoveExpression(ModelNode* node);

			void				_RestoreExpressionNodes();

			void				_AddExpressionNode(ExpressionInfo* info,
									status_t result, ExpressionResult* value);

			void				_HandleTypecastResult(status_t result,
									ExpressionResult* value);

			void				_HandleEditVariableRequest(ModelNode* node,
									Value* value);

			status_t			_GetTypeForTypeCode(int32 typeCode,
									Type*& _resultType) const;

private:
			Thread*				fThread;
			StackFrame*			fStackFrame;
			TreeTable*			fVariableTable;
			VariableTableModel*	fVariableTableModel;
			ContainerListener*	fContainerListener;
			VariablesViewState*	fPreviousViewState;
			VariablesViewStateHistory* fViewStateHistory;
			ExpressionInfoTable* fExpressions;
			ExpressionChildList	fExpressionChildren;
			TableCellContextMenuTracker* fTableCellContextMenuTracker;
			VariablesExpressionInfo* fPendingTypecastInfo;
			ExpressionInfo*		fTemporaryExpression;
			bool				fFrameClearPending;
			VariableEditWindow*	fEditWindow;
			Listener*			fListener;
};


class VariablesView::Listener {
public:
	virtual						~Listener();

	virtual	void				ValueNodeValueRequested(CpuState* cpuState,
									ValueNodeContainer* container,
									ValueNode* valueNode) = 0;

	virtual	void				ExpressionEvaluationRequested(
									ExpressionInfo* info,
									StackFrame* frame,
									Thread* thread) = 0;

	virtual	void				ValueNodeWriteRequested(
									ValueNode* node,
									CpuState* state,
									Value* newValue) = 0;

};


#endif	// VARIABLES_VIEW_H
