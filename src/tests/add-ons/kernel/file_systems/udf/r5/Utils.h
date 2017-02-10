//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_UTILS_H
#define _UDF_UTILS_H

/*! \file Utils.h

	Miscellaneous Udf utility functions.
*/

#ifndef _IMPEXP_KERNEL
#	define _IMPEXP_KERNEL
#endif
#ifdef COMPILE_FOR_R5
extern "C" {
#endif
	#include "fsproto.h"
#ifdef COMPILE_FOR_R5
}
#endif

#include "UdfStructures.h"

namespace Udf {

long_address to_long_address(vnode_id id, uint32 length = 0);

vnode_id to_vnode_id(long_address address);

time_t make_time(timestamp &timestamp);

status_t get_block_shift(uint32 blockSize, uint32 &blockShift);

const char* bool_to_string(bool value);

status_t check_size_error(ssize_t bytesReturned, ssize_t bytesExpected);

uint16 calculate_crc(uint8 *data, uint16 length);

} // namespace Udf

#endif	// _UDF_UTILS_H

