/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ENUMERATION_VALUE_NODE_H
#define ENUMERATION_VALUE_NODE_H


#include "ValueNode.h"


class EnumerationType;


class EnumerationValueNode : public ChildlessValueNode {
public:
								EnumerationValueNode(ValueNodeChild* nodeChild,
									EnumerationType* type);
	virtual						~EnumerationValueNode();

	virtual	Type*				GetType() const;

	virtual	status_t			ResolvedLocationAndValue(
									ValueLoader* valueLoader,
									ValueLocation*& _location,
									Value*& _value);

private:
			EnumerationType*	fType;
};


#endif	// ENUMERATION_VALUE_NODE_H
