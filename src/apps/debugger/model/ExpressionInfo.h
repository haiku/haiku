/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXPRESSION_INFO_H
#define EXPRESSION_INFO_H


#include <String.h>

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>

class Type;
class Value;


class ExpressionInfo : public BReferenceable {
public:
	class Listener;

public:
								ExpressionInfo();
								ExpressionInfo(const ExpressionInfo& other);
								ExpressionInfo(const BString& expression,
									Type* resultType);
	virtual						~ExpressionInfo();

			void				SetTo(const BString& expression,
									Type* resultType);

			const BString&		Expression() const		{ return fExpression; }
			void 				SetExpression(const BString& expression);

			Type*				ResultType() const	{ return fResultType; }
			void				SetResultType(Type* resultType);

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

			void				NotifyExpressionEvaluated(status_t result,
									Value* value);

private:
			typedef 			DoublyLinkedList<Listener> ListenerList;

private:
			BString				fExpression;
			Type*				fResultType;
			ListenerList		fListeners;
};


class ExpressionInfo::Listener : public DoublyLinkedListLinkImpl<Listener> {
public:
	virtual						~Listener();

	virtual	void				ExpressionEvaluated(ExpressionInfo* info,
									status_t result, Value* value) = 0;
};


#endif	// EXPRESSION_INFO_H
