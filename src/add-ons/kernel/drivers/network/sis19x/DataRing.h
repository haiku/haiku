/*
 *	SiS 190/191 NIC Driver.
 *	Copyright (c) 2009 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _SiS19X_DATARING_H_
#define _SiS19X_DATARING_H_


#include <KernelExport.h>

#include "Driver.h"
#include "Registers.h"
#include "Settings.h"


class Device;

template<typename __type, uint32 __count>
class DataRing {
public:
						DataRing(Device* device, bool isTx);
						~DataRing();

			status_t	Open();
			void		CleanUp();
			status_t	Close();

			status_t	Read(uint8* buffer, size_t* numBytes);
			status_t	Write(const uint8* buffer, size_t* numBytes);

			int32		InterruptHandler();

			void		Trace();
			void		Dump();

private:
			status_t	_InitArea();
			void		_SetBaseAddress(phys_addr_t address);
			
			Device*		fDevice;
			bool		fIsTx;
			status_t	fStatus;
			area_id		fArea;
			int32		fSpinlock;
			sem_id		fSemaphore;
			uint32		fHead;
			uint32		fTail;
						
	volatile __type*	fDescriptors;
	volatile uint8*		fBuffers[__count];
};



template<typename __type, uint32 __count>
DataRing<__type, __count>::DataRing(Device* device, bool isTx)
							:
							fDevice(device),
							fIsTx(isTx),
							fStatus(B_NO_INIT),
							fArea(-1),
							fSpinlock(0),
							fSemaphore(0),
							fHead(0),
							fTail(0),
							fDescriptors(NULL)
{
	memset(fBuffers, 0, sizeof(fBuffers));
}


template<typename __type, uint32 __count>
DataRing<__type, __count>::~DataRing()	
{
	delete_sem(fSemaphore);
	delete_area(fArea);
}


template<typename __type, uint32 __count>
status_t
DataRing<__type, __count>::_InitArea()
{
	// create area for xfer data descriptors and buffers...
	//
	// layout is following:
	// | descriptors array | buffers array |
	//
	uint32 buffSize = BufferSize + sizeof(__type);
	buffSize *=	__count;
	buffSize = (buffSize + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
	fArea = create_area(DRIVER_NAME "_data_ring", (void**)&fDescriptors,
			B_ANY_KERNEL_ADDRESS, buffSize,
			B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (fArea < 0) {
		TRACE_ALWAYS("Cannot create area with size %d bytes:%#010x\n",
				buffSize, fArea);
		return fStatus = fArea;
	}	

	// setup descriptors and buffers layout
	uint8* buffersData = (uint8*)fDescriptors;
	uint32 descriptorsSize = sizeof(__type) * __count;
	buffersData += descriptorsSize;

	physical_entry table = {0};

	for (size_t i = 0; i < __count; i++) {
		fBuffers[i] = buffersData + BufferSize * i;

		get_memory_map((void*)fBuffers[i], BufferSize, &table, 1);
		fDescriptors[i].Init(table.address, i == (__count - 1));
	}

	get_memory_map((void*)fDescriptors, descriptorsSize, &table, 1);

	_SetBaseAddress(table.address);

	return fStatus = B_OK;
}


template<typename __type, uint32 __count>
status_t
DataRing<__type, __count>::Open()
{
	if (fStatus != B_OK && _InitArea() != B_OK) {
		return fStatus;
	}

	if (fIsTx) {
		fSemaphore = create_sem(__count, "SiS19X Transmit");
	} else {
		fSemaphore = create_sem(0, "SiS19X Receive");
	}

	if (fSemaphore < 0) {
		TRACE_ALWAYS("Cannot create %s semaphore:%#010x\n",
				fIsTx ? "transmit" : "receive", fSemaphore);
		return fStatus = fSemaphore;
	}

	set_sem_owner(fSemaphore, B_SYSTEM_TEAM);

	return fStatus = B_OK;
}


template<typename __type, uint32 __count>
status_t
DataRing<__type, __count>::Close()
{
	delete_sem(fSemaphore);
	fSemaphore = 0;

	return B_OK;
}


template<typename __type, uint32 __count>
void
DataRing<__type, __count>::Trace()
{
	int32 count = 0;
	get_sem_count(fSemaphore, &count);
	TRACE_ALWAYS("%s:[count:%d] n:%lu l:%lu d:%lu\n", fIsTx ? "Tx" : "Rx",
			count, fHead, fTail, fHead - fTail);
}


#endif //_SiS19X_DATARING_H_

