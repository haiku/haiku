//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_UTILS_H
#define _UDF_UTILS_H

/*! \file Utils.h

	Miscellaneous Udf utility functions.
*/

extern "C" {
	#ifndef _IMPEXP_KERNEL
	#	define _IMPEXP_KERNEL
	#endif
	
	#include <fsproto.h>
}

#include "UdfStructures.h"

namespace Udf {

long_address to_long_address(vnode_id id, uint32 length = 0);

vnode_id to_vnode_id(long_address address);

time_t make_time(timestamp &timestamp);

status_t get_block_shift(uint32 blockSize, uint32 &blockShift);

} // namespace Udf

#endif	// _UDF_UTILS_H

