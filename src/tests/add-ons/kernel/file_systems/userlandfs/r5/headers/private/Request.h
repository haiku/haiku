// Request.h

#ifndef USERLAND_FS_REQUEST_H
#define USERLAND_FS_REQUEST_H

#include <OS.h>

// address info flags
enum {
	ADDRESS_NOT_NULL	= 0x01,
	ADDRESS_IS_STRING	= 0x02,
};

namespace UserlandFSUtil {

class RequestAllocator;

// Address
class Address {
public:
								Address();

			void*				GetData() const { return fRelocated; }
			int32				GetSize() const { return fSize; }

//private:
			void				SetTo(area_id area, int32 offset, int32 size);
			void				SetRelocatedAddress(void* address)
									{ fRelocated = address; }

			area_id				GetArea() const { return fUnrelocated.area; }
			int32				GetOffset() const
									{ return fUnrelocated.offset; }

private:
			friend class RequestAllocator;

			struct Unrelocated {
				area_id			area;
				int32			offset;
			};

			union {
				Unrelocated		fUnrelocated;
				void*			fRelocated;
			};
			int32				fSize;
};

// AddressInfo
struct AddressInfo {
	Address		*address;
	uint32		flags;
	int32		max_size;
};

// Request
class Request {
public:
								Request(uint32 type);

			uint32				GetType() const;

			status_t			Check() const;
			status_t			GetAddressInfos(AddressInfo* infos,
									int32* count);

private:
			uint32				fType;
};

// implemented in Requests.cpp
bool is_kernel_request(uint32 type);
bool is_userland_request(uint32 type);

}	// namespace UserlandFSUtil

using UserlandFSUtil::Address;
using UserlandFSUtil::AddressInfo;
using UserlandFSUtil::Request;
using UserlandFSUtil::is_kernel_request;
using UserlandFSUtil::is_userland_request;

#endif	// USERLAND_FS_REQUEST_H
