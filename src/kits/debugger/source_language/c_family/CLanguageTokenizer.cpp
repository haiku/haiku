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


#include "CLanguageTokenizer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>


using CLanguage::ParseException;
using CLanguage::Token;
using CLanguage::Tokenizer;


// #pragma mark - Token


Token::Token()
	:
	string(""),
	type(TOKEN_NONE),
	value(),
	position(0)
{
}


Token::Token(const Token& other)
	:
	string(other.string),
	type(other.type),
	value(other.value),
	position(other.position)
{
}


Token::Token(const char* string, int32 length, int32 position, int32 type)
	:
	string(string, length),
	type(type),
	value(),
	position(position)
{
}


Token&
Token::operator=(const Token& other)
{
	string = other.string;
	type = other.type;
	value = other.value;
	position = other.position;
	return *this;
}


// #pragma mark - Tokenizer


Tokenizer::Tokenizer()
	:
	fString(""),
	fCurrentChar(NULL),
	fCurrentToken(),
	fReuseToken(false)
{
}


void
Tokenizer::SetTo(const char* string)
{
	fString = string;
	fCurrentChar = fString.String();
	fCurrentToken = Token();
	fReuseToken = false;
}


const Token&
Tokenizer::NextToken()
{
	if (fCurrentToken.type == TOKEN_END_OF_LINE)
		return fCurrentToken;

	if (fReuseToken) {
		fReuseToken = false;
		return fCurrentToken;
	}

	while (*fCurrentChar != 0 && isspace(*fCurrentChar))
		fCurrentChar++;

	if (*fCurrentChar == 0) {
		return fCurrentToken = Token("", 0, _CurrentPos(),
			TOKEN_END_OF_LINE);
	}

	bool decimal = *fCurrentChar == '.';

	if (decimal || isdigit(*fCurrentChar)) {
		if (*fCurrentChar == '0' && fCurrentChar[1] == 'x')
			return _ParseHexOperand();

		BString temp;

		const char* begin = fCurrentChar;

		// optional digits before the comma
		while (isdigit(*fCurrentChar)) {
			temp << *fCurrentChar;
			fCurrentChar++;
		}

		// optional post decimal part
		// (required if there are no digits before the decimal)
		if (*fCurrentChar == '.') {
			decimal = true;
			temp << '.';
			fCurrentChar++;

			// optional post decimal digits
			while (isdigit(*fCurrentChar)) {
				temp << *fCurrentChar;
				fCurrentChar++;
			}
		}

		int32 length = fCurrentChar - begin;
		if (length == 1 && decimal) {
			// check for . operator
			fCurrentChar = begin;
			if (!_ParseOperator())
				throw ParseException("unexpected character", _CurrentPos());

			return fCurrentToken;
		}

		BString test = temp;
		test << "&_";
		double value;
		char t[2];
		int32 matches = sscanf(test.String(), "%lf&%s", &value, t);
		if (matches != 2)
			throw ParseException("error in constant", _CurrentPos() - length);

		fCurrentToken = Token(begin, length, _CurrentPos() - length,
			TOKEN_CONSTANT);
		if (decimal)
			fCurrentToken.value.SetTo(value);
		else
			fCurrentToken.value.SetTo((int64)strtoll(temp.String(), NULL, 10));
	} else if (isalpha(*fCurrentChar) || *fCurrentChar == '_') {
		const char* begin = fCurrentChar;
		while (*fCurrentChar != 0 && (isalpha(*fCurrentChar)
			|| isdigit(*fCurrentChar) || *fCurrentChar == '_')) {
			fCurrentChar++;
		}
		int32 length = fCurrentChar - begin;
		fCurrentToken = Token(begin, length, _CurrentPos() - length,
			TOKEN_IDENTIFIER);
	} else if (*fCurrentChar == '"' || *fCurrentChar == '\'') {
		bool terminatorFound = false;
		const char* begin = fCurrentChar++;
		while (*fCurrentChar != 0) {
			if (*fCurrentChar == '\\') {
				if (*(fCurrentChar++) != 0)
					fCurrentChar++;
			} else if (*(fCurrentChar++) == *begin) {
				terminatorFound = true;
				break;
			}
		}
		int32 tokenType = TOKEN_STRING_LITERAL;
		if (!terminatorFound) {
			tokenType = *begin == '"' ? TOKEN_DOUBLE_QUOTE
					: TOKEN_SINGLE_QUOTE;
			fCurrentChar = begin + 1;
		}

		int32 length = fCurrentChar - begin;
		fCurrentToken = Token(begin, length, _CurrentPos() - length,
			tokenType);
	} else {
		if (!_ParseOperator()) {
			int32 type = TOKEN_NONE;
			switch (*fCurrentChar) {
				case '\n':
					type = TOKEN_END_OF_LINE;
					break;

				case '(':
					type = TOKEN_OPENING_PAREN;
					break;
				case ')':
					type = TOKEN_CLOSING_PAREN;
					break;

				case '[':
					type = TOKEN_OPENING_SQUARE_BRACKET;
					break;
				case ']':
					type = TOKEN_CLOSING_SQUARE_BRACKET;
					break;

				case '{':
					type = TOKEN_OPENING_CURLY_BRACE;
					break;
				case '}':
					type = TOKEN_CLOSING_CURLY_BRACE;
					break;

				case '\\':
					type = TOKEN_BACKSLASH;
					break;

				case ':':
					type = TOKEN_COLON;
					break;

				case ';':
					type = TOKEN_SEMICOLON;
					break;

				case ',':
					type = TOKEN_COMMA;
					break;

				case '.':
					type = TOKEN_PERIOD;
					break;

				case '#':
					type = TOKEN_POUND;
					break;

				default:
					throw ParseException("unexpected character",
						_CurrentPos());
			}
			fCurrentToken = Token(fCurrentChar, 1, _CurrentPos(),
				type);
			fCurrentChar++;
		}
	}

	return fCurrentToken;
}


bool
Tokenizer::_ParseOperator()
{
	int32 type = TOKEN_NONE;
	int32 length = 0;
	switch (*fCurrentChar) {
		case '+':
			type = TOKEN_PLUS;
			length = 1;
			break;

		case '-':
			 if (_Peek() == '>') {
			 	type = TOKEN_MEMBER_PTR;
			 	length = 2;
			 } else {
				type = TOKEN_MINUS;
				length = 1;
			 }
			break;

		case '*':
			switch (_Peek()) {
				case '/':
					type = TOKEN_END_COMMENT_BLOCK;
					length = 2;
					break;
				default:
					type = TOKEN_STAR;
					length = 1;
					break;
			}
			break;

		case '/':
			switch (_Peek()) {
				case '*':
					type = TOKEN_BEGIN_COMMENT_BLOCK;
					length = 2;
					break;
				case '/':
					type = TOKEN_INLINE_COMMENT;
					length = 2;
					break;
				default:
					type = TOKEN_SLASH;
					length = 1;
					break;
			}
			break;

		case '%':
			type = TOKEN_MODULO;
			length = 1;
			break;

		case '^':
			type = TOKEN_BITWISE_XOR;
			length = 1;
			break;

		case '&':
			if (_Peek() == '&') {
			 	type = TOKEN_LOGICAL_AND;
			 	length = 2;
			} else {
				type = TOKEN_BITWISE_AND;
				length = 1;
			}
			break;

		case '|':
			if (_Peek() == '|') {
				type = TOKEN_LOGICAL_OR;
				length = 2;
			} else {
				type = TOKEN_BITWISE_OR;
				length = 1;
			}
			break;

		case '!':
			if (_Peek() == '=') {
				type = TOKEN_NE;
				length = 2;
			} else {
				type = TOKEN_LOGICAL_NOT;
				length = 1;
			}
			break;

		case '=':
			if (_Peek() == '=') {
				type = TOKEN_EQ;
				length = 2;
			} else {
				type = TOKEN_ASSIGN;
				length = 1;
			}
			break;

		case '>':
			if (_Peek() == '=') {
				type = TOKEN_GE;
				length = 2;
			} else {
				type = TOKEN_GT;
				length = 1;
			}
			break;

		case '<':
			if (_Peek() == '=') {
				type = TOKEN_LE;
				length = 2;
			} else {
				type = TOKEN_LT;
				length = 1;
			}
			break;

		case '~':
			type = TOKEN_BITWISE_NOT;
			length = 1;
			break;


		case '?':
			type = TOKEN_CONDITION;
			length = 1;
			break;

		case '.':
			type = TOKEN_MEMBER_PTR;
			length = 1;
			break;

		default:
			break;
	}

	if (length == 0)
		return false;

	fCurrentToken = Token(fCurrentChar, length, _CurrentPos(), type);
	fCurrentChar += length;

	return true;
}


void
Tokenizer::RewindToken()
{
	fReuseToken = true;
}


char
Tokenizer::_Peek() const
{
	if (_CurrentPos() < fString.Length())
		return *(fCurrentChar + 1);

	return '\0';
}


/*static*/ bool
Tokenizer::_IsHexDigit(char c)
{
	return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}


Token&
Tokenizer::_ParseHexOperand()
{
	const char* begin = fCurrentChar;
	fCurrentChar += 2;
		// skip "0x"

	if (!_IsHexDigit(*fCurrentChar))
		throw ParseException("expected hex digit", _CurrentPos());

	fCurrentChar++;
	while (_IsHexDigit(*fCurrentChar))
		fCurrentChar++;

	int32 length = fCurrentChar - begin;
	fCurrentToken = Token(begin, length, _CurrentPos() - length,
		TOKEN_CONSTANT);

	if (length <= 10) {
		// including the leading 0x, a 32-bit constant will be at most
		// 10 characters. Anything larger, and 64 is necessary.
		fCurrentToken.value.SetTo((uint32)strtoul(
			fCurrentToken.string.String(), NULL, 16));
	} else {
		fCurrentToken.value.SetTo((uint64)strtoull(
			fCurrentToken.string.String(), NULL, 16));
	}
	return fCurrentToken;
}


int32
Tokenizer::_CurrentPos() const
{
	return fCurrentChar - fString.String();
}
