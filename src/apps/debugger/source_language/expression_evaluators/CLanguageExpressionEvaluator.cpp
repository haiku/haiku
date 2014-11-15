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

#include <algorithm>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "AutoLocker.h"

#include "ExpressionInfo.h"
#include "FloatValue.h"
#include "IntegerValue.h"
#include "StackFrame.h"
#include "Thread.h"
#include "Type.h"
#include "Value.h"
#include "ValueNode.h"
#include "ValueNodeManager.h"
#include "Variable.h"
#include "VariableValueNodeChild.h"


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


enum operand_kind {
	OPERAND_KIND_UNKNOWN = 0,
	OPERAND_KIND_PRIMITIVE,
	OPERAND_KIND_TYPE,
	OPERAND_KIND_VALUE_NODE
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


// #pragma mark - CLanguageExpressionEvaluator::Operand


class CLanguageExpressionEvaluator::Operand {
public:
	Operand()
		:
		fPrimitive(),
		fValueNode(NULL),
		fType(NULL),
		fKind(OPERAND_KIND_UNKNOWN)
	{
	}

	Operand(int64 value)
		:
		fPrimitive(value),
		fValueNode(NULL),
		fType(NULL),
		fKind(OPERAND_KIND_PRIMITIVE)
	{
	}

	Operand(double value)
		:
		fPrimitive(value),
		fValueNode(NULL),
		fType(NULL),
		fKind(OPERAND_KIND_PRIMITIVE)
	{
	}

	Operand(ValueNode* node)
		:
		fPrimitive(),
		fValueNode(NULL),
		fType(NULL),
		fKind(OPERAND_KIND_UNKNOWN)
	{
		SetTo(node);
	}

	Operand(Type* type)
		:
		fPrimitive(),
		fValueNode(NULL),
		fType(NULL),
		fKind(OPERAND_KIND_UNKNOWN)
	{
		SetTo(type);
	}

	Operand(const Operand& X)
		:
		fPrimitive(),
		fValueNode(NULL),
		fType(NULL),
		fKind(OPERAND_KIND_UNKNOWN)
	{
		*this = X;
	}


	virtual ~Operand()
	{
		Unset();
	}

	Operand& operator=(const Operand& X)
	{
		switch (X.fKind) {
			case OPERAND_KIND_UNKNOWN:
				Unset();
				break;

			case OPERAND_KIND_PRIMITIVE:
				SetTo(X.fPrimitive);
				break;

			case OPERAND_KIND_VALUE_NODE:
				SetTo(X.fValueNode);
				break;

			case OPERAND_KIND_TYPE:
				SetTo(X.fType);
				break;
		}

		return *this;
	}

	void SetTo(const BVariant& value)
	{
		Unset();
		fPrimitive = value;
		fKind = OPERAND_KIND_PRIMITIVE;
	}

	void SetTo(ValueNode* node)
	{
		Unset();
		fValueNode = node;
		fValueNode->AcquireReference();

		Value* value = node->GetValue();
		if (value != NULL)
			value->ToVariant(fPrimitive);

		fKind = OPERAND_KIND_VALUE_NODE;
	}

	void SetTo(Type* type)
	{
		Unset();
		fType = type;
		fType->AcquireReference();

		fKind = OPERAND_KIND_TYPE;
	}

	void Unset()
	{
		if (fValueNode != NULL)
			fValueNode->ReleaseReference();

		if (fType != NULL)
			fType->ReleaseReference();

		fValueNode = NULL;
		fType = NULL;
		fKind = OPERAND_KIND_UNKNOWN;
	}

	inline operand_kind Kind() const
	{
		return fKind;
	}

	inline const BVariant& PrimitiveValue() const
	{
		return fPrimitive;
	}

	inline ValueNode* GetValueNode() const
	{
		return fValueNode;

	}

	inline Type* GetType() const
	{
		return fType;
	}

	Operand& operator+=(const Operand& rhs)
	{
		Operand temp = rhs;
		_ResolveTypesIfNeeded(temp);

		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				fPrimitive.SetTo((int8)(fPrimitive.ToInt8()
					+ temp.fPrimitive.ToInt8()));
				break;
			}

			case B_UINT8_TYPE:
			{
				fPrimitive.SetTo((uint8)(fPrimitive.ToUInt8()
					+ temp.fPrimitive.ToUInt8()));
				break;
			}

			case B_INT16_TYPE:
			{
				fPrimitive.SetTo((int16)(fPrimitive.ToInt16()
					+ temp.fPrimitive.ToInt16()));
				break;
			}

			case B_UINT16_TYPE:
			{
				fPrimitive.SetTo((uint16)(fPrimitive.ToUInt16()
					+ temp.fPrimitive.ToUInt16()));
				break;
			}

			case B_INT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt32()
					+ temp.fPrimitive.ToInt32());
				break;
			}

			case B_UINT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt32()
					+ temp.fPrimitive.ToUInt32());
				break;
			}

			case B_INT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt64()
					+ temp.fPrimitive.ToInt64());
				break;
			}

			case B_UINT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt64()
					+ temp.fPrimitive.ToUInt64());
				break;
			}

			case B_FLOAT_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToFloat()
					+ temp.fPrimitive.ToFloat());
				break;
			}

			case B_DOUBLE_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToDouble()
					+ temp.fPrimitive.ToDouble());
				break;
			}
		}

		return *this;
	}

	Operand& operator-=(const Operand& rhs)
	{
		Operand temp = rhs;
		_ResolveTypesIfNeeded(temp);

		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				fPrimitive.SetTo((int8)(fPrimitive.ToInt8()
					- temp.fPrimitive.ToInt8()));
				break;
			}

			case B_UINT8_TYPE:
			{
				fPrimitive.SetTo((uint8)(fPrimitive.ToUInt8()
					- temp.fPrimitive.ToUInt8()));
				break;
			}

			case B_INT16_TYPE:
			{
				fPrimitive.SetTo((int16)(fPrimitive.ToInt16()
					- temp.fPrimitive.ToInt16()));
				break;
			}

			case B_UINT16_TYPE:
			{
				fPrimitive.SetTo((uint16)(fPrimitive.ToUInt16()
					- temp.fPrimitive.ToUInt16()));
				break;
			}

			case B_INT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt32()
					- temp.fPrimitive.ToInt32());
				break;
			}

			case B_UINT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt32()
					- temp.fPrimitive.ToUInt32());
				break;
			}

			case B_INT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt64()
					- temp.fPrimitive.ToInt64());
				break;
			}

			case B_UINT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt64()
					- temp.fPrimitive.ToUInt64());
				break;
			}

			case B_FLOAT_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToFloat()
					- temp.fPrimitive.ToFloat());
				break;
			}

			case B_DOUBLE_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToDouble()
					- temp.fPrimitive.ToDouble());
				break;
			}
		}

		return *this;
	}

	Operand& operator/=(const Operand& rhs)
	{
		Operand temp = rhs;
		_ResolveTypesIfNeeded(temp);

		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				fPrimitive.SetTo((int8)(fPrimitive.ToInt8()
					/ temp.fPrimitive.ToInt8()));
				break;
			}

			case B_UINT8_TYPE:
			{
				fPrimitive.SetTo((uint8)(fPrimitive.ToUInt8()
					/ temp.fPrimitive.ToUInt8()));
				break;
			}

			case B_INT16_TYPE:
			{
				fPrimitive.SetTo((int16)(fPrimitive.ToInt16()
					/ temp.fPrimitive.ToInt16()));
				break;
			}

			case B_UINT16_TYPE:
			{
				fPrimitive.SetTo((uint16)(fPrimitive.ToUInt16()
					/ temp.fPrimitive.ToUInt16()));
				break;
			}

			case B_INT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt32()
					/ temp.fPrimitive.ToInt32());
				break;
			}

			case B_UINT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt32()
					/ temp.fPrimitive.ToUInt32());
				break;
			}

			case B_INT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt64()
					/ temp.fPrimitive.ToInt64());
				break;
			}

			case B_UINT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt64()
					/ temp.fPrimitive.ToUInt64());
				break;
			}

			case B_FLOAT_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToFloat()
					/ temp.fPrimitive.ToFloat());
				break;
			}

			case B_DOUBLE_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToDouble()
					/ temp.fPrimitive.ToDouble());
				break;
			}
		}

		return *this;
	}

	Operand& operator*=(const Operand& rhs)
	{
		Operand temp = rhs;
		_ResolveTypesIfNeeded(temp);

		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				fPrimitive.SetTo((int8)(fPrimitive.ToInt8()
					* temp.fPrimitive.ToInt8()));
				break;
			}

			case B_UINT8_TYPE:
			{
				fPrimitive.SetTo((uint8)(fPrimitive.ToUInt8()
					* temp.fPrimitive.ToUInt8()));
				break;
			}

			case B_INT16_TYPE:
			{
				fPrimitive.SetTo((int16)(fPrimitive.ToInt16()
					* temp.fPrimitive.ToInt16()));
				break;
			}

			case B_UINT16_TYPE:
			{
				fPrimitive.SetTo((uint16)(fPrimitive.ToUInt16()
					* temp.fPrimitive.ToUInt16()));
				break;
			}

			case B_INT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt32()
					* temp.fPrimitive.ToInt32());
				break;
			}

			case B_UINT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt32()
					* temp.fPrimitive.ToUInt32());
				break;
			}

			case B_INT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt64()
					* temp.fPrimitive.ToInt64());
				break;
			}

			case B_UINT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt64()
					* temp.fPrimitive.ToUInt64());
				break;
			}

			case B_FLOAT_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToFloat()
					* temp.fPrimitive.ToFloat());
				break;
			}

			case B_DOUBLE_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToDouble()
					* temp.fPrimitive.ToDouble());
				break;
			}
		}

		return *this;
	}

	Operand& operator%=(const Operand& rhs)
	{
		Operand temp = rhs;
		_ResolveTypesIfNeeded(temp);

		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				fPrimitive.SetTo((int8)(fPrimitive.ToInt8()
					% temp.fPrimitive.ToInt8()));
				break;
			}

			case B_UINT8_TYPE:
			{
				fPrimitive.SetTo((uint8)(fPrimitive.ToUInt8()
					% temp.fPrimitive.ToUInt8()));
				break;
			}

			case B_INT16_TYPE:
			{
				fPrimitive.SetTo((int16)(fPrimitive.ToInt16()
					% temp.fPrimitive.ToInt16()));
				break;
			}

			case B_UINT16_TYPE:
			{
				fPrimitive.SetTo((uint16)(fPrimitive.ToUInt16()
					% temp.fPrimitive.ToUInt16()));
				break;
			}

			case B_INT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt32()
					% temp.fPrimitive.ToInt32());
				break;
			}

			case B_UINT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt32()
					% temp.fPrimitive.ToUInt32());
				break;
			}

			case B_INT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt64()
					% temp.fPrimitive.ToInt64());
				break;
			}

			case B_UINT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt64()
					% temp.fPrimitive.ToUInt64());
				break;
			}
		}

		return *this;
	}

	Operand& operator&=(const Operand& rhs)
	{
		Operand temp = rhs;
		_ResolveTypesIfNeeded(temp);

		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				fPrimitive.SetTo((int8)(fPrimitive.ToInt8()
					& temp.fPrimitive.ToInt8()));
				break;
			}

			case B_UINT8_TYPE:
			{
				fPrimitive.SetTo((uint8)(fPrimitive.ToUInt8()
					& temp.fPrimitive.ToUInt8()));
				break;
			}

			case B_INT16_TYPE:
			{
				fPrimitive.SetTo((int16)(fPrimitive.ToInt16()
					& temp.fPrimitive.ToInt16()));
				break;
			}

			case B_UINT16_TYPE:
			{
				fPrimitive.SetTo((uint16)(fPrimitive.ToUInt16()
					& temp.fPrimitive.ToUInt16()));
				break;
			}

			case B_INT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt32()
					& temp.fPrimitive.ToInt32());
				break;
			}

			case B_UINT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt32()
					& temp.fPrimitive.ToUInt32());
				break;
			}

			case B_INT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt64()
					& temp.fPrimitive.ToInt64());
				break;
			}

			case B_UINT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt64()
					& temp.fPrimitive.ToUInt64());
				break;
			}
		}

		return *this;
	}

	Operand& operator|=(const Operand& rhs)
	{
		Operand temp = rhs;
		_ResolveTypesIfNeeded(temp);

		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				fPrimitive.SetTo((int8)(fPrimitive.ToInt8()
					| temp.fPrimitive.ToInt8()));
				break;
			}

			case B_UINT8_TYPE:
			{
				fPrimitive.SetTo((uint8)(fPrimitive.ToUInt8()
					| temp.fPrimitive.ToUInt8()));
				break;
			}

			case B_INT16_TYPE:
			{
				fPrimitive.SetTo((int16)(fPrimitive.ToInt16()
					| temp.fPrimitive.ToInt16()));
				break;
			}

			case B_UINT16_TYPE:
			{
				fPrimitive.SetTo((uint16)(fPrimitive.ToUInt16()
					| temp.fPrimitive.ToUInt16()));
				break;
			}

			case B_INT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt32()
					| temp.fPrimitive.ToInt32());
				break;
			}

			case B_UINT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt32()
					| temp.fPrimitive.ToUInt32());
				break;
			}

			case B_INT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt64()
					| temp.fPrimitive.ToInt64());
				break;
			}

			case B_UINT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt64()
					| temp.fPrimitive.ToUInt64());
				break;
			}
		}

		return *this;
	}

	Operand& operator^=(const Operand& rhs)
	{
		Operand temp = rhs;
		_ResolveTypesIfNeeded(temp);

		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				fPrimitive.SetTo((int8)(fPrimitive.ToInt8()
					^ temp.fPrimitive.ToInt8()));
				break;
			}

			case B_UINT8_TYPE:
			{
				fPrimitive.SetTo((uint8)(fPrimitive.ToUInt8()
					^ temp.fPrimitive.ToUInt8()));
				break;
			}

			case B_INT16_TYPE:
			{
				fPrimitive.SetTo((int16)(fPrimitive.ToInt16()
					^ temp.fPrimitive.ToInt16()));
				break;
			}

			case B_UINT16_TYPE:
			{
				fPrimitive.SetTo((uint16)(fPrimitive.ToUInt16()
					^ temp.fPrimitive.ToUInt16()));
				break;
			}

			case B_INT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt32()
					^ temp.fPrimitive.ToInt32());
				break;
			}

			case B_UINT32_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt32()
					^ temp.fPrimitive.ToUInt32());
				break;
			}

			case B_INT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToInt64()
					^ temp.fPrimitive.ToInt64());
				break;
			}

			case B_UINT64_TYPE:
			{
				fPrimitive.SetTo(fPrimitive.ToUInt64()
					^ temp.fPrimitive.ToUInt64());
				break;
			}
		}

		return *this;
	}

	Operand operator-() const
	{
		Operand value(*this);
		value._ResolveToPrimitive();

		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				value.fPrimitive.SetTo((int8)-fPrimitive.ToInt8());
				break;
			}

			case B_UINT8_TYPE:
			{
				value.fPrimitive.SetTo((uint8)-fPrimitive.ToUInt8());
				break;
			}

			case B_INT16_TYPE:
			{
				value.fPrimitive.SetTo((int16)-fPrimitive.ToInt16());
				break;
			}

			case B_UINT16_TYPE:
			{
				value.fPrimitive.SetTo((uint16)-fPrimitive.ToUInt16());
				break;
			}

			case B_INT32_TYPE:
			{
				value.fPrimitive.SetTo(-fPrimitive.ToInt32());
				break;
			}

			case B_UINT32_TYPE:
			{
				value.fPrimitive.SetTo(-fPrimitive.ToUInt32());
				break;
			}

			case B_INT64_TYPE:
			{
				value.fPrimitive.SetTo(-fPrimitive.ToInt64());
				break;
			}

			case B_UINT64_TYPE:
			{
				value.fPrimitive.SetTo(-fPrimitive.ToUInt64());
				break;
			}

			case B_FLOAT_TYPE:
			{
				value.fPrimitive.SetTo(-fPrimitive.ToFloat());
				break;
			}

			case B_DOUBLE_TYPE:
			{
				value.fPrimitive.SetTo(-fPrimitive.ToDouble());
				break;
			}
		}

		return value;
	}

	Operand operator~() const
	{
		Operand value(*this);
		value._ResolveToPrimitive();

		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				value.fPrimitive.SetTo((int8)~fPrimitive.ToInt8());
				break;
			}

			case B_UINT8_TYPE:
			{
				value.fPrimitive.SetTo((uint8)~fPrimitive.ToUInt8());
				break;
			}

			case B_INT16_TYPE:
			{
				value.fPrimitive.SetTo((int16)~fPrimitive.ToInt16());
				break;
			}

			case B_UINT16_TYPE:
			{
				value.fPrimitive.SetTo((uint16)~fPrimitive.ToUInt16());
				break;
			}

			case B_INT32_TYPE:
			{
				value.fPrimitive.SetTo(~fPrimitive.ToInt32());
				break;
			}

			case B_UINT32_TYPE:
			{
				value.fPrimitive.SetTo(~fPrimitive.ToUInt32());
				break;
			}

			case B_INT64_TYPE:
			{
				value.fPrimitive.SetTo(~fPrimitive.ToInt64());
				break;
			}

			case B_UINT64_TYPE:
			{
				value.fPrimitive.SetTo(~fPrimitive.ToUInt64());
				break;
			}
		}

		return value;
	}

	int operator<(const Operand& rhs) const
	{
		Operand lhs = *this;
		Operand temp = rhs;

		lhs._ResolveTypesIfNeeded(temp);

		int result = 0;
		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				result = lhs.fPrimitive.ToInt8() < temp.fPrimitive.ToInt8();
				break;
			}

			case B_UINT8_TYPE:
			{
				result = lhs.fPrimitive.ToUInt8() < temp.fPrimitive.ToUInt8();
				break;
			}

			case B_INT16_TYPE:
			{
				result = lhs.fPrimitive.ToInt16() < temp.fPrimitive.ToInt16();
				break;
			}

			case B_UINT16_TYPE:
			{
				result = lhs.fPrimitive.ToUInt16()
					< temp.fPrimitive.ToUInt16();
				break;
			}

			case B_INT32_TYPE:
			{
				result = lhs.fPrimitive.ToInt32() < temp.fPrimitive.ToInt32();
				break;
			}

			case B_UINT32_TYPE:
			{
				result = lhs.fPrimitive.ToUInt32()
					< temp.fPrimitive.ToUInt32();
				break;
			}

			case B_INT64_TYPE:
			{
				result = lhs.fPrimitive.ToInt64() < temp.fPrimitive.ToInt64();
				break;
			}

			case B_UINT64_TYPE:
			{
				result = lhs.fPrimitive.ToUInt64()
					< temp.fPrimitive.ToUInt64();
				break;
			}

			case B_FLOAT_TYPE:
			{
				result = lhs.fPrimitive.ToFloat() < temp.fPrimitive.ToFloat();
				break;
			}

			case B_DOUBLE_TYPE:
			{
				result = lhs.fPrimitive.ToDouble()
					< temp.fPrimitive.ToDouble();
				break;
			}
		}

		return result;
	}

	int operator<=(const Operand& rhs) const
	{
		return (*this < rhs) || (*this == rhs);
	}

	int operator>(const Operand& rhs) const
	{
		Operand lhs = *this;
		Operand temp = rhs;
		lhs._ResolveTypesIfNeeded(temp);

		int result = 0;
		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				result = lhs.fPrimitive.ToInt8() > temp.fPrimitive.ToInt8();
				break;
			}

			case B_UINT8_TYPE:
			{
				result = lhs.fPrimitive.ToUInt8() > temp.fPrimitive.ToUInt8();
				break;
			}

			case B_INT16_TYPE:
			{
				result = lhs.fPrimitive.ToInt16() > temp.fPrimitive.ToInt16();
				break;
			}

			case B_UINT16_TYPE:
			{
				result = lhs.fPrimitive.ToUInt16()
					> temp.fPrimitive.ToUInt16();
				break;
			}

			case B_INT32_TYPE:
			{
				result = lhs.fPrimitive.ToInt32() > temp.fPrimitive.ToInt32();
				break;
			}

			case B_UINT32_TYPE:
			{
				result = lhs.fPrimitive.ToUInt32()
					> temp.fPrimitive.ToUInt32();
				break;
			}

			case B_INT64_TYPE:
			{
				result = lhs.fPrimitive.ToInt64() > temp.fPrimitive.ToInt64();
				break;
			}

			case B_UINT64_TYPE:
			{
				result = lhs.fPrimitive.ToUInt64()
					> temp.fPrimitive.ToUInt64();
				break;
			}

			case B_FLOAT_TYPE:
			{
				result = lhs.fPrimitive.ToFloat() > temp.fPrimitive.ToFloat();
				break;
			}

			case B_DOUBLE_TYPE:
			{
				result = lhs.fPrimitive.ToDouble()
					> temp.fPrimitive.ToDouble();
				break;
			}
		}

		return result;
	}

	int operator>=(const Operand& rhs) const
	{
		return (*this > rhs) || (*this == rhs);
	}

	int	operator==(const Operand& rhs) const
	{
		Operand lhs = *this;
		Operand temp = rhs;
		lhs._ResolveTypesIfNeeded(temp);

		int result = 0;
		switch (fPrimitive.Type()) {
			case B_INT8_TYPE:
			{
				result = lhs.fPrimitive.ToInt8() == temp.fPrimitive.ToInt8();
				break;
			}

			case B_UINT8_TYPE:
			{
				result = lhs.fPrimitive.ToUInt8() == temp.fPrimitive.ToUInt8();
				break;
			}

			case B_INT16_TYPE:
			{
				result = lhs.fPrimitive.ToInt16() == temp.fPrimitive.ToInt16();
				break;
			}

			case B_UINT16_TYPE:
			{
				result = lhs.fPrimitive.ToUInt16()
					== temp.fPrimitive.ToUInt16();
				break;
			}

			case B_INT32_TYPE:
			{
				result = lhs.fPrimitive.ToInt32() == temp.fPrimitive.ToInt32();
				break;
			}

			case B_UINT32_TYPE:
			{
				result = lhs.fPrimitive.ToUInt32()
					== temp.fPrimitive.ToUInt32();
				break;
			}

			case B_INT64_TYPE:
			{
				result = lhs.fPrimitive.ToInt64() == temp.fPrimitive.ToInt64();
				break;
			}

			case B_UINT64_TYPE:
			{
				result = lhs.fPrimitive.ToUInt64()
					== temp.fPrimitive.ToUInt64();
				break;
			}

			case B_FLOAT_TYPE:
			{
				result = lhs.fPrimitive.ToFloat() == temp.fPrimitive.ToFloat();
				break;
			}

			case B_DOUBLE_TYPE:
			{
				result = lhs.fPrimitive.ToDouble()
					== temp.fPrimitive.ToDouble();
				break;
			}
		}

		return result;
	}

	int operator!=(const Operand& rhs) const
	{
		return !(*this == rhs);
	}

private:
	void _GetAsType(type_code type)
	{
		switch (type) {
			case B_INT8_TYPE:
				fPrimitive.SetTo(fPrimitive.ToInt8());
				break;
			case B_UINT8_TYPE:
				fPrimitive.SetTo(fPrimitive.ToUInt8());
				break;
			case B_INT16_TYPE:
				fPrimitive.SetTo(fPrimitive.ToInt16());
				break;
			case B_UINT16_TYPE:
				fPrimitive.SetTo(fPrimitive.ToUInt16());
				break;
			case B_INT32_TYPE:
				fPrimitive.SetTo(fPrimitive.ToInt32());
				break;
			case B_UINT32_TYPE:
				fPrimitive.SetTo(fPrimitive.ToUInt32());
				break;
			case B_INT64_TYPE:
				fPrimitive.SetTo(fPrimitive.ToInt64());
				break;
			case B_UINT64_TYPE:
				fPrimitive.SetTo(fPrimitive.ToUInt64());
				break;
			case B_FLOAT_TYPE:
				fPrimitive.SetTo(fPrimitive.ToFloat());
				break;
			case B_DOUBLE_TYPE:
				fPrimitive.SetTo(fPrimitive.ToDouble());
				break;
		}
	}

	void _ResolveTypesIfNeeded(Operand& other)
	{
		_ResolveToPrimitive();
		other._ResolveToPrimitive();

		if (!fPrimitive.IsNumber() || !other.fPrimitive.IsNumber()) {
				throw ParseException("Cannot perform mathematical operations "
					"between non-numerical objects.", 0);
		}

		type_code thisType = fPrimitive.Type();
		type_code otherType = other.fPrimitive.Type();

		if (thisType == otherType)
			return;

		type_code resolvedType = _ResolvePriorityType(thisType, otherType);
		if (thisType != resolvedType)
			_GetAsType(resolvedType);

		if (otherType != resolvedType)
			other._GetAsType(resolvedType);
	}

	void _ResolveToPrimitive()
	{
		if (Kind() == OPERAND_KIND_PRIMITIVE)
			return;
		else if (Kind() == OPERAND_KIND_TYPE) {
			throw ParseException("Cannot perform mathematical operations "
				"between type objects.", 0);
		}

		status_t error = fValueNode->LocationAndValueResolutionState();
		if (error != B_OK) {
			BString errorMessage;
			errorMessage.SetToFormat("Failed to resolve value of %s: %"
				B_PRId32 ".", fValueNode->Name().String(), error);
			throw ParseException(errorMessage.String(), 0);
		}

		Value* value = fValueNode->GetValue();
		BVariant tempValue;
		if (value->ToVariant(tempValue))
			SetTo(tempValue);
		else {
			BString error;
			error.SetToFormat("Failed to retrieve value of %s.",
				fValueNode->Name().String());
			throw ParseException(error.String(), 0);
		}
	}

	type_code _ResolvePriorityType(type_code lhs, type_code rhs) const
	{
		size_t byteSize = std::max(BVariant::SizeOfType(lhs),
			BVariant::SizeOfType(rhs));
		bool isFloat = BVariant::TypeIsFloat(lhs)
			|| BVariant::TypeIsFloat(rhs);
		bool isSigned = isFloat;
		if (!isFloat) {
			BVariant::TypeIsInteger(lhs, &isSigned);
			if (!isSigned)
				BVariant::TypeIsInteger(rhs, &isSigned);
		}

		if (isFloat) {
			if (byteSize == sizeof(float))
				return B_FLOAT_TYPE;
			return B_DOUBLE_TYPE;
		}

		switch (byteSize) {
			case 1:
				return isSigned ? B_INT8_TYPE : B_UINT8_TYPE;
			case 2:
				return isSigned ? B_INT16_TYPE : B_UINT16_TYPE;
			case 4:
				return isSigned ? B_INT32_TYPE : B_UINT32_TYPE;
			case 8:
				return isSigned ? B_INT64_TYPE : B_UINT64_TYPE;
			default:
				break;
		}

		BString error;
		error.SetToFormat("Unable to reconcile types %#" B_PRIx32
			" and %#" B_PRIx32, lhs, rhs);
		throw ParseException(error.String(), 0);
	}

private:
	BVariant 		fPrimitive;
	ValueNode*		fValueNode;
	Type*			fType;
	operand_kind	fKind;
};


// #pragma mark - CLanguageExpressionEvaluator::Token


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
	BVariant	value;

	int32		position;
};


// #pragma mark - CLanguageExpressionEvaluator::Tokenizer


class CLanguageExpressionEvaluator::Tokenizer {
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
			if (*fCurrentChar == '0' && fCurrentChar[1] == 'x')
				return _ParseHexOperand();

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
				decimal = true;
				temp << '.';
				fCurrentChar++;

				// optional post comma digits
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
			if (decimal)
				fCurrentToken.value.SetTo(value);
			else {
				fCurrentToken.value.SetTo((int64)strtoll(temp.String(), NULL,
						10));
			}

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

	Token& _ParseHexOperand()
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

		fCurrentToken.value.SetTo((int64)strtoull(
				fCurrentToken.string.String(), NULL, 16));
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
};


// #pragma mark - CLanguageExpressionEvaluator


CLanguageExpressionEvaluator::CLanguageExpressionEvaluator()
	:
	fTokenizer(new Tokenizer()),
	fNodeManager(NULL)
{
}


CLanguageExpressionEvaluator::~CLanguageExpressionEvaluator()
{
	delete fTokenizer;
}


ExpressionResult*
CLanguageExpressionEvaluator::Evaluate(const char* expressionString,
	ValueNodeManager* manager)
{
	fNodeManager = manager;
	fTokenizer->SetTo(expressionString);

	Operand value = _ParseSum();
	Token token = fTokenizer->NextToken();
	if (token.type != TOKEN_END_OF_LINE)
		throw ParseException("parse error", token.position);

	ExpressionResult* result = new(std::nothrow)ExpressionResult;
	if (result != NULL) {
		BReference<ExpressionResult> resultReference(result, true);
		Value* outputValue = NULL;
		BVariant primitive = value.PrimitiveValue();
		if (primitive.IsInteger())
			outputValue = new(std::nothrow) IntegerValue(primitive);
		else if (primitive.IsFloat()) {
			outputValue = new(std::nothrow) FloatValue(
				primitive.ToDouble());
		}

		BReference<Value> valueReference;
		if (outputValue != NULL)
			valueReference.SetTo(outputValue, true);

		if (value.Kind() == OPERAND_KIND_PRIMITIVE) {
			if (outputValue == NULL)
				return NULL;

			result->SetToPrimitive(outputValue);
		} else if (value.Kind() == OPERAND_KIND_VALUE_NODE)
			result->SetToValueNode(value.GetValueNode()->NodeChild());

		resultReference.Detach();
	}

	return result;
}


CLanguageExpressionEvaluator::Operand
CLanguageExpressionEvaluator::_ParseSum()
{
	Operand value = _ParseProduct();

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


CLanguageExpressionEvaluator::Operand
CLanguageExpressionEvaluator::_ParseProduct()
{
	static Operand zero(int64(0LL));

	Operand value = _ParsePower();

	while (true) {
		Token token = fTokenizer->NextToken();
		switch (token.type) {
			case TOKEN_STAR:
				value *= _ParsePower();
				break;
			case TOKEN_SLASH:
			{
				Operand rhs = _ParsePower();
				if (rhs == zero)
					throw ParseException("division by zero", token.position);
				value /= rhs;
				break;
			}

			case TOKEN_MODULO:
			{
				Operand rhs = _ParsePower();
				if (rhs == zero)
					throw ParseException("modulo by zero", token.position);
				value %= rhs;
				break;
			}

			case TOKEN_LOGICAL_AND:
			{
				value.SetTo((value != zero)
					&& (_ParsePower() != zero));

				break;
			}

			case TOKEN_LOGICAL_OR:
			{
				value.SetTo((value != zero)
					|| (_ParsePower() != zero));
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
				value.SetTo((int64)(value == _ParsePower()));
				break;

			case TOKEN_NE:
				value.SetTo((int64)(value != _ParsePower()));
				break;

			case TOKEN_GT:
				value.SetTo((int64)(value > _ParsePower()));
				break;

			case TOKEN_GE:
				value.SetTo((int64)(value >= _ParsePower()));
				break;

			case TOKEN_LT:
				value.SetTo((int64)(value < _ParsePower()));
				break;

			case TOKEN_LE:
				value.SetTo((int64)(value <= _ParsePower()));
				break;

			default:
				fTokenizer->RewindToken();
				return value;
		}
	}
}


CLanguageExpressionEvaluator::Operand
CLanguageExpressionEvaluator::_ParsePower()
{
	Operand value = _ParseUnary();

	while (true) {
		Token token = fTokenizer->NextToken();
		if (token.type != TOKEN_POWER) {
			fTokenizer->RewindToken();
			return value;
		}

		Operand power = _ParseUnary();
		Operand temp = value;
		int64 powerValue = power.PrimitiveValue().ToInt64();
		bool handleNegativePower = false;
		if (powerValue < 0) {
			powerValue = abs(powerValue);
			handleNegativePower = true;
		}

		if (powerValue == 0)
			value.SetTo((int64)1);
		else {
			for (; powerValue > 1; powerValue--)
				value *= temp;
		}

		if (handleNegativePower) {
			temp.SetTo((int64)1);
			temp /= value;
			value = temp;
		}
	}
}


CLanguageExpressionEvaluator::Operand
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
		{
			Operand zero((int64)0);
			return Operand((int64)(_ParseUnary() == zero));
		}

		case TOKEN_IDENTIFIER:
			fTokenizer->RewindToken();
			return _ParseIdentifier();

		default:
			fTokenizer->RewindToken();
			return _ParseAtom();
	}

	return Operand();
}


CLanguageExpressionEvaluator::Operand
CLanguageExpressionEvaluator::_ParseIdentifier(ValueNode* parentNode)
{
	Token token = fTokenizer->NextToken();

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

	return Operand(node);
}


CLanguageExpressionEvaluator::Operand
CLanguageExpressionEvaluator::_ParseAtom()
{
	Token token = fTokenizer->NextToken();
	if (token.type == TOKEN_END_OF_LINE)
		throw ParseException("unexpected end of expression", token.position);

	Operand value;

	if (token.type == TOKEN_CONSTANT)
		value.SetTo(token.value);
	else {
		fTokenizer->RewindToken();

		_EatToken(TOKEN_OPENING_BRACKET);

		value = _ParseSum();

		_EatToken(TOKEN_CLOSING_BRACKET);
	}

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
