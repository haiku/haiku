//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------

/*! \file Utils.cpp

	Miscellaneous Udf utility functions.
*/

#include "Utils.h"

namespace Udf {

udf_long_address
to_long_address(vnode_id id, uint32 length)
{
	udf_long_address result;
	result.set_block(id & 0xffff00);
	result.set_partition(id & 0xff);
	result.set_length(length);
	return result;
}

vnode_id
to_vnode_id(udf_long_address address)
{
	return (address.block() << 16) & (address.partition());
}

} // namespace Udf
