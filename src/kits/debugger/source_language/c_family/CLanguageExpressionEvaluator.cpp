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

#include "AutoLocker.h"

#include "CLanguageTokenizer.h"
#include "ExpressionInfo.h"
#include "FloatValue.h"
#include "IntegerFormatter.h"
#include "IntegerValue.h"
#include "ObjectID.h"
#include "StackFrame.h"
#include "SyntheticPrimitiveType.h"
#include "TeamTypeInformation.h"
#include "Thread.h"
#include "Type.h"
#include "TypeHandlerRoster.h"
#include "TypeLookupConstraints.h"
#include "Value.h"
#include "ValueLocation.h"
#include "ValueNode.h"
#include "ValueNodeManager.h"
#include "Variable.h"
#include "VariableValueNodeChild.h"


using namespace CLanguage;


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

		case TOKEN_OPENING_PAREN:
			token = "(";
			break;

		case TOKEN_CLOSING_PAREN:
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


// #pragma mark - CLanguageExpressionEvaluator::InternalVariableID


class CLanguageExpressionEvaluator::InternalVariableID : public ObjectID {
public:
	InternalVariableID(const BVariant& value)
		:
		fValue(value)
	{
	}

	virtual ~InternalVariableID()
	{
	}

	virtual	bool operator==(const ObjectID& other) const
	{
		const InternalVariableID* otherID
			= dynamic_cast<const InternalVariableID*>(&other);
		if (otherID == NULL)
			return false;

		return fValue == otherID->fValue;
	}

protected:
	virtual	uint32 ComputeHashValue() const
	{
		return *(uint32*)(&fValue);
	}

private:
	BVariant fValue;
};


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


// #pragma mark - CLanguageExpressionEvaluator


CLanguageExpressionEvaluator::CLanguageExpressionEvaluator()
	:
	fTokenizer(new Tokenizer()),
	fTypeInfo(NULL),
	fNodeManager(NULL)
{
}


CLanguageExpressionEvaluator::~CLanguageExpressionEvaluator()
{
	delete fTokenizer;
}


ExpressionResult*
CLanguageExpressionEvaluator::Evaluate(const char* expressionString,
	ValueNodeManager* manager, TeamTypeInformation* info)
{
	fNodeManager = manager;
	fTypeInfo = info;
	fTokenizer->SetTo(expressionString);

	Operand value = _ParseSum();
	Token token = fTokenizer->NextToken();
	if (token.type != TOKEN_END_OF_LINE)
		throw ParseException("parse error", token.position);

	ExpressionResult* result = new(std::nothrow)ExpressionResult;
	if (result != NULL) {
		BReference<ExpressionResult> resultReference(result, true);
		if (value.Kind() == OPERAND_KIND_PRIMITIVE) {
			Value* outputValue = NULL;
			BVariant primitive = value.PrimitiveValue();
			if (primitive.IsInteger())
				outputValue = new(std::nothrow) IntegerValue(primitive);
			else if (primitive.IsFloat()) {
				outputValue = new(std::nothrow) FloatValue(
					primitive.ToDouble());
			}

			BReference<Value> valueReference;
			if (outputValue != NULL) {
				valueReference.SetTo(outputValue, true);
				result->SetToPrimitive(outputValue);
			} else
				return NULL;
		} else if (value.Kind() == OPERAND_KIND_VALUE_NODE)
			result->SetToValueNode(value.GetValueNode()->NodeChild());
		else if (value.Kind() == OPERAND_KIND_TYPE)
			result->SetToType(value.GetType());

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

	Operand value = _ParseUnary();

	while (true) {
		Token token = fTokenizer->NextToken();
		switch (token.type) {
			case TOKEN_STAR:
				value *= _ParseUnary();
				break;
			case TOKEN_SLASH:
			{
				Operand rhs = _ParseUnary();
				if (rhs == zero)
					throw ParseException("division by zero", token.position);
				value /= rhs;
				break;
			}

			case TOKEN_MODULO:
			{
				Operand rhs = _ParseUnary();
				if (rhs == zero)
					throw ParseException("modulo by zero", token.position);
				value %= rhs;
				break;
			}

			case TOKEN_LOGICAL_AND:
			{
				value.SetTo((value != zero)
					&& (_ParseUnary() != zero));

				break;
			}

			case TOKEN_LOGICAL_OR:
			{
				value.SetTo((value != zero)
					|| (_ParseUnary() != zero));
				break;
			}

			case TOKEN_BITWISE_AND:
				value &= _ParseUnary();
				break;

			case TOKEN_BITWISE_OR:
				value |= _ParseUnary();
				break;

			case TOKEN_BITWISE_XOR:
				value ^= _ParseUnary();
				break;

			case TOKEN_EQ:
				value.SetTo((int64)(value == _ParseUnary()));
				break;

			case TOKEN_NE:
				value.SetTo((int64)(value != _ParseUnary()));
				break;

			case TOKEN_GT:
				value.SetTo((int64)(value > _ParseUnary()));
				break;

			case TOKEN_GE:
				value.SetTo((int64)(value >= _ParseUnary()));
				break;

			case TOKEN_LT:
				value.SetTo((int64)(value < _ParseUnary()));
				break;

			case TOKEN_LE:
				value.SetTo((int64)(value <= _ParseUnary()));
				break;

			default:
				fTokenizer->RewindToken();
				return value;
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
	const BString& identifierName = token.string;

	ValueNodeChild* child = NULL;
	if (fNodeManager != NULL) {
		ValueNodeContainer* container = fNodeManager->GetContainer();
		AutoLocker<ValueNodeContainer> containerLocker(container);

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
	}

	if (child == NULL && fTypeInfo != NULL) {
		Type* resultType = NULL;
		status_t error = fTypeInfo->LookupTypeByName(identifierName,
			TypeLookupConstraints(), resultType);
		if (error == B_OK) {
			BReference<Type> typeReference(resultType, true);
			return _ParseType(resultType);
		} else if (error != B_ENTRY_NOT_FOUND) {
			BString errorMessage;
			errorMessage.SetToFormat("Failed to look up type name '%s': %"
				B_PRId32 ".", identifierName.String(), error);
			throw ParseException(errorMessage.String(), token.position);
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
		throw ParseException("Unexpected end of expression", token.position);

	Operand value;

	if (token.type == TOKEN_CONSTANT)
		value.SetTo(token.value);
	else {
		fTokenizer->RewindToken();

		_EatToken(TOKEN_OPENING_PAREN);

		value = _ParseSum();

		_EatToken(TOKEN_CLOSING_PAREN);
	}

	if (value.Kind() == OPERAND_KIND_TYPE) {
		token = fTokenizer->NextToken();
		if (token.type == TOKEN_END_OF_LINE)
			return value;

		Type* castType = value.GetType();
		// if our evaluated result was a type, and there still remain
		// further tokens to evaluate, then this is a typecast for
		// a subsequent expression. Attempt to evaluate it, and then
		// apply the cast to the result.
		fTokenizer->RewindToken();
		value = _ParseSum();
		ValueNodeChild* child = NULL;
		if (value.Kind() != OPERAND_KIND_PRIMITIVE
			&& value.Kind() != OPERAND_KIND_VALUE_NODE) {
			throw ParseException("Expected value or variable expression after"
				" typecast.", token.position);
		}

		if (value.Kind() == OPERAND_KIND_VALUE_NODE)
			child = value.GetValueNode()->NodeChild();
		else if (value.Kind() == OPERAND_KIND_PRIMITIVE)
			_GetNodeChildForPrimitive(token, value.PrimitiveValue(), child);

		ValueNode* newNode = NULL;
		status_t error = TypeHandlerRoster::Default()->CreateValueNode(child,
			castType, newNode);
		if (error != B_OK) {
			throw ParseException("Unable to create value node for typecast"
				" operation.", token.position);
		}
		child->SetNode(newNode);
		value.SetTo(newNode);
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

			case TOKEN_SLASH:
				expected = "'/', '\\', or ':'";
				break;

			case TOKEN_END_OF_LINE:
				expected = "'\\n'";
				break;

			default:
				expected << "'" << TokenTypeToString(type) << "'";
				break;
		}

		BString temp;
		temp << "Expected " << expected.String() << " got '" << token.string
			<< "'";
		throw ParseException(temp.String(), token.position);
	}
}


CLanguageExpressionEvaluator::Operand
CLanguageExpressionEvaluator::_ParseType(Type* baseType)
{
	BReference<Type> typeReference;
	Type* finalType = baseType;

	bool arraySpecifierEncountered = false;
	status_t error;
	for (;;) {
		Token token = fTokenizer->NextToken();
		if (token.type == TOKEN_STAR || token.type == TOKEN_BITWISE_AND) {
			if (arraySpecifierEncountered)
				break;

			address_type_kind addressKind = (token.type == TOKEN_STAR)
					? DERIVED_TYPE_POINTER : DERIVED_TYPE_REFERENCE;
			AddressType* derivedType = NULL;
			error = finalType->CreateDerivedAddressType(addressKind,
				derivedType);
			if (error != B_OK) {
				BString errorMessage;
				errorMessage.SetToFormat("Failed to create derived address"
					" type %d for base type %s: %s (%" B_PRId32 ")",
					addressKind, finalType->Name().String(), strerror(error),
					error);
				throw ParseException(errorMessage, token.position);
			}

			finalType = derivedType;
			typeReference.SetTo(finalType, true);
		} else if (token.type == TOKEN_OPENING_SQUARE_BRACKET) {
			Operand indexSize = _ParseSum();
			if (indexSize.Kind() == OPERAND_KIND_TYPE) {
				throw ParseException("Cannot specify type name as array"
					" subscript.", token.position);
			}

			_EatToken(TOKEN_CLOSING_SQUARE_BRACKET);

			uint32 resolvedSize = indexSize.PrimitiveValue().ToUInt32();
			if (resolvedSize == 0) {
				throw ParseException("Non-zero array size required in type"
					" specifier.", token.position);
			}

			ArrayType* derivedType = NULL;
			error = finalType->CreateDerivedArrayType(0, resolvedSize, true,
				derivedType);
			if (error != B_OK) {
				BString errorMessage;
				errorMessage.SetToFormat("Failed to create derived array type"
					" of size %" B_PRIu32 " for base type %s: %s (%"
					B_PRId32 ")", resolvedSize, finalType->Name().String(),
					strerror(error), error);
				throw ParseException(errorMessage, token.position);
			}

			arraySpecifierEncountered = true;
			finalType = derivedType;
			typeReference.SetTo(finalType, true);
		} else
			break;
	}

	typeReference.Detach();
	fTokenizer->RewindToken();
	return Operand(finalType);
}


void
CLanguageExpressionEvaluator::_RequestValueIfNeeded(
	const Token& token, ValueNodeChild* child)
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
CLanguageExpressionEvaluator::_GetNodeChildForPrimitive(const Token& token,
	const BVariant& value, ValueNodeChild*& _output) const
{
	Type* type = new(std::nothrow) SyntheticPrimitiveType(value.Type());
	if (type == NULL) {
		throw ParseException("Out of memory while creating type object.",
			token.position);
	}

	BReference<Type> typeReference(type, true);
	ValueLocation* location = new(std::nothrow) ValueLocation();
	if (location == NULL) {
		throw ParseException("Out of memory while creating location object.",
			token.position);
	}

	BReference<ValueLocation> locationReference(location, true);
	ValuePieceLocation piece;
	if (!piece.SetToValue(value.Bytes(), value.Size())
		|| !location->AddPiece(piece)) {
		throw ParseException("Out of memory populating location"
			" object.", token.position);
	}

	char variableName[32];
	if (!IntegerFormatter::FormatValue(value, INTEGER_FORMAT_HEX_DEFAULT,
		variableName, sizeof(variableName))) {
		throw ParseException("Failed to generate internal variable name.",
			token.position);
	}

	InternalVariableID* id = new(std::nothrow) InternalVariableID(value);
	if (id == NULL) {
		throw ParseException("Out of memory while creating ID object.",
			token.position);
	}

	BReference<ObjectID> idReference(id, true);
	Variable* variable = new(std::nothrow) Variable(id, variableName, type,
		location);
	if (variable == NULL) {
		throw ParseException("Out of memory while creating variable object.",
			token.position);
	}

	BReference<Variable> variableReference(variable, true);
	_output = new(std::nothrow) VariableValueNodeChild(
		variable);
	if (_output == NULL) {
		throw ParseException("Out of memory while creating node child object.",
			token.position);
	}
}
