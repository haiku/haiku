/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2018, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TYPE_HANDLER_H
#define TYPE_HANDLER_H


#include <Referenceable.h>


class Type;
class ValueNode;
class ValueNodeChild;


class TypeHandler : public BReferenceable {
public:
	virtual						~TypeHandler();

	virtual	const char*			Name() const = 0;
	virtual	float				SupportsType(Type* type) const = 0;
	virtual	status_t			CreateValueNode(ValueNodeChild* nodeChild,
									Type* type, ValueNode*& _node) = 0;
									// returns a reference
};


#endif	// TYPE_HANDLER_H
