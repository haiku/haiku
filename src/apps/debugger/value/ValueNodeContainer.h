/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VALUE_NODE_CONTAINER_H
#define VALUE_NODE_CONTAINER_H


#include <Locker.h>

#include <ObjectList.h>
#include <Referenceable.h>


class ValueNode;
class ValueNodeChild;


class ValueNodeContainer : public BReferenceable {
public:
	class Listener;

public:
								ValueNodeContainer();
	virtual						~ValueNodeContainer();

			status_t			Init();

	inline	bool				Lock()		{ return fLock.Lock(); }
	inline	void				Unlock()	{ fLock.Unlock(); }

			int32				CountChildren() const;
			ValueNodeChild*		ChildAt(int32 index) const;
			bool				AddChild(ValueNodeChild* child);
			void				RemoveChild(ValueNodeChild* child);
			void				RemoveAllChildren();

			bool				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

			// container must be locked

			void				NotifyValueNodeChanged(
									ValueNodeChild* nodeChild,
									ValueNode* oldNode, ValueNode* newNode);
			void				NotifyValueNodeChildrenCreated(ValueNode* node);
			void				NotifyValueNodeChildrenDeleted(ValueNode* node);
			void				NotifyValueNodeValueChanged(ValueNode* node);

private:
			typedef BObjectList<ValueNodeChild> NodeChildList;
			typedef BObjectList<Listener> ListenerList;

private:
			BLocker				fLock;
			NodeChildList		fChildren;
			ListenerList		fListeners;
};


class ValueNodeContainer::Listener {
public:
	virtual						~Listener();

	// container is locked

	virtual	void				ValueNodeChanged(ValueNodeChild* nodeChild,
									ValueNode* oldNode, ValueNode* newNode);
	virtual	void				ValueNodeChildrenCreated(ValueNode* node);
	virtual	void				ValueNodeChildrenDeleted(ValueNode* node);
	virtual	void				ValueNodeValueChanged(ValueNode* node);
};


#endif	// VALUE_NODE_CONTAINER_H
