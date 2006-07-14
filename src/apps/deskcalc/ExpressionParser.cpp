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

#include "ExpressionParser.h"

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
		  value(0.0),
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
		  value(0.0),
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
	double		value;

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
			temp << "&_";
			double value;
			char t[2];
			int32 matches = sscanf(temp.String(), "%lf&%s", &value, t);
			if (matches != 2)
				throw ParseException("error in constant", _CurrentPos() - length);

			fCurrentToken = Token(begin, length, _CurrentPos() - length, TOKEN_CONSTANT);
			fCurrentToken.value = value;;

		} else if (isalpha(*fCurrentChar)) {

			const char* begin = fCurrentChar;
			while (*fCurrentChar != 0 && (isalpha(*fCurrentChar) || isdigit(*fCurrentChar)))
				fCurrentChar++;
			int32 length = fCurrentChar - begin;
			fCurrentToken = Token(begin, length, _CurrentPos() - length, TOKEN_IDENTIFIER);

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
					fCurrentToken = Token(fCurrentChar, 1, _CurrentPos(), *fCurrentChar);
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


double
ExpressionParser::Evaluate(const char* expressionString)
{
	fTokenizer->SetTo(expressionString);

	double value = _ParseBinary();
	Token token = fTokenizer->NextToken();
	if (token.type != TOKEN_END_OF_LINE)
		throw ParseException("parse error", token.position);

	return value;
}


double
ExpressionParser::_ParseBinary()
{
	double value = _ParseSum();

	while (true) {
		Token token = fTokenizer->NextToken();
		switch (token.type) {
			case TOKEN_AND:
				value = (uint64)value & (uint64)_ParseSum();
				break;
			case TOKEN_OR:
				value = (uint64)value | (uint64)_ParseSum();
				break;

			default:
				fTokenizer->RewindToken();
				return value;
		}
	}
}


double
ExpressionParser::_ParseSum()
{
	// TODO: check isnan()...
	double value = _ParseProduct();

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


double
ExpressionParser::_ParseProduct()
{
	// TODO: check isnan()...
	double value = _ParsePower();

	while (true) {
		Token token = fTokenizer->NextToken();
		switch (token.type) {
			case TOKEN_STAR:
				value = value * _ParsePower();
				break;
			case TOKEN_SLASH: {
				double rhs = _ParsePower();
				if (rhs == 0.0)
					throw ParseException("division by zero", token.position);
				value = value / rhs;
				break;
			}
			case TOKEN_MODULO: {
				double rhs = _ParsePower();
				if (rhs == 0.0)
					throw ParseException("modulo by zero", token.position);
				value = fmod(value, rhs);
				break;
			}

			default:
				fTokenizer->RewindToken();
				return value;
		}
	}
}


double
ExpressionParser::_ParsePower()
{
	double value = _ParseUnary();

	while (true) {
		Token token = fTokenizer->NextToken();
		if (token.type != TOKEN_POWER) {
			fTokenizer->RewindToken();
			return value;
		}
		value = pow(value, _ParseUnary());
	}
}


double
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
		case TOKEN_NOT:
			return ~(uint64)_ParseUnary();

		case TOKEN_IDENTIFIER:
			return _ParseFunction(token);

		default:
			fTokenizer->RewindToken();
			return _ParseAtom();
	}

	return 0.0;
}


struct Function {
	const char*	name;
	int			argumentCount;
	void*		function;
	double		value;
};

static const Function kFunctions[] = {
	{ "e",		0, NULL, M_E },
	{ "pi",		0, NULL, M_PI },

	{ "abs",	1, (void*)&fabs },
	{ "acos",	1, (void*)&acos },
	{ "asin",	1, (void*)&asin },
	{ "atan",	1, (void*)&atan },
	{ "atan2",	2, (void*)&atan2 },
	{ "ceil",	1, (void*)&ceil },
	{ "cos",	1, (void*)&cos },
	{ "cosh",	1, (void*)&cosh },
	{ "exp",	1, (void*)&exp },
	{ "fabs",	1, (void*)&fabs },
	{ "floor",	1, (void*)&floor },
	{ "fmod",	2, (void*)&fmod },
	{ "log",	1, (void*)&log },
	{ "log10",	1, (void*)&log10 },
	{ "pow",	2, (void*)&pow },
	{ "sin",	1, (void*)&sin },
	{ "sinh",	1, (void*)&sinh },
	{ "sqrt",	1, (void*)&sqrt },
	{ "tan",	1, (void*)&tan },
	{ "tanh",	1, (void*)&tanh },
	{ "hypot",	2, (void*)&hypot },

	{ NULL },
};

double
ExpressionParser::_ParseFunction(const Token& token)
{
	const Function* function = _FindFunction(token.string.String());
	if (!function)
		throw ParseException("unknown identifier", token.position);

	if (function->argumentCount == 0)
		return function->value;

	_EatToken(TOKEN_OPENING_BRACKET);

	double values[function->argumentCount];
	for (int32 i = 0; i < function->argumentCount; i++) {
		values[i] = _ParseBinary();
	}

	_EatToken(TOKEN_CLOSING_BRACKET);

	// hard coded cases for different count of arguments
	switch (function->argumentCount) {
		case 1:
			return ((double (*)(double))function->function)(values[0]);
		case 2:
			return ((double (*)(double, double))function->function)(values[0],
				values[1]);
		case 3:
			return ((double (*)(double, double, double))function->function)(
				values[0], values[1], values[2]);

		default:
			throw ParseException("unsupported function argument count",
				token.position);
	}
}


double
ExpressionParser::_ParseAtom()
{
	Token token = fTokenizer->NextToken();
	if (token.type == TOKEN_END_OF_LINE)
		throw ParseException("unexpected end of expression", token.position);

	if (token.type == TOKEN_CONSTANT)
		return token.value;

	fTokenizer->RewindToken();

	_EatToken(TOKEN_OPENING_BRACKET);

	double value = _ParseBinary();

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


const Function*
ExpressionParser::_FindFunction(const char* name) const
{
	for (int32 i = 0; kFunctions[i].name; i++) {
		if (strcasecmp(kFunctions[i].name, name) == 0)
			return &kFunctions[i];
	}

	return NULL;
}

