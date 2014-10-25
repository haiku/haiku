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

struct Function;
class MAPM;

class CLanguageExpressionEvaluator {

 public:
								CLanguageExpressionEvaluator();
								~CLanguageExpressionEvaluator();

			void				SetSupportHexInput(bool enabled);

			BString				Evaluate(const char* expressionString);
			int64				EvaluateToInt64(const char* expressionString);
			double				EvaluateToDouble(const char* expressionString);

 private:
			struct Token;
			class Tokenizer;

 private:
			MAPM				_ParseBinary();
			MAPM				_ParseSum();
			MAPM				_ParseProduct();
			MAPM				_ParsePower();
			MAPM				_ParseUnary();
			MAPM				_ParseIdentifier();
			void				_InitArguments(MAPM values[],
									int32 argumentCount);
			MAPM				_ParseAtom();

			void				_EatToken(int32 type);

			Tokenizer*			fTokenizer;
};

#endif // C_LANGUAGE_EXPRESSION_EVALUATOR_H
