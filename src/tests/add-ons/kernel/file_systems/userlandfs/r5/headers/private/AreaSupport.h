// AreaSupport.h

#ifndef USERLAND_FS_AREA_SUPPORT_H
#define USERLAND_FS_AREA_SUPPORT_H

#include <OS.h>

namespace UserlandFSUtil {

status_t get_area_for_address(void* address, int32 size, area_id* area,
	int32* offset, void** areaBaseAddress = NULL);

}	// namespace UserlandFSUtil

using UserlandFSUtil::get_area_for_address;

#endif	// USERLAND_FS_AREA_SUPPORT_H
