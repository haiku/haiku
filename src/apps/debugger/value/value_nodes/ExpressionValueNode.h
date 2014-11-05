/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXPRESSION_VALUE_NODE_H
#define EXPRESSION_VALUE_NODE_H


#include "ValueNode.h"


class ExpressionValueNodeChild;


class ExpressionValueNode : public ChildlessValueNode {
public:
								ExpressionValueNode(
									ExpressionValueNodeChild* nodeChild,
									Type* type);
	virtual						~ExpressionValueNode();

	virtual	Type*				GetType() const;

	virtual	status_t			ResolvedLocationAndValue(
									ValueLoader* valueLoader,
									ValueLocation*& _location,
									Value*& _value);

private:
			Type*				fType;
};


class ExpressionValueNodeChild : public ValueNodeChild {
public:
								ExpressionValueNodeChild(
									const BString& expression,
									Type* type);
	virtual						~ExpressionValueNodeChild();

	virtual	const BString&		Name() const;
	virtual	Type*				GetType() const;
	virtual	ValueNode*			Parent() const;

	const BString&				GetExpression() const { return fExpression; };

	virtual	status_t			ResolveLocation(ValueLoader* valueLoader,
									ValueLocation*& _location);

private:
			BString				fExpression;
			Type*				fResultType;
};


#endif	// EXPRESSION_VALUE_NODE_H
