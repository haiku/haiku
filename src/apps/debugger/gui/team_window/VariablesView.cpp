/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "VariablesView.h"

#include <stdio.h>

#include <new>

#include <AutoLocker.h>

#include "table/TableColumns.h"

#include "Architecture.h"
#include "StackFrame.h"
#include "StackFrameValues.h"
#include "Team.h"
#include "Thread.h"
#include "TypeComponentPath.h"
#include "Variable.h"


class VariablesView::ValueNode : Referenceable {
public:
	ValueNode(ValueNode* parent, Variable* variable, TypeComponentPath* path,
		const BString& name, Type* type)
		:
		fParent(parent),
		fVariable(variable),
		fPath(path),
		fName(name),
		fType(type)
	{
		fVariable->AcquireReference();
		fPath->AcquireReference();
		fType->AcquireReference();
	}

	~ValueNode()
	{
		for (int32 i = 0; ValueNode* child = fChildren.ItemAt(i); i++)
			child->ReleaseReference();

		fVariable->ReleaseReference();
		fPath->ReleaseReference();
		fType->ReleaseReference();
	}

	ValueNode* Parent() const
	{
		return fParent;
	}

	Variable* GetVariable() const
	{
		return fVariable;
	}

	TypeComponentPath* Path() const
	{
		return fPath;
	}

	const BString& Name() const
	{
		return fName;
	}

	Type* GetType() const
	{
		return fType;
	}

	const BVariant& Value() const
	{
		return fValue;
	}

	void SetValue(const BVariant& value)
	{
		fValue = value;
	}

	int32 CountChildren() const
	{
		return fChildren.CountItems();
	}

	ValueNode* ChildAt(int32 index) const
	{
		return fChildren.ItemAt(index);
	}

	int32 IndexOf(ValueNode* child) const
	{
		return fChildren.IndexOf(child);
	}

	bool AddChild(ValueNode* child)
	{
		if (!fChildren.AddItem(child))
			return false;

		child->AcquireReference();
		return true;
	}

private:
	typedef BObjectList<ValueNode> ChildList;

private:
	ValueNode*			fParent;
	Variable*			fVariable;
	TypeComponentPath*	fPath;
	BString				fName;
	Type*				fType;
	BVariant			fValue;
	ChildList			fChildren;
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
	virtual BField* PrepareField(const BVariant& value) const
	{
		char buffer[64];
		return StringTableColumn::PrepareField(
			BVariant(_ToString(value, buffer, sizeof(buffer)),
				B_VARIANT_DONT_COPY_DATA));
	}

	virtual int CompareValues(const BVariant& a, const BVariant& b)
	{
		// If neither value is a number, compare the strings. If only one value
		// is a number, it is considered to be greater.
		if (!a.IsNumber()) {
			if (b.IsNumber())
				return -1;
			char bufferA[64];
			char bufferB[64];
			return StringTableColumn::CompareValues(
				BVariant(_ToString(a, bufferA, sizeof(bufferA)),
					B_VARIANT_DONT_COPY_DATA),
				BVariant(_ToString(b, bufferB, sizeof(bufferB)),
					B_VARIANT_DONT_COPY_DATA));
		}

		if (!b.IsNumber())
			return 1;

		// If either value is floating point, we compare floating point values.
		if (a.IsFloat() || b.IsFloat()) {
			double valueA = a.ToDouble();
			double valueB = b.ToDouble();
			return valueA < valueB ? -1 : (valueA == valueB ? 0 : 1);
		}

		uint64 valueA = a.ToUInt64();
		uint64 valueB = b.ToUInt64();
		return valueA < valueB ? -1 : (valueA == valueB ? 0 : 1);
	}

private:
	const char* _ToString(const BVariant& value, char* buffer,
		size_t bufferSize) const
	{
		switch (value.Type()) {
			case B_BOOL_TYPE:
				return value.ToBool() ? "true" : "false";
			case B_FLOAT_TYPE:
			case B_DOUBLE_TYPE:
				snprintf(buffer, bufferSize, "%g", value.ToDouble());
				break;
			case B_INT8_TYPE:
			case B_UINT8_TYPE:
				snprintf(buffer, bufferSize, "0x%02x", value.ToUInt8());
				break;
			case B_INT16_TYPE:
			case B_UINT16_TYPE:
				snprintf(buffer, bufferSize, "0x%04x", value.ToUInt16());
				break;
			case B_INT32_TYPE:
			case B_UINT32_TYPE:
				snprintf(buffer, bufferSize, "0x%08lx", value.ToUInt32());
				break;
			case B_INT64_TYPE:
			case B_UINT64_TYPE:
				snprintf(buffer, bufferSize, "0x%016llx", value.ToUInt64());
				break;
			case B_STRING_TYPE:
				return value.ToString();
			default:
				return NULL;
		}

		return buffer;
	}
};


// #pragma mark - VariableTableModel


class VariablesView::VariableTableModel : public TreeTableModel {
public:
	VariableTableModel()
		:
		fStackFrame(NULL),
		fNodes(20, true)
	{
	}

	~VariableTableModel()
	{
		SetStackFrame(NULL, NULL);
	}

	void SetStackFrame(Thread* thread, StackFrame* stackFrame)
	{
		if (!fNodes.IsEmpty()) {
			int32 count = fNodes.CountItems();
			fNodes.MakeEmpty();
			NotifyNodesRemoved(TreeTablePath(), 0, count);
		}

		fStackFrame = stackFrame;
		fThread = thread;

		if (fStackFrame != NULL) {
			for (int32 i = 0; Variable* variable = fStackFrame->ParameterAt(i);
					i++) {
				_AddNode(variable);
			}

			for (int32 i = 0; Variable* variable
					= fStackFrame->LocalVariableAt(i); i++) {
				_AddNode(variable);
			}

			if (!fNodes.IsEmpty())
				NotifyNodesAdded(TreeTablePath(), 0, fNodes.CountItems());
		}
	}

	void StackFrameValueRetrieved(StackFrame* stackFrame, Variable* variable,
		TypeComponentPath* path)
	{
		if (stackFrame != fStackFrame)
			return;

		// update the respective node's value
		AutoLocker<Team> locker(fThread->GetTeam());
		BVariant value;
		if (fStackFrame->Values()->GetValue(variable->ID(), path, value)) {
			ValueNode* node = _GetNode(variable, path);
			TreeTablePath treePath;
			if (node != NULL && _GetTreePath(node, treePath)) {
				node->SetValue(value);
				int32 index = treePath.RemoveLastComponent();
				NotifyNodesChanged(treePath, index, 1);
			}
		}
	}

	virtual int32 CountColumns() const
	{
		return 2;
	}

	virtual void* Root() const
	{
		return (void*)this;
	}

	virtual int32 CountChildren(void* parent) const
	{
		if (parent == this)
			return fNodes.CountItems();

		return ((ValueNode*)parent)->CountChildren();
	}

	virtual void* ChildAt(void* parent, int32 index) const
	{
		if (parent == this)
			return fNodes.ItemAt(index);

		return ((ValueNode*)parent)->ChildAt(index);
	}

	virtual bool GetValueAt(void* object, int32 columnIndex, BVariant& _value)
	{
		ValueNode* node = (ValueNode*)object;

		switch (columnIndex) {
			case 0:
				_value.SetTo(node->Name(), B_VARIANT_DONT_COPY_DATA);
				return true;
			case 1:
				if (node->Value().Type() == 0)
					return false;

				_value = node->Value();
				return true;
			default:
				return false;
		}
	}

private:
	typedef BObjectList<ValueNode> ValueList;

private:
	void _AddNode(Variable* variable)
	{
		TypeComponentPath* path = new(std::nothrow) TypeComponentPath;
		if (path == NULL)
			return;
		Reference<TypeComponentPath> pathReference(path, true);

		ValueNode* node = new(std::nothrow) ValueNode(NULL, variable, path,
			variable->Name(), variable->GetType());
		if (node == NULL || !fNodes.AddItem(node)) {
			delete node;
			return;
		}
	}

	ValueNode* _GetNode(Variable* variable, TypeComponentPath* path) const
	{
		// find the variable node
		ValueNode* node;
		for (int32 i = 0; (node = fNodes.ItemAt(i)) != NULL; i++) {
			if (node->GetVariable() == variable)
				break;
		}
		if (node == NULL)
			return NULL;

		// now walk along the path, finding the respective child node for each
		// component
		int32 componentCount = path->CountComponents();
		for (int32 i = 0; i < componentCount; i++) {
			ValueNode* childNode = NULL;
			TypeComponent typeComponent = path->ComponentAt(i);
			for (int32 k = 0; (childNode = node->ChildAt(k)) != NULL; k++) {
				if (childNode->Path()->ComponentAt(i) == typeComponent)
					break;
			}

			if (childNode == NULL)
				return NULL;

			node = childNode;
		}

		return node;
	}

	bool _GetTreePath(ValueNode* node, TreeTablePath& _path) const
	{
		// recurse, if the node has a parent
		if (ValueNode* parent = node->Parent()) {
			if (!_GetTreePath(parent, _path))
				return false;
			return _path.AddComponent(parent->IndexOf(node));
		}

		// no parent -- get the index and start the path
		int32 index = fNodes.IndexOf(node);
		_path.Clear();
		return index >= 0 && _path.AddComponent(index);
	}

private:
	Thread*				fThread;
	StackFrame*			fStackFrame;
	ValueList			fNodes;
};


// #pragma mark - VariablesView


VariablesView::VariablesView(Listener* listener)
	:
	BGroupView(B_VERTICAL),
	fThread(NULL),
	fStackFrame(NULL),
	fVariableTable(NULL),
	fVariableTableModel(NULL),
	fListener(listener)
{
	SetName("Variables");
}


VariablesView::~VariablesView()
{
	SetStackFrame(NULL, NULL);
	fVariableTable->SetTreeTableModel(NULL);
	delete fVariableTableModel;
}


/*static*/ VariablesView*
VariablesView::Create(Listener* listener)
{
	VariablesView* self = new VariablesView(listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
VariablesView::SetStackFrame(Thread* thread, StackFrame* stackFrame)
{
	if (thread == fThread && stackFrame == fStackFrame)
		return;

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

		for (int32 i = 0; Variable* variable = fStackFrame->ParameterAt(i); i++)
			_RequestVariableValue(variable);

		for (int32 i = 0; Variable* variable = fStackFrame->LocalVariableAt(i);
				i++) {
			_RequestVariableValue(variable);
		}
	}
}


void
VariablesView::StackFrameValueRetrieved(StackFrame* stackFrame,
	Variable* variable, TypeComponentPath* path)
{
	fVariableTableModel->StackFrameValueRetrieved(stackFrame, variable, path);
}


void
VariablesView::_Init()
{
	fVariableTable = new TreeTable("variable list", 0, B_FANCY_BORDER);
	AddChild(fVariableTable->ToView());

	// columns
	fVariableTable->AddColumn(new StringTableColumn(0, "Variable", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fVariableTable->AddColumn(new VariableValueColumn(1, "Value", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT));

	fVariableTableModel = new VariableTableModel;
	fVariableTable->SetTreeTableModel(fVariableTableModel);

	fVariableTable->AddTreeTableListener(this);
}


void
VariablesView::_RequestVariableValue(Variable* variable)
{
	StackFrameValues* values = fStackFrame->Values();
	if (values->HasValue(variable->ID(), TypeComponentPath()))
		return;

	TypeComponentPath* path = new(std::nothrow) TypeComponentPath;
	if (path == NULL)
		return;
	Reference<TypeComponentPath> pathReference(path, true);

	fListener->StackFrameValueRequested(fThread, fStackFrame,
		variable, path);
}


// #pragma mark - Listener


VariablesView::Listener::~Listener()
{
}
