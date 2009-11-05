/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PRIMITIVE_VALUE_NODE_H
#define PRIMITIVE_VALUE_NODE_H


#include "ValueNode.h"


class PrimitiveType;


class PrimitiveValueNode : public ChildlessValueNode {
public:
								PrimitiveValueNode(ValueNodeChild* nodeChild,
									PrimitiveType* type);
	virtual						~PrimitiveValueNode();

	virtual	Type*				GetType() const;

	virtual	status_t			ResolvedLocationAndValue(
									ValueLoader* valueLoader,
									ValueLocation*& _location,
									Value*& _value);

private:
			PrimitiveType*		fType;
};


#endif	// PRIMITIVE_VALUE_NODE_H
