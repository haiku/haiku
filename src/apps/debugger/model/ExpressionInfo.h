/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXPRESSION_INFO_H
#define EXPRESSION_INFO_H


#include <String.h>

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>
#include <Variant.h>


class Type;
class Value;
class ValueNodeChild;


enum expression_result_kind {
	EXPRESSION_RESULT_KIND_UNKNOWN = 0,
	EXPRESSION_RESULT_KIND_PRIMITIVE,
	EXPRESSION_RESULT_KIND_VALUE_NODE,
	EXPRESSION_RESULT_KIND_TYPE
};


class ExpressionResult : public BReferenceable {
public:
								ExpressionResult();
	virtual						~ExpressionResult();


			expression_result_kind Kind() const { return fResultKind; }

			Value*				PrimitiveValue() const
									{ return fPrimitiveValue; }
			ValueNodeChild*		ValueNodeValue() const
									{ return fValueNodeValue; }
			Type*				GetType() const
									{ return fTypeResult; }

			void				SetToPrimitive(Value* value);
			void				SetToValueNode(ValueNodeChild* child);
			void				SetToType(Type* type);
private:
			void				_Unset();

private:
			expression_result_kind fResultKind;
			Value*				fPrimitiveValue;
			ValueNodeChild*		fValueNodeValue;
			Type*				fTypeResult;
};


class ExpressionInfo : public BReferenceable {
public:
	class Listener;

public:
								ExpressionInfo();
								ExpressionInfo(const ExpressionInfo& other);
								ExpressionInfo(const BString& expression);
	virtual						~ExpressionInfo();

			void				SetTo(const BString& expression);

			const BString&		Expression() const		{ return fExpression; }

			void				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

			void				NotifyExpressionEvaluated(status_t result,
									ExpressionResult* value);

private:
			typedef 			DoublyLinkedList<Listener> ListenerList;

private:
			BString				fExpression;
			ListenerList		fListeners;
};


class ExpressionInfo::Listener : public DoublyLinkedListLinkImpl<Listener> {
public:
	virtual						~Listener();

	virtual	void				ExpressionEvaluated(ExpressionInfo* info,
									status_t result,
									ExpressionResult* value) = 0;
};


#endif	// EXPRESSION_INFO_H
