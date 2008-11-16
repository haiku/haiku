/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "m_apm.h"

#include "ExpressionParser.h"

static const int32 kMaxDigits = 64;

enum {
	TOKEN_IDENTIFIER			= 'a',

	TOKEN_CONSTANT				= '0',

	TOKEN_PLUS					= '+',
	TOKEN_MINUS					= '-',

	TOKEN_STAR					= '*',
	TOKEN_SLASH					= '/',
	TOKEN_MODULO				= '%',

	TOKEN_POWER					= '^',

	TOKEN_OPENING_BRACKET		= '(',
	TOKEN_CLOSING_BRACKET		= ')',

	TOKEN_AND					= '&',
	TOKEN_OR					= '|',
	TOKEN_NOT					= '~',

	TOKEN_NONE					= ' ',
	TOKEN_END_OF_LINE			= '\n',
};


struct Token {
	Token()
		: string(""),
		  type(TOKEN_NONE),
		  value(0),
		  position(0)
	{
	}

	Token(const Token& other)
		: string(other.string),
		  type(other.type),
		  value(other.value),
		  position(other.position)
	{
	}

	Token(const char* string, int32 length, int32 position, int32 type)
		: string(string, length),
		  type(type),
		  value(0),
		  position(position)
	{
	}

	Token& operator=(const Token& other)
	{
		string = other.string;
		type = other.type;
		value = other.value;
		position = other.position;
		return *this;
	}

	BString		string;
	int32		type;
	MAPM		value;

	int32		position;
};


class Tokenizer {
 public:
	Tokenizer()
		: fString(""),
		  fCurrentChar(NULL),
		  fCurrentToken(),
		  fReuseToken(false)
	{
	}

	void SetTo(const char* string)
	{
		fString = string;
		fCurrentChar = fString.String();
		fCurrentToken = Token();
		fReuseToken = false;
	}

	const Token& NextToken()
	{
		if (fCurrentToken.type == TOKEN_END_OF_LINE)
			return fCurrentToken;

		if (fReuseToken) {
			fReuseToken = false;
//printf("next token (recycled): '%s'\n", fCurrentToken.string.String());
			return fCurrentToken;
		}

		while (*fCurrentChar != 0 && isspace(*fCurrentChar))
			fCurrentChar++;

		if (*fCurrentChar == 0)
			return fCurrentToken = Token("", 0, _CurrentPos(), TOKEN_END_OF_LINE);

		bool decimal = *fCurrentChar == '.' || *fCurrentChar == ',';

		if (decimal || isdigit(*fCurrentChar)) {

			BString temp;

			const char* begin = fCurrentChar;
			while (*fCurrentChar != 0) {
				if (!isdigit(*fCurrentChar)) {
					if (!(*fCurrentChar == '.' || *fCurrentChar == ','
						|| *fCurrentChar == 'e' || *fCurrentChar == 'E'))
						break;
				}
				if (*fCurrentChar == ',')
					temp << '.';
				else
					temp << *fCurrentChar;
				fCurrentChar++;
			}
			int32 length = fCurrentChar - begin;
			BString test = temp;
			test << "&_";
			double value;
			char t[2];
			int32 matches = sscanf(test.String(), "%lf&%s", &value, t);
			if (matches != 2) {
				throw ParseException("error in constant",
					_CurrentPos() - length);
			}

			fCurrentToken = Token(begin, length, _CurrentPos() - length,
				TOKEN_CONSTANT);
			fCurrentToken.value = temp.String();

		} else if (isalpha(*fCurrentChar)) {

			const char* begin = fCurrentChar;
			while (*fCurrentChar != 0 && (isalpha(*fCurrentChar)
				|| isdigit(*fCurrentChar))) {
				fCurrentChar++;
			}
			int32 length = fCurrentChar - begin;
			fCurrentToken = Token(begin, length, _CurrentPos() - length,
				TOKEN_IDENTIFIER);

		} else {

			switch (*fCurrentChar) {
				case '+':
				case '-':
				case '*':
				case '/':

				case '^':
				case '%':

				case '(':
				case ')':

				case '&':
				case '|':
				case '~':
					fCurrentToken = Token(fCurrentChar, 1, _CurrentPos(),
						*fCurrentChar);
					fCurrentChar++;
					break;
			
				default:
					throw ParseException("unexpected character", _CurrentPos());
			}
		}

//printf("next token: '%s'\n", fCurrentToken.string.String());
		return fCurrentToken;
	}

	void RewindToken()
	{
		fReuseToken = true;
	}

 private:
	int32 _CurrentPos() const
	{
		return fCurrentChar - fString.String();
	}

	BString		fString;
	const char*	fCurrentChar;
	Token		fCurrentToken;
	bool		fReuseToken;
};


ExpressionParser::ExpressionParser()
	: fTokenizer(new Tokenizer())
{
}


ExpressionParser::~ExpressionParser()
{
	delete fTokenizer;
}


BString
ExpressionParser::Evaluate(const char* expressionString)
{
	fTokenizer->SetTo(expressionString);

	MAPM value = _ParseBinary();
	Token token = fTokenizer->NextToken();
	if (token.type != TOKEN_END_OF_LINE)
		throw ParseException("parse error", token.position);

	if (value == 0)
		return BString("0");

	BString result;
	char* buffer = result.LockBuffer(kMaxDigits + 1);
		// + 1 for the decimal point
	if (buffer == NULL)
		throw ParseException("out of memory", 0);

	value.toFixPtString(buffer, kMaxDigits);

	// remove surplus zeros
	int32 lastChar = strlen(buffer) - 1;
	if (strchr(buffer, '.')) {
		while (buffer[lastChar] == '0')
			lastChar--;
		if (buffer[lastChar] == '.')
			lastChar--;
	}
	result.UnlockBuffer(lastChar + 1);

	return result;
}


MAPM
ExpressionParser::_ParseBinary()
{
	return _ParseSum();
	// binary operation appearantly not supported by m_apm library,
	// should not be too hard to implement though....

//	double value = _ParseSum();
//
//	while (true) {
//		Token token = fTokenizer->NextToken();
//		switch (token.type) {
//			case TOKEN_AND:
//				value = (uint64)value & (uint64)_ParseSum();
//				break;
//			case TOKEN_OR:
//				value = (uint64)value | (uint64)_ParseSum();
//				break;
//
//			default:
//				fTokenizer->RewindToken();
//				return value;
//		}
//	}
}


MAPM
ExpressionParser::_ParseSum()
{
	// TODO: check isnan()...
	MAPM value = _ParseProduct();

	while (true) {
		Token token = fTokenizer->NextToken();
		switch (token.type) {
			case TOKEN_PLUS:
				value = value + _ParseProduct();
				break;
			case TOKEN_MINUS:
				value = value - _ParseProduct();
				break;

			default:
				fTokenizer->RewindToken();
				return value;
		}
	}
}


MAPM
ExpressionParser::_ParseProduct()
{
	// TODO: check isnan()...
	MAPM value = _ParsePower();

	while (true) {
		Token token = fTokenizer->NextToken();
		switch (token.type) {
			case TOKEN_STAR:
				value = value * _ParsePower();
				break;
			case TOKEN_SLASH: {
				MAPM rhs = _ParsePower();
				if (rhs == MAPM(0))
					throw ParseException("division by zero", token.position);
				value = value / rhs;
				break;
			}
			case TOKEN_MODULO: {
				MAPM rhs = _ParsePower();
				if (rhs == MAPM(0))
					throw ParseException("modulo by zero", token.position);
				value = value % rhs;
				break;
			}

			default:
				fTokenizer->RewindToken();
				return value;
		}
	}
}


MAPM
ExpressionParser::_ParsePower()
{
	MAPM value = _ParseUnary();

	while (true) {
		Token token = fTokenizer->NextToken();
		if (token.type != TOKEN_POWER) {
			fTokenizer->RewindToken();
			return value;
		}
		value = value.pow(_ParseUnary());
	}
}


MAPM
ExpressionParser::_ParseUnary()
{
	Token token = fTokenizer->NextToken();
	if (token.type == TOKEN_END_OF_LINE)
		throw ParseException("unexpected end of expression", token.position);

	switch (token.type) {
		case TOKEN_PLUS:
			return _ParseUnary();
		case TOKEN_MINUS:
			return -_ParseUnary();
// TODO: Implement !
//		case TOKEN_NOT:
//			return ~(uint64)_ParseUnary();

		case TOKEN_IDENTIFIER:
			return _ParseFunction(token);

		default:
			fTokenizer->RewindToken();
			return _ParseAtom();
	}

	return MAPM(0);
}


struct Function {
	const char*	name;
	int			argumentCount;
	void*		function;
	MAPM		value;
};


void
ExpressionParser::_InitArguments(MAPM values[], int32 argumentCount)
{
	_EatToken(TOKEN_OPENING_BRACKET);

	for (int32 i = 0; i < argumentCount; i++)
		values[i] = _ParseBinary();

	_EatToken(TOKEN_CLOSING_BRACKET);
}


MAPM
ExpressionParser::_ParseFunction(const Token& token)
{
	if (strcasecmp("e", token.string.String()) == 0)
		return MAPM(M_E);
	else if (strcasecmp("pi", token.string.String()) == 0)
		return MAPM(M_PI);

	// hard coded cases for different count of arguments
	// supports functions with 3 arguments at most

	MAPM values[3];

	if (strcasecmp("abs", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].abs();
	} else if (strcasecmp("acos", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].acos();
	} else if (strcasecmp("asin", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].asin();
	} else if (strcasecmp("atan", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].atan();
	} else if (strcasecmp("atan2", token.string.String()) == 0) {
		_InitArguments(values, 2);
		return values[0].atan2(values[1]);
	} else if (strcasecmp("ceil", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].ceil();
	} else if (strcasecmp("cos", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].cos();
	} else if (strcasecmp("cosh", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].cosh();
	} else if (strcasecmp("exp", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].exp();
	} else if (strcasecmp("floor", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].floor();
	} else if (strcasecmp("log", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].log();
	} else if (strcasecmp("log10", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].log10();
	} else if (strcasecmp("pow", token.string.String()) == 0) {
		_InitArguments(values, 2);
		return values[0].pow(values[1]);
	} else if (strcasecmp("sin", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].sin();
	} else if (strcasecmp("sinh", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].sinh();
	} else if (strcasecmp("sqrt", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].sqrt();
	} else if (strcasecmp("tan", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].tan();
	} else if (strcasecmp("tanh", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return values[0].tanh();
	}

	throw ParseException("unknown identifier", token.position);
}


MAPM
ExpressionParser::_ParseAtom()
{
	Token token = fTokenizer->NextToken();
	if (token.type == TOKEN_END_OF_LINE)
		throw ParseException("unexpected end of expression", token.position);

	if (token.type == TOKEN_CONSTANT)
		return token.value;

	fTokenizer->RewindToken();

	_EatToken(TOKEN_OPENING_BRACKET);

	MAPM value = _ParseBinary();

	_EatToken(TOKEN_CLOSING_BRACKET);

	return value;
}


void
ExpressionParser::_EatToken(int32 type)
{
	Token token = fTokenizer->NextToken();
	if (token.type != type) {
		BString temp("expected '");
		temp << (char)type << "' got '" << token.string << "'";
		throw ParseException(temp.String(), token.position);
	}
}

