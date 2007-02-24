// Request.cpp

#include "Request.h"

// Address

// constructor
Address::Address()
	: fSize(0)
{
	fUnrelocated.area = -1;
	fUnrelocated.offset = 0;
}

// SetTo
void
Address::SetTo(area_id area, int32 offset, int32 size)
{
	fUnrelocated.area = area;
	fUnrelocated.offset = offset;
	fSize = size;
}


// Request

// constructor
Request::Request(uint32 type)
	: fType(type)
{
}

// GetType
uint32
Request::GetType() const
{
	return fType;
}

// Check
status_t
Request::Check() const
{
	return B_OK;
}

// GetAddressInfos
status_t
Request::GetAddressInfos(AddressInfo* infos, int32* count)
{
	*count = 0;
	return B_OK;
}

