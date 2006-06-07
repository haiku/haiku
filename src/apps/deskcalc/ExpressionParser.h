/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include <String.h>

class Tokenizer;

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
struct Token;

class ExpressionParser {
 public:
								ExpressionParser();
								~ExpressionParser();

			double				Evaluate(const char* expressionString);

 private:

			double				_ParseBinary();
			double				_ParseSum();
			double				_ParseProduct();
			double				_ParsePower();
			double				_ParseUnary();
			double				_ParseFunction(const Token& token);
			double				_ParseAtom();

			void				_EatToken(int32 type);
			const Function*		_FindFunction(const char* name) const;

			Tokenizer*			fTokenizer;
};

#endif // EXPRESSION_PARSER_H
