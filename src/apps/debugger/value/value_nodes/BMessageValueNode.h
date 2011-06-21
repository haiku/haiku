/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef BMESSAGE_VALUE_NODE_H
#define BMESSAGE_VALUE_NODE_H


#include "ValueNode.h"


class BMessageValueNode : public ValueNode {
public:
								BMessageValueNode(ValueNodeChild* nodeChild,
									Type* type);
	virtual						~BMessageValueNode();

	virtual	Type*				GetType() const;

	virtual	status_t			ResolvedLocationAndValue(
									ValueLoader* valueLoader,
									ValueLocation*& _location,
									Value*& _value);

	virtual status_t			CreateChildren();
	virtual int32				CountChildren() const;
	virtual ValueNodeChild*		ChildAt(int32 index) const;

private:
			Type*				fType;
};

#endif	// BMESSAGE_VALUE_NODE_H
