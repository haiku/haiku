/*
 * Copyright 2010, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CSTRING_VALUE_NODE_H
#define CSTRING_VALUE_NODE_H


#include "ValueNode.h"


class AddressType;


class CStringValueNode : public ChildlessValueNode {
public:
								CStringValueNode(ValueNodeChild* nodeChild,
									AddressType* type);
	virtual						~CStringValueNode();

	virtual	Type*				GetType() const;

	virtual	status_t			ResolvedLocationAndValue(
									ValueLoader* valueLoader,
									ValueLocation*& _location,
									Value*& _value);

private:
			AddressType*		fType;
};

#endif	// CSTRING_VALUE_NODE_H
