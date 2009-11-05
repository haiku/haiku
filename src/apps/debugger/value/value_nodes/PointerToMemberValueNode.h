/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef POINTER_TO_MEMBER_VALUE_NODE_H
#define POINTER_TO_MEMBER_VALUE_NODE_H


#include "ValueNode.h"


class PointerToMemberType;


class PointerToMemberValueNode : public ChildlessValueNode {
public:
								PointerToMemberValueNode(
									ValueNodeChild* nodeChild,
									PointerToMemberType* type);
	virtual						~PointerToMemberValueNode();

	virtual	Type*				GetType() const;

	virtual	status_t			ResolvedLocationAndValue(
									ValueLoader* valueLoader,
									ValueLocation*& _location,
									Value*& _value);

private:
			PointerToMemberType*		fType;
};


#endif	// POINTER_TO_MEMBER_VALUE_NODE_H
