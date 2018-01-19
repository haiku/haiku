/*
 * Copyright 2012-2018, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLIST_TYPE_HANDLER_H
#define BLIST_TYPE_HANDLER_H


#include "TypeHandler.h"


class BListTypeHandler : public TypeHandler {
public:
	virtual					~BListTypeHandler();

	virtual	const char*		Name() const;
	virtual float			SupportsType(Type* type) const;
	virtual status_t		CreateValueNode(ValueNodeChild* nodeChild,
								Type* type, ValueNode*& _node);
};

#endif // BLIST_TYPE_HANDLER_H
