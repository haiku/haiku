/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef VALUE_NODE_MANAGER_H
#define VALUE_NODE_MANAGER_H

#include <Referenceable.h>

#include "ValueNodeContainer.h"

class StackFrame;
class Thread;
class Variable;


class ValueNodeManager : public BReferenceable,
	private ValueNodeContainer::Listener {
public:
								ValueNodeManager();
	virtual						~ValueNodeManager();

			status_t			SetStackFrame(Thread* thread,
									StackFrame* frame);

			bool				AddListener(
									ValueNodeContainer::Listener* listener);
			void				RemoveListener(
									ValueNodeContainer::Listener* listener);

	virtual	void				ValueNodeChanged(ValueNodeChild* nodeChild,
									ValueNode* oldNode, ValueNode* newNode);
	virtual	void				ValueNodeChildrenCreated(ValueNode* node);
	virtual	void				ValueNodeChildrenDeleted(ValueNode* node);
	virtual	void				ValueNodeValueChanged(ValueNode* node);

			ValueNodeContainer*	GetContainer() const { return fContainer; };

			status_t			AddChildNodes(ValueNodeChild* nodeChild);

private:
			typedef BObjectList<ValueNodeContainer::Listener> ListenerList;

			void				_AddNode(Variable* variable);
			status_t			_CreateValueNode(ValueNodeChild* nodeChild);

private:
			ValueNodeContainer*	fContainer;
			StackFrame*			fStackFrame;
			Thread*				fThread;
			ListenerList		fListeners;
};

#endif	// VALUE_NODE_MANAGER_H
