/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXPRESSION_INFO_H
#define EXPRESSION_INFO_H


#include <String.h>

#include <Referenceable.h>


class Type;


class ExpressionInfo : public BReferenceable {
public:
								ExpressionInfo();
								ExpressionInfo(const ExpressionInfo& other);
								ExpressionInfo(const BString& expression,
									Type* resultType);
	virtual						~ExpressionInfo();

			void				SetTo(const BString& expression,
									Type* resultType);

			const BString&		Expression() const		{ return fExpression; }
			Type*				ResultType() const	{ return fResultType; }

private:
			BString				fExpression;
			Type*				fResultType;
};


#endif	// EXPRESSION_INFO_H
