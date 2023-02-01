/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef _XFS_CKSUM_H
#define _XFS_CKSUM_H


#include "CRCTable.h"
#include "xfs_types.h"
#include "system_dependencies.h"


#define XFS_CRC_SEED	(~(uint32)0)


/*
   Calculate the intermediate checksum for a buffer that has the CRC field
   inside it.  The offset of the 32bit crc fields is passed as the
   cksum_offset parameter. We do not modify the buffer during verification,
   hence we have to split the CRC calculation across the cksum_offset.
*/
static inline uint32
xfs_start_cksum_safe(const char *buffer, size_t length, uint32 cksum_offset)
{
	uint32 zero = 0;
	uint32 crc;

	// Calculate CRC up to the checksum.
	crc = calculate_crc32c(XFS_CRC_SEED, (uint8*)buffer, cksum_offset);

	// Skip checksum field
	crc = calculate_crc32c(crc, (uint8*)&zero, sizeof(uint32));

	// Calculate the rest of the CRC.
	return calculate_crc32c(crc, (uint8*)buffer + cksum_offset + sizeof(uint32),
		      length - (cksum_offset + sizeof(uint32)));
}


/*
   Fast CRC method where the buffer is modified. Callers must have exclusive
   access to the buffer while the calculation takes place.
*/
static inline uint32
xfs_start_cksum_update(const char *buffer, size_t length, uint32 cksum_offset)
{
	// zero the CRC field
	*(uint32 *)(buffer + cksum_offset) = 0;

	// single pass CRC calculation for the entire buffer
	return calculate_crc32c(XFS_CRC_SEED, (uint8*)buffer, length);
}


/*
   Helper to generate the checksum for a buffer.

   This modifies the buffer temporarily - callers must have exclusive
   access to the buffer while the calculation takes place.
*/
static inline void
xfs_update_cksum(const char *buffer, size_t length, uint32 cksum_offset)
{
	uint32 crc = xfs_start_cksum_update(buffer, length, cksum_offset);

	*(uint32 *)(buffer + cksum_offset) = ~crc;
}


/*
   Helper to verify the checksum for a buffer.
*/
static inline int
xfs_verify_cksum(const char *buffer, size_t length, uint32 cksum_offset)
{
	uint32 crc = xfs_start_cksum_safe(buffer, length, cksum_offset);

	TRACE("calculated crc: (%" B_PRIu32 ")\n", ~crc);

	TRACE("buffer = %s, cksum_offset: (%" B_PRIu32 ")\n", buffer, cksum_offset);

	return *(uint32 *)(buffer + cksum_offset) == (~crc);
}

#endif