/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VMBUS_PRIVATE_H_
#define _VMBUS_PRIVATE_H_

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ACPI.h>
#include "acpi.h"
#include <AutoDeleter.h>
#include <AutoDeleterOS.h>
#include <condition_variable.h>
#include <cpu.h>
#include <dpc.h>
#include <lock.h>
#include <smp.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <hyperv.h>

#include "Driver.h"
#include "hyperv_spec_private.h"
#include "VMBusRequest.h"

//#define TRACE_VMBUS
#ifdef TRACE_VMBUS
#	define TRACE(x...) dprintf("\33[35mvmbus:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[35mvmbus:\33[0m " x)
#define ERROR(x...)			dprintf("\33[35mvmbus:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)

status_t vmbus_detect_hyperv();

class VMBus;

// CPU index to VMBus linkage
typedef struct {
	int32_t	cpu;
	VMBus*	vmbus;
} VMBusCPU;


// Channel GPADL info
struct VMBusGPADL : DoublyLinkedListLinkImpl<VMBusGPADL> {
	uint32	gpadl_id;
	uint32	length;
	area_id areaid;
};
typedef DoublyLinkedList<VMBusGPADL> VMBusGPADLList;


// VMBus channel and linked GPADLs
struct VMBusChannel : DoublyLinkedListLinkImpl<VMBusChannel> {
	VMBusChannel(uint32 channelID, vmbus_guid_t typeID, vmbus_guid_t instanceID)
		:
		channel_id(channelID),
		type_id(typeID),
		instance_id(instanceID),
		dedicated_int(false),
		connection_id(VMBUS_CONNID_EVENTS),
		node(NULL),
		callback(NULL),
		callback_data(NULL)
	{
		mutex_init(&lock, "vmbus channel");
	}

	~VMBusChannel()
	{
		mutex_lock(&lock);

		// Free any stray GPADL buffers
		VMBusGPADLList::Iterator iterator = gpadls.GetIterator();
		while (iterator.HasNext()) {
			VMBusGPADL* gpadl = iterator.Next();
			gpadls.Remove(gpadl);
			delete_area(gpadl->areaid);
			delete gpadl;
		}

		mutex_destroy(&lock);
	}

	uint32				channel_id;
	vmbus_guid_t		type_id;
	vmbus_guid_t		instance_id;
	bool				dedicated_int;
	uint32				connection_id;
	uint32				mmio_size;

	mutex				lock;
	device_node*		node;
	VMBusGPADLList		gpadls;
	hyperv_bus_callback	callback;
	void*				callback_data;
};
typedef DoublyLinkedList<VMBusChannel> VMBusChannelList;


typedef void (VMBus::*VMBusEventFlagsHandler)(int32 cpu);


class VMBus {
public:
									VMBus(device_node* node);
									~VMBus();
			status_t				InitCheck() const { return fStatus; }

			uint32					GetVersion() const { return fVersion; }
			status_t				RequestChannels();

			status_t				OpenChannel(uint32 channelID, uint32 gpadlID, uint32 rxOffset,
										hyperv_bus_callback callback, void* callbackData);
			status_t				CloseChannel(uint32 channelID);
			status_t				AllocateGPADL(uint32 channelID, uint32 length, void** _buffer,
										uint32* _gpadlID);
			status_t				FreeGPADL(uint32 channelID, uint32 gpadlID);
			status_t				SignalChannel(uint32 channelID);

private:
			status_t				_EnableHypercalls();
			void					_DisableHypercalls();
			uint16					_HypercallPostMessage(phys_addr_t physAddr);
			uint16					_HypercallSignalEvent(uint32 connId);

			status_t				_EnableInterrupts();
			void					_DisableInterrupts();
	static	void					_EnableInterruptCPUHandler(void* data, int cpu);
			void					_EnableInterruptCPU(int32 cpu);
	static	void					_DisableInterruptCPUHandler(void* data, int cpu);
			void					_DisableInterruptCPU(int32 cpu);
	static	acpi_status				_InterruptACPICallback(ACPI_RESOURCE* res, void* context);
	static	int32					_InterruptHandler(void* data);
			int32					_Interrupt();
			void					_InterruptEventFlags(int32 cpu);
			void					_InterruptEventFlagsLegacy(int32 cpu);
			void					_InterruptEventFlagsNull(int32 cpu);

	static	void					_MessageDPCHandler(void* arg);
			void					_ProcessPendingMessage(int32_t cpu);
			void					_SendEndOfMessage(int32_t cpu);
	static	void					_SignalEom(void*, int);

			status_t				_SendRequest(VMBusRequest* request,
										ConditionVariableEntry* waitEntry = NULL,
										bool wait = true);
			status_t				_WaitForRequest(VMBusRequest* request,
										ConditionVariableEntry* waitEntry);
			void					_CancelRequest(VMBusRequest* request);

			status_t				_ConnectVersion(uint32 version);
			status_t				_Disconnect();
			status_t				_Connect();
	static	status_t				_ChannelQueueThreadHandler(void* arg);
			status_t				_ChannelQueueThread();
			VMBusChannel*			_GetChannel(uint32 channelID, MutexLocker& channelLocker);
			status_t				_RegisterChannel(VMBusChannel* channel);
			status_t				_UnregisterChannel(VMBusChannel* channel);
			area_id					_AllocateBuffer(const char* name, size_t length,
										uint32 protection, void** _buffer, phys_addr_t* _physAddr);
	inline	uint32					_GetGPADLHandle();

private:
			device_node*			fNode;
			status_t				fStatus;
			void*					fMessageDPCHandle;
			VMBusEventFlagsHandler	fEventFlagsHandler;

			void*					fHypercallPage;
			area_id					fHyperCallArea;
			phys_addr_t				fHyperCallPhys;

			int32					fIRQ;
			uint8					fInterruptVector;
			int32					fCPUCount;
			VMBusCPU*				fCPUData;

			volatile hv_message_page*	fCPUMessages;
			area_id						fCPUMessagesArea;
			phys_addr_t					fCPUMessagesPhys;
			hv_event_flags_page*		fCPUEventFlags;
			area_id						fCPUEventFlagsArea;
			phys_addr_t					fCPUEventFlagsPhys;

			bool					fConnected;
			uint32					fVersion;
			uint32					fConnectionId;
			vmbus_event_flags_page*	fEventFlags;
			void*					fMonitor1;
			void*					fMonitor2;
			area_id					fVMBusDataArea;
			phys_addr_t				fVMBusDataPhys;

			VMBusRequestList		fRequestList;
			mutex					fRequestLock;

			int32					fCurrentGPADLHandle;

			uint32					fMaxChannelsCount;
			uint32					fHighestChannelID;
			VMBusChannel**			fChannels;
			spinlock				fChannelsSpinlock;
			rw_lock					fChannelsLock;

			VMBusChannelList		fChannelOfferList;
			VMBusChannelList		fChannelRescindList;
			mutex					fChannelQueueLock;
			sem_id					fChannelQueueSem;
			thread_id				fChannelQueueThread;
};


#endif // VMBUS_PRIVATE_H
