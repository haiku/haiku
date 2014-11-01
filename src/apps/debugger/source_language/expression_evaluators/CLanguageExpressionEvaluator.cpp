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
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "AutoLocker.h"

#include "Number.h"
#include "StackFrame.h"
#include "Thread.h"
#include "Type.h"
#include "Value.h"
#include "ValueNode.h"
#include "ValueNodeManager.h"
#include "Variable.h"


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
	TOKEN_BITWISE_XOR,
	TOKEN_EQ,
	TOKEN_NE,
	TOKEN_GT,
	TOKEN_GE,
	TOKEN_LT,
	TOKEN_LE,

	TOKEN_MEMBER_PTR
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
			token = "**";
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

		case TOKEN_BITWISE_XOR:
			token = "^";
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

		case TOKEN_MEMBER_PTR:
			token = "->";
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
		  value(0L),
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
		  value(),
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
	Number		value;

	int32		position;
};


class CLanguageExpressionEvaluator::Tokenizer {
 public:
	Tokenizer()
		: fString(""),
		  fCurrentChar(NULL),
		  fCurrentToken(),
		  fReuseToken(false),
		  fType(B_INT32_TYPE)
	{
	}

	void SetTo(const char* string)
	{
		fString = string;
		fCurrentChar = fString.String();
		fCurrentToken = Token();
		fReuseToken = false;
	}

	void SetType(type_code type)
	{
		fType = type;
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
			if (*fCurrentChar == '0' && fCurrentChar[1] == 'x')
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
			if (length == 1 && decimal) {
				// check for . operator
				fCurrentChar = begin;
				if (!_ParseOperator()) {
					throw ParseException("unexpected character",
						_CurrentPos());
				}

				return fCurrentToken;
			}

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
			fCurrentToken.value.SetTo(fType, temp.String());
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

					default:
						throw ParseException("unexpected character",
							_CurrentPos());
				}
				fCurrentToken = Token(fCurrentChar, 1, _CurrentPos(), type);
				fCurrentChar++;
			}
		}

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
				 if (Peek() == '>') {
				 	type = TOKEN_MEMBER_PTR;
				 	length = 2;
				 } else {
					type = TOKEN_MINUS;
					length = 1;
				 }
				break;

			case '*':
				if (Peek() == '*')  {
					type = TOKEN_POWER;
					length = 2;
				} else {
					type = TOKEN_STAR;
					length = 1;
				}
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
				type = TOKEN_BITWISE_XOR;
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

		fCurrentToken.value.SetTo(fType, fCurrentToken.string.String(), 16);
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
	type_code	fType;
};


CLanguageExpressionEvaluator::CLanguageExpressionEvaluator()
	:
	fTokenizer(new Tokenizer()),
	fCurrentType(B_INT64_TYPE),
	fNodeManager(NULL)
{
}


CLanguageExpressionEvaluator::~CLanguageExpressionEvaluator()
{
	delete fTokenizer;
}


Number
CLanguageExpressionEvaluator::Evaluate(const char* expressionString,
	type_code type, ValueNodeManager* manager)
{
	fCurrentType = type;
	fNodeManager = manager;
	fTokenizer->SetType(type);
	fTokenizer->SetTo(expressionString);

	Number value = _ParseSum();
	Token token = fTokenizer->NextToken();
	if (token.type != TOKEN_END_OF_LINE)
		throw ParseException("parse error", token.position);

	return value;
}


Number
CLanguageExpressionEvaluator::_ParseSum()
{
	Number value = _ParseProduct();

	while (true) {
		Token token = fTokenizer->NextToken();
		switch (token.type) {
			case TOKEN_PLUS:
				value += _ParseProduct();
				break;
			case TOKEN_MINUS:
				value -= _ParseProduct();
				break;

			default:
				fTokenizer->RewindToken();
				return value;
		}
	}
}


Number
CLanguageExpressionEvaluator::_ParseProduct()
{
	Number value = _ParsePower();

	while (true) {
		Token token = fTokenizer->NextToken();
		switch (token.type) {
			case TOKEN_STAR:
				value *= _ParsePower();
				break;
			case TOKEN_SLASH:
			{
				Number rhs = _ParsePower();
				if (rhs == Number(fCurrentType, 0))
					throw ParseException("division by zero", token.position);
				value /= rhs;
				break;
			}

			case TOKEN_MODULO:
			{
				Number rhs = _ParsePower();
				if (rhs == Number())
					throw ParseException("modulo by zero", token.position);
				value %= rhs;
				break;
			}

			case TOKEN_LOGICAL_AND:
			{
				Number zero(BVariant(0L));
				value.SetTo(BVariant((int32)((value != zero)
					&& (_ParsePower() != zero))));

				break;
			}

			case TOKEN_LOGICAL_OR:
			{
				Number zero(BVariant(0L));
				value.SetTo(BVariant((int32)((value != zero)
					|| (_ParsePower() != zero))));
				break;
			}

			case TOKEN_BITWISE_AND:
				value &= _ParsePower();
				break;

			case TOKEN_BITWISE_OR:
				value |= _ParsePower();
				break;

			case TOKEN_BITWISE_XOR:
				value ^= _ParsePower();
				break;

			case TOKEN_EQ:
				value.SetTo(BVariant((int32)(value == _ParsePower())));
				break;

			case TOKEN_NE:
				value.SetTo(BVariant((int32)(value != _ParsePower())));
				break;

			case TOKEN_GT:
				value.SetTo(BVariant((int32)(value > _ParsePower())));
				break;

			case TOKEN_GE:
				value.SetTo(BVariant((int32)(value >= _ParsePower())));
				break;

			case TOKEN_LT:
				value.SetTo(BVariant((int32)(value < _ParsePower())));
				break;

			case TOKEN_LE:
				value.SetTo(BVariant((int32)(value <= _ParsePower())));
				break;

			default:
				fTokenizer->RewindToken();
				return value;
		}
	}
}


Number
CLanguageExpressionEvaluator::_ParsePower()
{
	Number value = _ParseUnary();

	while (true) {
		Token token = fTokenizer->NextToken();
		if (token.type != TOKEN_POWER) {
			fTokenizer->RewindToken();
			return value;
		}

		Number power = _ParseUnary();
		Number temp = value;
		int32 powerValue = power.GetValue().ToInt32();
		bool handleNegativePower = false;
		if (powerValue < 0) {
			powerValue = abs(powerValue);
			handleNegativePower = true;
		}

		if (powerValue == 0)
			value.SetTo(fCurrentType, "1");
		else {
			for (; powerValue > 1; powerValue--)
				value *= temp;
		}

		if (handleNegativePower) {
			temp.SetTo(fCurrentType, "1");
			temp /= value;
			value = temp;
		}
	}
}


Number
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

		case TOKEN_BITWISE_NOT:
			return ~_ParseUnary();

		case TOKEN_LOGICAL_NOT:
			return Number((int32)(_ParseUnary() == Number(BVariant(0L))));

		case TOKEN_IDENTIFIER:
			fTokenizer->RewindToken();
			return _ParseIdentifier();

		default:
			fTokenizer->RewindToken();
			return _ParseAtom();
	}

	return Number();
}


Number
CLanguageExpressionEvaluator::_ParseIdentifier(ValueNode* parentNode)
{
	Token token = fTokenizer->NextToken();
	Number value;

	if (fNodeManager == NULL) {
		throw ParseException("Identifiers not resolvable without manager.",
			token.position);
	}

	const BString& identifierName = token.string;

	ValueNodeContainer* container = fNodeManager->GetContainer();
	AutoLocker<ValueNodeContainer> containerLocker(container);

	ValueNodeChild* child = NULL;
	if (parentNode == NULL) {
		ValueNodeChild* thisChild = NULL;
		for (int32 i = 0; i < container->CountChildren(); i++) {
			ValueNodeChild* current = container->ChildAt(i);
			const BString& nodeName = current->Name();
			if (nodeName == identifierName) {
				child = current;
				break;
			} else if (nodeName == "this")
				thisChild = current;
		}

		if (child == NULL && thisChild != NULL) {
			// the name was not found in the variables or parameters,
			// but we have a class pointer. Try to find the name in the
			// list of members.
			_RequestValueIfNeeded(token, thisChild);
			ValueNode* thisNode = thisChild->Node();
			fTokenizer->RewindToken();
			return _ParseIdentifier(thisNode);
		}
	} else {
		// skip intermediate address nodes
		if (parentNode->GetType()->Kind() == TYPE_ADDRESS
			&& parentNode->CountChildren() == 1) {
			child = parentNode->ChildAt(0);

			_RequestValueIfNeeded(token, child);
			parentNode = child->Node();
			fTokenizer->RewindToken();
			return _ParseIdentifier(parentNode);
		}

		for (int32 i = 0; i < parentNode->CountChildren(); i++) {
			ValueNodeChild* current = parentNode->ChildAt(i);
			const BString& nodeName = current->Name();
			if (nodeName == identifierName) {
				child = current;
				break;
			}
		}
	}

	BString errorMessage;
	if (child == NULL) {
		errorMessage.SetToFormat("Unable to resolve variable name: '%s'",
			identifierName.String());
		throw ParseException(errorMessage, token.position);
	}

	_RequestValueIfNeeded(token, child);
	ValueNode* node = child->Node();

	token = fTokenizer->NextToken();
	if (token.type == TOKEN_MEMBER_PTR) {
		token = fTokenizer->NextToken();
		if (token.type == TOKEN_IDENTIFIER) {
			fTokenizer->RewindToken();
			return _ParseIdentifier(node);
		} else {
			throw ParseException("Expected identifier after member "
				"dereference.", token.position);
		}
	} else
		fTokenizer->RewindToken();

	BVariant variant;
	Value* nodeValue = node->GetValue();
	nodeValue->ToVariant(variant);
	value.SetTo(variant);
	_CoerceTypeIfNeeded(token, value);

	return value;
}


Number
CLanguageExpressionEvaluator::_ParseAtom()
{
	Token token = fTokenizer->NextToken();
	if (token.type == TOKEN_END_OF_LINE)
		throw ParseException("unexpected end of expression", token.position);

	if (token.type == TOKEN_CONSTANT)
		return token.value;

	fTokenizer->RewindToken();

	_EatToken(TOKEN_OPENING_BRACKET);

	Number value = _ParseSum();

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


void
CLanguageExpressionEvaluator::_RequestValueIfNeeded(const Token& token,
	ValueNodeChild* child)
{
	status_t state;
	BString errorMessage;
	if (child->Node() == NULL) {
		state = fNodeManager->AddChildNodes(child);
		if (state != B_OK) {
			errorMessage.SetToFormat("Unable to add children for node '%s': "
				"%s", child->Name().String(), strerror(state));
			throw ParseException(errorMessage, token.position);
		}
	}

	state = child->LocationResolutionState();
	if (state == VALUE_NODE_UNRESOLVED)
		throw ValueNeededException(child->Node());
	else if (state != B_OK) {
		errorMessage.SetToFormat("Unable to resolve variable value for '%s': "
			"%s", child->Name().String(), strerror(state));
		throw ParseException(errorMessage, token.position);
	}

	ValueNode* node = child->Node();
	state = node->LocationAndValueResolutionState();
	if (state == VALUE_NODE_UNRESOLVED)
		throw ValueNeededException(child->Node());
	else if (state != B_OK) {
		errorMessage.SetToFormat("Unable to resolve variable value for '%s': "
			"%s", child->Name().String(), strerror(state));
		throw ParseException(errorMessage, token.position);
	}
}


void
CLanguageExpressionEvaluator::_CoerceTypeIfNeeded(const Token& token,
	Number& _number)
{
	if (_number.Type() == 0) {
		throw ParseException("Unable to resolve value type.",
			token.position);
	}

	BVariant value = _number.GetValue();
	type_code valueType = value.Type();

	if (valueType == fCurrentType) {
		// nothing to do.
		return;
	}

	if (BVariant::TypeIsInteger(fCurrentType)) {
		if (BVariant::TypeIsFloat(valueType)) {
			value.SetTo((int64)value.ToDouble());
			valueType = value.Type();
		}

		if (BVariant::TypeIsInteger(valueType)) {
			switch (fCurrentType) {
				case B_INT8_TYPE:
					value.SetTo(value.ToInt8());
					break;

				case B_UINT8_TYPE:
					value.SetTo(value.ToUInt8());
					break;

				case B_INT16_TYPE:
					value.SetTo(value.ToInt16());
					break;

				case B_UINT16_TYPE:
					value.SetTo(value.ToUInt16());
					break;

				case B_INT32_TYPE:
					value.SetTo(value.ToInt32());
					break;

				case B_UINT32_TYPE:
					value.SetTo(value.ToUInt32());
					break;

				case B_INT64_TYPE:
					value.SetTo(value.ToInt64());
					break;

				case B_UINT64_TYPE:
					value.SetTo(value.ToUInt64());
					break;
			}
		}
	} else if (BVariant::TypeIsFloat(fCurrentType)) {
		if (BVariant::TypeIsInteger(valueType)) {
			value.SetTo((double)value.ToInt64());
			valueType = value.Type();
		}

		switch (fCurrentType) {
			case B_FLOAT_TYPE:
				value.SetTo(value.ToFloat());
				break;

			case B_DOUBLE_TYPE:
				value.SetTo(value.ToDouble());
				break;
		}
	}

	_number.SetTo(value);
}
