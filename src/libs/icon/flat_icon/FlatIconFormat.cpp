/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "FlatIconFormat.h"

#include "LittleEndianBuffer.h"


_BEGIN_ICON_NAMESPACE


const uint32 FLAT_ICON_MAGIC = 'ficn';

const char* kVectorAttrNodeName = "BEOS:ICON";
const char* kVectorAttrMimeName = "META:ICON";


// read_coord
bool
read_coord(LittleEndianBuffer& buffer, float& coord)
{
	uint8 value;
	if (!buffer.Read(value))
		return false;

	if (value & 128) {
		// high bit set, the next byte is part of the coord
		uint8 lowValue;
		if (!buffer.Read(lowValue))
			return false;
		value &= 127;
		uint16 coordValue = (value << 8) | lowValue;
		coord = (float)coordValue / 102.0 - 128.0;
	} else {
		// simple coord
		coord = (float)value - 32.0;
	}
	return true;
}

// write_coord
bool
write_coord(LittleEndianBuffer& buffer, float coord)
{
	// clamp coord
	if (coord < -128.0)
		coord = -128.0;
	if (coord > 192.0)
		coord = 192.0;

	if (int(coord * 100.0) == (int)coord * 100
		&& coord >= - 32.0 && coord <= 95.0) {
		// saving coord in 7 bit is sufficient
		uint8 value = (uint8)(coord + 32.0);
		return buffer.Write(value);
	} else {
		// needing to save coord in 15 bits
		uint16 value = (uint16)((coord + 128.0) * 102.0);
		// set high bit to indicate there is only one byte
		value |= 32768;
		uint8 highValue = value >> 8;
		uint8 lowValue = value & 255;
		return buffer.Write(highValue) && buffer.Write(lowValue);
	}
}

// read_float_24
bool
read_float_24(LittleEndianBuffer& buffer, float& _value)
{
	uint8 bufferValue[3];
	if (!buffer.Read(bufferValue[0]) || !buffer.Read(bufferValue[1])
		|| !buffer.Read(bufferValue[2]))
		return false;

	int shortValue = (bufferValue[0] << 16)
		| (bufferValue[1] << 8) | bufferValue[2];

	int sign = (shortValue & 0x800000) >> 23;
	int exponent = ((shortValue & 0x7e0000) >> 17) - 32;
	int mantissa = (shortValue & 0x01ffff) << 6;

	if (shortValue == 0)
		_value = 0.0;
	else {
		union {
			uint32 intValue;
			float floatValue;
		} i2f;
		i2f.intValue = (sign << 31) | ((exponent + 127) << 23) | mantissa;
		_value = i2f.floatValue;
	}

	return true;
}

// write_float_24
bool
write_float_24(LittleEndianBuffer& buffer, float _value)
{
	// 1 bit sign
	// 6 bit exponent
	// 17 bit mantissa
	// TODO: fixme for non-IEEE 754 architectures
	union {
		float floatValue;
		uint32 intValue;
	} f2i;
	f2i.floatValue = _value;

	int sign = (f2i.intValue & 0x80000000) >> 31;
	int exponent = ((f2i.intValue & 0x7f800000) >> 23) - 127;
	int mantissa = f2i.intValue & 0x007fffff;

	if (exponent >= 32 || exponent < -32) {
		uint8 zero = 0;
		return buffer.Write(zero) && buffer.Write(zero)
			&& buffer.Write(zero);
	}

	int shortValue = (sign << 23)
					 | ((exponent + 32) << 17)
					 | (mantissa >> 6);

	return buffer.Write((uint8)(shortValue >> 16))
		&& buffer.Write((uint8)((shortValue >> 8) & 0xff))
		&& buffer.Write((uint8)(shortValue & 0xff));
}


_END_ICON_NAMESPACE
