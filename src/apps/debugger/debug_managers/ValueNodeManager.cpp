/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ValueNodeManager.h"

#include "AutoLocker.h"

#include "StackFrame.h"
#include "Thread.h"
#include "TypeHandlerRoster.h"
#include "ValueNode.h"
#include "Variable.h"
#include "VariableValueNodeChild.h"


ValueNodeManager::ValueNodeManager()
	:
	fContainer(NULL),
	fStackFrame(NULL),
	fThread(NULL)
{
}


ValueNodeManager::~ValueNodeManager()
{
	SetStackFrame(NULL, NULL);
}


status_t
ValueNodeManager::SetStackFrame(Thread* thread,
	StackFrame* stackFrame)
{
	if (fContainer != NULL) {
		AutoLocker<ValueNodeContainer> containerLocker(fContainer);

		fContainer->RemoveListener(this);

		fContainer->RemoveAllChildren();
		containerLocker.Unlock();
		fContainer->ReleaseReference();
		fContainer = NULL;
	}

	fStackFrame = stackFrame;
	fThread = thread;

	if (fStackFrame != NULL) {
		fContainer = new(std::nothrow) ValueNodeContainer;
		if (fContainer == NULL)
			return B_NO_MEMORY;

		status_t error = fContainer->Init();
		if (error != B_OK) {
			delete fContainer;
			fContainer = NULL;
			return error;
		}

		AutoLocker<ValueNodeContainer> containerLocker(fContainer);

		fContainer->AddListener(this);

		for (int32 i = 0; Variable* variable = fStackFrame->ParameterAt(i);
				i++) {
			_AddNode(variable);
		}

		for (int32 i = 0; Variable* variable
				= fStackFrame->LocalVariableAt(i); i++) {
			_AddNode(variable);
		}
	}

	return B_OK;
}


bool
ValueNodeManager::AddListener(ValueNodeContainer::Listener* listener)
{
	return fListeners.AddItem(listener);
}


void
ValueNodeManager::RemoveListener(ValueNodeContainer::Listener* listener)
{
	fListeners.RemoveItem(listener);
}


void
ValueNodeManager::ValueNodeChanged(ValueNodeChild* nodeChild,
	ValueNode* oldNode, ValueNode* newNode)
{
	if (fContainer == NULL)
		return;

	AutoLocker<ValueNodeContainer> containerLocker(fContainer);

	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--)
		fListeners.ItemAt(i)->ValueNodeChanged(nodeChild, oldNode, newNode);

	if (oldNode != NULL)
		newNode->CreateChildren();

}


void
ValueNodeManager::ValueNodeChildrenCreated(ValueNode* node)
{
	if (fContainer == NULL)
		return;

	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--)
		fListeners.ItemAt(i)->ValueNodeChildrenCreated(node);
}


void
ValueNodeManager::ValueNodeChildrenDeleted(ValueNode* node)
{
	if (fContainer == NULL)
		return;

	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--)
		fListeners.ItemAt(i)->ValueNodeChildrenDeleted(node);
}


void
ValueNodeManager::ValueNodeValueChanged(ValueNode* valueNode)
{
	if (fContainer == NULL)
		return;

	AutoLocker<ValueNodeContainer> containerLocker(fContainer);

	// check whether we know the node
	ValueNodeChild* nodeChild = valueNode->NodeChild();
	if (nodeChild == NULL)
		return;

	if (valueNode->ChildCreationNeedsValue()
		&& !valueNode->ChildrenCreated()) {
		status_t error = valueNode->CreateChildren();
		if (error == B_OK) {
			for (int32 i = 0; i < valueNode->CountChildren(); i++) {
				ValueNodeChild* child = valueNode->ChildAt(i);
				_CreateValueNode(child);
				AddChildNodes(child);
			}
		}
	}

	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--)
		fListeners.ItemAt(i)->ValueNodeValueChanged(valueNode);
}


void
ValueNodeManager::_AddNode(Variable* variable)
{
	// create the node child for the variable
	ValueNodeChild* nodeChild = new (std::nothrow) VariableValueNodeChild(
		variable);
	BReference<ValueNodeChild> nodeChildReference(nodeChild, true);
	if (nodeChild == NULL || !fContainer->AddChild(nodeChild)) {
		delete nodeChild;
		return;
	}

	// automatically add child nodes for the top level nodes
	AddChildNodes(nodeChild);
}


status_t
ValueNodeManager::_CreateValueNode(ValueNodeChild* nodeChild)
{
	if (nodeChild->Node() != NULL)
		return B_OK;

	// create the node
	ValueNode* valueNode;
	status_t error;
	if (nodeChild->IsInternal()) {
		error = nodeChild->CreateInternalNode(valueNode);
	} else {
		error = TypeHandlerRoster::Default()->CreateValueNode(nodeChild,
			nodeChild->GetType(), valueNode);
	}

	if (error != B_OK)
		return error;

	nodeChild->SetNode(valueNode);
	valueNode->ReleaseReference();

	return B_OK;
}


status_t
ValueNodeManager::AddChildNodes(ValueNodeChild* nodeChild)
{
	AutoLocker<ValueNodeContainer> containerLocker(fContainer);

	// create a value node for the value node child, if doesn't have one yet
	ValueNode* valueNode = nodeChild->Node();
	if (valueNode == NULL) {
		status_t error = _CreateValueNode(nodeChild);
		if (error != B_OK)
			return error;
		valueNode = nodeChild->Node();
	}

	// check if this node requires child creation
	// to be deferred until after its location/value have been resolved
	if (valueNode->ChildCreationNeedsValue())
		return B_OK;

	// create the children, if not done yet
	if (valueNode->ChildrenCreated())
		return B_OK;

	return valueNode->CreateChildren();
}
