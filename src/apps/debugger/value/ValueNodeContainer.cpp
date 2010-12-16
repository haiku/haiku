/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ValueNodeContainer.h"

#include <AutoLocker.h>

#include "ValueNode.h"


// #pragma mark - ValueNodeContainer


ValueNodeContainer::ValueNodeContainer()
	:
	fLock("value node container")
{
}


ValueNodeContainer::~ValueNodeContainer()
{
	RemoveAllChildren();
}


status_t
ValueNodeContainer::Init()
{
	return fLock.InitCheck();
}


int32
ValueNodeContainer::CountChildren() const
{
	return fChildren.CountItems();
}


ValueNodeChild*
ValueNodeContainer::ChildAt(int32 index) const
{
	return fChildren.ItemAt(index);
}


bool
ValueNodeContainer::AddChild(ValueNodeChild* child)
{
	AutoLocker<ValueNodeContainer> locker(this);

	if (!fChildren.AddItem(child))
		return false;

	child->AcquireReference();
	child->SetContainer(this);

	return true;
}


void
ValueNodeContainer::RemoveChild(ValueNodeChild* child)
{
	if (child->Container() != this || !fChildren.RemoveItem(child))
		return;

	child->SetContainer(NULL);
	child->ReleaseReference();
}


void
ValueNodeContainer::RemoveAllChildren()
{
	for (int32 i = 0; ValueNodeChild* child = ChildAt(i); i++) {
		child->SetContainer(NULL);
		child->ReleaseReference();
	}

	fChildren.MakeEmpty();
}


bool
ValueNodeContainer::AddListener(Listener* listener)
{
	return fListeners.AddItem(listener);
}


void
ValueNodeContainer::RemoveListener(Listener* listener)
{
	fListeners.RemoveItem(listener);
}


void
ValueNodeContainer::NotifyValueNodeChanged(ValueNodeChild* nodeChild,
	ValueNode* oldNode, ValueNode* newNode)
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--)
		fListeners.ItemAt(i)->ValueNodeChanged(nodeChild, oldNode, newNode);
}


void
ValueNodeContainer::NotifyValueNodeChildrenCreated(ValueNode* node)
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--)
		fListeners.ItemAt(i)->ValueNodeChildrenCreated(node);
}


void
ValueNodeContainer::NotifyValueNodeChildrenDeleted(ValueNode* node)
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--)
		fListeners.ItemAt(i)->ValueNodeChildrenDeleted(node);
}


void
ValueNodeContainer::NotifyValueNodeValueChanged(ValueNode* node)
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--)
		fListeners.ItemAt(i)->ValueNodeValueChanged(node);
}


// #pragma mark - ValueNodeContainer


ValueNodeContainer::Listener::~Listener()
{
}


void
ValueNodeContainer::Listener::ValueNodeChanged(ValueNodeChild* nodeChild,
	ValueNode* oldNode, ValueNode* newNode)
{
}


void
ValueNodeContainer::Listener::ValueNodeChildrenCreated(ValueNode* node)
{
}


void
ValueNodeContainer::Listener::ValueNodeChildrenDeleted(ValueNode* node)
{
}


void
ValueNodeContainer::Listener::ValueNodeValueChanged(ValueNode* node)
{
}
