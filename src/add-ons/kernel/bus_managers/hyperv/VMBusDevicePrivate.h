/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VMBUS_DEVICE_PRIVATE_H_
#define _VMBUS_DEVICE_PRIVATE_H_

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lock.h>
#include <util/AutoLock.h>

#include <hyperv.h>

#include "Driver.h"
#include "hyperv_spec_private.h"

//#define TRACE_VMBUS_DEVICE
#ifdef TRACE_VMBUS_DEVICE
#	define TRACE(x...) dprintf("\33[36mvmbus_device:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
//#define TRACE_VMBUS_DEVICE_TX
#ifdef TRACE_VMBUS_DEVICE_TX
#	define TRACE_TX(x...) dprintf("\33[36mvmbus_device:\33[0m " x)
#else
#	define TRACE_TX(x...) ;
#endif
//#define TRACE_VMBUS_DEVICE_RX
#ifdef TRACE_VMBUS_DEVICE_RX
#	define TRACE_RX(x...) dprintf("\33[36mvmbus_device:\33[0m " x)
#else
#	define TRACE_RX(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[36mvmbus_device:\33[0m " x)
#define ERROR(x...)			dprintf("\33[36mvmbus_device:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

class VMBusDevice {
public:
									VMBusDevice(device_node* node);
									~VMBusDevice();
			status_t				InitCheck() const { return fStatus; }

			uint32					GetBusVersion();
			status_t				GetReferenceCounter(uint64* _count);
			status_t				Open(uint32 txLength, uint32 rxLength,
										hyperv_device_callback callback, void* callbackData);
			void					Close();

			status_t				WritePacket(uint16 type, const void* buffer, uint32 length,
										bool responseRequired, uint64 transactionID);
			status_t				WriteGPAPacket(uint32 rangeCount,
										const vmbus_gpa_range* rangesList, uint32 rangesLength,
										const void* buffer, uint32 length, bool responseRequired,
										uint64 transactionID);
			status_t				ReadPacket(void* buffer, uint32* bufferLength,
										uint32* _headerLength, uint32* _dataLength);

			status_t				AllocateGPADL(uint32 length, void** _buffer, uint32* _gpadl);
			status_t				FreeGPADL(uint32 gpadl);

private:
			bool					_IsReferenceCounterSupported();
	static	void					_CallbackHandler(void* arg);
	static	void					_DPCHandler(void* arg);

	inline	uint32					_AvailableTX();
	inline  uint32					_WriteTX(uint32 writeIndex, const void* buffer, uint32 length);
			status_t				_WriteTXData(const iovec txData[], size_t txDataCount);
	inline	uint32					_AvailableRX();
	inline	uint32					_SeekRX(uint32 readIndex, uint32 length);
	inline  uint32					_ReadRX(uint32 readIndex, void* buffer, uint32 length);

private:
			device_node*			fNode;
			status_t				fStatus;
			uint32					fChannelID;
			bool					fReferenceCounterSupported;
			mutex					fLock;
			void*					fDPCHandle;
			bool					fIsOpen;

			uint32					fRingGPADL;
			void*					fRingBuffer;
			uint32					fRingBufferLength;
			vmbus_ring_buffer*		fTXRing;
			uint32					fTXRingLength;
			vmbus_ring_buffer*		fRXRing;
			uint32					fRXRingLength;

			spinlock				fTXLock;
			spinlock				fRXLock;

			hyperv_device_callback	fCallback;
			void*					fCallbackData;

			hyperv_bus_interface*	fVMBus;
			hyperv_bus				fVMBusCookie;
};


#endif // _VMBUS_DEVICE_PRIVATE_H_
