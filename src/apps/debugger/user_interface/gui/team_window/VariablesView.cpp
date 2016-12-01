/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "VariablesView.h"

#include <new>

#include <debugger.h>

#include <Alert.h>
#include <Clipboard.h>
#include <Looper.h>
#include <PopUpMenu.h>
#include <ToolTip.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <PromptWindow.h>

#include "table/TableColumns.h"

#include "ActionMenuItem.h"
#include "AppMessageCodes.h"
#include "Architecture.h"
#include "ExpressionInfo.h"
#include "ExpressionValues.h"
#include "FileSourceCode.h"
#include "Function.h"
#include "FunctionID.h"
#include "FunctionInstance.h"
#include "GuiSettingsUtils.h"
#include "MessageCodes.h"
#include "RangeList.h"
#include "Register.h"
#include "SettingsMenu.h"
#include "SourceLanguage.h"
#include "StackTrace.h"
#include "StackFrame.h"
#include "StackFrameValues.h"
#include "StringUtils.h"
#include "StringValue.h"
#include "SyntheticPrimitiveType.h"
#include "TableCellValueEditor.h"
#include "TableCellValueRenderer.h"
#include "Team.h"
#include "TeamDebugInfo.h"
#include "Thread.h"
#include "Tracing.h"
#include "TypeComponentPath.h"
#include "TypeHandlerRoster.h"
#include "TypeLookupConstraints.h"
#include "UiUtils.h"
#include "Value.h"
#include "ValueHandler.h"
#include "ValueHandlerRoster.h"
#include "ValueLocation.h"
#include "ValueNode.h"
#include "ValueNodeManager.h"
#include "Variable.h"
#include "VariableEditWindow.h"
#include "VariableValueNodeChild.h"
#include "VariablesViewState.h"
#include "VariablesViewStateHistory.h"


enum {
	VALUE_NODE_TYPE	= 'valn'
};


enum {
	MSG_MODEL_NODE_HIDDEN			= 'monh',
	MSG_VALUE_NODE_NEEDS_VALUE		= 'mvnv',
	MSG_RESTORE_PARTIAL_VIEW_STATE	= 'mpvs',
	MSG_ADD_WATCH_EXPRESSION		= 'awex',
	MSG_REMOVE_WATCH_EXPRESSION		= 'rwex'
};


// maximum number of array elements to show by default
static const uint64 kMaxArrayElementCount = 10;


// #pragma mark - FunctionKey


struct VariablesView::FunctionKey {
	FunctionID*			function;

	FunctionKey(FunctionID* function)
		:
		function(function)
	{
	}

	uint32 HashValue() const
	{
		return function->HashValue();
	}

	bool operator==(const FunctionKey& other) const
	{
		return *function == *other.function;
	}
};


// #pragma mark - ExpressionInfoEntry


struct VariablesView::ExpressionInfoEntry : FunctionKey, ExpressionInfoList {
	ExpressionInfoEntry* next;

	ExpressionInfoEntry(FunctionID* function)
		:
		FunctionKey(function),
		ExpressionInfoList(10, false)
	{
		function->AcquireReference();
	}

	~ExpressionInfoEntry()
	{
		_Cleanup();
	}

	void SetInfo(const ExpressionInfoList& infoList)
	{
		_Cleanup();

		for (int32 i = 0; i < infoList.CountItems(); i++) {
			ExpressionInfo* info = infoList.ItemAt(i);
			if (!AddItem(info))
				break;

			info->AcquireReference();
		}
	}

private:
	void _Cleanup()
	{
		for (int32 i = 0; i < CountItems(); i++)
			ItemAt(i)->ReleaseReference();

		MakeEmpty();
	}
};


// #pragma mark - ExpressionInfoEntryHashDefinition


struct VariablesView::ExpressionInfoEntryHashDefinition {
	typedef FunctionKey		KeyType;
	typedef	ExpressionInfoEntry	ValueType;

	size_t HashKey(const FunctionKey& key) const
	{
		return key.HashValue();
	}

	size_t Hash(const ExpressionInfoEntry* value) const
	{
		return value->HashValue();
	}

	bool Compare(const FunctionKey& key,
		const ExpressionInfoEntry* value) const
	{
		return key == *value;
	}

	ExpressionInfoEntry*& GetLink(ExpressionInfoEntry* value) const
	{
		return value->next;
	}
};


// #pragma mark - ContainerListener


class VariablesView::ContainerListener : public ValueNodeContainer::Listener {
public:
								ContainerListener(BHandler* indirectTarget);

			void				SetModel(VariableTableModel* model);

	virtual	void				ValueNodeChanged(ValueNodeChild* nodeChild,
									ValueNode* oldNode, ValueNode* newNode);
	virtual	void				ValueNodeChildrenCreated(ValueNode* node);
	virtual	void				ValueNodeChildrenDeleted(ValueNode* node);
	virtual	void				ValueNodeValueChanged(ValueNode* node);

	virtual void				ModelNodeHidden(ModelNode* node);

	virtual void				ModelNodeValueRequested(ModelNode* node);

	virtual void				ModelNodeRestoreViewStateRequested(ModelNode* node);

private:
			BHandler*			fIndirectTarget;
			VariableTableModel*	fModel;
};


// #pragma mark - ExpressionVariableID


class VariablesView::ExpressionVariableID : public ObjectID {
public:
	ExpressionVariableID(ExpressionInfo* info)
		:
		fInfo(info)
	{
		fInfo->AcquireReference();
	}

	virtual ~ExpressionVariableID()
	{
		fInfo->ReleaseReference();
	}

	virtual	bool operator==(const ObjectID& other) const
	{
		const ExpressionVariableID* otherID
			= dynamic_cast<const ExpressionVariableID*>(&other);
		if (otherID == NULL)
			return false;

		return fInfo == otherID->fInfo;
	}

protected:
	virtual	uint32 ComputeHashValue() const
	{
		uint32 hash = *(uint32*)(&fInfo);
		hash = hash * 19 + StringUtils::HashValue(fInfo->Expression());

		return hash;
	}

private:
	ExpressionInfo*	fInfo;
};


// #pragma mark - ModelNode


class VariablesView::ModelNode : public BReferenceable {
public:
	ModelNode(ModelNode* parent, Variable* variable, ValueNodeChild* nodeChild,
		bool isPresentationNode)
		:
		fParent(parent),
		fNodeChild(nodeChild),
		fVariable(variable),
		fValue(NULL),
		fPreviousValue(),
		fValueHandler(NULL),
		fTableCellRenderer(NULL),
		fLastRendererSettings(),
		fCastedType(NULL),
		fComponentPath(NULL),
		fIsPresentationNode(isPresentationNode),
		fHidden(false),
		fValueChanged(false),
		fPresentationName()
	{
		fVariable->AcquireReference();
		fNodeChild->AcquireReference();
	}

	~ModelNode()
	{
		SetTableCellRenderer(NULL);
		SetValueHandler(NULL);
		SetValue(NULL);

		for (int32 i = 0; ModelNode* child = fChildren.ItemAt(i); i++)
			child->ReleaseReference();

		fNodeChild->ReleaseReference();
		fVariable->ReleaseReference();

		if (fComponentPath != NULL)
			fComponentPath->ReleaseReference();

		if (fCastedType != NULL)
			fCastedType->ReleaseReference();
	}

	status_t Init()
	{
		fComponentPath = new(std::nothrow) TypeComponentPath();
		if (fComponentPath == NULL)
			return B_NO_MEMORY;

		if (fParent != NULL)
			*fComponentPath = *fParent->GetPath();

		TypeComponent component;
		// TODO: this should actually discriminate between different
		// classes of type component kinds
		component.SetToBaseType(fNodeChild->GetType()->Kind(),
			0, fNodeChild->Name());

		fComponentPath->AddComponent(component);

		return B_OK;
	}

	ModelNode* Parent() const
	{
		return fParent;
	}

	ValueNodeChild* NodeChild() const
	{
		return fNodeChild;
	}

	const BString& Name() const
	{
		return fPresentationName.IsEmpty()
			? fNodeChild->Name() : fPresentationName;
	}

	void SetPresentationName(const BString& name)
	{
		fPresentationName = name;
	}

	Type* GetType() const
	{
		if (fCastedType != NULL)
			return fCastedType;

		return fNodeChild->GetType();
	}

	Variable* GetVariable() const
	{
		return fVariable;
	}

	Value* GetValue() const
	{
		return fValue;
	}

	void SetValue(Value* value)
	{
		if (value == fValue)
			return;

		if (fValue != NULL)
			fValue->ReleaseReference();

		fValue = value;

		if (fValue != NULL)
			fValue->AcquireReference();

		_CompareValues();
	}

	const BVariant& PreviousValue() const
	{
		return fPreviousValue;
	}

	void SetPreviousValue(const BVariant& value)
	{
		fPreviousValue = value;
	}

	Type* GetCastedType() const
	{
		return fCastedType;
	}

	void SetCastedType(Type* type)
	{
		if (fCastedType != NULL)
			fCastedType->ReleaseReference();

		fCastedType = type;
		if (type != NULL)
			fCastedType->AcquireReference();
	}

	const BMessage& GetLastRendererSettings() const
	{
		return fLastRendererSettings;
	}

	void SetLastRendererSettings(const BMessage& settings)
	{
		fLastRendererSettings = settings;
	}

	TypeComponentPath* GetPath() const
	{
		return fComponentPath;
	}

	ValueHandler* GetValueHandler() const
	{
		return fValueHandler;
	}

	void SetValueHandler(ValueHandler* handler)
	{
		if (handler == fValueHandler)
			return;

		if (fValueHandler != NULL)
			fValueHandler->ReleaseReference();

		fValueHandler = handler;

		if (fValueHandler != NULL)
			fValueHandler->AcquireReference();
	}


	TableCellValueRenderer* TableCellRenderer() const
	{
		return fTableCellRenderer;
	}

	void SetTableCellRenderer(TableCellValueRenderer* renderer)
	{
		if (renderer == fTableCellRenderer)
			return;

		if (fTableCellRenderer != NULL)
			fTableCellRenderer->ReleaseReference();

		fTableCellRenderer = renderer;

		if (fTableCellRenderer != NULL)
			fTableCellRenderer->AcquireReference();
	}

	bool IsPresentationNode() const
	{
		return fIsPresentationNode;
	}

	bool IsHidden() const
	{
		return fHidden;
	}

	void SetHidden(bool hidden)
	{
		fHidden = hidden;
	}

	bool ValueChanged() const
	{
		return fValueChanged;
	}

	int32 CountChildren() const
	{
		return fChildren.CountItems();
	}

	ModelNode* ChildAt(int32 index) const
	{
		return fChildren.ItemAt(index);
	}

	int32 IndexOf(ModelNode* child) const
	{
		return fChildren.IndexOf(child);
	}

	bool AddChild(ModelNode* child)
	{
		if (!fChildren.AddItem(child))
			return false;

		child->AcquireReference();
		return true;
	}

	bool RemoveChild(ModelNode* child)
	{
		if (!fChildren.RemoveItem(child))
			return false;

		child->ReleaseReference();
		return true;
	}

	bool RemoveAllChildren()
	{
		for (int32 i = 0; i < fChildren.CountItems(); i++)
			RemoveChild(fChildren.ItemAt(i));

		return true;
	}

private:
	typedef BObjectList<ModelNode> ChildList;

private:
	void _CompareValues()
	{
		fValueChanged = false;
		if (fValue != NULL) {
			if (fPreviousValue.Type() != 0) {
				BVariant newValue;
				fValue->ToVariant(newValue);
				fValueChanged = (fPreviousValue != newValue);
			} else {
				// for expression variables, always consider the initial
				// value as changed, since their evaluation has just been
				// requested, and thus their initial value is by definition
				// new/of interest
				fValueChanged = dynamic_cast<ExpressionVariableID*>(
					fVariable->ID()) != NULL;
			}
		}
	}

private:
	ModelNode*				fParent;
	ValueNodeChild*			fNodeChild;
	Variable*				fVariable;
	Value*					fValue;
	BVariant				fPreviousValue;
	ValueHandler*			fValueHandler;
	TableCellValueRenderer*	fTableCellRenderer;
	BMessage				fLastRendererSettings;
	Type*					fCastedType;
	ChildList				fChildren;
	TypeComponentPath*		fComponentPath;
	bool					fIsPresentationNode;
	bool					fHidden;
	bool					fValueChanged;
	BString					fPresentationName;

public:
	ModelNode*				fNext;
};


// #pragma mark - VariablesExpressionInfo


class VariablesView::VariablesExpressionInfo : public ExpressionInfo {
public:
	VariablesExpressionInfo(const BString& expression, ModelNode* node)
		:
		ExpressionInfo(expression),
		fTargetNode(node)
	{
		fTargetNode->AcquireReference();
	}

	virtual ~VariablesExpressionInfo()
	{
		fTargetNode->ReleaseReference();
	}

	inline ModelNode* TargetNode() const
	{
		return fTargetNode;
	}

private:
	ModelNode* fTargetNode;
};


// #pragma mark - VariableValueColumn


class VariablesView::VariableValueColumn : public StringTableColumn {
public:
	VariableValueColumn(int32 modelIndex, const char* title, float width,
		float minWidth, float maxWidth, uint32 truncate = B_TRUNCATE_MIDDLE,
		alignment align = B_ALIGN_RIGHT)
		:
		StringTableColumn(modelIndex, title, width, minWidth, maxWidth,
			truncate, align)
	{
	}

protected:
	void DrawValue(const BVariant& value, BRect rect, BView* targetView)
	{
		// draw the node's value with the designated renderer
		if (value.Type() == VALUE_NODE_TYPE) {
			ModelNode* node = dynamic_cast<ModelNode*>(value.ToReferenceable());
			if (node != NULL && node->GetValue() != NULL
				&& node->TableCellRenderer() != NULL) {
				node->TableCellRenderer()->RenderValue(node->GetValue(),
					node->ValueChanged(), rect, targetView);
				return;
			}
		} else if (value.Type() == B_STRING_TYPE) {
			fField.SetString(value.ToString());
		} else {
			// fall back to drawing an empty string
			fField.SetString("");
		}
		fField.SetWidth(Width());
		fColumn.DrawField(&fField, rect, targetView);
	}

	float GetPreferredWidth(const BVariant& value, BView* targetView) const
	{
		// get the preferred width from the node's designated renderer
		if (value.Type() == VALUE_NODE_TYPE) {
			ModelNode* node = dynamic_cast<ModelNode*>(value.ToReferenceable());
			if (node != NULL && node->GetValue() != NULL
				&& node->TableCellRenderer() != NULL) {
				return node->TableCellRenderer()->PreferredValueWidth(
					node->GetValue(), targetView);
			}
		}

		return fColumn.BTitledColumn::GetPreferredWidth(NULL, targetView);
	}

	virtual BField* PrepareField(const BVariant& _value) const
	{
		return NULL;
	}
};


// #pragma mark - VariableTableModel


class VariablesView::VariableTableModel : public TreeTableModel,
	public TreeTableToolTipProvider {
public:
								VariableTableModel(ValueNodeManager* manager);
								~VariableTableModel();

			status_t			Init();

			void				SetContainerListener(
									ContainerListener* listener);

			void				SetStackFrame(::Thread* thread,
									StackFrame* stackFrame);

			void				ValueNodeChanged(ValueNodeChild* nodeChild,
									ValueNode* oldNode, ValueNode* newNode);
			void				ValueNodeChildrenCreated(ValueNode* node);
			void				ValueNodeChildrenDeleted(ValueNode* node);
			void				ValueNodeValueChanged(ValueNode* node);

	virtual	int32				CountColumns() const;
	virtual	void*				Root() const;
	virtual	int32				CountChildren(void* parent) const;
	virtual	void*				ChildAt(void* parent, int32 index) const;
	virtual	bool				GetValueAt(void* object, int32 columnIndex,
									BVariant& _value);

			bool				GetTreePath(ModelNode* node,
									TreeTablePath& _path) const;

			void				NodeExpanded(ModelNode* node);

			void				NotifyNodeChanged(ModelNode* node);
			void				NotifyNodeHidden(ModelNode* node);

	virtual	bool				GetToolTipForTablePath(
									const TreeTablePath& path,
									int32 columnIndex, BToolTip** _tip);

			status_t			AddSyntheticNode(Variable* variable,
									ValueNodeChild*& _child,
									const char* presentationName = NULL);
			void				RemoveSyntheticNode(ModelNode* node);

private:
			struct NodeHashDefinition {
				typedef ValueNodeChild*	KeyType;
				typedef	ModelNode		ValueType;

				size_t HashKey(const ValueNodeChild* key) const
				{
					return (size_t)key;
				}

				size_t Hash(const ModelNode* value) const
				{
					return HashKey(value->NodeChild());
				}

				bool Compare(const ValueNodeChild* key,
					const ModelNode* value) const
				{
					return value->NodeChild() == key;
				}

				ModelNode*& GetLink(ModelNode* value) const
				{
					return value->fNext;
				}
			};

			typedef BObjectList<ModelNode> NodeList;
			typedef BOpenHashTable<NodeHashDefinition> NodeTable;

private:
			// container must be locked

			status_t			_AddNode(Variable* variable, ModelNode* parent,
									ValueNodeChild* nodeChild,
									bool isPresentationNode = false,
									bool isOnlyChild = false);

private:
			::Thread*			fThread;
			ValueNodeManager*	fNodeManager;
			ContainerListener*	fContainerListener;
			NodeList			fNodes;
			NodeTable			fNodeTable;
};


class VariablesView::ContextMenu : public BPopUpMenu {
public:
	ContextMenu(const BMessenger& parent, const char* name)
		: BPopUpMenu(name, false, false),
		  fParent(parent)
	{
	}

	virtual void Hide()
	{
		BPopUpMenu::Hide();

		BMessage message(MSG_VARIABLES_VIEW_CONTEXT_MENU_DONE);
		message.AddPointer("menu", this);
		fParent.SendMessage(&message);
	}

private:
	BMessenger	fParent;
};


// #pragma mark - TableCellContextMenuTracker


class VariablesView::TableCellContextMenuTracker : public BReferenceable,
	Settings::Listener {
public:
	TableCellContextMenuTracker(ModelNode* node, BLooper* parentLooper,
		const BMessenger& parent)
		:
		fNode(node),
		fParentLooper(parentLooper),
		fParent(parent),
		fRendererSettings(NULL),
		fRendererSettingsMenu(NULL),
		fRendererMenuAdded(false),
		fMenuPreparedToShow(false)
	{
		fNode->AcquireReference();
	}

	~TableCellContextMenuTracker()
	{
		FinishMenu(true);

		if (fRendererSettingsMenu != NULL)
			fRendererSettingsMenu->ReleaseReference();

		if (fRendererSettings != NULL)
			fRendererSettings->ReleaseReference();

		fNode->ReleaseReference();
	}

	status_t Init(Settings* rendererSettings,
		SettingsMenu* rendererSettingsMenu,
		ContextActionList* preSettingsActions = NULL,
		ContextActionList* postSettingsActions = NULL)
	{
		if (rendererSettings == NULL && preSettingsActions == NULL
			&& postSettingsActions == NULL) {
			return B_BAD_VALUE;
		}

		if (rendererSettings != NULL) {
			fRendererSettings = rendererSettings;
			fRendererSettings->AcquireReference();


			fRendererSettingsMenu = rendererSettingsMenu;
			fRendererSettingsMenu->AcquireReference();
		}

		fContextMenu = new(std::nothrow) ContextMenu(fParent,
			"table cell settings popup");
		if (fContextMenu == NULL)
			return B_NO_MEMORY;

		status_t error = B_OK;
		if (preSettingsActions != NULL
			&& preSettingsActions->CountItems() > 0) {
			error = _AddActionItems(preSettingsActions);
			if (error != B_OK)
				return error;

			if (fRendererSettingsMenu != NULL || postSettingsActions != NULL)
				fContextMenu->AddSeparatorItem();
		}

		if (fRendererSettingsMenu != NULL) {
			error = fRendererSettingsMenu->AddToMenu(fContextMenu,
				fContextMenu->CountItems());
			if (error != B_OK)
				return error;

			if (postSettingsActions != NULL)
				fContextMenu->AddSeparatorItem();
		}

		if (postSettingsActions != NULL) {
			error = _AddActionItems(postSettingsActions);
			if (error != B_OK)
				return error;

		}

		if (fRendererSettings != NULL) {
			AutoLocker<Settings> settingsLocker(fRendererSettings);
			fRendererSettings->AddListener(this);
			fRendererMenuAdded = true;
		}

		return B_OK;
	}

	void ShowMenu(BPoint screenWhere)
	{
		if (fRendererMenuAdded)
			fRendererSettingsMenu->PrepareToShow(fParentLooper);

		for (int32 i = 0; i < fContextMenu->CountItems(); i++) {
			ActionMenuItem* item = dynamic_cast<ActionMenuItem*>(
				fContextMenu->ItemAt(i));
			if (item != NULL)
				item->PrepareToShow(fParentLooper, fParent.Target(NULL));
		}

		fMenuPreparedToShow = true;

		BRect mouseRect(screenWhere, screenWhere);
		mouseRect.InsetBy(-4.0, -4.0);
		fContextMenu->Go(screenWhere, true, false, mouseRect, true);
	}

	bool FinishMenu(bool force)
	{
		bool stillActive = false;

		if (fMenuPreparedToShow) {
			if (fRendererMenuAdded)
				stillActive = fRendererSettingsMenu->Finish(fParentLooper,
					force);
			for (int32 i = 0; i < fContextMenu->CountItems(); i++) {
				ActionMenuItem* item = dynamic_cast<ActionMenuItem*>(
					fContextMenu->ItemAt(i));
				if (item != NULL) {
					stillActive |= item->Finish(fParentLooper,
						fParent.Target(NULL), force);
				}
			}

			fMenuPreparedToShow = stillActive;
		}

		if (fRendererMenuAdded) {
			fRendererSettingsMenu->RemoveFromMenu();
			fRendererSettings->RemoveListener(this);
			fRendererMenuAdded = false;
		}

		if (fContextMenu != NULL) {
			delete fContextMenu;
			fContextMenu = NULL;
		}

		return stillActive;
	}

private:
	// Settings::Listener

	virtual void SettingValueChanged(Setting* setting)
	{
		BMessage message(MSG_VARIABLES_VIEW_NODE_SETTINGS_CHANGED);
		fNode->AcquireReference();
		if (message.AddPointer("node", fNode) != B_OK
			|| fParent.SendMessage(&message) != B_OK) {
			fNode->ReleaseReference();
		}
	}

	status_t _AddActionItems(ContextActionList* actions)
	{
		if (fContextMenu == NULL)
			return B_BAD_VALUE;

		int32 index = fContextMenu->CountItems();
		for (int32 i = 0; ActionMenuItem* item = actions->ItemAt(i); i++) {
			if (!fContextMenu->AddItem(item, index + i)) {
				for (i--; i >= 0; i--)
					fContextMenu->RemoveItem(fContextMenu->ItemAt(index + i));

				return B_NO_MEMORY;
			}
		}

		return B_OK;
	}

private:
	ModelNode*		fNode;
	BLooper*		fParentLooper;
	BMessenger		fParent;
	ContextMenu*	fContextMenu;
	Settings*		fRendererSettings;
	SettingsMenu*	fRendererSettingsMenu;
	bool			fRendererMenuAdded;
	bool			fMenuPreparedToShow;
};


// #pragma mark - ContainerListener


VariablesView::ContainerListener::ContainerListener(BHandler* indirectTarget)
	:
	fIndirectTarget(indirectTarget),
	fModel(NULL)
{
}


void
VariablesView::ContainerListener::SetModel(VariableTableModel* model)
{
	fModel = model;
}


void
VariablesView::ContainerListener::ValueNodeChanged(ValueNodeChild* nodeChild,
	ValueNode* oldNode, ValueNode* newNode)
{
	// If the looper is already locked, invoke the model's hook synchronously.
	if (fIndirectTarget->Looper()->IsLocked()) {
		fModel->ValueNodeChanged(nodeChild, oldNode, newNode);
		return;
	}

	// looper not locked yet -- call asynchronously to avoid reverse locking
	// order
	BReference<ValueNodeChild> nodeChildReference(nodeChild);
	BReference<ValueNode> oldNodeReference(oldNode);
	BReference<ValueNode> newNodeReference(newNode);

	BMessage message(MSG_VALUE_NODE_CHANGED);
	if (message.AddPointer("nodeChild", nodeChild) == B_OK
		&& message.AddPointer("oldNode", oldNode) == B_OK
		&& message.AddPointer("newNode", newNode) == B_OK
		&& fIndirectTarget->Looper()->PostMessage(&message, fIndirectTarget)
			== B_OK) {
		nodeChildReference.Detach();
		oldNodeReference.Detach();
		newNodeReference.Detach();
	}
}


void
VariablesView::ContainerListener::ValueNodeChildrenCreated(ValueNode* node)
{
	// If the looper is already locked, invoke the model's hook synchronously.
	if (fIndirectTarget->Looper()->IsLocked()) {
		fModel->ValueNodeChildrenCreated(node);
		return;
	}

	// looper not locked yet -- call asynchronously to avoid reverse locking
	// order
	BReference<ValueNode> nodeReference(node);

	BMessage message(MSG_VALUE_NODE_CHILDREN_CREATED);
	if (message.AddPointer("node", node) == B_OK
		&& fIndirectTarget->Looper()->PostMessage(&message, fIndirectTarget)
			== B_OK) {
		nodeReference.Detach();
	}
}


void
VariablesView::ContainerListener::ValueNodeChildrenDeleted(ValueNode* node)
{
	// If the looper is already locked, invoke the model's hook synchronously.
	if (fIndirectTarget->Looper()->IsLocked()) {
		fModel->ValueNodeChildrenDeleted(node);
		return;
	}

	// looper not locked yet -- call asynchronously to avoid reverse locking
	// order
	BReference<ValueNode> nodeReference(node);

	BMessage message(MSG_VALUE_NODE_CHILDREN_DELETED);
	if (message.AddPointer("node", node) == B_OK
		&& fIndirectTarget->Looper()->PostMessage(&message, fIndirectTarget)
			== B_OK) {
		nodeReference.Detach();
	}
}


void
VariablesView::ContainerListener::ValueNodeValueChanged(ValueNode* node)
{
	// If the looper is already locked, invoke the model's hook synchronously.
	if (fIndirectTarget->Looper()->IsLocked()) {
		fModel->ValueNodeValueChanged(node);
		return;
	}

	// looper not locked yet -- call asynchronously to avoid reverse locking
	// order
	BReference<ValueNode> nodeReference(node);

	BMessage message(MSG_VALUE_NODE_VALUE_CHANGED);
	if (message.AddPointer("node", node) == B_OK
		&& fIndirectTarget->Looper()->PostMessage(&message, fIndirectTarget)
			== B_OK) {
		nodeReference.Detach();
	}
}


void
VariablesView::ContainerListener::ModelNodeHidden(ModelNode* node)
{
	BReference<ModelNode> nodeReference(node);

	BMessage message(MSG_MODEL_NODE_HIDDEN);
	if (message.AddPointer("node", node) == B_OK
		&& fIndirectTarget->Looper()->PostMessage(&message, fIndirectTarget)
			== B_OK) {
		nodeReference.Detach();
	}
}


void
VariablesView::ContainerListener::ModelNodeValueRequested(ModelNode* node)
{
	BReference<ModelNode> nodeReference(node);

	BMessage message(MSG_VALUE_NODE_NEEDS_VALUE);
	if (message.AddPointer("node", node) == B_OK
		&& fIndirectTarget->Looper()->PostMessage(&message, fIndirectTarget)
			== B_OK) {
		nodeReference.Detach();
	}
}


void
VariablesView::ContainerListener::ModelNodeRestoreViewStateRequested(
	ModelNode* node)
{
	BReference<ModelNode> nodeReference(node);

	BMessage message(MSG_RESTORE_PARTIAL_VIEW_STATE);
	if (message.AddPointer("node", node) == B_OK
		&& fIndirectTarget->Looper()->PostMessage(&message, fIndirectTarget)
			== B_OK) {
		nodeReference.Detach();
	}
}


// #pragma mark - VariableTableModel


VariablesView::VariableTableModel::VariableTableModel(
	ValueNodeManager* manager)
	:
	fThread(NULL),
	fNodeManager(manager),
	fContainerListener(NULL),
	fNodeTable()
{
	fNodeManager->AcquireReference();
}


VariablesView::VariableTableModel::~VariableTableModel()
{
	fNodeManager->ReleaseReference();
}


status_t
VariablesView::VariableTableModel::Init()
{
	return fNodeTable.Init();
}


void
VariablesView::VariableTableModel::SetContainerListener(
	ContainerListener* listener)
{
	if (listener == fContainerListener)
		return;

	if (fContainerListener != NULL) {
		if (fNodeManager != NULL)
			fNodeManager->RemoveListener(fContainerListener);

		fContainerListener->SetModel(NULL);
	}

	fContainerListener = listener;

	if (fContainerListener != NULL) {
		fContainerListener->SetModel(this);

		if (fNodeManager != NULL)
			fNodeManager->AddListener(fContainerListener);
	}
}


void
VariablesView::VariableTableModel::SetStackFrame(::Thread* thread,
	StackFrame* stackFrame)
{
	fThread = thread;

	fNodeManager->SetStackFrame(thread, stackFrame);

	int32 count = fNodes.CountItems();
	fNodeTable.Clear(true);

	if (!fNodes.IsEmpty()) {
		for (int32 i = 0; i < count; i++)
			fNodes.ItemAt(i)->ReleaseReference();
		fNodes.MakeEmpty();
	}

	NotifyNodesRemoved(TreeTablePath(), 0, count);

	if (stackFrame == NULL)
		return;

	ValueNodeContainer* container = fNodeManager->GetContainer();
	AutoLocker<ValueNodeContainer> containerLocker(container);

	for (int32 i = 0; i < container->CountChildren(); i++) {
		VariableValueNodeChild* child = dynamic_cast<VariableValueNodeChild *>(
			container->ChildAt(i));
		_AddNode(child->GetVariable(), NULL, child);
		// top level nodes get their children added immediately
		// so those won't invoke our callback hook. Add them directly here.
		ValueNodeChildrenCreated(child->Node());
	}
}


void
VariablesView::VariableTableModel::ValueNodeChanged(ValueNodeChild* nodeChild,
	ValueNode* oldNode, ValueNode* newNode)
{
	AutoLocker<ValueNodeContainer> containerLocker(
		fNodeManager->GetContainer());
	ModelNode* modelNode = fNodeTable.Lookup(nodeChild);
	if (modelNode == NULL)
		return;

	if (oldNode != NULL) {
		ValueNodeChildrenDeleted(oldNode);
		NotifyNodeChanged(modelNode);
	}
}


void
VariablesView::VariableTableModel::ValueNodeChildrenCreated(
	ValueNode* valueNode)
{
	AutoLocker<ValueNodeContainer> containerLocker(
		fNodeManager->GetContainer());

	// check whether we know the node
	ValueNodeChild* nodeChild = valueNode->NodeChild();
	if (nodeChild == NULL)
		return;

	ModelNode* modelNode = fNodeTable.Lookup(nodeChild);
	if (modelNode == NULL)
		return;

	// Iterate through the children and create model nodes for the ones we
	// don't know yet.
	int32 childCount = valueNode->CountChildren();
	for (int32 i = 0; i < childCount; i++) {
		ValueNodeChild* child = valueNode->ChildAt(i);
		if (fNodeTable.Lookup(child) == NULL) {
			_AddNode(modelNode->GetVariable(), modelNode, child,
				child->IsInternal(), childCount == 1);
		}

		ModelNode* childNode = fNodeTable.Lookup(child);
		if (childNode != NULL)
			fContainerListener->ModelNodeValueRequested(childNode);
	}

	if (valueNode->ChildCreationNeedsValue())
		fContainerListener->ModelNodeRestoreViewStateRequested(modelNode);
}


void
VariablesView::VariableTableModel::ValueNodeChildrenDeleted(ValueNode* node)
{
	AutoLocker<ValueNodeContainer> containerLocker(
		fNodeManager->GetContainer());

	// check whether we know the node
	ValueNodeChild* nodeChild = node->NodeChild();
	if (nodeChild == NULL)
		return;

	ModelNode* modelNode = fNodeTable.Lookup(nodeChild);
	if (modelNode == NULL)
		return;

	// in the case of an address node with a hidden child,
	// we want to send removal notifications for the children
	// instead.
	BReference<ModelNode> hiddenChild;
	if (modelNode->CountChildren() == 1
		&& modelNode->ChildAt(0)->IsHidden()) {
		hiddenChild.SetTo(modelNode->ChildAt(0));
		modelNode->RemoveChild(hiddenChild);
		modelNode = hiddenChild;
		fNodeTable.Remove(hiddenChild);
	}

	for (int32 i = modelNode->CountChildren() - 1; i >= 0 ; i--) {
		BReference<ModelNode> childNode = modelNode->ChildAt(i);
		// recursively remove the current node's child hierarchy.
		if (childNode->CountChildren() != 0)
			ValueNodeChildrenDeleted(childNode->NodeChild()->Node());

		TreeTablePath treePath;
		if (GetTreePath(childNode, treePath)) {
			int32 index = treePath.RemoveLastComponent();
			NotifyNodesRemoved(treePath, index, 1);
		}
		modelNode->RemoveChild(childNode);
		fNodeTable.Remove(childNode);
	}
}


void
VariablesView::VariableTableModel::ValueNodeValueChanged(ValueNode* valueNode)
{
	AutoLocker<ValueNodeContainer> containerLocker(
		fNodeManager->GetContainer());

	// check whether we know the node
	ValueNodeChild* nodeChild = valueNode->NodeChild();
	if (nodeChild == NULL)
		return;

	ModelNode* modelNode = fNodeTable.Lookup(nodeChild);
	if (modelNode == NULL)
		return;

	// check whether the value actually changed
	Value* value = valueNode->GetValue();
	if (value == modelNode->GetValue())
		return;

	// get a value handler
	ValueHandler* valueHandler;
	status_t error = ValueHandlerRoster::Default()->FindValueHandler(value,
		valueHandler);
	if (error != B_OK)
		return;
	BReference<ValueHandler> handlerReference(valueHandler, true);

	// create a table cell renderer for the value
	TableCellValueRenderer* renderer = NULL;
	error = valueHandler->GetTableCellValueRenderer(value, renderer);
	if (error != B_OK)
		return;

	// set value/handler/renderer
	modelNode->SetValue(value);
	modelNode->SetValueHandler(valueHandler);
	modelNode->SetTableCellRenderer(renderer);

	// we have to restore renderer settings here since until this point
	// we don't yet know what renderer is in use.
	if (renderer != NULL) {
		Settings* settings = renderer->GetSettings();
		if (settings != NULL)
			settings->RestoreValues(modelNode->GetLastRendererSettings());
	}



	// notify table model listeners
	NotifyNodeChanged(modelNode);
}


int32
VariablesView::VariableTableModel::CountColumns() const
{
	return 3;
}


void*
VariablesView::VariableTableModel::Root() const
{
	return (void*)this;
}


int32
VariablesView::VariableTableModel::CountChildren(void* parent) const
{
	if (parent == this)
		return fNodes.CountItems();

	// If the node only has a hidden child, pretend the node directly has the
	// child's children.
	ModelNode* modelNode = (ModelNode*)parent;
	int32 childCount = modelNode->CountChildren();
	if (childCount == 1) {
		ModelNode* child = modelNode->ChildAt(0);
		if (child->IsHidden())
			return child->CountChildren();
	}

	return childCount;
}


void*
VariablesView::VariableTableModel::ChildAt(void* parent, int32 index) const
{
	if (parent == this)
		return fNodes.ItemAt(index);

	// If the node only has a hidden child, pretend the node directly has the
	// child's children.
	ModelNode* modelNode = (ModelNode*)parent;
	int32 childCount = modelNode->CountChildren();
	if (childCount == 1) {
		ModelNode* child = modelNode->ChildAt(0);
		if (child->IsHidden())
			return child->ChildAt(index);
	}

	return modelNode->ChildAt(index);
}


bool
VariablesView::VariableTableModel::GetValueAt(void* object, int32 columnIndex,
	BVariant& _value)
{
	ModelNode* node = (ModelNode*)object;

	switch (columnIndex) {
		case 0:
			_value.SetTo(node->Name(), B_VARIANT_DONT_COPY_DATA);
			return true;
		case 1:
			if (node->GetValue() == NULL) {
				ValueLocation* location = node->NodeChild()->Location();
				if (location == NULL)
					return false;

				ValueNode* childNode = node->NodeChild()->Node();
				if (childNode == NULL)
					return false;

				Type* nodeChildRawType = childNode->GetType()->ResolveRawType(
					false);
				if (nodeChildRawType->Kind() == TYPE_COMPOUND)
				{
					if (location->CountPieces() > 1)
						return false;

					BString data;
					ValuePieceLocation piece = location->PieceAt(0);
					if (piece.type != VALUE_PIECE_LOCATION_MEMORY)
						return false;

					data.SetToFormat("[@ %#" B_PRIx64 "]", piece.address);
					_value.SetTo(data);
					return true;
				}
				return false;
			}

			_value.SetTo(node, VALUE_NODE_TYPE);
			return true;
		case 2:
		{
			// use the type of the underlying value node, as it may
			// be different from the initially assigned top level type
			// due to casting
			ValueNode* childNode = node->NodeChild()->Node();
			if (childNode == NULL)
				return false;

			Type* type = childNode->GetType();
			if (type == NULL)
				return false;

			_value.SetTo(type->Name(), B_VARIANT_DONT_COPY_DATA);
			return true;
		}
		default:
			return false;
	}
}


void
VariablesView::VariableTableModel::NodeExpanded(ModelNode* node)
{
	AutoLocker<ValueNodeContainer> containerLocker(
		fNodeManager->GetContainer());
	// add children of all children

	// If the node only has a hidden child, add the child's children instead.
	if (node->CountChildren() == 1) {
		ModelNode* child = node->ChildAt(0);
		if (child->IsHidden())
			node = child;
	}

	// add the children
	for (int32 i = 0; ModelNode* child = node->ChildAt(i); i++)
		fNodeManager->AddChildNodes(child->NodeChild());
}


void
VariablesView::VariableTableModel::NotifyNodeChanged(ModelNode* node)
{
	if (!node->IsHidden()) {
		TreeTablePath treePath;
		if (GetTreePath(node, treePath)) {
			int32 index = treePath.RemoveLastComponent();
			NotifyNodesChanged(treePath, index, 1);
		}
	}
}


void
VariablesView::VariableTableModel::NotifyNodeHidden(ModelNode* node)
{
	fContainerListener->ModelNodeHidden(node);
}


bool
VariablesView::VariableTableModel::GetToolTipForTablePath(
	const TreeTablePath& path, int32 columnIndex, BToolTip** _tip)
{
	ModelNode* node = (ModelNode*)NodeForPath(path);
	if (node == NULL)
		return false;

	BString tipData;
	ValueNodeChild* child = node->NodeChild();
	status_t error = child->LocationResolutionState();
	if (error != B_OK)
		tipData.SetToFormat("Unable to resolve location: %s", strerror(error));
	else {
		ValueNode* valueNode = child->Node();
		if (valueNode == NULL)
			return false;
		error = valueNode->LocationAndValueResolutionState();
		if (error != B_OK) {
			tipData.SetToFormat("Unable to resolve value: %s\n\n",
				strerror(error));
		}

		switch (columnIndex) {
			case 0:
			{
				ValueLocation* location = child->Location();
				for (int32 i = 0; i < location->CountPieces(); i++) {
					ValuePieceLocation piece = location->PieceAt(i);
					BString pieceData;
					switch (piece.type) {
						case VALUE_PIECE_LOCATION_MEMORY:
							pieceData.SetToFormat("(%" B_PRId32 "): Address: "
								"%#" B_PRIx64 ", Size: %" B_PRId64 " bytes\n",
								i, piece.address, piece.size);
							break;
						case VALUE_PIECE_LOCATION_REGISTER:
						{
							Architecture* architecture = fThread->GetTeam()
								->GetArchitecture();
							pieceData.SetToFormat("(%" B_PRId32 "): Register "
								"(%s)\n", i,
								architecture->Registers()[piece.reg].Name());
							break;
						}
						default:
							break;
					}

					tipData	+= pieceData;
				}
				tipData += "Editable: ";
				tipData += error == B_OK && location->IsWritable()
					? "Yes" : "No";
				break;
			}
			case 1:
			{
				Value* value = node->GetValue();
				if (value != NULL)
					value->ToString(tipData);

				break;
			}
			default:
				break;
		}
	}

	if (tipData.IsEmpty())
		return false;

	*_tip = new(std::nothrow) BTextToolTip(tipData);
	if (*_tip == NULL)
		return false;

	return true;
}


status_t
VariablesView::VariableTableModel::AddSyntheticNode(Variable* variable,
	ValueNodeChild*& _child, const char* presentationName)
{
	ValueNodeContainer* container = fNodeManager->GetContainer();
	AutoLocker<ValueNodeContainer> containerLocker(container);

	status_t error;
	if (_child == NULL) {
		_child = new(std::nothrow) VariableValueNodeChild(variable);
		if (_child == NULL)
			return B_NO_MEMORY;

		BReference<ValueNodeChild> childReference(_child, true);
		ValueNode* valueNode;
		if (_child->IsInternal())
			error = _child->CreateInternalNode(valueNode);
		else {
			error = TypeHandlerRoster::Default()->CreateValueNode(_child,
				_child->GetType(), valueNode);
		}

		if (error != B_OK)
			return error;

		_child->SetNode(valueNode);
		valueNode->ReleaseReference();
	}

	container->AddChild(_child);

	error = _AddNode(variable, NULL, _child);
	if (error != B_OK) {
		container->RemoveChild(_child);
		return error;
	}

	// since we're injecting these nodes synthetically,
	// we have to manually ask the node manager to create any
	// applicable children; this would normally be done implicitly
	// for top level nodes, as they're added from the parameters/locals,
	// but not here.
	fNodeManager->AddChildNodes(_child);

	ModelNode* childNode = fNodeTable.Lookup(_child);
	if (childNode != NULL) {
		if (presentationName != NULL)
			childNode->SetPresentationName(presentationName);

		ValueNode* valueNode = _child->Node();
		if (valueNode->LocationAndValueResolutionState()
			== VALUE_NODE_UNRESOLVED) {
			fContainerListener->ModelNodeValueRequested(childNode);
		} else
			ValueNodeValueChanged(valueNode);
	}
	ValueNodeChildrenCreated(_child->Node());

	return B_OK;
}


void
VariablesView::VariableTableModel::RemoveSyntheticNode(ModelNode* node)
{
	int32 index = fNodes.IndexOf(node);
	if (index < 0)
		return;

	fNodeTable.Remove(node);

	fNodes.RemoveItemAt(index);

	NotifyNodesRemoved(TreeTablePath(), index, 1);

	node->ReleaseReference();
}


status_t
VariablesView::VariableTableModel::_AddNode(Variable* variable,
	ModelNode* parent, ValueNodeChild* nodeChild, bool isPresentationNode,
	bool isOnlyChild)
{
	// Don't create nodes for unspecified types -- we can't get/show their
	// value anyway.
	Type* nodeChildRawType = nodeChild->GetType()->ResolveRawType(false);
	if (nodeChildRawType->Kind() == TYPE_UNSPECIFIED)
		return B_OK;

	ModelNode* node = new(std::nothrow) ModelNode(parent, variable, nodeChild,
		isPresentationNode);
	BReference<ModelNode> nodeReference(node, true);
	if (node == NULL || node->Init() != B_OK)
		return B_NO_MEMORY;

	int32 childIndex;

	if (parent != NULL) {
		childIndex = parent->CountChildren();

		if (!parent->AddChild(node))
			return B_NO_MEMORY;
		// the parent has a reference, now
	} else {
		childIndex = fNodes.CountItems();

		if (!fNodes.AddItem(node))
			return B_NO_MEMORY;
		nodeReference.Detach();
			// the fNodes list has a reference, now
	}

	fNodeTable.Insert(node);

	// if an address type node has only a single child, and that child
	// is a compound type, mark it hidden
	if (isOnlyChild && parent != NULL) {
		ValueNode* parentValueNode = parent->NodeChild()->Node();
		if (parentValueNode != NULL) {
			if (parentValueNode->GetType()->ResolveRawType(false)->Kind()
				== TYPE_ADDRESS) {
				type_kind childKind = nodeChildRawType->Kind();
				if (childKind == TYPE_COMPOUND || childKind == TYPE_ARRAY) {
					node->SetHidden(true);

					// we need to tell the listener about nodes like this so
					// any necessary actions can be taken for them (i.e. value
					// resolution), since they're otherwise invisible to
					// outsiders.
					NotifyNodeHidden(node);
				}
			}
		}
	}

	// notify table model listeners
	if (!node->IsHidden()) {
		TreeTablePath path;
		if (parent == NULL || GetTreePath(parent, path))
			NotifyNodesAdded(path, childIndex, 1);
	}

	// if the node is hidden, add its children
	if (node->IsHidden())
		fNodeManager->AddChildNodes(nodeChild);

	return B_OK;
}


bool
VariablesView::VariableTableModel::GetTreePath(ModelNode* node,
	TreeTablePath& _path) const
{
	// recurse, if the node has a parent
	if (ModelNode* parent = node->Parent()) {
		if (!GetTreePath(parent, _path))
			return false;

		if (node->IsHidden())
			return true;

		return _path.AddComponent(parent->IndexOf(node));
	}

	// no parent -- get the index and start the path
	int32 index = fNodes.IndexOf(node);
	_path.Clear();
	return index >= 0 && _path.AddComponent(index);
}


// #pragma mark - VariablesView


VariablesView::VariablesView(Listener* listener)
	:
	BGroupView(B_VERTICAL),
	fThread(NULL),
	fStackFrame(NULL),
	fVariableTable(NULL),
	fVariableTableModel(NULL),
	fContainerListener(NULL),
	fPreviousViewState(NULL),
	fViewStateHistory(NULL),
	fExpressions(NULL),
	fExpressionChildren(10, false),
	fTableCellContextMenuTracker(NULL),
	fPendingTypecastInfo(NULL),
	fTemporaryExpression(NULL),
	fFrameClearPending(false),
	fEditWindow(NULL),
	fListener(listener)
{
	SetName("Variables");
}


VariablesView::~VariablesView()
{
	if (fEditWindow != NULL)
		BMessenger(fEditWindow).SendMessage(B_QUIT_REQUESTED);

	SetStackFrame(NULL, NULL);
	fVariableTable->SetTreeTableModel(NULL);

	if (fPreviousViewState != NULL)
		fPreviousViewState->ReleaseReference();
	delete fViewStateHistory;

	if (fVariableTableModel != NULL) {
		fVariableTableModel->SetContainerListener(NULL);
		delete fVariableTableModel;
	}

	delete fContainerListener;
	if (fPendingTypecastInfo != NULL)
		fPendingTypecastInfo->ReleaseReference();

	if (fTemporaryExpression != NULL)
		fTemporaryExpression->ReleaseReference();
}


/*static*/ VariablesView*
VariablesView::Create(Listener* listener, ValueNodeManager* manager)
{
	VariablesView* self = new VariablesView(listener);

	try {
		self->_Init(manager);
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
VariablesView::SetStackFrame(::Thread* thread, StackFrame* stackFrame)
{
	bool updateValues = fFrameClearPending;
		// We only want to save previous values if we've continued
		// execution (i.e. thread/frame are being cleared).
		// Otherwise, we'll overwrite our previous values simply
		// by switching frames within the same stack trace, which isn't
		// desired behavior.

	fFrameClearPending = false;

	if (thread == fThread && stackFrame == fStackFrame)
		return;

	_SaveViewState(updateValues);

	_FinishContextMenu(true);

	for (int32 i = 0; i < fExpressionChildren.CountItems(); i++)
		fExpressionChildren.ItemAt(i)->ReleaseReference();
	fExpressionChildren.MakeEmpty();

	if (fThread != NULL)
		fThread->ReleaseReference();
	if (fStackFrame != NULL)
		fStackFrame->ReleaseReference();

	fThread = thread;
	fStackFrame = stackFrame;

	if (fThread != NULL)
		fThread->AcquireReference();
	if (fStackFrame != NULL)
		fStackFrame->AcquireReference();

	fVariableTableModel->SetStackFrame(fThread, fStackFrame);

	// request loading the parameter and variable values
	if (fThread != NULL && fStackFrame != NULL) {
		AutoLocker<Team> locker(fThread->GetTeam());

		void* root = fVariableTableModel->Root();
		int32 count = fVariableTableModel->CountChildren(root);
		for (int32 i = 0; i < count; i++) {
			ModelNode* node = (ModelNode*)fVariableTableModel->ChildAt(root, i);
			_RequestNodeValue(node);
		}

		_RestoreExpressionNodes();
	}

	_RestoreViewState();
}


void
VariablesView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SHOW_INSPECTOR_WINDOW:
		{
			// TODO: it'd probably be more ideal to extend the context
			// action mechanism to allow one to specify an explicit
			// target for each action rather than them all defaulting
			// to targetting here.
			Looper()->PostMessage(message);
			break;
		}
		case MSG_SHOW_VARIABLE_EDIT_WINDOW:
		{
			if (fEditWindow != NULL)
				fEditWindow->Activate();
			else {
				ModelNode* node = NULL;
				if (message->FindPointer("node", reinterpret_cast<void**>(
						&node)) != B_OK) {
					break;
				}

				Value* value = NULL;
				if (message->FindPointer("value", reinterpret_cast<void**>(
						&value)) != B_OK) {
					break;
				}

				_HandleEditVariableRequest(node, value);
			}
			break;
		}
		case MSG_VARIABLE_EDIT_WINDOW_CLOSED:
		{
			fEditWindow = NULL;
			break;
		}
		case MSG_WRITE_VARIABLE_VALUE:
		{
			Value* value = NULL;
			if (message->FindPointer("value", reinterpret_cast<void**>(
					&value)) != B_OK) {
				break;
			}

			BReference<Value> valueReference(value, true);

			ValueNode* node = NULL;
			if (message->FindPointer("node", reinterpret_cast<void**>(
					&node)) != B_OK) {
				break;
			}

			fListener->ValueNodeWriteRequested(node,
				fStackFrame->GetCpuState(), value);
			break;
		}
		case MSG_SHOW_TYPECAST_NODE_PROMPT:
		{
			BMessage* promptMessage = new(std::nothrow) BMessage(
				MSG_TYPECAST_NODE);

			if (promptMessage == NULL)
				return;

			ObjectDeleter<BMessage> messageDeleter(promptMessage);
			promptMessage->AddPointer("node", fVariableTable
				->SelectionModel()->NodeAt(0));
			PromptWindow* promptWindow = new(std::nothrow) PromptWindow(
				"Specify Type", "Type: ", NULL, BMessenger(this),
				promptMessage);
			if (promptWindow == NULL)
				return;

			messageDeleter.Detach();
			promptWindow->CenterOnScreen();
			promptWindow->Show();
			break;
		}
		case MSG_TYPECAST_NODE:
		{
			ModelNode* node = NULL;
			if (message->FindPointer("node", reinterpret_cast<void **>(&node))
					!= B_OK) {
				break;
			}

			BString typeExpression;
			if (message->FindString("text", &typeExpression) == B_OK) {
				if (typeExpression.IsEmpty())
					break;

				if (fPendingTypecastInfo != NULL)
					fPendingTypecastInfo->ReleaseReference();

				fPendingTypecastInfo = new(std::nothrow)
					VariablesExpressionInfo(typeExpression, node);
				if (fPendingTypecastInfo == NULL) {
					// TODO: notify user
					break;
				}

				fPendingTypecastInfo->AddListener(this);
				fListener->ExpressionEvaluationRequested(fPendingTypecastInfo,
					fStackFrame, fThread);
			}
			break;
		}
		case MSG_TYPECAST_TO_ARRAY:
		{
			ModelNode* node = NULL;
			if (message->FindPointer("node", reinterpret_cast<void **>(&node))
				!= B_OK) {
				break;
			}

			Type* baseType = dynamic_cast<AddressType*>(node->NodeChild()
					->Node()->GetType())->BaseType();
			ArrayType* arrayType = NULL;
			if (baseType->CreateDerivedArrayType(0, kMaxArrayElementCount,
				false, arrayType) != B_OK) {
				break;
			}

			AddressType* addressType = NULL;
			BReference<Type> typeRef(arrayType, true);
			if (arrayType->CreateDerivedAddressType(DERIVED_TYPE_POINTER,
					addressType) != B_OK) {
				break;
			}

			typeRef.Detach();
			typeRef.SetTo(addressType, true);
			ValueNode* valueNode = NULL;
			if (TypeHandlerRoster::Default()->CreateValueNode(
					node->NodeChild(), addressType, valueNode) != B_OK) {
				break;
			}

			typeRef.Detach();
			node->NodeChild()->SetNode(valueNode);
			node->SetCastedType(addressType);
			fVariableTableModel->NotifyNodeChanged(node);
			break;
		}
		case MSG_SHOW_CONTAINER_RANGE_PROMPT:
		{
			ModelNode* node = (ModelNode*)fVariableTable
				->SelectionModel()->NodeAt(0);
			int32 lowerBound, upperBound;
			ValueNode* valueNode = node->NodeChild()->Node();
			if (!valueNode->IsRangedContainer()) {
				valueNode = node->ChildAt(0)->NodeChild()->Node();
				if (!valueNode->IsRangedContainer())
					break;
			}

			bool fixedRange = valueNode->IsContainerRangeFixed();
			if (valueNode->SupportedChildRange(lowerBound, upperBound)
				!= B_OK) {
				break;
			}

			BMessage* promptMessage = new(std::nothrow) BMessage(
				MSG_SET_CONTAINER_RANGE);
			if (promptMessage == NULL)
				break;

			ObjectDeleter<BMessage> messageDeleter(promptMessage);
			promptMessage->AddPointer("node", node);
			promptMessage->AddBool("fixedRange", fixedRange);
			BString infoText;
			if (fixedRange) {
				infoText.SetToFormat("Allowed range: %" B_PRId32
					"-%" B_PRId32 ".", lowerBound, upperBound);
			} else {
				infoText.SetToFormat("Current range: %" B_PRId32
					"-%" B_PRId32 ".", lowerBound, upperBound);
			}

			PromptWindow* promptWindow = new(std::nothrow) PromptWindow(
				"Set Range", "Range: ", infoText.String(), BMessenger(this),
				promptMessage);
			if (promptWindow == NULL)
				return;

			messageDeleter.Detach();
			promptWindow->CenterOnScreen();
			promptWindow->Show();
			break;
		}
		case MSG_SET_CONTAINER_RANGE:
		{
			ModelNode* node = (ModelNode*)fVariableTable
				->SelectionModel()->NodeAt(0);
			int32 lowerBound, upperBound;
			ValueNode* valueNode = node->NodeChild()->Node();
			if (!valueNode->IsRangedContainer())
				valueNode = node->ChildAt(0)->NodeChild()->Node();
			if (valueNode->SupportedChildRange(lowerBound, upperBound) != B_OK)
				break;

			bool fixedRange = message->FindBool("fixedRange");

			BString rangeExpression = message->FindString("text");
			if (rangeExpression.Length() == 0)
				break;

			RangeList ranges;
			status_t result = UiUtils::ParseRangeExpression(
				rangeExpression, lowerBound, upperBound, fixedRange, ranges);
			if (result != B_OK)
				break;

			valueNode->ClearChildren();
			for (int32 i = 0; i < ranges.CountRanges(); i++) {
				const Range* range = ranges.RangeAt(i);
				result = valueNode->CreateChildrenInRange(
					fThread->GetTeam()->GetTeamTypeInformation(),
					range->lowerBound, range->upperBound);
				if (result != B_OK)
					break;
			}
			break;
		}
		case MSG_SHOW_WATCH_VARIABLE_PROMPT:
		{
			ModelNode* node = reinterpret_cast<ModelNode*>(
				fVariableTable->SelectionModel()->NodeAt(0));
			ValueLocation* location = node->NodeChild()->Location();
			ValuePieceLocation piece = location->PieceAt(0);
			if (piece.type != VALUE_PIECE_LOCATION_MEMORY)
				break;

			BMessage looperMessage(*message);
			looperMessage.AddUInt64("address", piece.address);
			looperMessage.AddInt32("length", piece.size);
			looperMessage.AddUInt32("type", B_DATA_READ_WRITE_WATCHPOINT);
			Looper()->PostMessage(&looperMessage);
			break;
		}
		case MSG_ADD_WATCH_EXPRESSION:
		{
			BMessage looperMessage(MSG_SHOW_EXPRESSION_PROMPT_WINDOW);
			looperMessage.AddPointer("target", this);
			Looper()->PostMessage(&looperMessage);
			break;
		}
		case MSG_REMOVE_WATCH_EXPRESSION:
		{
			ModelNode* node;
			if (message->FindPointer("node", reinterpret_cast<void**>(&node))
				!= B_OK) {
				break;
			}

			_RemoveExpression(node);
			break;
		}
		case MSG_ADD_NEW_EXPRESSION:
		{
			const char* expression;
			if (message->FindString("expression", &expression) != B_OK)
				break;

			bool persistentExpression = message->FindBool("persistent");

			ExpressionInfo* info;
			status_t error = _AddExpression(expression, persistentExpression,
				info);
			if (error != B_OK) {
				// TODO: notify user of failure
				break;
			}

			fListener->ExpressionEvaluationRequested(info, fStackFrame,
				fThread);
			break;
		}
		case MSG_EXPRESSION_EVALUATED:
		{
			ExpressionInfo* info;
			status_t result;
			ExpressionResult* value = NULL;
			if (message->FindPointer("info",
					reinterpret_cast<void**>(&info)) != B_OK
				|| message->FindInt32("result", &result) != B_OK) {
				break;
			}

			BReference<ExpressionResult> valueReference;
			if (message->FindPointer("value", reinterpret_cast<void**>(&value))
				== B_OK) {
				valueReference.SetTo(value, true);
			}

			VariablesExpressionInfo* variableInfo
				= dynamic_cast<VariablesExpressionInfo*>(info);
			if (variableInfo != NULL) {
				if (fPendingTypecastInfo == variableInfo) {
					_HandleTypecastResult(result, value);
					fPendingTypecastInfo->ReleaseReference();
					fPendingTypecastInfo = NULL;
				}
			} else {
				_AddExpressionNode(info, result, value);
				if (info == fTemporaryExpression) {
					info->ReleaseReference();
					fTemporaryExpression = NULL;
				}
			}

			break;
		}
		case MSG_VALUE_NODE_CHANGED:
		{
			ValueNodeChild* nodeChild;
			ValueNode* oldNode;
			ValueNode* newNode;
			if (message->FindPointer("nodeChild", (void**)&nodeChild) == B_OK
				&& message->FindPointer("oldNode", (void**)&oldNode) == B_OK
				&& message->FindPointer("newNode", (void**)&newNode) == B_OK) {
				BReference<ValueNodeChild> nodeChildReference(nodeChild, true);
				BReference<ValueNode> oldNodeReference(oldNode, true);
				BReference<ValueNode> newNodeReference(newNode, true);

				fVariableTableModel->ValueNodeChanged(nodeChild, oldNode,
					newNode);
			}

			break;
		}
		case MSG_VALUE_NODE_CHILDREN_CREATED:
		{
			ValueNode* node;
			if (message->FindPointer("node", (void**)&node) == B_OK) {
				BReference<ValueNode> newNodeReference(node, true);
				fVariableTableModel->ValueNodeChildrenCreated(node);
			}

			break;
		}
		case MSG_VALUE_NODE_CHILDREN_DELETED:
		{
			ValueNode* node;
			if (message->FindPointer("node", (void**)&node) == B_OK) {
				BReference<ValueNode> newNodeReference(node, true);
				fVariableTableModel->ValueNodeChildrenDeleted(node);
			}

			break;
		}
		case MSG_VALUE_NODE_VALUE_CHANGED:
		{
			ValueNode* node;
			if (message->FindPointer("node", (void**)&node) == B_OK) {
				BReference<ValueNode> newNodeReference(node, true);
				fVariableTableModel->ValueNodeValueChanged(node);
			}

			break;
		}
		case MSG_RESTORE_PARTIAL_VIEW_STATE:
		{
			ModelNode* node;
			if (message->FindPointer("node", (void**)&node) == B_OK) {
				TreeTablePath path;
				if (fVariableTableModel->GetTreePath(node, path)) {
					FunctionID* functionID = fStackFrame->Function()
						->GetFunctionID();
					if (functionID == NULL)
						return;
					BReference<FunctionID> functionIDReference(functionID,
						true);
					VariablesViewState* viewState = fViewStateHistory
						->GetState(fThread->ID(), functionID);
					if (viewState != NULL) {
						_ApplyViewStateDescendentNodeInfos(viewState, node,
							path);
					}
				}
			}
			break;
		}
		case MSG_VALUE_NODE_NEEDS_VALUE:
		case MSG_MODEL_NODE_HIDDEN:
		{
			ModelNode* node;
			if (message->FindPointer("node", (void**)&node) == B_OK) {
				BReference<ModelNode> modelNodeReference(node, true);
				_RequestNodeValue(node);
			}

			break;
		}
		case MSG_VARIABLES_VIEW_CONTEXT_MENU_DONE:
		{
			_FinishContextMenu(false);
			break;
		}
		case MSG_VARIABLES_VIEW_NODE_SETTINGS_CHANGED:
		{
			ModelNode* node;
			if (message->FindPointer("node", (void**)&node) != B_OK)
				break;
			BReference<ModelNode> nodeReference(node, true);

			fVariableTableModel->NotifyNodeChanged(node);
			break;
		}
		case B_COPY:
		{
			_CopyVariableValueToClipboard();
			break;
		}
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
VariablesView::DetachedFromWindow()
{
	_FinishContextMenu(true);
}


void
VariablesView::LoadSettings(const BMessage& settings)
{
	BMessage tableSettings;
	if (settings.FindMessage("variableTable", &tableSettings) == B_OK) {
		GuiSettingsUtils::UnarchiveTableSettings(tableSettings,
			fVariableTable);
	}
}


status_t
VariablesView::SaveSettings(BMessage& settings)
{
	settings.MakeEmpty();

	BMessage tableSettings;
	status_t result = GuiSettingsUtils::ArchiveTableSettings(tableSettings,
		fVariableTable);
	if (result == B_OK)
		result = settings.AddMessage("variableTable", &tableSettings);

	return result;
}


void
VariablesView::SetStackFrameClearPending()
{
	fFrameClearPending = true;
}


void
VariablesView::TreeTableNodeExpandedChanged(TreeTable* table,
	const TreeTablePath& path, bool expanded)
{
	if (fFrameClearPending)
		return;

	if (expanded) {
		ModelNode* node = (ModelNode*)fVariableTableModel->NodeForPath(path);
		if (node == NULL)
			return;

		fVariableTableModel->NodeExpanded(node);

		// request the values of all children that don't have any yet

		// If the node only has a hidden child, directly load the child's
		// children's values.
		if (node->CountChildren() == 1) {
			ModelNode* child = node->ChildAt(0);
			if (child->IsHidden())
				node = child;
		}

		// request the values
		for (int32 i = 0; ModelNode* child = node->ChildAt(i); i++) {
			if (child->IsPresentationNode())
				continue;

			_RequestNodeValue(child);
		}
	}
}


void
VariablesView::TreeTableNodeInvoked(TreeTable* table,
	const TreeTablePath& path)
{
	ModelNode* node = (ModelNode*)fVariableTableModel->NodeForPath(path);
	if (node == NULL)
		return;

	ValueNodeChild* child = node->NodeChild();

	if (child->LocationResolutionState() != B_OK)
		return;

	ValueLocation* location = child->Location();
	if (!location->IsWritable())
		return;

	Value* value = node->GetValue();
	if (value == NULL)
		return;

	BMessage message(MSG_SHOW_VARIABLE_EDIT_WINDOW);
	message.AddPointer("node", node);
	message.AddPointer("value", value);

	BMessenger(this).SendMessage(&message);
}


void
VariablesView::TreeTableCellMouseDown(TreeTable* table,
	const TreeTablePath& path, int32 columnIndex, BPoint screenWhere,
	uint32 buttons)
{
	if ((buttons & B_SECONDARY_MOUSE_BUTTON) == 0)
		return;

	if (fFrameClearPending)
		return;

	_FinishContextMenu(true);

	ModelNode* node = (ModelNode*)fVariableTableModel->NodeForPath(path);
	if (node == NULL)
		return;

	Settings* settings = NULL;
	SettingsMenu* settingsMenu = NULL;
	BReference<SettingsMenu> settingsMenuReference;
	status_t error = B_OK;
	TableCellValueRenderer* cellRenderer = node->TableCellRenderer();
	if (cellRenderer != NULL) {
		settings = cellRenderer->GetSettings();
		if (settings != NULL) {
			error = node->GetValueHandler()
				->CreateTableCellValueSettingsMenu(node->GetValue(), settings,
					settingsMenu);
			settingsMenuReference.SetTo(settingsMenu, true);
			if (error != B_OK)
				return;
		}
	}

	TableCellContextMenuTracker* tracker = new(std::nothrow)
		TableCellContextMenuTracker(node, Looper(), this);
	BReference<TableCellContextMenuTracker> trackerReference(tracker);

	ContextActionList* preActionList;
	ContextActionList* postActionList;

	error = _GetContextActionsForNode(node, preActionList, postActionList);
	if (error != B_OK)
		return;

	BPrivate::ObjectDeleter<ContextActionList> preActionListDeleter(
		preActionList);

	BPrivate::ObjectDeleter<ContextActionList> postActionListDeleter(
		postActionList);

	if (tracker == NULL || tracker->Init(settings, settingsMenu, preActionList,
		postActionList) != B_OK) {
		return;
	}

	fTableCellContextMenuTracker = trackerReference.Detach();
	fTableCellContextMenuTracker->ShowMenu(screenWhere);
}


void
VariablesView::ExpressionEvaluated(ExpressionInfo* info, status_t result,
	ExpressionResult* value)
{
	BMessage message(MSG_EXPRESSION_EVALUATED);
	message.AddPointer("info", info);
	message.AddInt32("result", result);
	BReference<ExpressionResult> valueReference;

	if (value != NULL) {
		valueReference.SetTo(value);
		message.AddPointer("value", value);
	}

	if (BMessenger(this).SendMessage(&message) == B_OK)
		valueReference.Detach();
}


void
VariablesView::_Init(ValueNodeManager* manager)
{
	fVariableTable = new TreeTable("variable list", 0, B_FANCY_BORDER);
	AddChild(fVariableTable->ToView());
	fVariableTable->SetSortingEnabled(false);

	// columns
	fVariableTable->AddColumn(new StringTableColumn(0, "Variable", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fVariableTable->AddColumn(new VariableValueColumn(1, "Value", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT));
	fVariableTable->AddColumn(new StringTableColumn(2, "Type", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));

	fVariableTableModel = new VariableTableModel(manager);
	if (fVariableTableModel->Init() != B_OK)
		throw std::bad_alloc();
	fVariableTable->SetTreeTableModel(fVariableTableModel);
	fVariableTable->SetToolTipProvider(fVariableTableModel);

	fContainerListener = new ContainerListener(this);
	fVariableTableModel->SetContainerListener(fContainerListener);

	fVariableTable->AddTreeTableListener(this);

	fViewStateHistory = new VariablesViewStateHistory;
	if (fViewStateHistory->Init() != B_OK)
		throw std::bad_alloc();

	fExpressions = new ExpressionInfoTable();
	if (fExpressions->Init() != B_OK)
		throw std::bad_alloc();
}


void
VariablesView::_RequestNodeValue(ModelNode* node)
{
	// get the node child and its container
	ValueNodeChild* nodeChild = node->NodeChild();
	ValueNodeContainer* container = nodeChild->Container();

	BReference<ValueNodeContainer> containerReference(container);
	AutoLocker<ValueNodeContainer> containerLocker(container);

	if (container == NULL || nodeChild->Container() != container)
		return;

	// get the value node and check whether its value has not yet been resolved
	ValueNode* valueNode = nodeChild->Node();
	if (valueNode == NULL) {
		ModelNode* parent = node->Parent();
		if (parent != NULL) {
			TreeTablePath path;
			if (!fVariableTableModel->GetTreePath(parent, path))
				return;

			// if the parent node was already expanded when the child was
			// added, we may not yet have added a value node.
			// Notify the table model that this needs to be done.
			if (fVariableTable->IsNodeExpanded(path))
				fVariableTableModel->NodeExpanded(parent);
		}
	}

	if (valueNode == NULL || valueNode->LocationAndValueResolutionState()
		!= VALUE_NODE_UNRESOLVED) {
		return;
	}

	BReference<ValueNode> valueNodeReference(valueNode);
	containerLocker.Unlock();

	// request resolution of the value
	fListener->ValueNodeValueRequested(fStackFrame != NULL
			? fStackFrame->GetCpuState() : NULL, container, valueNode);
}


status_t
VariablesView::_GetContextActionsForNode(ModelNode* node,
	ContextActionList*& _preActions, ContextActionList*& _postActions)
{
	_preActions = NULL;
	_postActions = NULL;

	ValueLocation* location = node->NodeChild()->Location();

	_preActions = new(std::nothrow) ContextActionList;
	if (_preActions == NULL)
		return B_NO_MEMORY;

	BPrivate::ObjectDeleter<ContextActionList> preActionListDeleter(
		_preActions);

	status_t result = B_OK;
	BMessage* message = NULL;

	// only show the Inspect option if the value is in fact located
	// in memory.
	if (location != NULL) {
		if (location->PieceAt(0).type  == VALUE_PIECE_LOCATION_MEMORY) {
			result = _AddContextAction("Inspect", MSG_SHOW_INSPECTOR_WINDOW,
				_preActions, message);
			if (result != B_OK)
				return result;
			message->AddUInt64("address", location->PieceAt(0).address);
		}

		ValueNode* valueNode = node->NodeChild()->Node();

		if (valueNode != NULL) {
			Value* value = valueNode->GetValue();
			if (location->IsWritable() && value != NULL) {
				result = _AddContextAction("Edit" B_UTF8_ELLIPSIS,
					MSG_SHOW_VARIABLE_EDIT_WINDOW, _preActions, message);
				if (result != B_OK)
					return result;
				message->AddPointer("node", node);
				message->AddPointer("value", value);
			}
			AddressType* type = dynamic_cast<AddressType*>(valueNode->GetType());
			if (type != NULL && type->BaseType() != NULL) {
				result = _AddContextAction("Cast to array", MSG_TYPECAST_TO_ARRAY,
					_preActions, message);
				if (result != B_OK)
					return result;
				message->AddPointer("node", node);
			}
		}

		result = _AddContextAction("Cast as" B_UTF8_ELLIPSIS,
			MSG_SHOW_TYPECAST_NODE_PROMPT, _preActions, message);
		if (result != B_OK)
			return result;

		result = _AddContextAction("Watch" B_UTF8_ELLIPSIS,
			MSG_SHOW_WATCH_VARIABLE_PROMPT, _preActions, message);
		if (result != B_OK)
			return result;

		if (valueNode == NULL)
			return B_OK;

		if (valueNode->LocationAndValueResolutionState() == B_OK) {
			result = _AddContextAction("Copy Value", B_COPY, _preActions, message);
			if (result != B_OK)
				return result;
		}

		bool addRangedContainerItem = false;
		// if the current node isn't itself a ranged container, check if it
		// contains a hidden node which is, since in the latter case we
		// want to present the range selection as well.
		if (valueNode->IsRangedContainer())
			addRangedContainerItem = true;
		else if (node->CountChildren() == 1 && node->ChildAt(0)->IsHidden()) {
			valueNode = node->ChildAt(0)->NodeChild()->Node();
			if (valueNode != NULL && valueNode->IsRangedContainer())
				addRangedContainerItem = true;
		}

		if (addRangedContainerItem) {
			result = _AddContextAction("Set visible range" B_UTF8_ELLIPSIS,
				MSG_SHOW_CONTAINER_RANGE_PROMPT, _preActions, message);
			if (result != B_OK)
				return result;
		}
	}

	_postActions = new(std::nothrow) ContextActionList;
	if (_postActions == NULL)
		return B_NO_MEMORY;

	BPrivate::ObjectDeleter<ContextActionList> postActionListDeleter(
		_postActions);

	result = _AddContextAction("Add watch expression" B_UTF8_ELLIPSIS,
		MSG_ADD_WATCH_EXPRESSION, _postActions, message);
	if (result != B_OK)
		return result;

	if (fExpressionChildren.HasItem(node->NodeChild())) {
		result = _AddContextAction("Remove watch expression",
			MSG_REMOVE_WATCH_EXPRESSION, _postActions, message);
		if (result != B_OK)
			return result;
		message->AddPointer("node", node);
	}

	preActionListDeleter.Detach();
	postActionListDeleter.Detach();

	return B_OK;
}


status_t
VariablesView::_AddContextAction(const char* action, uint32 what,
	ContextActionList* actions, BMessage*& _message)
{
	_message = new(std::nothrow) BMessage(what);
	if (_message == NULL)
		return B_NO_MEMORY;

	ObjectDeleter<BMessage> messageDeleter(_message);

	ActionMenuItem* item = new(std::nothrow) ActionMenuItem(action,
		_message);
	if (item == NULL)
		return B_NO_MEMORY;

	messageDeleter.Detach();
	ObjectDeleter<ActionMenuItem> actionDeleter(item);
	if (!actions->AddItem(item))
		return B_NO_MEMORY;

	actionDeleter.Detach();

	return B_OK;
}


void
VariablesView::_FinishContextMenu(bool force)
{
	if (fTableCellContextMenuTracker != NULL) {
		if (!fTableCellContextMenuTracker->FinishMenu(force) || force) {
			fTableCellContextMenuTracker->ReleaseReference();
			fTableCellContextMenuTracker = NULL;
		}
	}
}



void
VariablesView::_SaveViewState(bool updateValues) const
{
	if (fThread == NULL || fStackFrame == NULL
		|| fStackFrame->Function() == NULL) {
		return;
	}

	// get the function ID
	FunctionID* functionID = fStackFrame->Function()->GetFunctionID();
	if (functionID == NULL)
		return;
	BReference<FunctionID> functionIDReference(functionID, true);

	StackFrameValues* values = NULL;
	ExpressionValues* expressionValues = NULL;
	BReference<StackFrameValues> valuesReference;
	BReference<ExpressionValues> expressionsReference;

	if (!updateValues) {
		VariablesViewState* viewState = fViewStateHistory->GetState(fThread->ID(),
			functionID);
		if (viewState != NULL) {
			values = viewState->Values();
			valuesReference.SetTo(values);

			expressionValues = viewState->GetExpressionValues();
			expressionsReference.SetTo(expressionValues);
		}
	}

	if (values == NULL) {
		values = new(std::nothrow) StackFrameValues;
		if (values == NULL)
			return;
		valuesReference.SetTo(values, true);

		if (values->Init() != B_OK)
			return;

		expressionValues = new(std::nothrow) ExpressionValues;
		if (expressionValues == NULL)
			return;
		expressionsReference.SetTo(expressionValues, true);

		if (expressionValues->Init() != B_OK)
			return;
	}

	// create an empty view state
	VariablesViewState* viewState = new(std::nothrow) VariablesViewState;
	if (viewState == NULL)
		return;
	BReference<VariablesViewState> viewStateReference(viewState, true);

	if (viewState->Init() != B_OK)
		return;

	viewState->SetValues(values);
	viewState->SetExpressionValues(expressionValues);

	// populate it
	TreeTablePath path;
	if (_AddViewStateDescendentNodeInfos(viewState,
			fVariableTableModel->Root(), path, updateValues) != B_OK) {
		return;
	}

	// add the view state to the history
	fViewStateHistory->SetState(fThread->ID(), functionID, viewState);
}


void
VariablesView::_RestoreViewState()
{
	if (fPreviousViewState != NULL) {
		fPreviousViewState->ReleaseReference();
		fPreviousViewState = NULL;
	}

	if (fThread == NULL || fStackFrame == NULL
		|| fStackFrame->Function() == NULL) {
		return;
	}

	// get the function ID
	FunctionID* functionID = fStackFrame->Function()->GetFunctionID();
	if (functionID == NULL)
		return;
	BReference<FunctionID> functionIDReference(functionID, true);

	// get the previous view state
	VariablesViewState* viewState = fViewStateHistory->GetState(fThread->ID(),
		functionID);
	if (viewState == NULL)
		return;

	// apply the view state
	TreeTablePath path;
	_ApplyViewStateDescendentNodeInfos(viewState, fVariableTableModel->Root(),
		path);
}


status_t
VariablesView::_AddViewStateDescendentNodeInfos(VariablesViewState* viewState,
	void* parent, TreeTablePath& path, bool updateValues) const
{
	int32 childCount = fVariableTableModel->CountChildren(parent);
	for (int32 i = 0; i < childCount; i++) {
		ModelNode* node = (ModelNode*)fVariableTableModel->ChildAt(parent, i);
		if (!path.AddComponent(i))
			return B_NO_MEMORY;

		// add the node's info
		VariablesViewNodeInfo nodeInfo;
		nodeInfo.SetNodeExpanded(fVariableTable->IsNodeExpanded(path));
		nodeInfo.SetCastedType(node->GetCastedType());
		TableCellValueRenderer* renderer = node->TableCellRenderer();
		if (renderer != NULL) {
			Settings* settings = renderer->GetSettings();
			if (settings != NULL)
				nodeInfo.SetRendererSettings(settings->Message());
		}

		Value* value = node->GetValue();
		Variable* variable = node->GetVariable();
		TypeComponentPath* componentPath = node->GetPath();
		ObjectID* id = variable->ID();

		status_t error = viewState->SetNodeInfo(id, componentPath, nodeInfo);
		if (error != B_OK)
			return error;

		if (value != NULL && updateValues) {
			BVariant variableValueData;
			if (value->ToVariant(variableValueData))
				error = viewState->Values()->SetValue(id, componentPath,
					variableValueData);
			if (error != B_OK)
				return error;
		}

		// recurse
		error = _AddViewStateDescendentNodeInfos(viewState, node, path,
			updateValues);
		if (error != B_OK)
			return error;

		path.RemoveLastComponent();
	}

	return B_OK;
}


status_t
VariablesView::_ApplyViewStateDescendentNodeInfos(VariablesViewState* viewState,
	void* parent, TreeTablePath& path)
{
	int32 childCount = fVariableTableModel->CountChildren(parent);
	for (int32 i = 0; i < childCount; i++) {
		ModelNode* node = (ModelNode*)fVariableTableModel->ChildAt(parent, i);
		if (!path.AddComponent(i))
			return B_NO_MEMORY;

		// apply the node's info, if any
		ObjectID* objectID = node->GetVariable()->ID();
		TypeComponentPath* componentPath = node->GetPath();
		const VariablesViewNodeInfo* nodeInfo = viewState->GetNodeInfo(
			objectID, componentPath);
		if (nodeInfo != NULL) {
			// NB: if the node info indicates that the node in question
			// was being cast to a different type, this *must* be applied
			// before any other view state restoration, since it
			// potentially changes the child hierarchy under that node.
			Type* type = nodeInfo->GetCastedType();
			if (type != NULL) {
				ValueNode* valueNode = NULL;
				if (TypeHandlerRoster::Default()->CreateValueNode(
					node->NodeChild(), type, valueNode) == B_OK) {
					node->NodeChild()->SetNode(valueNode);
					node->SetCastedType(type);
				}
			}

			// we don't have a renderer yet so we can't apply the settings
			// at this stage. Store them on the model node so we can lazily
			// apply them once the value is retrieved.
			node->SetLastRendererSettings(nodeInfo->GetRendererSettings());

			fVariableTable->SetNodeExpanded(path,
				nodeInfo->IsNodeExpanded());

			BVariant previousValue;
			if (viewState->Values()->GetValue(objectID, componentPath,
				previousValue)) {
				node->SetPreviousValue(previousValue);
			}
		}

		// recurse
		status_t error = _ApplyViewStateDescendentNodeInfos(viewState, node,
			path);
		if (error != B_OK)
			return error;

		path.RemoveLastComponent();
	}

	return B_OK;
}


void
VariablesView::_CopyVariableValueToClipboard()
{
	ModelNode* node = reinterpret_cast<ModelNode*>(
		fVariableTable->SelectionModel()->NodeAt(0));

	Value* value = node->GetValue();
	BString valueData;
	if (value != NULL && value->ToString(valueData)) {
		be_clipboard->Lock();
		be_clipboard->Data()->RemoveData("text/plain");
		be_clipboard->Data()->AddData ("text/plain",
			B_MIME_TYPE, valueData.String(),
			valueData.Length());
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
}


status_t
VariablesView::_AddExpression(const char* expression,
	bool persistentExpression, ExpressionInfo*& _info)
{
	ExpressionInfoEntry* entry = NULL;
	if (persistentExpression) {
		// if our stack frame doesn't have an associated function,
		// we can't add an expression
		FunctionInstance* function = fStackFrame->Function();
		if (function == NULL)
			return B_NOT_ALLOWED;

		FunctionID* id = function->GetFunctionID();
		if (id == NULL)
			return B_NO_MEMORY;

		BReference<FunctionID> idReference(id, true);

		entry = fExpressions->Lookup(FunctionKey(id));
		if (entry == NULL) {
			entry = new(std::nothrow) ExpressionInfoEntry(id);
			if (entry == NULL)
				return B_NO_MEMORY;
			status_t error = fExpressions->Insert(entry);
			if (error != B_OK) {
				delete entry;
				return error;
			}
		}
	}

	ExpressionInfo* info = new(std::nothrow) ExpressionInfo(expression);

	if (info == NULL)
		return B_NO_MEMORY;

	BReference<ExpressionInfo> infoReference(info, true);

	if (persistentExpression) {
		if (!entry->AddItem(info))
			return B_NO_MEMORY;
	} else
		fTemporaryExpression = info;

	info->AddListener(this);
	infoReference.Detach();
	_info = info;
	return B_OK;
}


void
VariablesView::_RemoveExpression(ModelNode* node)
{
	if (!fExpressionChildren.HasItem(node->NodeChild()))
		return;

	FunctionID* id = fStackFrame->Function()->GetFunctionID();
	BReference<FunctionID> idReference(id, true);

	ExpressionInfoEntry* entry = fExpressions->Lookup(FunctionKey(id));
	if (entry == NULL)
		return;

	for (int32 i = 0; i < entry->CountItems(); i++) {
		ExpressionInfo* info = entry->ItemAt(i);
		if (info->Expression() == node->Name()) {
			entry->RemoveItemAt(i);
			info->RemoveListener(this);
			info->ReleaseReference();
			break;
		}
	}

	fVariableTableModel->RemoveSyntheticNode(node);
}


void
VariablesView::_RestoreExpressionNodes()
{
	FunctionInstance* instance = fStackFrame->Function();
	if (instance == NULL)
		return;

	FunctionID* id = instance->GetFunctionID();
	if (id == NULL)
		return;

	BReference<FunctionID> idReference(id, true);

	ExpressionInfoEntry* entry = fExpressions->Lookup(FunctionKey(id));
	if (entry == NULL)
		return;

	for (int32 i = 0; i < entry->CountItems(); i++) {
		ExpressionInfo* info = entry->ItemAt(i);
		fListener->ExpressionEvaluationRequested(info, fStackFrame, fThread);
	}
}


void
VariablesView::_AddExpressionNode(ExpressionInfo* info, status_t result,
	ExpressionResult* value)
{
	bool temporaryExpression = (info == fTemporaryExpression);
	Variable* variable = NULL;
	BReference<Variable> variableReference;
	BVariant valueData;

	ExpressionVariableID* id
		= new(std::nothrow) ExpressionVariableID(info);
	if (id == NULL)
		return;
	BReference<ObjectID> idReference(id, true);

	Type* type = NULL;
	ValueLocation* location = NULL;
	ValueNodeChild* child = NULL;
	BReference<Type> typeReference;
	BReference<ValueLocation> locationReference;
	if (value->Kind() == EXPRESSION_RESULT_KIND_PRIMITIVE) {
		value->PrimitiveValue()->ToVariant(valueData);
		if (_GetTypeForTypeCode(valueData.Type(), type) != B_OK)
			return;
		typeReference.SetTo(type, true);

		location = new(std::nothrow) ValueLocation();
		if (location == NULL)
			return;
		locationReference.SetTo(location, true);

		if (valueData.IsNumber()) {

			ValuePieceLocation piece;
			if (!piece.SetToValue(valueData.Bytes(), valueData.Size())
				|| !location->AddPiece(piece)) {
				return;
			}
		}
	} else if (value->Kind() == EXPRESSION_RESULT_KIND_VALUE_NODE) {
		child = value->ValueNodeValue();
		type = child->GetType();
		typeReference.SetTo(type);
		location = child->Location();
		locationReference.SetTo(location);
	}

	variable = new(std::nothrow) Variable(id,
		info->Expression(), type, location);
	if (variable == NULL)
		return;
	variableReference.SetTo(variable, true);

	status_t error = fVariableTableModel->AddSyntheticNode(variable, child,
		info->Expression());
	if (error != B_OK)
		return;

	// In the case of either an evaluation error, or an unsupported result
	// type, set an explanatory string for the result directly.
	if (result != B_OK || valueData.Type() == B_STRING_TYPE) {
		StringValue* explicitValue = new(std::nothrow) StringValue(
			valueData.ToString());
		if (explicitValue == NULL)
			return;

		child->Node()->SetLocationAndValue(NULL, explicitValue, B_OK);
	}

	if (temporaryExpression || fExpressionChildren.AddItem(child)) {
		child->AcquireReference();

		if (temporaryExpression)
			return;

		// attempt to restore our newly added node's view state,
		// if applicable.
		FunctionID* functionID = fStackFrame->Function()
			->GetFunctionID();
		if (functionID == NULL)
			return;
		BReference<FunctionID> functionIDReference(functionID,
			true);
		VariablesViewState* viewState = fViewStateHistory
			->GetState(fThread->ID(), functionID);
		if (viewState != NULL) {
			TreeTablePath path;
			_ApplyViewStateDescendentNodeInfos(viewState,
				fVariableTableModel->Root(), path);
		}
	}
}


void
VariablesView::_HandleTypecastResult(status_t result, ExpressionResult* value)
{
	BString errorMessage;
	if (value == NULL) {
		errorMessage.SetToFormat("Failed to evaluate expression \"%s\": %s (%"
			B_PRId32 ")", fPendingTypecastInfo->Expression().String(),
			strerror(result), result);
	} else if (result != B_OK) {
		BVariant valueData;
		value->PrimitiveValue()->ToVariant(valueData);

		// usually, the evaluation can give us back an error message to
		// specifically indicate why it failed. If it did, simply use
		// the message directly, otherwise fall back to generating an error
		// message based on the error code
		if (valueData.Type() == B_STRING_TYPE)
			errorMessage = valueData.ToString();
		else {
			errorMessage.SetToFormat("Failed to evaluate expression \"%s\":"
				" %s (%" B_PRId32 ")",
				fPendingTypecastInfo->Expression().String(), strerror(result),
				result);
		}

	} else if (value->Kind() != EXPRESSION_RESULT_KIND_TYPE) {
		errorMessage.SetToFormat("Expression \"%s\" does not evaluate to a"
			" type.", fPendingTypecastInfo->Expression().String());
	}

	if (!errorMessage.IsEmpty()) {
		BAlert* alert = new(std::nothrow) BAlert("Typecast error",
			errorMessage, "Close");
		if (alert != NULL)
			alert->Go();

		return;
	}

	Type* type = value->GetType();
	BReference<Type> typeRef(type);
	ValueNode* valueNode = NULL;
	ModelNode* node = fPendingTypecastInfo->TargetNode();
	if (TypeHandlerRoster::Default()->CreateValueNode(node->NodeChild(), type,
			valueNode) != B_OK) {
		return;
	}

	node->NodeChild()->SetNode(valueNode);
	node->SetCastedType(type);
	fVariableTableModel->NotifyNodeChanged(node);
}


void
VariablesView::_HandleEditVariableRequest(ModelNode* node, Value* value)
{
	// get a value handler
	ValueHandler* valueHandler;
	status_t error = ValueHandlerRoster::Default()->FindValueHandler(value,
		valueHandler);
	if (error != B_OK)
		return;

	ValueNode* valueNode = node->NodeChild()->Node();

	BReference<ValueHandler> handlerReference(valueHandler, true);
	TableCellValueRenderer* renderer = node->TableCellRenderer();
	TableCellValueEditor* editor = NULL;
	error = valueHandler->GetTableCellValueEditor(value,
		renderer != NULL ? renderer->GetSettings() : NULL, editor);
	if (error != B_OK || editor == NULL)
		return;

	BReference<TableCellValueEditor> editorReference(editor, true);

	try {
		fEditWindow = VariableEditWindow::Create(value, valueNode, editor,
			this);
	} catch (...) {
		fEditWindow = NULL;
		return;
	}

	fEditWindow->Show();
}


status_t
VariablesView::_GetTypeForTypeCode(int32 type, Type*& _resultType) const
{
	if (BVariant::TypeIsNumber(type) || type == B_STRING_TYPE) {
		_resultType = new(std::nothrow) SyntheticPrimitiveType(type);
		if (_resultType == NULL)
			return B_NO_MEMORY;

		return B_OK;
	}

	return B_NOT_SUPPORTED;
}


// #pragma mark - Listener


VariablesView::Listener::~Listener()
{
}
