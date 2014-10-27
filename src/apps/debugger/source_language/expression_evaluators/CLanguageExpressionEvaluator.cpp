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

#include "CLanguageExpressionEvaluator.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <m_apm.h>


static const int32 kMaxDecimalPlaces = 32;


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

	TOKEN_POWER,

	TOKEN_OPENING_BRACKET,
	TOKEN_CLOSING_BRACKET,

	TOKEN_LOGICAL_AND,
	TOKEN_LOGICAL_OR,
	TOKEN_LOGICAL_NOT,
	TOKEN_BITWISE_AND,
	TOKEN_BITWISE_OR,
	TOKEN_BITWISE_NOT,
	TOKEN_EQ,
	TOKEN_NE,
	TOKEN_GT,
	TOKEN_GE,
	TOKEN_LT,
	TOKEN_LE
};

static BString TokenTypeToString(int32 type)
{
	BString token;

	switch (type) {
		case TOKEN_PLUS:
			token = "+";
			break;

		case TOKEN_MINUS:
			token = "-";
			break;

		case TOKEN_STAR:
			token = "*";
			break;

		case TOKEN_SLASH:
			token = "/";
			break;

		case TOKEN_MODULO:
			token = "%";
			break;

		case TOKEN_POWER:
			token = "^";
			break;

		case TOKEN_OPENING_BRACKET:
			token = "(";
			break;

		case TOKEN_CLOSING_BRACKET:
			token = ")";
			break;

		case TOKEN_LOGICAL_AND:
			token = "&&";
			break;

		case TOKEN_LOGICAL_OR:
			token = "||";
			break;

		case TOKEN_LOGICAL_NOT:
			token = "!";
			break;

		case TOKEN_BITWISE_AND:
			token = "&";
			break;

		case TOKEN_BITWISE_OR:
			token = "|";
			break;

		case TOKEN_BITWISE_NOT:
			token = "~";
			break;

		case TOKEN_EQ:
			token = "==";
			break;

		case TOKEN_NE:
			token = "!=";
			break;

		case TOKEN_GT:
			token = ">";
			break;

		case TOKEN_GE:
			token = ">=";
			break;

		case TOKEN_LT:
			token = "<";
			break;

		case TOKEN_LE:
			token = "<=";
			break;

		default:
			token.SetToFormat("Unknown token type %" B_PRId32, type);
			break;
	}

	return token;
}


struct CLanguageExpressionEvaluator::Token {
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


class CLanguageExpressionEvaluator::Tokenizer {
 public:
	Tokenizer()
		: fString(""),
		  fCurrentChar(NULL),
		  fCurrentToken(),
		  fReuseToken(false),
		  fHexSupport(false)
	{
	}

	void SetSupportHexInput(bool enabled)
	{
		fHexSupport = enabled;
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
			if (fHexSupport && *fCurrentChar == '0' && fCurrentChar[1] == 'x')
				return _ParseHexNumber();

			BString temp;

			const char* begin = fCurrentChar;

			// optional digits before the comma
			while (isdigit(*fCurrentChar)) {
				temp << *fCurrentChar;
				fCurrentChar++;
			}

			// optional post comma part
			// (required if there are no digits before the comma)
			if (*fCurrentChar == '.' || *fCurrentChar == ',') {
				temp << '.';
				fCurrentChar++;

				// optional post comma digits
				while (isdigit(*fCurrentChar)) {
					temp << *fCurrentChar;
					fCurrentChar++;
				}
			}

			// optional exponent part
			if (*fCurrentChar == 'E') {
				temp << *fCurrentChar;
				fCurrentChar++;

				// optional exponent sign
				if (*fCurrentChar == '+' || *fCurrentChar == '-') {
					temp << *fCurrentChar;
					fCurrentChar++;
				}

				// required exponent digits
				if (!isdigit(*fCurrentChar)) {
					throw ParseException("missing exponent in constant",
						fCurrentChar - begin);
				}

				while (isdigit(*fCurrentChar)) {
					temp << *fCurrentChar;
					fCurrentChar++;
				}
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
		} else if (isalpha(*fCurrentChar) && *fCurrentChar != 'x') {
			const char* begin = fCurrentChar;
			while (*fCurrentChar != 0 && (isalpha(*fCurrentChar)
				|| isdigit(*fCurrentChar))) {
				fCurrentChar++;
			}
			int32 length = fCurrentChar - begin;
			fCurrentToken = Token(begin, length, _CurrentPos() - length,
				TOKEN_IDENTIFIER);
		} else if (strncmp(fCurrentChar, "π", 2) == 0) {
			fCurrentToken = Token(fCurrentChar, 2, _CurrentPos() - 1,
				TOKEN_IDENTIFIER);
			fCurrentChar += 2;
		} else {
			if (!_ParseOperator()) {
				int32 type = TOKEN_NONE;
				switch (*fCurrentChar) {
					case '\n':
						type = TOKEN_END_OF_LINE;
						break;

					case '(':
						type = TOKEN_OPENING_BRACKET;
						break;
					case ')':
						type = TOKEN_CLOSING_BRACKET;
						break;

					case '\\':
					case ':':
						type = TOKEN_SLASH;
						break;

					case 'x':
						if (!fHexSupport) {
							type = TOKEN_STAR;
							break;
						}
						// fall through

					default:
						throw ParseException("unexpected character",
							_CurrentPos());
				}
				fCurrentToken = Token(fCurrentChar, 1, _CurrentPos(), type);
				fCurrentChar++;
			}
		}

//printf("next token: '%s'\n", fCurrentToken.string.String());
		return fCurrentToken;
	}

	bool _ParseOperator()
	{
		int32 type = TOKEN_NONE;
		int32 length = 0;
		switch (*fCurrentChar) {
			case '+':
				type = TOKEN_PLUS;
				length = 1;
				break;

			case '-':
				type = TOKEN_MINUS;
				length = 1;
				break;

			case '*':
				type = TOKEN_STAR;
				length = 1;
				break;

			case '/':
				type = TOKEN_SLASH;
				length = 1;
				break;

			case '%':
				type = TOKEN_MODULO;
				length = 1;
				break;

			case '^':
				type = TOKEN_POWER;
				length = 1;
				break;

			case '&':
				if (Peek() == '&') {
				 	type = TOKEN_LOGICAL_AND;
				 	length = 2;
				} else {
					type = TOKEN_BITWISE_AND;
					length = 1;
				}
				break;

			case '|':
				if (Peek() == '|') {
					type = TOKEN_LOGICAL_OR;
					length = 2;
				} else {
					type = TOKEN_BITWISE_OR;
					length = 1;
				}
				break;

			case '!':
				if (Peek() == '=') {
					type = TOKEN_NE;
					length = 2;
				} else {
					type = TOKEN_LOGICAL_NOT;
					length = 1;
				}
				break;

			case '=':
				if (Peek() == '=') {
					type = TOKEN_EQ;
					length = 2;
				}
				break;

			case '>':
				if (Peek() == '=') {
					type = TOKEN_GE;
					length = 2;
				} else {
					type = TOKEN_GT;
					length = 1;
				}
				break;

			case '<':
				if (Peek() == '=') {
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

			default:
				break;
		}

		if (length == 0)
			return false;

		fCurrentToken = Token(fCurrentChar, length, _CurrentPos(), type);
		fCurrentChar += length;

		return true;
	}

	void RewindToken()
	{
		fReuseToken = true;
	}

 private:
 	char Peek() const
 	{
 		if (_CurrentPos() < fString.Length())
 			return *(fCurrentChar + 1);

 		return '\0';
 	}

	static bool _IsHexDigit(char c)
	{
		return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
	}

	Token& _ParseHexNumber()
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

		// MAPM has no conversion from long long, so we need to improvise.
		uint64 value = strtoll(fCurrentToken.string.String(), NULL, 0);
		if (value <= 0x7fffffff) {
			fCurrentToken.value = (long)value;
		} else {
			fCurrentToken.value = (int)(value >> 60);
			fCurrentToken.value *= 1 << 30;
			fCurrentToken.value += (int)((value >> 30) & 0x3fffffff);
			fCurrentToken.value *= 1 << 30;
			fCurrentToken.value += (int)(value& 0x3fffffff);
		}

		return fCurrentToken;
	}

	int32 _CurrentPos() const
	{
		return fCurrentChar - fString.String();
	}

	BString		fString;
	const char*	fCurrentChar;
	Token		fCurrentToken;
	bool		fReuseToken;
	bool		fHexSupport;
};


CLanguageExpressionEvaluator::CLanguageExpressionEvaluator()
	:	fTokenizer(new Tokenizer())
{
}


CLanguageExpressionEvaluator::~CLanguageExpressionEvaluator()
{
	delete fTokenizer;
}


void
CLanguageExpressionEvaluator::SetSupportHexInput(bool enabled)
{
	fTokenizer->SetSupportHexInput(enabled);
}


BString
CLanguageExpressionEvaluator::Evaluate(const char* expressionString)
{
	fTokenizer->SetTo(expressionString);

	MAPM value = _ParseBinary();
	Token token = fTokenizer->NextToken();
	if (token.type != TOKEN_END_OF_LINE)
		throw ParseException("parse error", token.position);

	if (value == 0)
		return BString("0");

	char* buffer = value.toFixPtStringExp(kMaxDecimalPlaces, '.', 0, 0);
	if (buffer == NULL)
		throw ParseException("out of memory", 0);

	// remove surplus zeros
	int32 lastChar = strlen(buffer) - 1;
	if (strchr(buffer, '.')) {
		while (buffer[lastChar] == '0')
			lastChar--;
		if (buffer[lastChar] == '.')
			lastChar--;
	}

	BString result(buffer, lastChar + 1);
	free(buffer);
	return result;
}


int64
CLanguageExpressionEvaluator::EvaluateToInt64(const char* expressionString)
{
	fTokenizer->SetTo(expressionString);

	MAPM value = _ParseBinary();
	Token token = fTokenizer->NextToken();
	if (token.type != TOKEN_END_OF_LINE)
		throw ParseException("parse error", token.position);

	char buffer[128];
	value.toIntegerString(buffer);

	return strtoll(buffer, NULL, 0);
}


double
CLanguageExpressionEvaluator::EvaluateToDouble(const char* expressionString)
{
	fTokenizer->SetTo(expressionString);

	MAPM value = _ParseBinary();
	Token token = fTokenizer->NextToken();
	if (token.type != TOKEN_END_OF_LINE)
		throw ParseException("parse error", token.position);

	char buffer[1024];
	value.toString(buffer, sizeof(buffer) - 4);

	return strtod(buffer, NULL);
}


MAPM
CLanguageExpressionEvaluator::_ParseBinary()
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
CLanguageExpressionEvaluator::_ParseSum()
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
CLanguageExpressionEvaluator::_ParseProduct()
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
CLanguageExpressionEvaluator::_ParsePower()
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
CLanguageExpressionEvaluator::_ParseUnary()
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
			return _ParseIdentifier();

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


MAPM
CLanguageExpressionEvaluator::_ParseIdentifier()
{
	throw ParseException("Identifiers not implemented", 0);

	return MAPM(0);
}


void
CLanguageExpressionEvaluator::_InitArguments(MAPM values[], int32 argumentCount)
{
	_EatToken(TOKEN_OPENING_BRACKET);

	for (int32 i = 0; i < argumentCount; i++)
		values[i] = _ParseBinary();

	_EatToken(TOKEN_CLOSING_BRACKET);
}


MAPM
CLanguageExpressionEvaluator::_ParseAtom()
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
CLanguageExpressionEvaluator::_EatToken(int32 type)
{
	Token token = fTokenizer->NextToken();
	if (token.type != type) {
		BString expected;
		switch (type) {
			case TOKEN_IDENTIFIER:
				expected = "an identifier";
				break;

			case TOKEN_CONSTANT:
				expected = "a constant";
				break;

			case TOKEN_PLUS:
			case TOKEN_MINUS:
			case TOKEN_STAR:
			case TOKEN_MODULO:
			case TOKEN_POWER:
			case TOKEN_OPENING_BRACKET:
			case TOKEN_CLOSING_BRACKET:
			case TOKEN_LOGICAL_AND:
			case TOKEN_BITWISE_AND:
			case TOKEN_LOGICAL_OR:
			case TOKEN_BITWISE_OR:
			case TOKEN_LOGICAL_NOT:
			case TOKEN_BITWISE_NOT:
			case TOKEN_EQ:
			case TOKEN_NE:
			case TOKEN_GT:
			case TOKEN_GE:
			case TOKEN_LT:
			case TOKEN_LE:
				expected << "'" << TokenTypeToString(type) << "'";
				break;

			case TOKEN_SLASH:
				expected = "'/', '\\', or ':'";
				break;

			case TOKEN_END_OF_LINE:
				expected = "'\\n'";
				break;
		}
		BString temp;
		temp << "Expected " << expected.String() << " got '" << token.string << "'";
		throw ParseException(temp.String(), token.position);
	}
}
