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
#include "Tracing.h"
#include "TypeComponentPath.h"
#include "Variable.h"


enum {
	VALUE_NODE_TYPE	= 'valn'
};

// maximum number of array elements to show by default
static const uint64 kMaxArrayElementCount = 10;


class VariablesView::ValueNode : public Referenceable {
public:
	ValueNode(ValueNode* parent, Variable* variable, TypeComponentPath* path,
		const BString& name, Type* type, bool isPresentationNode)
		:
		fParent(parent),
		fVariable(variable),
		fPath(path),
		fName(name),
		fType(type),
		fRawType(type->ResolveRawType()),
		fChildrenAdded(false),
		fIsPresentationNode(isPresentationNode)
	{
		fVariable->AcquireReference();
		fPath->AcquireReference();
		fType->AcquireReference();
		fRawType->AcquireReference();
	}

	~ValueNode()
	{
		for (int32 i = 0; ValueNode* child = fChildren.ItemAt(i); i++)
			child->ReleaseReference();

		fVariable->ReleaseReference();
		fPath->ReleaseReference();
		fType->ReleaseReference();
		fRawType->ReleaseReference();
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

	Type* RawType() const
	{
		return fRawType;
	}

	const BVariant& Value() const
	{
		return fValue;
	}

	void SetValue(const BVariant& value)
	{
		fValue = value;
	}

	bool IsPresentationNode() const
	{
		return fIsPresentationNode;
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

	bool ChildrenAdded() const
	{
		return fChildrenAdded;
	}

	void SetChildrenAdded(bool added)
	{
		fChildrenAdded = added;
	}

private:
	typedef BObjectList<ValueNode> ChildList;

private:
	ValueNode*			fParent;
	Variable*			fVariable;
	TypeComponentPath*	fPath;
	BString				fName;
	Type*				fType;
	Type*				fRawType;
	BVariant			fValue;
	ChildList			fChildren;
	bool				fChildrenAdded;
	bool				fIsPresentationNode;
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
	virtual BField* PrepareField(const BVariant& _value) const
	{
		BVariant value = _ResolveValue(_value);

		char buffer[64];
		return StringTableColumn::PrepareField(
			BVariant(_ToString(value, buffer, sizeof(buffer)),
				B_VARIANT_DONT_COPY_DATA));
	}

	virtual int CompareValues(const BVariant& _a, const BVariant& _b)
	{
		BVariant a = _ResolveValue(_a);
		BVariant b = _ResolveValue(_b);

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
	BVariant _ResolveValue(const BVariant& nodeValue) const
	{
		BVariant value;
		if (nodeValue.Type() != VALUE_NODE_TYPE)
			return BVariant();

		ValueNode* node = dynamic_cast<ValueNode*>(nodeValue.ToReferenceable());

		// replace enumerations values with their names
		if (node->RawType()->Kind() == TYPE_ENUMERATION) {
			EnumerationValue* enumValue
				= dynamic_cast<EnumerationType*>(node->RawType())
					->ValueFor(node->Value());
			if (enumValue != NULL)
				return enumValue->Name();
		}

		return node->Value();
	}

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

				_value.SetTo(node, VALUE_NODE_TYPE);
				return true;
			default:
				return false;
		}
	}

	void NodeExpanded(ValueNode* node)
	{
		// add children of all children
		for (int32 i = 0; ValueNode* child = node->ChildAt(i); i++)
			_AddChildNodes(child);
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
			variable->Name(), variable->GetType(), false);
		if (node == NULL || !fNodes.AddItem(node)) {
			delete node;
			return;
		}

		// automatically add child nodes for the top level nodes
		_AddChildNodes(node);
	}

	void _AddChildNodes(ValueNode* node)
	{
		if (node == NULL || node->ChildrenAdded())
			return;

		_AddChildNodesInternal(node);
		node->SetChildrenAdded(true);

		// If the node is already known, notify the model listeners about the
		// new child nodes. We assume that this holds true for the all but the
		// top-level nodes.
		TreeTablePath treePath;
		if (node->Parent() != NULL && node->CountChildren() > 0
			&& _GetTreePath(node, treePath)) {
			NotifyNodesAdded(treePath, 0, node->CountChildren());
		}
	}

	void _AddChildNodesInternal(ValueNode* node)
	{
		TRACE_LOCALS("_AddChildNodesInternal(%p)\n", node);

		Type* type = node->GetType();
		TypeComponentPath* path
			= new(std::nothrow) TypeComponentPath(*node->Path());
		if (path == NULL
			|| path->CountComponents() != node->Path()->CountComponents()) {
			delete path;
			return;
		}
		Reference<TypeComponentPath> pathReference(path, true);

		bool dereferencedType = false;
		while (true) {
			bool done = false;
			TypeComponent component;

			switch (type->Kind()) {
				case TYPE_PRIMITIVE:
					TRACE_LOCALS("TYPE_PRIMITIVE\n");
					done = true;
					break;
				case TYPE_COMPOUND:
				{
					TRACE_LOCALS("TYPE_COMPOUND\n");
					CompoundType* compoundType
						= dynamic_cast<CompoundType*>(type);

					// base types
					for (int32 i = 0; BaseType* baseType
							= compoundType->BaseTypeAt(i); i++) {
						TRACE_LOCALS("  base %ld\n", i);

						component.SetToBaseType(type->Kind(), i);
						TypeComponentPath* baseTypePath
							= new(std::nothrow) TypeComponentPath(*path);
						if (baseTypePath == NULL
							|| baseTypePath->CountComponents()
								!= path->CountComponents()
							|| !baseTypePath->AddComponent(component)) {
							delete baseTypePath;
							return;
						}
						Reference<TypeComponentPath> baseTypePathReference(
							baseTypePath, true);
						_AddChildNode(node, node->GetVariable(), baseTypePath,
							baseType->GetType()->Name(), baseType->GetType());
					}

					// members
					for (int32 i = 0; DataMember* member
							= compoundType->DataMemberAt(i); i++) {
						BString name = member->Name();

						TRACE_LOCALS("  member %ld: \"%s\"\n", i, name.String());

						component.SetToDataMember(type->Kind(), i, name);
						TypeComponentPath* memberPath
							= new(std::nothrow) TypeComponentPath(*path);
						if (memberPath == NULL
							|| memberPath->CountComponents()
								!= path->CountComponents()
							|| !memberPath->AddComponent(component)) {
							delete memberPath;
							return;
						}
						Reference<TypeComponentPath> memberPathReference(
							memberPath, true);
						_AddChildNode(node, node->GetVariable(), memberPath,
							name, member->GetType());
					}
					return;
				}
				case TYPE_MODIFIED:
					TRACE_LOCALS("TYPE_MODIFIED\n");
					component.SetToBaseType(type->Kind());
					type = dynamic_cast<ModifiedType*>(type)->BaseType();
					break;
				case TYPE_TYPEDEF:
					TRACE_LOCALS("TYPE_TYPEDEF\n");
					component.SetToBaseType(type->Kind());
					type = dynamic_cast<TypedefType*>(type)->BaseType();
					break;
				case TYPE_ADDRESS:
					TRACE_LOCALS("TYPE_ADDRESS\n");
					// don't dereference twice
					if (dereferencedType) {
						done = true;
						break;
					}

					component.SetToBaseType(type->Kind());
					type = dynamic_cast<AddressType*>(type)->BaseType();
					dereferencedType = true;
					break;
				case TYPE_ARRAY:
				{
					TRACE_LOCALS("TYPE_ARRAY\n");
					ArrayType* arrayType = dynamic_cast<ArrayType*>(type);
					int32 dimensionCount = arrayType->CountDimensions();

					// get the base array indices
					ArrayIndexPath baseIndexPath;
					int32 baseDimension = 0;
					if (node->Parent() != NULL
						&& node->Parent()->GetType() == type) {
						TypeComponent arrayComponent
							= path->ComponentAt(path->CountComponents() - 1);
						if (arrayComponent.componentKind
								!= TYPE_COMPONENT_ARRAY_ELEMENT) {
							ERROR("Unexpected array type path component!\n");
							return;
						}

						status_t error
							= baseIndexPath.SetTo(arrayComponent.name.String());
						if (error != B_OK)
							return;

						baseDimension = baseIndexPath.CountIndices();
					}

					if (baseDimension >= dimensionCount) {
						ERROR("Unexpected array base dimension!\n");
						return;
					}
					bool isFinalDimension = baseDimension + 1 == dimensionCount;

					ArrayDimension* dimension = arrayType->DimensionAt(
						baseDimension);
					uint64 elementCount = dimension->CountElements();
					if (elementCount == 0
						|| elementCount > kMaxArrayElementCount) {
						elementCount = kMaxArrayElementCount;
					}

					// create children for the array elements
					for (int32 i = 0; i < (int32)elementCount; i++) {
						ArrayIndexPath indexPath(baseIndexPath);
						if (indexPath.CountIndices()
								!= baseIndexPath.CountIndices()
							|| !indexPath.AddIndex(i)) {
							return;
						}

						component.SetToArrayElement(type->Kind(), indexPath);
						TypeComponentPath* elementPath
							= path->CreateSubPath(path->CountComponents() - 1);
						if (elementPath == NULL
							|| !elementPath->AddComponent(component)) {
							delete elementPath;
							return;
						}
						Reference<TypeComponentPath> elementPathReference(
							elementPath, true);

						BString name(node->Name());
						name << '[' << i << ']';

						_AddChildNode(node, node->GetVariable(), elementPath,
							name,
							isFinalDimension ? arrayType->BaseType() : type,
							!isFinalDimension);
					}

					return;
				}
				case TYPE_ENUMERATION:
					TRACE_LOCALS("TYPE_ENUMERATION\n");
					done = true;
					break;
				case TYPE_SUBRANGE:
					TRACE_LOCALS("TYPE_SUBRANGE -> unsupported\n");
					// TODO: Support!
					return;
				case TYPE_UNSPECIFIED:
					// Should never get here -- we don't create nodes for
					// unspecified types.
					TRACE_LOCALS("TYPE_UNSPECIFIED\n");
					return;
				case TYPE_FUNCTION:
					TRACE_LOCALS("TYPE_FUNCTION -> unsupported\n");
					// TODO: Support!
					return;
				case TYPE_POINTER_TO_MEMBER:
					TRACE_LOCALS("TYPE_POINTER_TO_MEMBER\n");
					done = true;
					break;
			}

			if (done) {
				if (dereferencedType) {
					_AddChildNode(node, node->GetVariable(), path,
						BString("*") << node->Name(), type);
				}
				return;
			}

			if (!path->AddComponent(component))
				return;
		}
	}

	void _AddChildNode(ValueNode* parent, Variable* variable,
		TypeComponentPath* path, const BString& name, Type* type,
		bool isPresentationNode = false)
	{
		// Don't create nodes for unspecified types -- we can't get/show their
		// value anyway.
		if (type->Kind() == TYPE_UNSPECIFIED)
			return;

		ValueNode* node = new(std::nothrow) ValueNode(parent, variable, path,
			name, type, isPresentationNode);
		if (node == NULL || !parent->AddChild(node)) {
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

		// Now walk along the path, finding the respective child node for each
		// component (might be several components at once).
		int32 componentCount = path->CountComponents();
		for (int32 i = 0; i < componentCount;) {
			ValueNode* childNode = NULL;

			for (int32 k = 0; (childNode = node->ChildAt(k)) != NULL; k++) {
				TypeComponentPath* childPath = childNode->Path();
				int32 childComponentCount = childPath->CountComponents();
				if (childComponentCount > componentCount)
					continue;

				for (int32 componentIndex = i;
					componentIndex < childComponentCount; componentIndex++) {
					TypeComponent childComponent
						= childPath->ComponentAt(componentIndex);
					TypeComponent pathComponent
						= path->ComponentAt(componentIndex);
					if (childComponent != pathComponent) {
						if (componentIndex + 1 == childComponentCount
							&& pathComponent.HasPrefix(childComponent)) {
							// The last child component is a prefix of the
							// corresponding path component. We consider this a
							// match, but need to recheck the component with the
							// next node level.
							childComponentCount--;
							break;
						}

						// mismatch -- skip the child
						childNode = NULL;
						break;
					}
				}

				if (childNode != NULL) {
					// got a match -- skip the matched children components
					i = childComponentCount;
					break;
				}
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
VariablesView::TreeTableNodeExpandedChanged(TreeTable* table,
	const TreeTablePath& path, bool expanded)
{
	if (expanded) {
		ValueNode* node = (ValueNode*)fVariableTableModel->NodeForPath(path);
		if (node == NULL)
			return;

		fVariableTableModel->NodeExpanded(node);

		// request the values of all children that don't have any yet
		for (int32 i = 0; ValueNode* child = node->ChildAt(i); i++) {
			if (child->IsPresentationNode())
				continue;

			Variable* variable = child->GetVariable();
			TypeComponentPath* path = child->Path();
			if (fStackFrame->Values()->HasValue(variable->ID(), *path))
				continue;

			fListener->StackFrameValueRequested(fThread, fStackFrame, variable,
				path);
		}
	}
}


void
VariablesView::_Init()
{
	fVariableTable = new TreeTable("variable list", 0, B_FANCY_BORDER);
	AddChild(fVariableTable->ToView());
	fVariableTable->SetSortingEnabled(false);

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

	fListener->StackFrameValueRequested(fThread, fStackFrame, variable, path);
}


// #pragma mark - Listener


VariablesView::Listener::~Listener()
{
}
