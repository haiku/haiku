/*
 * Copyright 2006-2014 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Rene Gollent <rene@gollent.com>
 *		John Scipione <jscipione@gmail.com>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef C_LANGUAGE_EXPRESSION_EVALUATOR_H
#define C_LANGUAGE_EXPRESSION_EVALUATOR_H


#include <String.h>


namespace CLanguage {
	struct Token;
	class Tokenizer;
}

class BVariant;
class TeamTypeInformation;
class Type;
class ValueNode;
class ValueNodeChild;
class ValueNodeManager;
class Variable;


class ValueNeededException {
public:
	ValueNeededException(ValueNode* node)
		:
		value(node)
	{
	}

	ValueNode* value;
};


class ExpressionResult;
class Number;


class CLanguageExpressionEvaluator {

public:
								CLanguageExpressionEvaluator();
								~CLanguageExpressionEvaluator();

			ExpressionResult*	Evaluate(const char* expressionString,
									ValueNodeManager* manager,
									TeamTypeInformation* info);

private:
 			class InternalVariableID;
			class Operand;

private:
			Operand				_ParseSum();
			Operand				_ParseProduct();
			Operand				_ParseUnary();
			Operand				_ParseIdentifier(ValueNode* parentNode = NULL);
			Operand				_ParseAtom();

			void				_EatToken(int32 type);

			Operand				_ParseType(Type* baseType);
									// the passed in Type object
									// is expected to be the initial
									// base type that was recognized by
									// e.g. ParseIdentifier. This function then
									// takes care of handling any modifiers
									// that go with it, and returns a
									// corresponding final type.

			void				_RequestValueIfNeeded(
									const CLanguage::Token& token,
									ValueNodeChild* child);

			void				_GetNodeChildForPrimitive(
									const CLanguage::Token& token,
									const BVariant& value,
									ValueNodeChild*& _output) const;

private:
			CLanguage::Tokenizer* fTokenizer;
			TeamTypeInformation* fTypeInfo;
			ValueNodeManager*	fNodeManager;
};

#endif // C_LANGUAGE_EXPRESSION_EVALUATOR_H
