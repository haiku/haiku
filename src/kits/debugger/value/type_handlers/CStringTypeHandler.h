/*
 * Copyright 2010, Rene Gollent, rene@gollent.com
 * Distributed under the terms of the MIT License.
 */
#ifndef CSTRING_TYPE_HANDLER_H
#define CSTRING_TYPE_HANDLER_H


#include "TypeHandler.h"


class CStringTypeHandler : public TypeHandler {
public:
	virtual					~CStringTypeHandler();

	virtual float			SupportsType(Type* type);
	virtual status_t		CreateValueNode(ValueNodeChild* nodeChild,
								Type* type, ValueNode*& _node);
};

#endif // CSTRING_TYPE_HANDLER_H
