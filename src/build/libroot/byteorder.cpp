
#ifdef BUILDING_FS_SHELL
#	include "compat.h"
#else
#	include <BeOSBuildCompatibility.h>
#endif

#include <ByteOrder.h>

uint32
__swap_int32(uint32 value)
{
	return (value >> 24) | ((value & 0xff0000) >> 8) | ((value & 0xff00) << 8)
		| (value << 24);
}

uint64
__swap_int64(uint64 value)
{
	return uint64(__swap_int32(uint32(value >> 32)))
		| (uint64(__swap_int32(uint32(value))) << 32);
}
