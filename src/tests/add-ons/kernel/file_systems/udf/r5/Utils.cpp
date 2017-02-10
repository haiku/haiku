//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------

/*! \file Utils.cpp

	Miscellaneous Udf utility functions.
*/

#include "Utils.h"

extern "C" {
	#ifndef _IMPEXP_KERNEL
	#	define _IMPEXP_KERNEL
	#endif
	
	extern int32 timezone_offset;
}


namespace Udf {

long_address
to_long_address(vnode_id id, uint32 length)
{
	DEBUG_INIT_ETC(NULL, ("vnode_id: %Ld (0x%Lx), length: %ld", id, id, length));
	long_address result;
	result.set_block((id >> 16) & 0xffffffff);
	result.set_partition(id & 0xffff);
	result.set_length(length);
	DUMP(result);
	return result;
}

vnode_id
to_vnode_id(long_address address)
{
	DEBUG_INIT(NULL);
	vnode_id result = address.block();
	result <<= 16;
	result |= address.partition();
	PRINT(("block:     %ld, 0x%lx\n", address.block(), address.block())); 
	PRINT(("partition: %d, 0x%x\n", address.partition(), address.partition())); 
	PRINT(("length:    %ld, 0x%lx\n", address.length(), address.length()));
	PRINT(("vnode_id:  %Ld, 0x%Lx\n", result, result));
	return result;
}

time_t
make_time(timestamp &timestamp)
{
	DEBUG_INIT_ETC(NULL, ("timestamp: (tnt: 0x%x, type: %d, timezone: %d = 0x%x, year: %d, " 
	           "month: %d, day: %d, hour: %d, minute: %d, second: %d)", timestamp.type_and_timezone(),
	           timestamp.type(), timestamp.timezone(),
	            timestamp.timezone(),timestamp.year(),
	           timestamp.month(), timestamp.day(), timestamp.hour(), timestamp.minute(), timestamp.second()));

	time_t result = 0;
	
	if (timestamp.year() >= 1970) {
		const int monthLengths[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

		int year = timestamp.year();
		int month = timestamp.month();
		int day = timestamp.day();
		int hour = timestamp.hour();
		int minute = timestamp.minute();
		int second = timestamp.second();

		// Range check the timezone offset, then round it down
		// to the nearest hour, since no one I know treats timezones
		// with a per-minute granularity, and none of the other OSes
		// I've looked at appear to either.
		int timezone_offset = timestamp.timezone();
		if (-1440 > timezone_offset || timezone_offset > 1440)
			timezone_offset = 0;
		timezone_offset -= timezone_offset % 60;
		
		int previousLeapYears = (year - 1968) / 4;
		bool isLeapYear = (year - 1968) % 4 == 0;
		if (isLeapYear)
			--previousLeapYears;
		
		// Years to days
		result = (year - 1970) * 365 + previousLeapYears;
		// Months to days
		for (int i = 0; i < month-1; i++) {
			result += monthLengths[i];
		}
		if (month > 2 && isLeapYear)
			++result;
		// Days to hours
		result = (result + day - 1) * 24;
		// Hours to minutes
		result = (result + hour) * 60 + timezone_offset;
		// Minutes to seconds
		result = (result + minute) * 60 + second;		
	}
	
	return result;
}

/*! \brief Calculates the block shift amount for the given
 	block size, which must be a positive power of 2.
*/
status_t
Udf::get_block_shift(uint32 blockSize, uint32 &blockShift)
{
	if (blockSize == 0)
		return B_BAD_VALUE;		
	uint32 bitCount = 0;
	uint32 result = 0;
	for (int i = 0; i < 32; i++) {
		// Zero out all bits except bit i
		uint32 block = blockSize & (uint32(1) << i);
		if (block) {
			if (++bitCount > 1) {
				return B_BAD_VALUE;
			} else {
				result = i;
			}			
		}
	}
	blockShift = result;
	return B_OK;
}	

/*! \brief Returns "true" if \a value is true, "false" otherwise.
*/
const char*
Udf::bool_to_string(bool value)
{
	return value ? "true" : "false";
}

/*! \brief Takes an overloaded ssize_t return value like those returned
	by BFile::Read() and friends, as well as an expected number of bytes,
	and returns B_OK if the byte counts match, or the appropriate error
	code otherwise.
*/
status_t
Udf::check_size_error(ssize_t bytesReturned, ssize_t bytesExpected)
{
	return bytesReturned == bytesExpected
	       ? B_OK
	       : (bytesReturned >= 0 ? B_IO_ERROR : status_t(bytesReturned));
}

/*! \brief Calculates the UDF crc checksum for the given byte stream.

	Based on crc code from UDF-2.50 6.5, as permitted.

	\param data Pointer to the byte stream.
	\param length Length of the byte stream in bytes.
	
	\return The crc checksum, or 0 if an error occurred.
*/
uint16
Udf::calculate_crc(uint8 *data, uint16 length)
{
	uint16 crc = 0;
	if (data) {
		for ( ; length > 0; length--, data++) 
			crc = Udf::kCrcTable[(crc >> 8 ^ *data) & 0xff] ^ (crc << 8);
	}
	return crc;
}

} // namespace Udf

