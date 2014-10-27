/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "Number.h"

#include <stdlib.h>

#include <String.h>


Number::Number()
	:
	fValue()
{
}


Number::~Number()
{
}


Number::Number(type_code type, const BString& value)
{
	SetTo(type, value);
}


Number::Number(const BVariant& value)
	:
	fValue(value)
{
}


Number::Number(const Number& other)
	:
	fValue(other.fValue)
{
}


void
Number::SetTo(type_code type, const BString& value, int32 base)
{
	switch (type) {
		case B_INT8_TYPE:
		{
			int8 tempValue = (int8)strtol(value.String(), NULL, base);
			fValue.SetTo(tempValue);
			break;
		}

		case B_UINT8_TYPE:
		{
			uint8 tempValue = (uint8)strtoul(value.String(), NULL, base);
			fValue.SetTo(tempValue);
			break;
		}

		case B_INT16_TYPE:
		{
			int16 tempValue = (int16)strtol(value.String(), NULL, base);
			fValue.SetTo(tempValue);
			break;
		}

		case B_UINT16_TYPE:
		{
			uint16 tempValue = (uint16)strtoul(value.String(), NULL, base);
			fValue.SetTo(tempValue);
			break;
		}

		case B_INT32_TYPE:
		{
			int32 tempValue = (int32)strtol(value.String(), NULL, base);
			fValue.SetTo(tempValue);
			break;
		}

		case B_UINT32_TYPE:
		{
			uint32 tempValue = (int32)strtoul(value.String(), NULL, base);
			fValue.SetTo(tempValue);
			break;
		}

		case B_INT64_TYPE:
		{
			int64 tempValue = (int64)strtoll(value.String(), NULL, base);
			fValue.SetTo(tempValue);
			break;
		}

		case B_UINT64_TYPE:
		{
			uint64 tempValue = (uint64)strtol(value.String(), NULL, base);
			fValue.SetTo(tempValue);
			break;
		}

		case B_FLOAT_TYPE:
		{
			float tempValue = strtof(value.String(), NULL);
			fValue.SetTo(tempValue);
			break;
		}

		case B_DOUBLE_TYPE:
		{
			double tempValue = strtod(value.String(), NULL);
			fValue.SetTo(tempValue);
			break;
		}
	}
}


void
Number::SetTo(const BVariant& value)
{
	fValue = value;
}


Number&
Number::operator=(const Number& rhs)
{
	fValue = rhs.fValue;
	return *this;
}


Number&
Number::operator+=(const Number& rhs)
{
	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			fValue.SetTo((int8)(fValue.ToInt8() + rhs.fValue.ToInt8()));
			break;
		}

		case B_UINT8_TYPE:
		{
			fValue.SetTo((uint8)(fValue.ToUInt8() + rhs.fValue.ToUInt8()));
			break;
		}

		case B_INT16_TYPE:
		{
			fValue.SetTo((int16)(fValue.ToInt16() + rhs.fValue.ToInt16()));
			break;
		}

		case B_UINT16_TYPE:
		{
			fValue.SetTo((uint16)(fValue.ToUInt16() + rhs.fValue.ToUInt16()));
			break;
		}

		case B_INT32_TYPE:
		{
			fValue.SetTo(fValue.ToInt32() + rhs.fValue.ToInt32());
			break;
		}

		case B_UINT32_TYPE:
		{
			fValue.SetTo(fValue.ToUInt32() + rhs.fValue.ToUInt32());
			break;
		}

		case B_INT64_TYPE:
		{
			fValue.SetTo(fValue.ToInt64() + rhs.fValue.ToInt64());
			break;
		}

		case B_UINT64_TYPE:
		{
			fValue.SetTo(fValue.ToUInt64() + rhs.fValue.ToUInt64());
			break;
		}

		case B_FLOAT_TYPE:
		{
			fValue.SetTo(fValue.ToFloat() + rhs.fValue.ToFloat());
			break;
		}

		case B_DOUBLE_TYPE:
		{
			fValue.SetTo(fValue.ToDouble() + rhs.fValue.ToDouble());
			break;
		}
	}

	return *this;
}


Number&
Number::operator-=(const Number& rhs)
{
	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			fValue.SetTo((int8)(fValue.ToInt8() - rhs.fValue.ToInt8()));
			break;
		}

		case B_UINT8_TYPE:
		{
			fValue.SetTo((uint8)(fValue.ToUInt8() - rhs.fValue.ToUInt8()));
			break;
		}

		case B_INT16_TYPE:
		{
			fValue.SetTo((int16)(fValue.ToInt16() - rhs.fValue.ToInt16()));
			break;
		}

		case B_UINT16_TYPE:
		{
			fValue.SetTo((uint16)(fValue.ToUInt16() - rhs.fValue.ToUInt16()));
			break;
		}

		case B_INT32_TYPE:
		{
			fValue.SetTo(fValue.ToInt32() - rhs.fValue.ToInt32());
			break;
		}

		case B_UINT32_TYPE:
		{
			fValue.SetTo(fValue.ToUInt32() - rhs.fValue.ToUInt32());
			break;
		}

		case B_INT64_TYPE:
		{
			fValue.SetTo(fValue.ToInt64() - rhs.fValue.ToInt64());
			break;
		}

		case B_UINT64_TYPE:
		{
			fValue.SetTo(fValue.ToUInt64() - rhs.fValue.ToUInt64());
			break;
		}

		case B_FLOAT_TYPE:
		{
			fValue.SetTo(fValue.ToFloat() - rhs.fValue.ToFloat());
			break;
		}

		case B_DOUBLE_TYPE:
		{
			fValue.SetTo(fValue.ToDouble() - rhs.fValue.ToDouble());
			break;
		}
	}

	return *this;
}


Number&
Number::operator/=(const Number& rhs)
{
	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			fValue.SetTo((int8)(fValue.ToInt8() / rhs.fValue.ToInt8()));
			break;
		}

		case B_UINT8_TYPE:
		{
			fValue.SetTo((uint8)(fValue.ToUInt8() / rhs.fValue.ToUInt8()));
			break;
		}

		case B_INT16_TYPE:
		{
			fValue.SetTo((int16)(fValue.ToInt16() / rhs.fValue.ToInt16()));
			break;
		}

		case B_UINT16_TYPE:
		{
			fValue.SetTo((uint16)(fValue.ToUInt16() / rhs.fValue.ToUInt16()));
			break;
		}

		case B_INT32_TYPE:
		{
			fValue.SetTo(fValue.ToInt32() / rhs.fValue.ToInt32());
			break;
		}

		case B_UINT32_TYPE:
		{
			fValue.SetTo(fValue.ToUInt32() / rhs.fValue.ToUInt32());
			break;
		}

		case B_INT64_TYPE:
		{
			fValue.SetTo(fValue.ToInt64() / rhs.fValue.ToInt64());
			break;
		}

		case B_UINT64_TYPE:
		{
			fValue.SetTo(fValue.ToUInt64() / rhs.fValue.ToUInt64());
			break;
		}

		case B_FLOAT_TYPE:
		{
			fValue.SetTo(fValue.ToFloat() / rhs.fValue.ToFloat());
			break;
		}

		case B_DOUBLE_TYPE:
		{
			fValue.SetTo(fValue.ToDouble() / rhs.fValue.ToDouble());
			break;
		}
	}

	return *this;
}


Number&
Number::operator%=(const Number& rhs)
{
	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			fValue.SetTo((int8)(fValue.ToInt8() % rhs.fValue.ToInt8()));
			break;
		}

		case B_UINT8_TYPE:
		{
			fValue.SetTo((uint8)(fValue.ToUInt8() % rhs.fValue.ToUInt8()));
			break;
		}

		case B_INT16_TYPE:
		{
			fValue.SetTo((int16)(fValue.ToInt16() % rhs.fValue.ToInt16()));
			break;
		}

		case B_UINT16_TYPE:
		{
			fValue.SetTo((uint16)(fValue.ToUInt16() % rhs.fValue.ToUInt16()));
			break;
		}

		case B_INT32_TYPE:
		{
			fValue.SetTo(fValue.ToInt32() % rhs.fValue.ToInt32());
			break;
		}

		case B_UINT32_TYPE:
		{
			fValue.SetTo(fValue.ToUInt32() % rhs.fValue.ToUInt32());
			break;
		}

		case B_INT64_TYPE:
		{
			fValue.SetTo(fValue.ToInt64() % rhs.fValue.ToInt64());
			break;
		}

		case B_UINT64_TYPE:
		{
			fValue.SetTo(fValue.ToUInt64() % rhs.fValue.ToUInt64());
			break;
		}
	}

	return *this;
}


Number&
Number::operator*=(const Number& rhs)
{
	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			fValue.SetTo((int8)(fValue.ToInt8() * rhs.fValue.ToInt8()));
			break;
		}

		case B_UINT8_TYPE:
		{
			fValue.SetTo((uint8)(fValue.ToUInt8() * rhs.fValue.ToUInt8()));
			break;
		}

		case B_INT16_TYPE:
		{
			fValue.SetTo((int16)(fValue.ToInt16() * rhs.fValue.ToInt16()));
			break;
		}

		case B_UINT16_TYPE:
		{
			fValue.SetTo((uint16)(fValue.ToUInt16() * rhs.fValue.ToUInt16()));
			break;
		}

		case B_INT32_TYPE:
		{
			fValue.SetTo(fValue.ToInt32() * rhs.fValue.ToInt32());
			break;
		}

		case B_UINT32_TYPE:
		{
			fValue.SetTo(fValue.ToUInt32() * rhs.fValue.ToUInt32());
			break;
		}

		case B_INT64_TYPE:
		{
			fValue.SetTo(fValue.ToInt64() * rhs.fValue.ToInt64());
			break;
		}

		case B_UINT64_TYPE:
		{
			fValue.SetTo(fValue.ToUInt64() * rhs.fValue.ToUInt64());
			break;
		}

		case B_FLOAT_TYPE:
		{
			fValue.SetTo(fValue.ToFloat() * rhs.fValue.ToFloat());
			break;
		}

		case B_DOUBLE_TYPE:
		{
			fValue.SetTo(fValue.ToDouble() * rhs.fValue.ToDouble());
			break;
		}
	}

	return *this;
}


Number&
Number::operator&=(const Number& rhs)
{
	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			fValue.SetTo((int8)(fValue.ToInt8() & rhs.fValue.ToInt8()));
			break;
		}

		case B_UINT8_TYPE:
		{
			fValue.SetTo((uint8)(fValue.ToUInt8() & rhs.fValue.ToUInt8()));
			break;
		}

		case B_INT16_TYPE:
		{
			fValue.SetTo((int16)(fValue.ToInt16() & rhs.fValue.ToInt16()));
			break;
		}

		case B_UINT16_TYPE:
		{
			fValue.SetTo((uint16)(fValue.ToUInt16() & rhs.fValue.ToUInt16()));
			break;
		}

		case B_INT32_TYPE:
		{
			fValue.SetTo(fValue.ToInt32() & rhs.fValue.ToInt32());
			break;
		}

		case B_UINT32_TYPE:
		{
			fValue.SetTo(fValue.ToUInt32() & rhs.fValue.ToUInt32());
			break;
		}

		case B_INT64_TYPE:
		{
			fValue.SetTo(fValue.ToInt64() & rhs.fValue.ToInt64());
			break;
		}

		case B_UINT64_TYPE:
		{
			fValue.SetTo(fValue.ToUInt64() & rhs.fValue.ToUInt64());
			break;
		}
	}

	return *this;
}


Number&
Number::operator|=(const Number& rhs)
{
	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			fValue.SetTo((int8)(fValue.ToInt8() | rhs.fValue.ToInt8()));
			break;
		}

		case B_UINT8_TYPE:
		{
			fValue.SetTo((uint8)(fValue.ToUInt8() | rhs.fValue.ToUInt8()));
			break;
		}

		case B_INT16_TYPE:
		{
			fValue.SetTo((int16)(fValue.ToInt16() | rhs.fValue.ToInt16()));
			break;
		}

		case B_UINT16_TYPE:
		{
			fValue.SetTo((uint16)(fValue.ToUInt16() | rhs.fValue.ToUInt16()));
			break;
		}

		case B_INT32_TYPE:
		{
			fValue.SetTo(fValue.ToInt32() | rhs.fValue.ToInt32());
			break;
		}

		case B_UINT32_TYPE:
		{
			fValue.SetTo(fValue.ToUInt32() | rhs.fValue.ToUInt32());
			break;
		}

		case B_INT64_TYPE:
		{
			fValue.SetTo(fValue.ToInt64() | rhs.fValue.ToInt64());
			break;
		}

		case B_UINT64_TYPE:
		{
			fValue.SetTo(fValue.ToUInt64() | rhs.fValue.ToUInt64());
			break;
		}
	}

	return *this;
}


Number&
Number::operator^=(const Number& rhs)
{
	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			fValue.SetTo((int8)(fValue.ToInt8() ^ rhs.fValue.ToInt8()));
			break;
		}

		case B_UINT8_TYPE:
		{
			fValue.SetTo((uint8)(fValue.ToUInt8() ^ rhs.fValue.ToUInt8()));
			break;
		}

		case B_INT16_TYPE:
		{
			fValue.SetTo((int16)(fValue.ToInt16() ^ rhs.fValue.ToInt16()));
			break;
		}

		case B_UINT16_TYPE:
		{
			fValue.SetTo((uint16)(fValue.ToUInt16() ^ rhs.fValue.ToUInt16()));
			break;
		}

		case B_INT32_TYPE:
		{
			fValue.SetTo(fValue.ToInt32() ^ rhs.fValue.ToInt32());
			break;
		}

		case B_UINT32_TYPE:
		{
			fValue.SetTo(fValue.ToUInt32() ^ rhs.fValue.ToUInt32());
			break;
		}

		case B_INT64_TYPE:
		{
			fValue.SetTo(fValue.ToInt64() ^ rhs.fValue.ToInt64());
			break;
		}

		case B_UINT64_TYPE:
		{
			fValue.SetTo(fValue.ToUInt64() ^ rhs.fValue.ToUInt64());
			break;
		}
	}

	return *this;
}


Number
Number::operator-() const
{
	Number value(*this);

	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			value.fValue.SetTo((int8)-fValue.ToInt8());
			break;
		}

		case B_UINT8_TYPE:
		{
			value.fValue.SetTo((uint8)-fValue.ToUInt8());
			break;
		}

		case B_INT16_TYPE:
		{
			value.fValue.SetTo((int16)-fValue.ToInt16());
			break;
		}

		case B_UINT16_TYPE:
		{
			value.fValue.SetTo((uint16)-fValue.ToUInt16());
			break;
		}

		case B_INT32_TYPE:
		{
			value.fValue.SetTo(-fValue.ToInt32());
			break;
		}

		case B_UINT32_TYPE:
		{
			value.fValue.SetTo(-fValue.ToUInt32());
			break;
		}

		case B_INT64_TYPE:
		{
			value.fValue.SetTo(-fValue.ToInt64());
			break;
		}

		case B_UINT64_TYPE:
		{
			value.fValue.SetTo(-fValue.ToUInt64());
			break;
		}

		case B_FLOAT_TYPE:
		{
			value.fValue.SetTo(-fValue.ToFloat());
			break;
		}

		case B_DOUBLE_TYPE:
		{
			value.fValue.SetTo(-fValue.ToDouble());
			break;
		}
	}

	return value;
}


Number
Number::operator~() const
{
	Number value(*this);

	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			value.fValue.SetTo((int8)~fValue.ToInt8());
			break;
		}

		case B_UINT8_TYPE:
		{
			value.fValue.SetTo((uint8)~fValue.ToUInt8());
			break;
		}

		case B_INT16_TYPE:
		{
			value.fValue.SetTo((int16)~fValue.ToInt16());
			break;
		}

		case B_UINT16_TYPE:
		{
			value.fValue.SetTo((uint16)~fValue.ToUInt16());
			break;
		}

		case B_INT32_TYPE:
		{
			value.fValue.SetTo(~fValue.ToInt32());
			break;
		}

		case B_UINT32_TYPE:
		{
			value.fValue.SetTo(~fValue.ToUInt32());
			break;
		}

		case B_INT64_TYPE:
		{
			value.fValue.SetTo(~fValue.ToInt64());
			break;
		}

		case B_UINT64_TYPE:
		{
			value.fValue.SetTo(~fValue.ToUInt64());
			break;
		}
	}

	return value;
}


int
Number::operator<(const Number& rhs) const
{
	int result = 0;
	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			result = fValue.ToInt8() < rhs.fValue.ToInt8();
			break;
		}

		case B_UINT8_TYPE:
		{
			result = fValue.ToUInt8() < rhs.fValue.ToUInt8();
			break;
		}

		case B_INT16_TYPE:
		{
			result = fValue.ToInt16() < rhs.fValue.ToInt16();
			break;
		}

		case B_UINT16_TYPE:
		{
			result = fValue.ToUInt16() < rhs.fValue.ToUInt16();
			break;
		}

		case B_INT32_TYPE:
		{
			result = fValue.ToInt32() < rhs.fValue.ToInt32();
			break;
		}

		case B_UINT32_TYPE:
		{
			result = fValue.ToUInt32() < rhs.fValue.ToUInt32();
			break;
		}

		case B_INT64_TYPE:
		{
			result = fValue.ToInt64() < rhs.fValue.ToInt64();
			break;
		}

		case B_UINT64_TYPE:
		{
			result = fValue.ToUInt64() < rhs.fValue.ToUInt64();
			break;
		}

		case B_FLOAT_TYPE:
		{
			result = fValue.ToFloat() < rhs.fValue.ToFloat();
			break;
		}

		case B_DOUBLE_TYPE:
		{
			result = fValue.ToDouble() < rhs.fValue.ToDouble();
			break;
		}
	}

	return result;
}


int
Number::operator<=(const Number& rhs) const
{
	return (*this < rhs) || (*this == rhs);
}


int
Number::operator>(const Number& rhs) const
{
	int result = 0;
	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			result = fValue.ToInt8() > rhs.fValue.ToInt8();
			break;
		}

		case B_UINT8_TYPE:
		{
			result = fValue.ToUInt8() > rhs.fValue.ToUInt8();
			break;
		}

		case B_INT16_TYPE:
		{
			result = fValue.ToInt16() > rhs.fValue.ToInt16();
			break;
		}

		case B_UINT16_TYPE:
		{
			result = fValue.ToUInt16() > rhs.fValue.ToUInt16();
			break;
		}

		case B_INT32_TYPE:
		{
			result = fValue.ToInt32() > rhs.fValue.ToInt32();
			break;
		}

		case B_UINT32_TYPE:
		{
			result = fValue.ToUInt32() > rhs.fValue.ToUInt32();
			break;
		}

		case B_INT64_TYPE:
		{
			result = fValue.ToInt64() > rhs.fValue.ToInt64();
			break;
		}

		case B_UINT64_TYPE:
		{
			result = fValue.ToUInt64() > rhs.fValue.ToUInt64();
			break;
		}

		case B_FLOAT_TYPE:
		{
			result = fValue.ToFloat() > rhs.fValue.ToFloat();
			break;
		}

		case B_DOUBLE_TYPE:
		{
			result = fValue.ToDouble() > rhs.fValue.ToDouble();
			break;
		}
	}

	return result;
}


int
Number::operator>=(const Number& rhs) const
{
	return (*this > rhs) || (*this == rhs);
}


int
Number::operator==(const Number& rhs) const
{
	int result = 0;
	switch (fValue.Type()) {
		case B_INT8_TYPE:
		{
			result = fValue.ToInt8() == rhs.fValue.ToInt8();
			break;
		}

		case B_UINT8_TYPE:
		{
			result = fValue.ToUInt8() == rhs.fValue.ToUInt8();
			break;
		}

		case B_INT16_TYPE:
		{
			result = fValue.ToInt16() == rhs.fValue.ToInt16();
			break;
		}

		case B_UINT16_TYPE:
		{
			result = fValue.ToUInt16() == rhs.fValue.ToUInt16();
			break;
		}

		case B_INT32_TYPE:
		{
			result = fValue.ToInt32() == rhs.fValue.ToInt32();
			break;
		}

		case B_UINT32_TYPE:
		{
			result = fValue.ToUInt32() == rhs.fValue.ToUInt32();
			break;
		}

		case B_INT64_TYPE:
		{
			result = fValue.ToInt64() == rhs.fValue.ToInt64();
			break;
		}

		case B_UINT64_TYPE:
		{
			result = fValue.ToUInt64() == rhs.fValue.ToUInt64();
			break;
		}

		case B_FLOAT_TYPE:
		{
			result = fValue.ToFloat() == rhs.fValue.ToFloat();
			break;
		}

		case B_DOUBLE_TYPE:
		{
			result = fValue.ToDouble() == rhs.fValue.ToDouble();
			break;
		}
	}

	return result;
}


int
Number::operator!=(const Number& rhs) const
{
	return !(*this == rhs);
}


BVariant
Number::GetValue() const
{
	return fValue;
}

