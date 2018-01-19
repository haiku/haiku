/*
 * Copyright 2011-2018, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef BMESSAGE_TYPE_HANDLER_H
#define BMESSAGE_TYPE_HANDLER_H


#include "TypeHandler.h"


class BMessageTypeHandler : public TypeHandler {
public:
	virtual					~BMessageTypeHandler();

	virtual	const char*		Name() const;
	virtual float			SupportsType(Type* type) const;
	virtual status_t		CreateValueNode(ValueNodeChild* nodeChild,
								Type* type, ValueNode*& _node);
};

#endif // BMESSAGE_TYPE_HANDLER_H
