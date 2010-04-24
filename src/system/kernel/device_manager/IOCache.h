/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IO_CACHE_H
#define IO_CACHE_H


#include <lock.h>
#include <vm/vm_page.h>

#include "dma_resources.h"
#include "IOCallback.h"
#include "IORequest.h"


struct VMCache;
struct vm_page;


class IOCache {
public:
								IOCache(DMAResource* resource,
									size_t cacheLineSize);
								~IOCache();

			status_t			Init(const char* name);

			void				SetCallback(IOCallback& callback);
			void				SetCallback(io_callback callback, void* data);

			void				SetDeviceCapacity(off_t deviceCapacity);

			status_t			ScheduleRequest(IORequest* request);

			void				OperationCompleted(IOOperation* operation,
									status_t status, size_t transferredBytes);

private:
			struct Operation;

private:
			status_t			_DoRequest(IORequest* request,
									size_t& _bytesTransferred);
			status_t			_TransferRequestLine(IORequest* request,
									off_t lineOffset, size_t lineSize,
									off_t requestOffset, size_t requestLength);
			status_t			_TransferRequestLineUncached(IORequest* request,
									off_t lineOffset, off_t requestOffset,
									size_t requestLength);
			status_t			_DoOperation(Operation& operation);

			status_t			_TransferPages(size_t firstPage,
									size_t pageCount, bool isWrite, bool isVIP);
			void				_DiscardPages(size_t firstPage,
									size_t pageCount);
			void				_CachePages(size_t firstPage, size_t pageCount);

			status_t			_CopyPages(IORequest* request,
									size_t pagesRelativeOffset,
									off_t requestOffset, size_t requestLength,
									bool toRequest);

			status_t			_MapPages(size_t firstPage, size_t endPage);
			void				_UnmapPages(size_t firstPage, size_t endPage);

private:
			mutex				fSerializationLock;
			off_t				fDeviceCapacity;
			size_t				fLineSize;
			uint32				fLineSizeShift;
			size_t				fPagesPerLine;
			DMAResource*		fDMAResource;
			io_callback			fIOCallback;
			void*				fIOCallbackData;
			area_id				fArea;
			void*				fAreaBase;
			vm_page_reservation	fMappingReservation;
			VMCache*			fCache;
			vm_page**			fPages;
			iovec*				fVecs;
};


#endif	// IO_CACHE_H
