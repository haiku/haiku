/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VARIABLE_VALUE_NODE_CHILD_H
#define VARIABLE_VALUE_NODE_CHILD_H


#include "ValueNode.h"


class Variable;


class VariableValueNodeChild : public ValueNodeChild {
public:
								VariableValueNodeChild(Variable* variable);
	virtual						~VariableValueNodeChild();

	virtual	const BString&		Name() const;
	virtual	Type*				GetType() const;
	virtual	ValueNode*			Parent() const;

	virtual	status_t			ResolveLocation(ValueLoader* valueLoader,
									ValueLocation*& _location);

private:
			Variable*			fVariable;
};


#endif	// VARIABLE_VALUE_NODE_CHILD_H
