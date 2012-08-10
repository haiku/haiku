/*
 *	SiS 190/191 NIC Driver.
 *	Copyright (c) 2009 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */

#include "DataRing.h"

#include <net/if_media.h>

#include "Driver.h"
#include "Settings.h"
#include "Device.h"


//
//	Tx stuff implementation
//

template<>
void
DataRing<TxDescriptor, TxDescriptorsCount>::_SetBaseAddress(phys_addr_t address)
{
	fDevice->WritePCI32(TxBase, (uint32)address);
}


template<>
status_t
DataRing<TxDescriptor, TxDescriptorsCount>::Write(const uint8* buffer,
													size_t* numBytes)
{
	*numBytes = min_c(*numBytes, MaxFrameSize);

	// wait for available tx descriptor
	status_t status = acquire_sem_etc(fSemaphore, 1, B_TIMEOUT, TransmitTimeout);
	if (status < B_NO_ERROR) {
		TRACE_ALWAYS("Cannot acquire sem:%#010x\n", status);
		return status;
	}

	cpu_status cpuStatus = disable_interrupts();
	acquire_spinlock(&fSpinlock);

	uint32 index = fHead % TxDescriptorsCount;
	volatile TxDescriptor& Descriptor = fDescriptors[index];

	// check if the buffer not owned by hardware
	uint32 descriptorStatus = Descriptor.fCommandStatus;
	if ((descriptorStatus & TDC_TXOWN) == 0) {

		// copy data into buffer
		status = user_memcpy((void*)fBuffers[index], buffer, *numBytes);

		// take care about tx descriptor
		Descriptor.fPacketSize = *numBytes;
		Descriptor.fEOD	|= *numBytes;
		Descriptor.fCommandStatus = TDC_PADEN | TDC_CRCEN
										| TDC_DEFEN | TDC_THOL3 | TDC_TXINT;
		if ((fDevice->LinkState().media & IFM_HALF_DUPLEX) != 0) {
			Descriptor.fCommandStatus |= TDC_BKFEN | TDC_CRSEN | TDC_COLSEN;
			if (fDevice->LinkState().speed == 1000000) {
				Descriptor.fCommandStatus |= TDC_BSTEN | TDC_EXTEN;
			}
		}

		Descriptor.fCommandStatus |= TDC_TXOWN;
		fHead++;
	}

	fDevice->WritePCI32(TxControl, fDevice->ReadPCI32(TxControl) | TxControlPoll);

	release_spinlock(&fSpinlock);
	restore_interrupts(cpuStatus);

	// if buffer was owned by hardware - notify about it
	if ((descriptorStatus & TDC_TXOWN) != 0) {
		release_sem_etc(fSemaphore, 1, B_DO_NOT_RESCHEDULE);
		TRACE_ALWAYS("Buffer is still owned by the card.\n");
		status = B_BUSY;
	}

	//	TRACE_ALWAYS("Write:%d bytes:%#010x!\n", *numBytes, status);

	return status;
}


template<>
int32
DataRing<TxDescriptor, TxDescriptorsCount>::InterruptHandler()
{
	uint32 releasedFrames = 0;

	acquire_spinlock(&fSpinlock);

	while (fTail != fHead) {

		uint32 index = fTail % TxDescriptorsCount;
		volatile TxDescriptor& Descriptor = fDescriptors[index];
		uint32 status = Descriptor.fCommandStatus;

#if STATISTICS
		fDevice->fStatistics.PutTxStatus(status, Descriptor.fEOD/*PacketSize*/);
#endif

		/*if (status & TDC_TXOWN) {
		//fDevice->WritePCI32(TxControl, fDevice->ReadPCI32(TxControl) | TxControlPoll);
		break; //still owned by hardware - poll again ...
		}*/

		Descriptor.fPacketSize = 0;
		Descriptor.fCommandStatus = 0;
		Descriptor.fEOD &= TxDescriptorEOD;

		releasedFrames++;

		fTail++;
	}

	release_spinlock(&fSpinlock);

	if (releasedFrames > 0) {
		release_sem_etc(fSemaphore, releasedFrames, B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER;
	}

	return B_HANDLED_INTERRUPT;
}


template<>
void
DataRing<TxDescriptor, TxDescriptorsCount>::CleanUp()
{
	cpu_status cpuStatus = disable_interrupts();
	acquire_spinlock(&fSpinlock);

	fDevice->WritePCI32(IntMask, 0 );

	uint32 txControl = fDevice->ReadPCI32(TxControl);
	txControl &= ~(TxControlPoll | TxControlEnable);
	fDevice->WritePCI32(TxControl, txControl);

	spin(50);

	uint32 droppedFrames = fHead - fTail;
	/*
		for (;fHead != fTail; fHead--, droppedFrames++) {
		uint32 index = fHead % TxDescriptorsCount;
		volatile TxDescriptor& Descriptor = fDescriptors[index];

		/ *	if (Descriptor.fCommandStatus & TDC_TXOWN) {
		continue; //still owned by hardware - ignore?
		}* /

		Descriptor.fPacketSize = 0;
		Descriptor.fCommandStatus = 0;
		Descriptor.fEOD &= TxDescriptorEOD;
		}
	*/
#if STATISTICS
	fDevice->fStatistics.fDropped += droppedFrames;
#endif

	fHead = fTail = 0;

	for (size_t i = 0; i < TxDescriptorsCount; i++) {
		fDescriptors[i].fPacketSize = 0;
		fDescriptors[i].fCommandStatus = 0;
		fDescriptors[i].fEOD &= TxDescriptorEOD;;
	}

	//	uint32 txBase = fDevice->ReadPCI32(TxBase);
	//uint32 index = fHead % TxDescriptorsCount;
	//fDevice->WritePCI32(TxStatus, txBase + 8 /*+ index * sizeof(TxDescriptor)*/);
	//fDevice->WritePCI32(TxBase, txBase);

	if (droppedFrames > 0) {
		release_sem_etc(fSemaphore, droppedFrames, B_DO_NOT_RESCHEDULE);
	}

	txControl |= TxControlEnable;
	fDevice->WritePCI32(TxControl, txControl);

	fDevice->WritePCI32(IntMask, knownInterruptsMask);

	release_spinlock(&fSpinlock);
	restore_interrupts(cpuStatus);
}


template<>
void
DataRing<TxDescriptor, TxDescriptorsCount>::Dump()
{
	int32 count = 0;
	get_sem_count(fSemaphore, &count);
	kprintf("Tx:[count:%" B_PRId32 "] head:%" B_PRIu32 " tail:%" B_PRIu32 " "
			"dirty:%" B_PRIu32 "\n", count, fHead, fTail, fHead - fTail);

	kprintf("\tPktSize\t\tCmdStat\t\tBufPtr\t\tEOD\n");

	for (size_t i = 0; i < TxDescriptorsCount; i++) {
		volatile TxDescriptor& D = fDescriptors[i];
		char marker = ((fTail % TxDescriptorsCount) == i) ? '=' : ' ';
		marker = ((fHead % TxDescriptorsCount) == i) ? '>' : marker;
		kprintf("%02lx %c\t%08" B_PRIx32 "\t%08" B_PRIx32 "\t%08" B_PRIx32
				"\t%08" B_PRIx32 "\n", i, marker, D.fPacketSize,
				D.fCommandStatus, D.fBufferPointer, D.fEOD);
	}
}


//
//	Rx stuff implementation
//

template<>
void
DataRing<RxDescriptor, RxDescriptorsCount>::_SetBaseAddress(phys_addr_t address)
{
	fDevice->WritePCI32(RxBase, (uint32)address);
}


template<>
int32
DataRing<RxDescriptor, RxDescriptorsCount>::InterruptHandler()
{
	uint32 receivedFrames = 0;

	acquire_spinlock(&fSpinlock);

	uint32 index	= fHead % RxDescriptorsCount;
	uint32 status	= fDescriptors[index].fStatusSize;
	uint32 info		= fDescriptors[index].fPacketInfo;

	while (((info & RDI_RXOWN) == 0) && (fHead - fTail) <= RxDescriptorsCount) {

#if STATISTICS
		fDevice->fStatistics.PutRxStatus(status);
#endif
		receivedFrames++;

		fHead++;

		index = fHead % RxDescriptorsCount;
		status = fDescriptors[index].fStatusSize;
		info = fDescriptors[index].fPacketInfo;
	}

	release_spinlock(&fSpinlock);

	if (receivedFrames > 0) {
		release_sem_etc(fSemaphore, receivedFrames, B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER;
	}

	return B_UNHANDLED_INTERRUPT; //XXX: ????
}


template<>
status_t
DataRing<RxDescriptor, RxDescriptorsCount>::Read(uint8* buffer, size_t* numBytes)
{
	status_t rstatus = B_ERROR;

	do {
		// wait for received rx descriptor
		uint32 flags = B_CAN_INTERRUPT | fDevice->fBlockFlag;
		status_t acquireStatus = acquire_sem_etc(fSemaphore, 1, flags, 0);
		if (acquireStatus != B_NO_ERROR) {
			TRACE_ALWAYS("Cannot acquire sem:%#010x\n", acquireStatus);
			return acquireStatus;
		}

		cpu_status cpuStatus = disable_interrupts();
		acquire_spinlock(&fSpinlock);

		uint32 index = fTail % RxDescriptorsCount;
		volatile RxDescriptor& Descriptor = fDescriptors[index];

		// check if the buffer owned by hardware - should never occure!
		uint32 status = Descriptor.fStatusSize;
		uint32 info = Descriptor.fPacketInfo;
		uint16 count = (status & 0x7f000000) >> 24;
		bool isFrameValid = false;
		//status_t rstatus = B_ERROR;

		if ((info & RDI_RXOWN) == 0) {
			isFrameValid = (status & rxErrorStatusBits) == 0 && (status & RDS_CRCOK) != 0;
			if (isFrameValid) {
				// frame is OK - copy it into buffer
				*numBytes = status & RDS_SIZE;
				rstatus = user_memcpy(buffer, (void*)fBuffers[index], *numBytes);
			}
		}

		// take care about rx descriptor
		Descriptor.fStatusSize = 0;
		Descriptor.fPacketInfo = RDI_RXOWN | RDI_RXINT;

		fTail++;

		release_spinlock(&fSpinlock);
		restore_interrupts(cpuStatus);

		if ((info & RDI_RXOWN) != 0) {
			TRACE_ALWAYS("Buffer is still owned by the card.\n");
		} else {
			if (!isFrameValid) {
				TRACE_ALWAYS("Invalid frame received, status:%#010x;info:%#010x!\n", status, info);
			} /*else {
				TRACE_ALWAYS("Read:%d bytes;st:%#010x;info:%#010x!\n", *numBytes, status, info);
				} */
			// we have free rx buffer - reenable potentially idle state machine
			fDevice->WritePCI32(RxControl, fDevice->ReadPCI32(RxControl) | RxControlPoll | RxControlEnable);
		}

		if (count > 1) {
			TRACE_ALWAYS("Warning:Descriptors count is %d!\n", count);
		}

	} while (rstatus != B_OK);

	return rstatus;
}


template<>
void
DataRing<RxDescriptor, RxDescriptorsCount>::Dump()
{
	int32 count = 0;
	get_sem_count(fSemaphore, &count);
	kprintf("Rx:[count:%" B_PRId32 "] head:%" B_PRIu32 " tail:%" B_PRIu32 " "
			"dirty:%" B_PRIu32 "\n", count, fHead, fTail, fHead - fTail);

	for (size_t i = 0; i < 2; i++) {
		kprintf("\tStatSize\tPktInfo\t\tBufPtr\t\tEOD      %c",
				i == 0 ? '|' : '\n');
	}

	for (size_t i = 0; i < RxDescriptorsCount / 2; i++) {
		const char* mask = "%02" B_PRIx32 " %c\t%08" B_PRIx32 "\t%08" B_PRIx32
			"\t%08" B_PRIx32 "\t%08" B_PRIx32 " %c";

		for (size_t ii = 0; ii < 2; ii++) {
			size_t index = ii == 0 ? i : (i + RxDescriptorsCount / 2);
			volatile RxDescriptor& D = fDescriptors[index];
			char marker = ((fTail % RxDescriptorsCount) == index) ? '=' : ' ';
			marker = ((fHead % RxDescriptorsCount) == index) ? '>' : marker;
			kprintf(mask, index, marker, D.fStatusSize, D.fPacketInfo,
					D.fBufferPointer, D.fEOD, ii == 0 ? '|' : '\n' );
		}
	}
}

