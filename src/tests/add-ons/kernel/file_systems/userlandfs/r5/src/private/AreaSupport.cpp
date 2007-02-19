// AreaSupport.cpp

#include "AreaSupport.h"

// get_area_for_address
status_t
UserlandFSUtil::get_area_for_address(void* address, int32 size, area_id* area,
	int32* offset, void** areaBaseAddress)
{
	// check parameters
	if (!area || !offset || size < 0)
		return B_BAD_VALUE;
	// catch NULL address case
	if (!address) {
		*area = -1;
		*offset = 0;
		return B_OK;
	}
	// get area and in-area offset
	*area = area_for(address);
	if (*area < 0)
		return *area;
	area_info areaInfo;
	status_t error = get_area_info(*area, &areaInfo);
	if (error != B_OK)
		return error;
	// check the size
	*offset = (uint8*)address - (uint8*)areaInfo.address;
	if (*offset + size > (int32)areaInfo.size)
		return B_BAD_VALUE;
	if (areaBaseAddress)
		*areaBaseAddress = areaInfo.address;
	return B_OK;
}
