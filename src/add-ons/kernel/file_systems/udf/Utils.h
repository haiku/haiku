/*
 * Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UDF_UTILS_H
#define _UDF_UTILS_H

#include <SupportDefs.h>

/*! \file Utils.h - Miscellaneous Udf utility functions. */

class timestamp;
struct long_address;

const char		*bool_to_string(bool value);
uint16			calculate_crc(uint8 *data, uint16 length);
status_t		check_size_error(ssize_t bytesReturned, ssize_t bytesExpected);
status_t		get_block_shift(uint32 blockSize, uint32 &blockShift);
status_t		decode_time(timestamp &timestamp, struct timespec &timespec);
long_address 	to_long_address(ino_t id, uint32 length = 0);
ino_t			to_vnode_id(long_address address);

#endif	// _UDF_UTILS_H
