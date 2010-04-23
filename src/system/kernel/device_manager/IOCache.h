/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IO_CACHE_H
#define IO_CACHE_H


#include <lock.h>
#include <util/DoublyLinkedList.h>

#include "dma_resources.h"
#include "IOCallback.h"
#include "IORequest.h"


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
			struct Line : DoublyLinkedListLinkImpl<Line> {
				Line*	hashNext;
				off_t	offset;
				off_t	size;
				uint8*	buffer;
				addr_t	physicalBuffer;
				area_id	area;
				bool	inUse;
			};

			struct Operation;
			struct LineHashDefinition;
			struct LineTable;
			typedef DoublyLinkedList<Line> LineList;

private:
			status_t			_DoRequest(IORequest* request,
									size_t& _bytesTransferred);
			status_t			_TransferRequestLine(IORequest* request,
									off_t lineOffset, off_t requestOffset,
									size_t requestLength);
			status_t			_TransferLine(Line* line, bool isWrite,
									bool isVIP);
			status_t			_DoOperation(Operation& operation);

			Line*				_LookupLine(off_t lineOffset);
			Line*				_PrepareLine(off_t lineOffset);
			void				_DiscardLine(Line* line);
			Line*				_AllocateLine();
			void				_FreeLine(Line* line);

	static	void				_LowMemoryHandlerEntry(void* data,
									uint32 resources, int32 level);
			void				_LowMemoryHandler(uint32 resources,
									int32 level);
	inline	bool				_MemoryIsLow() const;

private:
			mutex				fLock;
			mutex				fSerializationLock;
			off_t				fDeviceCapacity;
			size_t				fLineSize;
			uint32				fLineSizeShift;
			DMAResource*		fDMAResource;
			io_callback			fIOCallback;
			void*				fIOCallbackData;
			LineTable*			fLineTable;
			LineList			fUsedLines;		// LRU first
			Line*				fUnusedLine;
			size_t				fLineCount;
			bool				fLowMemoryHandlerRegistered;
};


#endif	// IO_CACHE_H
