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
									ValueNodeManager* manager);

 private:
			class Operand;

 private:
			Operand				_ParseSum();
			Operand				_ParseProduct();
			Operand				_ParsePower();
			Operand				_ParseUnary();
			Operand				_ParseIdentifier(ValueNode* parentNode = NULL);
			Operand				_ParseAtom();

			void				_EatToken(int32 type);

			void				_RequestValueIfNeeded(
									const CLanguage::Token& token,
									ValueNodeChild* child);

			CLanguage::Tokenizer* fTokenizer;
			ValueNodeManager*	fNodeManager;
};

#endif // C_LANGUAGE_EXPRESSION_EVALUATOR_H
