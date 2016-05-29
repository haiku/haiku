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
#ifndef C_LANGUAGE_TOKENIZER
#define C_LANGUAGE_TOKENIZER


#include <String.h>

#include <Variant.h>


namespace CLanguage {


enum {
	TOKEN_NONE					= 0,
	TOKEN_IDENTIFIER,
	TOKEN_CONSTANT,
	TOKEN_END_OF_LINE,

	TOKEN_PLUS,
	TOKEN_MINUS,

	TOKEN_STAR,
	TOKEN_SLASH,
	TOKEN_MODULO,

	TOKEN_OPENING_PAREN,
	TOKEN_CLOSING_PAREN,

	TOKEN_OPENING_SQUARE_BRACKET,
	TOKEN_CLOSING_SQUARE_BRACKET,

	TOKEN_OPENING_CURLY_BRACE,
	TOKEN_CLOSING_CURLY_BRACE,

	TOKEN_ASSIGN,
	TOKEN_LOGICAL_AND,
	TOKEN_LOGICAL_OR,
	TOKEN_LOGICAL_NOT,
	TOKEN_BITWISE_AND,
	TOKEN_BITWISE_OR,
	TOKEN_BITWISE_NOT,
	TOKEN_BITWISE_XOR,
	TOKEN_EQ,
	TOKEN_NE,
	TOKEN_GT,
	TOKEN_GE,
	TOKEN_LT,
	TOKEN_LE,

	TOKEN_BACKSLASH,
	TOKEN_CONDITION,
	TOKEN_COLON,
	TOKEN_SEMICOLON,
	TOKEN_COMMA,
	TOKEN_PERIOD,
	TOKEN_POUND,

	TOKEN_SINGLE_QUOTE,
	TOKEN_DOUBLE_QUOTE,

	TOKEN_STRING_LITERAL,
	TOKEN_BEGIN_COMMENT_BLOCK,
	TOKEN_END_COMMENT_BLOCK,
	TOKEN_INLINE_COMMENT,

	TOKEN_MEMBER_PTR
};


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


struct Token {
								Token();
								Token(const Token& other);
								Token(const char* string, int32 length,
								int32 position, int32 type);
			Token& 	operator=(const Token& other);

	BString						string;
	int32						type;
	BVariant					value;
	int32						position;
};


class Tokenizer {
public:
								Tokenizer();

			void 				SetTo(const char* string);

			const Token& 		NextToken();
			void 				RewindToken();
private:
			bool 				_ParseOperator();
 			char 				_Peek() const;

	static 	bool 				_IsHexDigit(char c);

			Token& 				_ParseHexOperand();
			int32 				_CurrentPos() const;

private:
	BString						fString;
	const char*					fCurrentChar;
	Token						fCurrentToken;
	bool						fReuseToken;
};


}	// namespace CLanguage


#endif	// C_LANGUAGE_TOKENIZER
