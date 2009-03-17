/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef USERLAND_FS_REQUEST_ALLOCATOR_H
#define USERLAND_FS_REQUEST_ALLOCATOR_H

#include <new>

#include <OS.h>

#include "Debug.h"
#include "Requests.h"

namespace UserlandFSUtil {

class Port;

// RequestAllocator
class RequestAllocator {
public:
								RequestAllocator(Port* port);
								~RequestAllocator();

			status_t			Init(Port* port);
			void				Uninit();

			status_t			Error() const;

			void				FinishDeferredInit();

			status_t			AllocateRequest(int32 size);
			status_t			ReadRequest(bigtime_t timeout);

			Request*			GetRequest() const;
			int32				GetRequestSize() const;

			status_t			AllocateAddress(Address& address, int32 size,
									int32 align, void** data,
									bool deferredInit = false);
			status_t			AllocateData(Address& address, const void* data,
									int32 size, int32 align,
									bool deferredInit = false);
			status_t			AllocateString(Address& address,
									const char* data,
									bool deferredInit = false);
//			status_t			SetAddress(Address& address, void* data,
//									int32 size = 0);

private:
			struct DeferredInitInfo {
				Address*	target;
				uint8*		data;		// only if in port buffer
				area_id		area;		// only if in area, otherwise -1
				int32		offset;
				int32		size;
				bool		inPortBuffer;
			};

			status_t			fError;
			Port*				fPort;
			Request*			fRequest;
			int32				fRequestSize;
			int32				fPortReservedOffset;
			int32				fRequestOffset;
			area_id				fAllocatedAreas[MAX_REQUEST_ADDRESS_COUNT];
			int32				fAllocatedAreaCount;
			DeferredInitInfo	fDeferredInitInfos[MAX_REQUEST_ADDRESS_COUNT];
			int32				fDeferredInitInfoCount;
			bool				fRequestInPortBuffer;
};

// AllocateRequest
// Should be a member, but we don't have member templates on PPC.
// TODO: Actually we seem to have. Check!
template<typename SpecificRequest>
status_t
AllocateRequest(RequestAllocator& allocator, SpecificRequest** request)
{
	if (!request)
		RETURN_ERROR(B_BAD_VALUE);
	status_t error = allocator.AllocateRequest(sizeof(SpecificRequest));
	if (error == B_OK)
		*request = new(allocator.GetRequest()) SpecificRequest;
	return error;
}

}	// namespace UserlandFSUtil

using UserlandFSUtil::RequestAllocator;
using UserlandFSUtil::AllocateRequest;

#endif	// USERLAND_FS_REQUEST_ALLOCATOR_H
