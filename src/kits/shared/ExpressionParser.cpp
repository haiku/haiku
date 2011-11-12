/*
 * Copyright 2006-2011 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <ExpressionParser.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <m_apm.h>


static const int32 kMaxDecimalPlaces = 32;

enum {
	TOKEN_IDENTIFIER			= 0,

	TOKEN_CONSTANT,

	TOKEN_PLUS,
	TOKEN_MINUS,

	TOKEN_STAR,
	TOKEN_SLASH,
	TOKEN_MODULO,

	TOKEN_POWER,
	TOKEN_FACTORIAL,

	TOKEN_OPENING_BRACKET,
	TOKEN_CLOSING_BRACKET,

	TOKEN_AND,
	TOKEN_OR,
	TOKEN_NOT,

	TOKEN_NONE,
	TOKEN_END_OF_LINE
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

		} else if ((unsigned char)fCurrentChar[0] == 0xCF
			&& (unsigned char)fCurrentChar[1] == 0x80) {
			// UTF-8 small greek letter PI
			fCurrentToken = Token(fCurrentChar, 2, _CurrentPos() - 1,
				TOKEN_IDENTIFIER);
			fCurrentChar += 2;

		} else {
			int32 type = TOKEN_NONE;

			switch (*fCurrentChar) {
				case '+':
					type = TOKEN_PLUS;
					break;
				case '-':
					type = TOKEN_MINUS;
					break;
				case '*':
					type = TOKEN_STAR;
					break;
				case '/':
				case '\\':
				case ':':
					type = TOKEN_SLASH;
					break;

				case '%':
					type = TOKEN_MODULO;
					break;
				case '^':
					type = TOKEN_POWER;
					break;
				case '!':
					type = TOKEN_FACTORIAL;
					break;

				case '(':
					type = TOKEN_OPENING_BRACKET;
					break;
				case ')':
					type = TOKEN_CLOSING_BRACKET;
					break;

				case '&':
					type = TOKEN_AND;
					break;
				case '|':
					type = TOKEN_OR;
					break;
				case '~':
					type = TOKEN_NOT;
					break;

				case '\n':
					type = TOKEN_END_OF_LINE;
					break;

				case 'x':
					if (!fHexSupport) {
						type = TOKEN_STAR;
						break;
					}
					// fall through

				default:
					throw ParseException("unexpected character", _CurrentPos());
			}
			fCurrentToken = Token(fCurrentChar, 1, _CurrentPos(), type);
			fCurrentChar++;
		}

//printf("next token: '%s'\n", fCurrentToken.string.String());
		return fCurrentToken;
	}

	void RewindToken()
	{
		fReuseToken = true;
	}

 private:
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


ExpressionParser::ExpressionParser()
	: fTokenizer(new Tokenizer())
{
}


ExpressionParser::~ExpressionParser()
{
	delete fTokenizer;
}


void
ExpressionParser::SetSupportHexInput(bool enabled)
{
	fTokenizer->SetSupportHexInput(enabled);
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
ExpressionParser::EvaluateToInt64(const char* expressionString)
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
ExpressionParser::EvaluateToDouble(const char* expressionString)
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
				return _ParseFactorial(value);
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
				return _ParseFactorial(value);
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
			return _ParseFactorial(value);
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
	if (strcmp("e", token.string.String()) == 0)
		return _ParseFactorial(MAPM(MM_E));
	else if (strcasecmp("pi", token.string.String()) == 0
		|| ((unsigned char)token.string.String()[0] == 0xCF 
			&& (unsigned char)token.string.String()[1] == 0x80)) {
		// UTF-8 small greek letter PI
		return _ParseFactorial(MAPM(MM_PI));
	}

	// hard coded cases for different count of arguments
	// supports functions with 3 arguments at most

	MAPM values[3];

	if (strcasecmp("abs", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return _ParseFactorial(values[0].abs());
	} else if (strcasecmp("acos", token.string.String()) == 0) {
		_InitArguments(values, 1);
		if (values[0] < -1 || values[0] > 1)
			throw ParseException("out of domain", token.position);
		return _ParseFactorial(values[0].acos());
	} else if (strcasecmp("asin", token.string.String()) == 0) {
		_InitArguments(values, 1);
		if (values[0] < -1 || values[0] > 1)
			throw ParseException("out of domain", token.position);
		return _ParseFactorial(values[0].asin());
	} else if (strcasecmp("atan", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return _ParseFactorial(values[0].atan());
	} else if (strcasecmp("atan2", token.string.String()) == 0) {
		_InitArguments(values, 2);
		return _ParseFactorial(values[0].atan2(values[1]));
	} else if (strcasecmp("cbrt", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return _ParseFactorial(values[0].cbrt());
	} else if (strcasecmp("ceil", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return _ParseFactorial(values[0].ceil());
	} else if (strcasecmp("cos", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return _ParseFactorial(values[0].cos());
	} else if (strcasecmp("cosh", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return _ParseFactorial(values[0].cosh());
	} else if (strcasecmp("exp", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return _ParseFactorial(values[0].exp());
	} else if (strcasecmp("floor", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return _ParseFactorial(values[0].floor());
	} else if (strcasecmp("ln", token.string.String()) == 0) {
		_InitArguments(values, 1);
		if (values[0] <= 0)
			throw ParseException("out of domain", token.position);
		return _ParseFactorial(values[0].log());
	} else if (strcasecmp("log", token.string.String()) == 0) {
		_InitArguments(values, 1);
		if (values[0] <= 0)
			throw ParseException("out of domain", token.position);
		return _ParseFactorial(values[0].log10());
	} else if (strcasecmp("pow", token.string.String()) == 0) {
		_InitArguments(values, 2);
		return _ParseFactorial(values[0].pow(values[1]));
	} else if (strcasecmp("sin", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return _ParseFactorial(values[0].sin());
	} else if (strcasecmp("sinh", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return _ParseFactorial(values[0].sinh());
	} else if (strcasecmp("sqrt", token.string.String()) == 0) {
		_InitArguments(values, 1);
		if (values[0] < 0)
			throw ParseException("out of domain", token.position);
		return _ParseFactorial(values[0].sqrt());
	} else if (strcasecmp("tan", token.string.String()) == 0) {
		_InitArguments(values, 1);
		MAPM divided_by_half_pi = values[0] / MM_HALF_PI;
		if (divided_by_half_pi.is_integer() && divided_by_half_pi.is_odd())
			throw ParseException("out of domain", token.position);
		return _ParseFactorial(values[0].tan());
	} else if (strcasecmp("tanh", token.string.String()) == 0) {
		_InitArguments(values, 1);
		return _ParseFactorial(values[0].tanh());
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
		return _ParseFactorial(token.value);

	fTokenizer->RewindToken();

	_EatToken(TOKEN_OPENING_BRACKET);

	MAPM value = _ParseBinary();

	_EatToken(TOKEN_CLOSING_BRACKET);

	return _ParseFactorial(value);
}


MAPM
ExpressionParser::_ParseFactorial(MAPM value)
{
	if (fTokenizer->NextToken().type == TOKEN_FACTORIAL) {
		fTokenizer->RewindToken();
		_EatToken(TOKEN_FACTORIAL);
		return value.factorial();
	}

	fTokenizer->RewindToken();
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

