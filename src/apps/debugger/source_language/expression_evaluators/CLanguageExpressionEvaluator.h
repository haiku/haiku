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


class ValueNode;
class ValueNodeManager;


class ParseException {
 public:
	ParseException(const char* message, int32 position)
		: message(message),
		  position(position)
	{
	}

	ParseException(const ParseException& other)
		: message(other.message),
		  position(other.position)
	{
	}

	BString	message;
	int32	position;
};

class ValueNeededException {
public:
	ValueNeededException(ValueNode* node)
		:
		value(node)
	{
	}

	ValueNode* value;
};


class Number;


class CLanguageExpressionEvaluator {

 public:
								CLanguageExpressionEvaluator();
								~CLanguageExpressionEvaluator();

			Number				Evaluate(const char* expressionString,
									type_code type, ValueNodeManager* manager);

 private:
			struct Token;
			class Tokenizer;

 private:
			Number				_ParseBinary();
			Number				_ParseSum();
			Number				_ParseProduct();
			Number				_ParsePower();
			Number				_ParseUnary();
			Number				_ParseIdentifier();
			Number				_ParseAtom();

			void				_EatToken(int32 type);

			Tokenizer*			fTokenizer;
			type_code			fCurrentType;
			ValueNodeManager*	fNodeManager;
};

#endif // C_LANGUAGE_EXPRESSION_EVALUATOR_H
