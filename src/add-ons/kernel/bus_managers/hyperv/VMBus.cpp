/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "VMBusPrivate.h"


VMBus::VMBus(device_node* node)
	:
	fNode(node),
	fStatus(B_NO_INIT),
	fMessageDPCHandle(NULL),
	fEventFlagsHandler(&VMBus::_InterruptEventFlagsNull),
	fHypercallPage(NULL),
	fHyperCallArea(0),
	fHyperCallPhys(0),
	fIRQ(-1),
	fInterruptVector(0),
	fCPUCount(0),
	fCPUData(NULL),
	fCPUMessages(NULL),
	fCPUMessagesArea(0),
	fCPUMessagesPhys(0),
	fCPUEventFlags(NULL),
	fCPUEventFlagsArea(0),
	fCPUEventFlagsPhys(0),
	fConnected(false),
	fVersion(0),
	fConnectionId(0),
	fEventFlags(NULL),
	fMonitor1(NULL),
	fMonitor2(NULL),
	fVMBusDataArea(0),
	fVMBusDataPhys(0),
	fCurrentGPADLHandle(VMBUS_GPADL_NULL),
	fMaxChannelsCount(0),
	fHighestChannelID(0),
	fChannels(NULL),
	fChannelQueueSem(-1),
	fChannelQueueThread(-1)
{
	CALLED();

	// Allocate an executable page for hypercall usage
	fHyperCallArea = _AllocateBuffer("hypercall", HV_PAGE_SIZE,
		B_KERNEL_READ_AREA | B_KERNEL_EXECUTE_AREA, &fHypercallPage, &fHyperCallPhys);
	if (fHyperCallArea < B_OK) {
		fStatus = fHyperCallArea;
		return;
	}

	// Hyper-V is able to send targeted interrupts to a specific CPU, each requires its
	// own set of event data
	fCPUCount = smp_get_num_cpus();
	fCPUData = new(std::nothrow) VMBusCPU[fCPUCount];
	if (fCPUData == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	for (int32 i = 0; i < fCPUCount; i++) {
		fCPUData[i].cpu = i;
		fCPUData[i].vmbus = this;
	}

	fCPUMessagesArea = _AllocateBuffer("hv msg", sizeof(*fCPUMessages) * fCPUCount,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fCPUMessages, &fCPUMessagesPhys);
	if (fCPUMessagesArea < B_OK) {
		fStatus = fCPUMessagesArea;
		return;
	}

	fCPUEventFlagsArea = _AllocateBuffer("hv eventflags", sizeof(*fCPUEventFlags) * fCPUCount,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fCPUEventFlags, &fCPUEventFlagsPhys);
	if (fCPUEventFlagsArea < B_OK) {
		fStatus = fCPUEventFlagsArea;
		return;
	}

	// VMBus event flags / monitoring pages
	fVMBusDataArea = _AllocateBuffer("vmbus", sizeof(*fEventFlags) + (HV_PAGE_SIZE * 2),
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fEventFlags, &fVMBusDataPhys);
	if (fVMBusDataArea < B_OK) {
		fStatus = fVMBusDataArea;
		return;
	}

	fMonitor1 = fEventFlags + 1;
	fMonitor2 = static_cast<uint8*>(fMonitor1) + HV_PAGE_SIZE;

	mutex_init(&fRequestLock, "vmbus request lock");
	mutex_init(&fChannelQueueLock, "vmbus channelqueue lock");
	rw_lock_init(&fChannelsLock, "vmbus channel lock");
	B_INITIALIZE_SPINLOCK(&fChannelsSpinlock);

	fStatus = gDPC->new_dpc_queue(&fMessageDPCHandle, "hyperv vmbus request dpc",
		B_NORMAL_PRIORITY);
	if (fStatus != B_OK)
		return;

	fChannelQueueSem = create_sem(0, "vmbus channel sem");
	if (fChannelQueueSem < B_OK) {
		fStatus = fChannelQueueSem;
		return;
	}

	fStatus = _EnableHypercalls();
	if (fStatus != B_OK) {
		ERROR("Hypercall initialization failed (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = _EnableInterrupts();
	if (fStatus != B_OK) {
		ERROR("Interrupt initialization failed (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = _Connect();
	if (fStatus != B_OK) {
		ERROR("VMBus connection failed (%s)\n", strerror(fStatus));
		return;
	}
}


VMBus::~VMBus()
{
	CALLED();

	sem_id channelQueueSem = fChannelQueueSem;
	fChannelQueueSem = -1;
	delete_sem(channelQueueSem);
	wait_for_thread(fChannelQueueThread, NULL);

	_Disconnect();
	_DisableInterrupts();
	_DisableHypercalls();

	gDPC->delete_dpc_queue(fMessageDPCHandle);
	mutex_destroy(&fChannelQueueLock);
	rw_lock_destroy(&fChannelsLock);
	mutex_destroy(&fRequestLock);

	delete_area(fVMBusDataArea);
	delete_area(fCPUMessagesArea);
	delete_area(fCPUEventFlagsArea);
	delete_area(fHyperCallArea);
	delete[] fCPUData;

	VMBusRequestList::Iterator iterator = fRequestList.GetIterator();
	while (iterator.HasNext()) {
		VMBusRequest* request = iterator.Next();
		delete request;
	}
}


status_t
VMBus::RequestChannels()
{
	VMBusRequest* request = new(std::nothrow) VMBusRequest(VMBUS_MSGTYPE_REQUEST_CHANNELS,
		VMBUS_CHANNEL_ID);
	if (request == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<VMBusRequest> requestDeleter(request);

	status_t status = request->InitCheck();
	if (status != B_OK)
		return status;

	ConditionVariableEntry entry;
	request->SetResponseType(VMBUS_MSGTYPE_REQUEST_CHANNELS_DONE);
	status = _SendRequest(request, &entry);
	if (status != B_OK) {
		ERROR("Failed to request channels (%s)\n", strerror(status));
		return status;
	}

	while (true) {
		MutexLocker locker(fChannelQueueLock);
		VMBusChannel* channel = fChannelOfferList.RemoveHead();
		locker.Unlock();

		if (channel == NULL)
			break;

		status_t status = _RegisterChannel(channel);
		if (status != B_OK) {
			ERROR("Failed to register channel %u (%s)\n", channel->channel_id, strerror(status));
			return status;
		}
	}

	// Startup channel queue thread to process any channels added or removed later
	fChannelQueueThread = spawn_kernel_thread(_ChannelQueueThreadHandler, "vmbus channel queue",
		B_NORMAL_PRIORITY, this);
	if (fChannelQueueThread < B_OK)
		return fChannelQueueThread;
	return resume_thread(fChannelQueueThread);
}


status_t
VMBus::OpenChannel(uint32 channelID, uint32 gpadlID, uint32 rxOffset, hyperv_bus_callback callback,
	void* callbackData)
{
	MutexLocker locker;
	VMBusChannel* channel = _GetChannel(channelID, locker);
	if (channel == NULL)
		return B_DEVICE_NOT_FOUND;

	VMBusRequest* request = new(std::nothrow) VMBusRequest(VMBUS_MSGTYPE_OPEN_CHANNEL, channelID);
	if (request == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<VMBusRequest> requestDeleter(request);

	status_t status = request->InitCheck();
	if (status != B_OK)
		return status;

	vmbus_msg_open_channel* message = &request->GetMessage()->open_channel;
	message->channel_id = channelID;
	message->open_id = channelID;
	message->gpadl_id = gpadlID;
	message->target_cpu = 0;
	message->rx_page_offset = rxOffset >> HV_PAGE_SHIFT;
	memset(message->user_data, 0, sizeof(message->user_data));

	// Register callback prior to sending request, some devices will immediately
	// begin to receive events after the channel is opened
	InterruptsSpinLocker spinLocker(fChannelsSpinlock);
	channel->callback = callback;
	channel->callback_data = callbackData;
	spinLocker.Unlock();

	TRACE("Opening channel %u with ring GPADL %u rx offset 0x%X\n", channelID, gpadlID, rxOffset);

	ConditionVariableEntry entry;
	request->SetResponseType(VMBUS_MSGTYPE_OPEN_CHANNEL_RESPONSE);
	status = _SendRequest(request, &entry);
	if (status != B_OK) {
		spinLocker.Lock();
		channel->callback = NULL;
		channel->callback_data = NULL;
		spinLocker.Unlock();

		return status;
	}

	status = (request->GetMessage()->open_channel_resp.result == 0
		&& request->GetMessage()->open_channel_resp.open_id == channelID)
		? B_OK : B_IO_ERROR;

	TRACE("Open channel %u status (%s)\n", channelID, strerror(status));
	return status;
}


status_t
VMBus::CloseChannel(uint32 channelID)
{
	MutexLocker locker;
	VMBusChannel* channel = _GetChannel(channelID, locker);
	if (channel == NULL)
		return B_DEVICE_NOT_FOUND;

	VMBusRequest* request = new(std::nothrow) VMBusRequest(VMBUS_MSGTYPE_CLOSE_CHANNEL, channelID);
	if (request == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<VMBusRequest> requestDeleter(request);

	status_t status = request->InitCheck();
	if (status != B_OK)
		return status;

	vmbus_msg_close_channel* message = &request->GetMessage()->close_channel;
	message->channel_id = channelID;

	TRACE("Closing channel %u\n", channelID);
	status = _SendRequest(request);
	if (status == B_OK) {
		InterruptsSpinLocker spinLocker(fChannelsSpinlock);
		channel->callback = NULL;
		channel->callback_data = NULL;
	}

	return status;
}


status_t
VMBus::AllocateGPADL(uint32 channelID, uint32 length, void** _buffer, uint32* _gpadlID)
{
	// Length must be page-aligned and within bounds
	if (length == 0 || length != HV_PAGE_ALIGN(length))
		return B_BAD_VALUE;

	uint32 pageTotalCount = HV_BYTES_TO_PAGES(length);
	if ((pageTotalCount + 1) > VMBUS_GPADL_MAX_PAGES)
		return B_BAD_VALUE;

	MutexLocker locker;
	VMBusChannel* channel = _GetChannel(channelID, locker);
	if (channel == NULL)
		return B_DEVICE_NOT_FOUND;

	// Allocate contiguous buffer to back the GPADL
	void* buffer;
	phys_addr_t physAddr;
	area_id areaid = _AllocateBuffer("hv gpadl", length, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		&buffer, &physAddr);
	if (areaid < B_OK)
		return B_NO_MEMORY;
	AreaDeleter areaDeleter(areaid);

	uint32 gpadlID = _GetGPADLHandle();

	// All GPADL setups require the starting message
	// Determine if this GPADL requires multiple messages to send all page numbers to Hyper-V
	bool multipleMessages = pageTotalCount > VMBUS_MSG_CREATE_GPADL_MAX_PAGES;
	TRACE("Creating GPADL %u for channel %u with %u pages (multiple: %s)\n", gpadlID, channelID,
		pageTotalCount, multipleMessages ? "yes" : "no");

	uint32 pageMessageCount = multipleMessages ? VMBUS_MSG_CREATE_GPADL_MAX_PAGES : pageTotalCount;
	uint32 messageLength = sizeof(vmbus_msg_create_gpadl) + (sizeof(uint64) * pageMessageCount);

	VMBusRequest* createRequest = new(std::nothrow) VMBusRequest(VMBUS_MSGTYPE_CREATE_GPADL,
		channelID, messageLength);
	if (createRequest == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<VMBusRequest> createRequestDeleter(createRequest);

	status_t status = createRequest->InitCheck();
	if (status != B_OK)
		return status;

	vmbus_msg_create_gpadl* createMessage = &createRequest->GetMessage()->create_gpadl;
	createMessage->channel_id = channelID;
	createMessage->gpadl_id = gpadlID;
	createMessage->total_range_length
		= sizeof(createMessage->ranges) + (pageTotalCount * sizeof(uint64));
	createMessage->range_count = 1;
	createMessage->ranges[0].offset = 0;
	createMessage->ranges[0].length = length;

	uint64 currentPageNum = (uint64)(physAddr >> HV_PAGE_SHIFT);
	for (uint32 i = 0; i < pageMessageCount; i++)
		createMessage->ranges[0].page_nums[i] = currentPageNum++;

	ConditionVariableEntry entry;
	createRequest->SetResponseType(VMBUS_MSGTYPE_CREATE_GPADL_RESPONSE);
	createRequest->SetResponseData(gpadlID);
	status = _SendRequest(createRequest, &entry, false);
	if (status != B_OK)
		return status;

	// Send remainder of buffer to Hyper-V within additional messages as needed
	if (multipleMessages) {
		VMBusRequest* additionalRequest = new(std::nothrow)
			VMBusRequest(VMBUS_MSGTYPE_CREATE_GPADL_ADDITIONAL, channelID,
				sizeof(vmbus_msg_create_gpadl_additional));
		if (additionalRequest == NULL) {
			_CancelRequest(createRequest);
			return B_NO_MEMORY;
		}
		ObjectDeleter<VMBusRequest> additionalRequestDeleter(additionalRequest);

		status = additionalRequest->InitCheck();
		if (status != B_OK) {
			_CancelRequest(createRequest);
			return status;
		}

		uint32 pagesRemaining = pageTotalCount - pageMessageCount;
		while (pagesRemaining > 0) {
			if (pagesRemaining > VMBUS_MSG_CREATE_GPADL_ADDITIONAL_MAX_PAGES)
				pageMessageCount = VMBUS_MSG_CREATE_GPADL_ADDITIONAL_MAX_PAGES;
			else
				pageMessageCount = pagesRemaining;

			messageLength = sizeof(vmbus_msg_create_gpadl_additional)
				+ (sizeof(uint64) * pageMessageCount);
			vmbus_msg_create_gpadl_additional* additionalMessage
				= &additionalRequest->GetMessage()->create_gpadl_additional;
			additionalMessage->gpadl_id = gpadlID;
			for (uint32 i = 0; i < pageMessageCount; i++)
				additionalMessage->page_nums[i] = currentPageNum++;

			additionalRequest->SetLength(messageLength);
			status = _SendRequest(additionalRequest);
			if (status != B_OK) {
				_CancelRequest(createRequest);
				return status;
			}

			pagesRemaining -= pageMessageCount;
		}
	}

	status = _WaitForRequest(createRequest, &entry);
	if (status != B_OK)
		return status;

	status = createRequest->GetMessage()->create_gpadl_resp.result == 0 ? B_OK : B_IO_ERROR;
	if (status != B_OK)
		return status;

	VMBusGPADL* gpadl = new(std::nothrow) VMBusGPADL;
	if (gpadl == NULL)
		return B_NO_MEMORY;
	areaDeleter.Detach();

	gpadl->gpadl_id = gpadlID;
	gpadl->length = length;
	gpadl->areaid = areaid;
	channel->gpadls.Add(gpadl);

	*_buffer = buffer;
	*_gpadlID = gpadlID;

	TRACE("Created GPADL %u for channel %u\n", gpadlID, channelID);
	return B_OK;
}


status_t
VMBus::FreeGPADL(uint32 channelID, uint32 gpadlID)
{
	MutexLocker locker;
	VMBusChannel* channel = _GetChannel(channelID, locker);
	if (channel == NULL)
		return B_DEVICE_NOT_FOUND;

	bool foundGPADL = false;
	VMBusGPADL* gpadl;
	VMBusGPADLList::Iterator iterator = channel->gpadls.GetIterator();
	while (iterator.HasNext()) {
		gpadl = iterator.Next();
		if (gpadl->gpadl_id == gpadlID) {
			foundGPADL = true;
			break;
		}
	}

	if (!foundGPADL)
		return B_BAD_VALUE;

	VMBusRequest* request = new(std::nothrow) VMBusRequest(VMBUS_MSGTYPE_FREE_GPADL, channelID);
	if (request == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<VMBusRequest> requestDeleter(request);

	status_t status = request->InitCheck();
	if (status != B_OK)
		return status;

	vmbus_msg_free_gpadl* message = &request->GetMessage()->free_gpadl;
	message->channel_id = channelID;
	message->gpadl_id = gpadlID;

	ConditionVariableEntry entry;
	request->SetResponseType(VMBUS_MSGTYPE_FREE_GPADL_RESPONSE);
	request->SetResponseData(gpadlID);
	status = _SendRequest(request, &entry);
	if (status != B_OK)
		return status;

	channel->gpadls.Remove(gpadl);
	delete_area(gpadl->areaid);
	delete gpadl;

	return B_OK;
}


status_t
VMBus::SignalChannel(uint32 channelID)
{
	if (channelID == VMBUS_CHANNEL_ID || channelID >= fMaxChannelsCount)
		return B_BAD_VALUE;

	InterruptsSpinLocker spinLocker(fChannelsSpinlock);
	if (fChannels[channelID] == NULL)
		return B_BAD_VALUE;
	bool dedicatedInterrupt = fChannels[channelID]->dedicated_int;
	uint32 connectionID = fChannels[channelID]->connection_id;
	spinLocker.Unlock();

	if (!dedicatedInterrupt)
		atomic_or((int32*)&fEventFlags->tx_event_flags.flags32[channelID / 32],
			1 << (channelID & 0x1F));

	uint16 hypercallStatus = _HypercallSignalEvent(connectionID);
	if (hypercallStatus != 0)
		TRACE("Signal hypercall failed 0x%X\n", hypercallStatus);
	return hypercallStatus == 0 ? B_OK : B_IO_ERROR;
}


status_t
VMBus::_EnableInterrupts()
{
	char acpiVMBusName[255];
	status_t status = gACPI->get_device(VMBUS_ACPI_HID_NAME, 0,
		acpiVMBusName, sizeof(acpiVMBusName));
	if (status != B_OK) {
		ERROR("Could not locate VMBus in ACPI\n");
		return status;
	}
	TRACE("VMBus ACPI: %s\n", acpiVMBusName);

	acpi_handle acpiVMBusHandle;
	status = gACPI->get_handle(NULL, acpiVMBusName, &acpiVMBusHandle);
	if (status != B_OK)
		return status;

	fIRQ = -1;
	status = gACPI->walk_resources(acpiVMBusHandle, (ACPI_STRING) "_CRS", _InterruptACPICallback,
		&fIRQ);
	if (status != B_OK)
		return status;
	if (fIRQ < 0)
		return B_IO_ERROR;

	fInterruptVector = fIRQ + ARCH_INTERRUPT_BASE;
	TRACE("VMBus irq interrupt line: %" B_PRId32 ", vector: %u\n", fIRQ, fInterruptVector);
	status = install_io_interrupt_handler(fIRQ, _InterruptHandler, this, 0);
	if (status != B_OK) {
		ERROR("Can't install interrupt handler for irq %" B_PRId32 " (%s)\n", fIRQ,
			strerror(status));
		return status;
	}

	// Each CPU has its own set of MSRs, enable on all
	call_all_cpus_sync(_EnableInterruptCPUHandler, this);
	return B_OK;
}


/*static*/ void
VMBus::_EnableInterruptCPUHandler(void* data, int cpu)
{
	VMBus* vmbus = reinterpret_cast<VMBus*>(data);
	vmbus->_EnableInterruptCPU(cpu);
}


void
VMBus::_DisableInterrupts()
{
	if (fIRQ < 0)
		return;

	// Each CPU has its own set of MSRs, disable on all
	call_all_cpus_sync(_DisableInterruptCPUHandler, this);

	remove_io_interrupt_handler(fIRQ, _InterruptHandler, this);
}


/*static*/ void
VMBus::_DisableInterruptCPUHandler(void* data, int cpu)
{
	VMBus* vmbus = reinterpret_cast<VMBus*>(data);
	vmbus->_DisableInterruptCPU(cpu);
}

/*static*/ acpi_status
VMBus::_InterruptACPICallback(ACPI_RESOURCE* res, void* context)
{
	int32* irq = static_cast<int32*>(context);

	// Grab the first IRQ only. Gen1 usually has two IRQs, Gen2 just one.
	// Only one IRQ is required for the VMBus device.
	if (res->Type == ACPI_RESOURCE_TYPE_IRQ && *irq < 0)
		*irq = res->Data.Irq.Interrupt;
	return B_OK;
}


/*static*/ int32
VMBus::_InterruptHandler(void* data)
{
	VMBus* vmbus = reinterpret_cast<VMBus*>(data);
	return vmbus->_Interrupt();
}


int32
VMBus::_Interrupt()
{
	int32 cpu = smp_get_current_cpu();

	// Check event flags first
	(this->*fEventFlagsHandler)(cpu);

	// Handoff new VMBus management message to DPC
	if (fCPUMessages[cpu].interrupts[VMBUS_SINT_MESSAGE].message_type != HYPERV_MSGTYPE_NONE)
		gDPC->queue_dpc(fMessageDPCHandle, _MessageDPCHandler, &fCPUData[cpu]);

	return B_HANDLED_INTERRUPT;
}


void
VMBus::_InterruptEventFlags(int32 cpu)
{
	SpinLocker spinLocker(&fChannelsSpinlock);

	// Check the SynIC event flags directly
	uint32* eventFlags = fCPUEventFlags[cpu].interrupts[VMBUS_SINT_MESSAGE].flags32;
	uint32 flags = (atomic_get_and_set((int32*)eventFlags, 0)) >> 1;
	for (uint32 i = 1; i <= fHighestChannelID; i++) {
		if ((i % 32) == 0)
			flags = atomic_get_and_set((int32*)eventFlags++, 0);

		if (flags & 0x1 && fChannels[i] != NULL && fChannels[i]->callback != NULL)
			fChannels[i]->callback(fChannels[i]->callback_data);
		flags >>= 1;
	}
}


void
VMBus::_InterruptEventFlagsLegacy(int32 cpu)
{
	// Check the SynIC event flags first, then the VMBus RX event flags
	if (atomic_get_and_set((int32*)&fCPUEventFlags[cpu].interrupts[VMBUS_SINT_MESSAGE].flags32, 0) == 0)
		return;

	SpinLocker spinLocker(&fChannelsSpinlock);

	uint32* rxFlags = fEventFlags->rx_event_flags.flags32;
	uint32 flags = atomic_get_and_set((int32*)rxFlags, 0) >> 1;
	for (uint32 i = 1; i <= fHighestChannelID; i++) {
		if ((i % 32) == 0)
			flags = atomic_get_and_set((int32*)rxFlags++, 0);

		if (flags & 0x1 && fChannels[i] != NULL && fChannels[i]->callback != NULL)
			fChannels[i]->callback(fChannels[i]->callback_data);
		flags >>= 1;
	}
}


void
VMBus::_InterruptEventFlagsNull(int32 cpu)
{
}


/*static*/ void
VMBus::_MessageDPCHandler(void* arg)
{
	VMBusCPU* cpuData = reinterpret_cast<VMBusCPU*>(arg);
	cpuData->vmbus->_ProcessPendingMessage(cpuData->cpu);
}


void
VMBus::_ProcessPendingMessage(int32_t cpu)
{
	volatile hv_message* hvMessage = &fCPUMessages[cpu].interrupts[VMBUS_SINT_MESSAGE];
	if (hvMessage->message_type != HYPERV_MSGTYPE_CHANNEL
			|| hvMessage->payload_size < sizeof(vmbus_msg_header)) {
		// Ignore any spurious pending messages
		if (hvMessage->message_type != HYPERV_MSGTYPE_NONE)
			ERROR("Invalid VMBus Hyper-V message type %u length 0x%X\n", hvMessage->message_type,
				hvMessage->payload_size);
		_SendEndOfMessage(cpu);
		return;
	}

	vmbus_msg* message = (vmbus_msg*)hvMessage->data;
	TRACE("New VMBus message type %u length 0x%X\n", message->header.type,
		hvMessage->payload_size);
	if (message->header.type >= VMBUS_MSGTYPE_MAX
			|| hvMessage->payload_size < vmbus_msg_lengths[message->header.type]) {
		ERROR("Invalid VMBus message type %u or length 0x%X\n", message->header.type,
			hvMessage->payload_size);
		_SendEndOfMessage(cpu);
		return;
	}

	if (message->header.type == VMBUS_MSGTYPE_CHANNEL_OFFER) {
		vmbus_msg_channel_offer* offerMessage = &message->channel_offer;
		uint32 channelOfferID = offerMessage->channel_id;

		if (channelOfferID != VMBUS_CHANNEL_ID && channelOfferID < fMaxChannelsCount) {
			VMBusChannel* channel = new(std::nothrow) VMBusChannel(channelOfferID,
				offerMessage->type_id, offerMessage->instance_id);
			if (channel != NULL) {
				if (fVersion > VMBUS_VERSION_WS2008) {
					channel->dedicated_int = offerMessage->dedicated_int != 0;
					channel->connection_id = offerMessage->connection_id;
				}
				channel->mmio_size = static_cast<uint32>(offerMessage->mmio_size_mb) * 1024 * 1024;

				// Add new channel to offer queue and signal the channel handler thread
				MutexLocker locker(fChannelQueueLock);
				fChannelOfferList.Add(channel);
				locker.Unlock();

				release_sem_etc(fChannelQueueSem, 1, B_DO_NOT_RESCHEDULE);
			}
		} else {
			TRACE("Invalid VMBus channel ID %u offer received!\n", channelOfferID);
		}
	} else if (message->header.type == VMBUS_MSGTYPE_RESCIND_CHANNEL_OFFER) {
		vmbus_msg_rescind_channel_offer* rescindMessage = &message->rescind_channel_offer;
		uint32 channelRescindID = rescindMessage->channel_id;

		if (channelRescindID != VMBUS_CHANNEL_ID && channelRescindID < fMaxChannelsCount) {
			// Remove the channel from the list of active channels
			InterruptsSpinLocker spinLocker(fChannelsSpinlock);
			VMBusChannel* channel = fChannels[channelRescindID];
			fChannels[channelRescindID] = NULL;
			spinLocker.Unlock();

			// Terminate any pending requests for the channel
			MutexLocker requestLocker(fRequestLock);

			VMBusRequestList::Iterator iterator = fRequestList.GetIterator();
			while (iterator.HasNext()) {
				VMBusRequest* request = iterator.Next();
				if (request->GetChannelID() == channelRescindID) {
					fRequestList.Remove(request);
					requestLocker.Unlock();

					request->Notify(B_CANCELED, NULL, 0);
					requestLocker.Lock();
				}
			}

			requestLocker.Unlock();

			// Add removed channel to rescind queue and signal the channel handler thread
			if (channel != NULL) {
				MutexLocker locker(fChannelQueueLock);
				fChannelRescindList.Add(channel);
				locker.Unlock();

				release_sem_etc(fChannelQueueSem, 1, B_DO_NOT_RESCHEDULE);
			}
		} else {
			TRACE("Invalid VMBus channel ID %u rescind received!\n", channelRescindID);
		}
	} else {
		bool matchChannelID = true;
		uint32 channelID = VMBUS_CHANNEL_ID;
		uint32 respData = 0;
		switch (message->header.type) {
			case VMBUS_MSGTYPE_OPEN_CHANNEL_RESPONSE:
				channelID = message->open_channel_resp.channel_id;
				break;

			case VMBUS_MSGTYPE_CREATE_GPADL_RESPONSE:
				channelID = message->create_gpadl_resp.channel_id;
				respData = message->create_gpadl_resp.gpadl_id;
				break;

			case VMBUS_MSGTYPE_FREE_GPADL_RESPONSE:
				matchChannelID = false;
				respData = message->free_gpadl_resp.gpadl_id;
				break;

			default:
				break;
		}

		// Complete any waiting requests
		MutexLocker requestLocker(fRequestLock);

		VMBusRequestList::Iterator iterator = fRequestList.GetIterator();
		while (iterator.HasNext()) {
			VMBusRequest* request = iterator.Next();
			if (matchChannelID && request->GetChannelID() != channelID)
				continue;

			if (request->GetResponseType() == message->header.type
					&& request->GetResponseData() == respData) {
				fRequestList.Remove(request);
				requestLocker.Unlock();

				request->Notify(B_OK, message, hvMessage->payload_size);
				break;
			}
		}
	}

	_SendEndOfMessage(cpu);
}


void
VMBus::_SendEndOfMessage(int32_t cpu)
{
	// Clear current message so Hyper-V can send another
	volatile hv_message* message = &fCPUMessages[cpu].interrupts[VMBUS_SINT_MESSAGE];
	message->message_type = HYPERV_MSGTYPE_NONE;
	memory_full_barrier();

	// Trigger end-of-message on target CPU if another message is pending
	if (message->message_flags & HV_MESSAGE_FLAGS_PENDING)
		call_single_cpu(cpu, _SignalEom, NULL);
}


status_t
VMBus::_SendRequest(VMBusRequest* request, ConditionVariableEntry* waitEntry, bool wait)
{
	uint16 hypercallStatus;
	bool complete = false;
	status_t status;

	// Add request to active list if a response is required
	if (request->GetResponseType() != VMBUS_MSGTYPE_INVALID && waitEntry != NULL) {
		request->Add(waitEntry);
		MutexLocker locker(fRequestLock);
		fRequestList.Add(request);
		locker.Unlock();
	}

	// Multiple hypercalls together may fail due to lack of host resources, just try again
	for (int i = 0; i < HYPERCALL_MAX_RETRY_COUNT; i++) {
		hypercallStatus = _HypercallPostMessage(request->GetHcPostPhys());
		switch (hypercallStatus) {
			case HYPERCALL_STATUS_SUCCESS:
				status = B_OK;
				complete = true;
				break;

			case HYPERCALL_STATUS_INSUFFICIENT_MEMORY:
			case HYPERCALL_STATUS_INSUFFICIENT_BUFFERS:
				status = B_NO_MEMORY;
				break;

			default:
				status = B_IO_ERROR;
				complete = true;
				break;
		}

		if (complete)
			break;

		snooze(20ULL);
	}

	if (status == B_OK) {
		if (request->GetResponseType() != VMBUS_MSGTYPE_INVALID && waitEntry != NULL && wait)
			status = _WaitForRequest(request, waitEntry);
	} else {
		if (request->GetResponseType() != VMBUS_MSGTYPE_INVALID)
			_CancelRequest(request);

		TRACE("Post hypercall failed 0x%X\n", hypercallStatus);
	}

	return status;
}


status_t
VMBus::_WaitForRequest(VMBusRequest* request, ConditionVariableEntry* waitEntry)
{
	status_t status = request->Wait(waitEntry);
	if (status != B_OK) {
		ERROR("Request wait for type %u failed (%s)\n", request->GetResponseType(),
			strerror(status));
		_CancelRequest(request);
	}

	return status;
}


void
VMBus::_CancelRequest(VMBusRequest* request)
{
	if (request->GetResponseType() == VMBUS_MSGTYPE_INVALID)
		return;

	MutexLocker locker(fRequestLock);
	if (fRequestList.Contains(request))
		fRequestList.Remove(request);
}


status_t
VMBus::_ConnectVersion(uint32 version)
{
	VMBusRequest* request = new(std::nothrow) VMBusRequest(VMBUS_MSGTYPE_CONNECT,
		VMBUS_CHANNEL_ID);
	if (request == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<VMBusRequest> requestDeleter(request);

	status_t status = request->InitCheck();
	if (status != B_OK)
		return status;

	vmbus_msg_connect* message = &request->GetMessage()->connect;
	message->version = version;
	message->target_cpu = 0;

	message->event_flags_physaddr = fVMBusDataPhys;
	message->monitor1_physaddr = message->event_flags_physaddr + sizeof(*fEventFlags);
	message->monitor2_physaddr = message->monitor1_physaddr + HV_PAGE_SIZE;

	TRACE("Connecting to VMBus version %u.%u\n", GET_VMBUS_VERSION_MAJOR(version),
		GET_VMBUS_VERSION_MINOR(version));

	ConditionVariableEntry entry;
	request->SetResponseType(VMBUS_MSGTYPE_CONNECT_RESPONSE);
	status = _SendRequest(request, &entry);
	if (status != B_OK)
		return status;

	status = request->GetMessage()->connect_resp.supported != 0 ? B_OK : B_NOT_SUPPORTED;
	if (status == B_OK)
		fConnectionId = request->GetMessage()->connect_resp.connection_id;

	TRACE("Connection status (%s)\n", strerror(status));
	return status;
}


status_t
VMBus::_Connect()
{
	uint32 versionCount = sizeof(vmbus_versions) / sizeof(vmbus_versions[0]);
	status_t status = B_NOT_INITIALIZED;

	for (uint32 i = 0; i < versionCount; i++) {
		status = _ConnectVersion(vmbus_versions[i]);
		if (status == B_OK) {
			fVersion = vmbus_versions[i];
			break;
		}
	}

	if (status != B_OK)
		return status;

	TRACE("Connected to VMBus version %u.%u conn id %u\n", GET_VMBUS_VERSION_MAJOR(fVersion),
		GET_VMBUS_VERSION_MINOR(fVersion), fConnectionId);

	if (fVersion == VMBUS_VERSION_WS2008 || fVersion == VMBUS_VERSION_WS2008R2)
		fMaxChannelsCount = VMBUS_MAX_CHANNELS_LEGACY;
	else
		fMaxChannelsCount = VMBUS_MAX_CHANNELS;

	fChannels = new(std::nothrow) VMBusChannel*[fMaxChannelsCount];
	if (fChannels == NULL)
		return B_NO_MEMORY;
	bzero(fChannels, sizeof(*fChannels) * fMaxChannelsCount);

	if (fVersion == VMBUS_VERSION_WS2008 || fVersion == VMBUS_VERSION_WS2008R2)
		fEventFlagsHandler = &VMBus::_InterruptEventFlagsLegacy;
	else
		fEventFlagsHandler = &VMBus::_InterruptEventFlags;

	fConnected = true;
	return B_OK;
}


status_t
VMBus::_Disconnect()
{
	if (!fConnected)
		return B_OK;

	fEventFlagsHandler = &VMBus::_InterruptEventFlagsNull;

	for (uint32 i = 0; i < fMaxChannelsCount; i++) {
		InterruptsSpinLocker spinLocker(fChannelsSpinlock);
		VMBusChannel* channel = fChannels[i];
		fChannels[i] = NULL;
		spinLocker.Unlock();

		if (channel == NULL)
			continue;

		_UnregisterChannel(channel);
	}
	delete[] fChannels;

	VMBusRequest* request = new(std::nothrow) VMBusRequest(VMBUS_MSGTYPE_DISCONNECT,
		VMBUS_CHANNEL_ID);
	if (request == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<VMBusRequest> requestDeleter(request);

	status_t status = request->InitCheck();
	if (status != B_OK)
		return status;

	ConditionVariableEntry entry;
	request->SetResponseType(VMBUS_MSGTYPE_DISCONNECT_RESPONSE);
	status = _SendRequest(request, &entry);
	if (status != B_OK)
		return status;

	fConnected = false;
	TRACE("Disconnection status (%s)\n", strerror(status));
	return status;
}


/*static*/ status_t
VMBus::_ChannelQueueThreadHandler(void* arg)
{
	VMBus* vmbus = reinterpret_cast<VMBus*>(arg);
	return vmbus->_ChannelQueueThread();
}


status_t
VMBus::_ChannelQueueThread()
{
	while (acquire_sem(fChannelQueueSem) == B_OK) {
		MutexLocker locker(fChannelQueueLock);
		VMBusChannel* newChannel = fChannelOfferList.RemoveHead();
		VMBusChannel* oldChannel = fChannelRescindList.RemoveHead();
		locker.Unlock();

		if (newChannel != NULL) {
			status_t status = _RegisterChannel(newChannel);
			if (status != B_OK)
				ERROR("Failed to register channel %u (%s)\n", newChannel->channel_id,
					strerror(status));
		}

		if (oldChannel != NULL) {
			uint32 oldChannelID = oldChannel->channel_id;
			status_t status = _UnregisterChannel(oldChannel);
			if (status != B_OK)
				ERROR("Failed to unregister channel %u (%s)\n", oldChannelID, strerror(status));
		}
	}

	TRACE("Exiting channel queue thread\n");
	return B_OK;
}


VMBusChannel*
VMBus::_GetChannel(uint32 channelID, MutexLocker& channelLocker)
{
	if (channelID == VMBUS_CHANNEL_ID || channelID >= fMaxChannelsCount)
		return NULL;

	ReadLocker locker(fChannelsLock);

	InterruptsSpinLocker spinLocker(fChannelsSpinlock);
	VMBusChannel* channel = fChannels[channelID];
	spinLocker.Unlock();

	if (channel != NULL)
		channelLocker.SetTo(channel->lock, false, true);
	return channel;
}


status_t
VMBus::_RegisterChannel(VMBusChannel* channel)
{
	char typeStr[37];
	snprintf(typeStr, sizeof(typeStr), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		channel->type_id.data1, channel->type_id.data2, channel->type_id.data3,
		channel->type_id.data4[0], channel->type_id.data4[1], channel->type_id.data4[2],
		channel->type_id.data4[3], channel->type_id.data4[4], channel->type_id.data4[5],
		channel->type_id.data4[6], channel->type_id.data4[7]);

#ifdef TRACE_VMBUS
	char instanceStr[37];
	snprintf(instanceStr, sizeof(instanceStr),
		"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		channel->instance_id.data1, channel->instance_id.data2, channel->instance_id.data3,
		channel->instance_id.data4[0], channel->instance_id.data4[1],
		channel->instance_id.data4[2], channel->instance_id.data4[3],
		channel->instance_id.data4[4], channel->instance_id.data4[5],
		channel->instance_id.data4[6], channel->instance_id.data4[7]);
	TRACE("Registering VMBus channel %u type %s inst %s\n", channel->channel_id, typeStr,
		instanceStr);
#endif

	char prettyName[sizeof(HYPERV_PRETTYNAME_VMBUS_DEVICE_FMT) + 8];
	snprintf(prettyName, sizeof(prettyName), HYPERV_PRETTYNAME_VMBUS_DEVICE_FMT,
		channel->channel_id);

	device_attr attributes[10] = {
		{ B_DEVICE_BUS, B_STRING_TYPE,
			{ .string = HYPERV_BUS_NAME }},
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ .string = prettyName }},
		{ HYPERV_CHANNEL_ID_ITEM, B_UINT32_TYPE,
			{ .ui32 = channel->channel_id }},
		{ HYPERV_DEVICE_TYPE_STRING_ITEM, B_STRING_TYPE,
			{ .string = typeStr }},
		{ HYPERV_INSTANCE_ID_ITEM, B_RAW_TYPE,
			{ .raw = { .data = &channel->instance_id, .length = sizeof(channel->instance_id) }}},
		{ NULL }
	};

	uint32 attributeCount = 5;

	// Add MMIO attribute if present (typically PCI passthrough and graphics)
	if (channel->mmio_size > 0) {
		attributes[attributeCount] = {
			HYPERV_MMIO_SIZE_ITEM, B_UINT32_TYPE,
				{ .ui32 = channel->mmio_size }
		};
		attributeCount++;
	}

	// Add to active channel list
	InterruptsSpinLocker spinLocker(fChannelsSpinlock);
	if (fHighestChannelID < channel->channel_id)
		fHighestChannelID = channel->channel_id;
	fChannels[channel->channel_id] = channel;
	spinLocker.Unlock();

	return gDeviceManager->register_node(fNode, HYPERV_DEVICE_MODULE_NAME, attributes, NULL,
		&channel->node);
}


status_t
VMBus::_UnregisterChannel(VMBusChannel* channel)
{
	uint32 channelID = channel->channel_id;
	TRACE("Unregistering channel %u\n", channelID);
	if (channel->node != NULL) {
		gDeviceManager->unregister_node(channel->node);
		channel->node = NULL;
	}

	WriteLocker removeLocker(fChannelsLock);

	// It's possible the channel is actively being used by another thread,
	// wait for that to complete. The channel will have been removed from the active
	// list by this point, preventing any further use.
	delete channel;

	removeLocker.Unlock();

	VMBusRequest* request = new(std::nothrow) VMBusRequest(VMBUS_MSGTYPE_FREE_CHANNEL,
		VMBUS_CHANNEL_ID);
	if (request == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<VMBusRequest> requestDeleter(request);

	status_t status = request->InitCheck();
	if (status != B_OK)
		return status;

	request->GetMessage()->free_channel.channel_id = channelID;
	status = _SendRequest(request);
	if (status != B_OK)
		return status;

	TRACE("Freed channel %u\n", channelID);
	return B_OK;
}


area_id
VMBus::_AllocateBuffer(const char* name, size_t length, uint32 protection, void** _buffer,
	phys_addr_t* _physAddr)
{
	TRACE("Allocating %ld bytes for %s\n", length, name);

	void* buffer;
	length = HV_PAGE_ALIGN(length);
	area_id areaid = create_area(name, &buffer, B_ANY_KERNEL_ADDRESS, length,
		B_CONTIGUOUS, protection);
	if (areaid < B_OK)
		return areaid;

	physical_entry entry;
	status_t status = get_memory_map(buffer, length, &entry, 1);
	if (status != B_OK) {
		delete_area(areaid);
		return status;
	}

	if ((protection & B_WRITE_AREA) != 0)
		bzero(buffer, length);

	TRACE("Allocated area %" B_PRId32 " length %ld buf %p phys %" B_PRIxPHYSADDR "\n", areaid,
		length, buffer, entry.address);

	*_buffer = buffer;
	*_physAddr = entry.address;
	return areaid;
}


inline uint32
VMBus::_GetGPADLHandle()
{
	uint32 gpadl;
	do {
		gpadl = static_cast<uint32>(atomic_add(&fCurrentGPADLHandle, 1));
	} while (gpadl == VMBUS_GPADL_NULL);
	return gpadl;
}
