/*
 * Copyright 2004-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hamza Dridi, <dridiha98@gmail.com>
 */

#include "ufshci.h"

#include <device_manager.h>
#include <drivers/KernelExport.h>
#include <new>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>

#include "ByteOrder.h"
#include "Errors.h"
#include "OS.h"
#include "PCI.h"
#include "SupportDefs.h"

#include "bus/SCSI.h"
#include "scsi_cmds.h"
#include "util/AutoLock.h"
#include "vm/vm.h"
#include "vm/vm_types.h"

extern scsi_for_sim_interface* gSCSI;


UfsHciController::UfsHciController(pci_device_module_info* pci, pci_device* pci_dev,
	struct pci_info* pci_device_info)
	:
	fRegisters(NULL),
	fRegisterArea(-1),
	fPci(pci),
	fPciDev(pci_dev),
	fPciDeviceInfo(pci_device_info),
	fUtrlBuffer(NULL),
	fUtmrlBuffer(NULL),
	fUcdBuffer(NULL),
	fFreeQueueSem(-1),
	fDeviceCommandCompletionSem(-1),
	fFreeSlotsSem(-1),
	fFreeBuffersSem(-1),
	fIRQ(-1),
	fUtrlBufferEntries(0),
	fUtmrlBufferEntries(0),
	fTaskTag(0)
{
}


UfsHciController::~UfsHciController()
{
	/* Disbale the interrupts */
	fRegisters->interrupt_enable = 0;

	/* Remove the interrupt handler */
	if (fIRQ != -1)
		remove_io_interrupt_handler(fIRQ, _Interrupt, (void*)this);


	/* Destroy spinlock */

	/* Free the queues */
	_FreeSlotsQueue(fFreeSlotsQueue);
	_FreeSlotsQueue(fActiveSlotsQueue);

	/* Delete the semaphores used */
	if (fFreeQueueSem != -1)
		delete_sem(fFreeQueueSem);

	if (fDeviceCommandCompletionSem != -1)
		delete_sem(fDeviceCommandCompletionSem);

	if (fFreeSlotsSem != -1)
		delete_sem(fFreeSlotsSem);

	if (fFreeBuffersSem != -1)
		delete_sem(fFreeBuffersSem);


	/* Delete the buffers */
	if (fUtmrlBuffer != NULL) {
		if (fUtmrlBuffer->area != -1)
			delete_area(fUtmrlBuffer->area);

		delete fUtmrlBuffer;
	}

	if (fUtrlBuffer != NULL) {
		if (fUtrlBuffer->area != -1)
			delete_area(fUtrlBuffer->area);
		delete fUtrlBuffer;
	}

	if (fUcdBuffer != NULL) {
		if (fUcdBuffer->area != -1)
			delete_area(fUcdBuffer->area);
		delete fUcdBuffer;
	}

	/* Delete the registers memory map */
	if (fRegisterArea != -1)
		delete_area(fRegisterArea);
}


status_t
UfsHciController::Init()
{
	B_INITIALIZE_SPINLOCK(&fSlotLock);

	fUtrlBuffer = new(std::nothrow) KernelBufferInfo;
	if (fUtrlBuffer == NULL) {
		TRACE("Init:: No enough memory available\n");
		return B_NO_MEMORY;
	}

	memset(fUtrlBuffer, 0, sizeof(KernelBufferInfo));
	fUtrlBuffer->area = -1;


	fUtmrlBuffer = new(std::nothrow) KernelBufferInfo;
	if (fUtmrlBuffer == NULL) {
		TRACE("Init:: No enough memory available\n");
		return B_NO_MEMORY;
	}

	memset(fUtmrlBuffer, 0, sizeof(KernelBufferInfo));
	fUtmrlBuffer->area = -1;

	fUcdBuffer = new(std::nothrow) KernelBufferInfo;
	if (fUcdBuffer == NULL) {
		TRACE("Init:: No enough memory available\n");
		return B_NO_MEMORY;
	}

	memset(fUcdBuffer, 0, sizeof(KernelBufferInfo));
	fUcdBuffer->area = -1;


	uint16 command = fPci->read_pci_config(fPciDev, PCI_command, 2);
	command |= PCI_command_memory | PCI_command_master | PCI_command_io;

	fPci->write_pci_config(fPciDev, PCI_command, 2, command);


	phys_addr_t physical_address = fPciDeviceInfo->u.h0.base_registers[0];
	size_t size = fPciDeviceInfo->u.h0.base_register_sizes[0];

	if ((fPciDeviceInfo->u.h0.base_register_flags[0] & PCI_address_type) == PCI_address_type_64) {
		physical_address |= (uint64)fPciDeviceInfo->u.h0.base_registers[1] << 32;
		size |= (uint64)fPciDeviceInfo->u.h0.base_register_sizes[1] << 32;
	}


	fRegisterArea = map_physical_memory("UFSHCI Mapped memory registers", physical_address, size,
		B_ANY_KERNEL_BLOCK_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&fRegisters);

	if (fRegisterArea < B_OK) {
		TRACE("Init:: Error mapping memory registers: %s\n", strerror(fRegisterArea));
		return fRegisterArea;
	}

	fIRQ = fPciDeviceInfo->u.h0.interrupt_line;

	status_t status = install_io_interrupt_handler(fIRQ, _Interrupt, (void*)this, 0);

	if (status < B_OK) {
		TRACE("Init:: Error installing the interrupt handler: %s\n", strerror(status));
		fIRQ = -1;
		return status;
	}

	fUtmrlBufferEntries = (((fRegisters->capabilities) >> 16) & 7) + 1;
	fUtrlBufferEntries = ((fRegisters->capabilities) & 31) + 1;

	fFreeQueueSem = create_sem(1, "Ufshci Mutex for accessing free slots queue concurrently");
	fDeviceCommandCompletionSem = create_sem(0, "Ufshci Sequential Command Semaphore");
	fFreeSlotsSem = create_sem(fUtrlBufferEntries, "Free slots semaphore");
	fFreeBuffersSem = create_sem(fUtrlBufferEntries, "Free DMA buffers semaphore");

	if (fFreeQueueSem < B_OK || fDeviceCommandCompletionSem < B_OK || fFreeSlotsSem < B_OK
		|| fFreeBuffersSem < B_OK) {
		TRACE("Init:: Error creating semaphores\n");
		return B_ERROR;
	}

	return _InitBus();
}


void
UfsHciController::ExecuteRequest(scsi_ccb* request)
{
	CALLED();
	uint8 cmd = request->cdb[0];

	if (cmd == SCSI_OP_TEST_UNIT_READY) {
		request->subsys_status = SCSI_REQ_CMP;
		gSCSI->finished(request, 1);
		return;
	}

	return _SubmitRequest(request);
}


status_t
UfsHciController::HandleUtrlInterrupt()
{
	UfsSlot* ufsList[UFS_MAX_UTRL_ENTRIES] = {0};
	uint8 completedCount = 0;


	{
		SpinLocker locker(&fSlotLock);

		UfsSlot* slot = fActiveSlotsQueue.First();

		while (slot) {
			UfsSlot* next = fActiveSlotsQueue.GetNext(slot);
			if (!(fRegisters->utrl_doorbell >> (slot->slotIndex))) {

				fActiveSlotsQueue.Remove(slot);
				ufsList[completedCount++] = slot;
			}

			slot = next;
		}
	}

	for (uint8 i = 0; i < completedCount; i++) {
		UfsSlot* slot = ufsList[i];
		release_sem_etc(slot->sem, 1, B_DO_NOT_RESCHEDULE);
	}

	return B_OK;
}


uchar
UfsHciController::PathInquiry(scsi_path_inquiry* info)
{
	memset(info, 0, sizeof(scsi_path_inquiry));

	info->initiator_id = UFSHCI_MAX_LUN_COUNT;
	info->hba_queue_size = fUtrlBufferEntries;
	info->hba_inquiry = SCSI_PI_TAG_ABLE;

	return SCSI_REQ_CMP;
}


void
UfsHciController::GetRestrictions(bool* isATAPI, bool* noAutoSense, uint32* maxBlocks)
{
	*isATAPI = false;
	*noAutoSense = false;
	*maxBlocks = UFS_UTRLD_MAX_PRDT_BLOCKS;
}


status_t
UfsHciController::_VerifyLinkStartup()
{
	uint8 retries = 3;
	for (uint8 i = 0; i < retries; i++) {

		/* Activate the interrupt for UIC */
		fRegisters->interrupt_enable |= (1 << 10);

		/* Send DME_LINKSTARTUP to UIC */
		fRegisters->uic_command |= 0x16;

		/* Wait for the Completion of the UIC command */
		acquire_sem(fDeviceCommandCompletionSem);


		/* Verify that the status DME_LINKSTARTUP is success */
		uint8 dme_linkstartup = (fRegisters->uic_command_argument_2) & 0x1;

		/* 0x1 means that there was a failure */
		if (!dme_linkstartup)
			return B_OK;
	}

	TRACE("_VerifyLinkStartup:: Failure in DME_LINKSTARTUP\n");
	return B_ERROR;
}


int32
UfsHciController::_Interrupt(void* data)
{
	UfsHciController* controller = (UfsHciController*)data;

	if (controller->Registers()->interrupt_status == 0)
		return B_UNHANDLED_INTERRUPT;

	uint8 uic_interrupt_status = (controller->Registers()->interrupt_status >> 10) & 0x1;
	uint8 uic_interrupt_enable = (controller->Registers()->interrupt_enable >> 10) & 0x1;
	uint8 utrl_interrupt_status = controller->Registers()->interrupt_status & 0x1;

	if (uic_interrupt_status != 0 && uic_interrupt_enable != 0) {
		controller->Registers()->interrupt_status = (1 << 10);
		release_sem_etc(controller->CompletionSem(), 1, B_DO_NOT_RESCHEDULE);
	}

	if (utrl_interrupt_status != 0) {
		controller->Registers()->interrupt_status = 0x1;
		controller->HandleUtrlInterrupt();
	}

	return B_HANDLED_INTERRUPT;
}


status_t
UfsHciController::_InitBus()
{
	/* write 1 to HCE */
	fRegisters->host_controller_enable = 0x1;
	snooze(1000);

	if (fRegisters->host_controller_enable != 0x1) {
		TRACE("_InitBus:: Unable to set the host controller register\n");
		return B_ERROR;
	}

	status_t status = _VerifyLinkStartup();
	if (status != B_OK)
		return status;

	/* Verify that a device is attached */
	if (!(fRegisters->host_controller_status & 0x1)) {
		TRACE("_InitBus:: DME_LINKSTARTUP: No device is attached to UFSHCI\n");
		return B_ERROR;
	}

	/* Allocate buffers for UTP transfer request list */
	status
		= _AllocateBuffers(B_PAGE_SIZE >> 2, sizeof(UtpTmRequestDescriptor) * fUtmrlBufferEntries,
			"UTP Task Management Request List", fUtmrlBuffer);

	if (status < B_OK)
		return status;


	fRegisters->utmrl_base_address_low = (fUtmrlBuffer->phys_entry.address & 0xFFFFFFFF);
	fRegisters->utmrl_base_address_high = ((fUtmrlBuffer->phys_entry.address >> 32) & 0xFFFFFFFF);

	fRegisters->utmrl_run_stop = 0x1;

	status = _AllocateBuffers(B_PAGE_SIZE >> 2,
		sizeof(UtpTransferRequestDescriptor) * fUtrlBufferEntries,
		"UTP Transfer request List Buffer", fUtrlBuffer);

	if (status < B_OK)
		return status;

	fRegisters->utrl_base_address_low = (fUtrlBuffer->phys_entry.address & 0xFFFFFFFF);
	fRegisters->utrl_base_address_high = ((fUtrlBuffer->phys_entry.address >> 32) & 0xFFFFFFFF);
	fRegisters->utrl_run_stop = 0x1;

	status = _AllocateBuffers(B_PAGE_SIZE, sizeof(UtpCommandDescriptor) * fUtrlBufferEntries,
		"UTP Command Descriptor List", fUcdBuffer);

	if (status < B_OK)
		return status;

	UfsSlot* slot = NULL;

	for (uint8 i = 0; i < fUtrlBufferEntries; i++) {
		slot = (UfsSlot*)malloc(sizeof(UfsSlot));

		if (slot == NULL) {
			TRACE("_InitBus:: Error allocating memory for UfsSlot\n");
			return B_NO_MEMORY;
		}

		char name[32] = {0};
		memset((void*)slot, 0, sizeof(UfsSlot));

		slot->slotIndex = i;
		slot->sem = -1;

		snprintf(name, sizeof(name), "Ufs Slot Metadata Semaphore %d", i);

		slot->sem = create_sem(0, name);

		if (slot->sem < B_OK) {
			TRACE("_InitBus:: Error creating semaphores\n");
			return slot->sem;
		}

		fFreeSlotsQueue.Add(slot);
	}

	/* Enable interrupts of UTP TRL and UTP TMRL */
	fRegisters->interrupt_enable |= (1 << 9) | 0x1;
	TRACE("_InitBus:: Hardware Initalisation completed successfully\n");

	return B_OK;
}


status_t
UfsHciController::_AllocateBuffers(uint32 physical_alignment, size_t size, const char* name,
	KernelBufferInfo* info)
{

	virtual_address_restrictions virtual_restrictions = {};
	virtual_restrictions.address_specification = B_ANY_KERNEL_BLOCK_ADDRESS;

	physical_address_restrictions physical_restrictions = {};
	physical_restrictions.alignment = physical_alignment;


	info->area = create_area_etc(B_SYSTEM_TEAM, name, size, B_CONTIGUOUS,
		B_KERNEL_WRITE_AREA | B_KERNEL_READ_AREA, 0, 0, &virtual_restrictions,
		&physical_restrictions, &(info->virtual_address));

	if (info->area < B_OK) {
		TRACE("_AllocateBuffers:: Error creating area: %s\n", strerror(info->area));
		return info->area;
	}

	if (get_memory_map(info->virtual_address, size, &(info->phys_entry), 1) != B_OK) {
		TRACE("_AllocateBuffers:: Unable to get memory map for %s\n", name);
		delete_area(info->area);
		info->area = -1;
		return B_ERROR;
	}

	return B_OK;
}


UfsSlot*
UfsHciController::_AcquireFreeSlot()
{
	status_t status = acquire_sem(fFreeSlotsSem);
	UfsSlot* slot = NULL;
	if (status < B_OK) {
		TRACE("_AcquireFreeSlot:: Unable to acquire a slot: %s\n", strerror(status));
		return slot;
	}

	AcquireFreeQueueLock();
	slot = fFreeSlotsQueue.RemoveHead();
	ReleaseFreeQueueLock();

	return slot;
}


void
UfsHciController::_FreeSlotsQueue(UfsSlotList& queue)
{
	UfsSlot* slot = queue.Head();
	uint8 count = queue.Count();

	for (uint8 i = 0; i < count; i++) {

		UfsSlot* next = queue.GetNext(slot);

		delete_sem(slot->sem);

		queue.Remove(slot);
		free(slot);

		slot = next;
	}
}


UfsSlot*
UfsHciController::_SubmitUtpTransferRequest(scsi_ccb* request)
{

	/* Acquire Free slot, if no slot available, we will block until a slot is free */
	UfsSlot* slot = _AcquireFreeSlot();

	if (slot == NULL) {
		TRACE("_SubmitUtpTransferRequest:: Unable to acquire a free slot\n");
		return NULL;
	}

	uint8 slotIndex = slot->slotIndex;

	struct UtpCommandDescriptor* ucd
		= (struct UtpCommandDescriptor*)fUcdBuffer->virtual_address + slotIndex;

	memset(ucd, 0, sizeof(struct UtpCommandDescriptor));

	phys_addr_t ucd_phys = (phys_addr_t)(fUcdBuffer->phys_entry.address
		+ slotIndex * sizeof(struct UtpCommandDescriptor));

	/* Init the Command UPIU header */
	ucd->command.header.transaction_type = UFS_UCD_COMMAND_UPIU;
	ucd->command.header.flags
		= (request->flags & SCSI_DIR_OUT) ? UFS_UCD_COMMAND_WRITE_FLAG : UFS_UCD_COMMAND_READ_FLAG;
	ucd->command.header.lun = request->target_lun;
	ucd->command.header.task_tag = fTaskTag.fetch_add(1);
	ucd->command.header.command_type = 0;

	ucd->command.header.total_ehs_length = 0;
	ucd->command.header.data_segment_length = 0;
	ucd->command.expected_data_transfer_length = B_HOST_TO_BENDIAN_INT32(request->data_length);

	memcpy(ucd->command.cdb, request->cdb, request->cdb_length);

	if ((request->flags & SCSI_DIR_MASK) != SCSI_DIR_NONE) {
		for (uint32 i = 0; i < request->sg_count; i++) {
			physical_entry entry = request->sg_list[i];

			/* Prdt base address and data byte count must be Dword aligned */
			ucd->prdt[i].data_base_address |= entry.address & 0xFFFFFFFF;
			ucd->prdt[i].data_base_address_upper = (entry.address >> 32) & 0xFFFFFFFF;
			ucd->prdt[i].data_byte_count |= (entry.size - 1);
		}
	}


	struct UtpTransferRequestDescriptor* descriptor
		= (struct UtpTransferRequestDescriptor*)fUtrlBuffer->virtual_address + slotIndex;

	memset(descriptor, 0, sizeof(struct UtpTransferRequestDescriptor));

	/* Set up the command type: SCSI : 0x0 */
	descriptor->reserved[0] &= UFS_UTRLD_SCSI_COMMAND_TYPE;

	/*
	 * Set up that the transfert is backed by an interrupt
	 * Set up the direction of the transfer
	 */
	descriptor->reserved[0] |= UFS_UTRLD_INTERRUPT
		| ((request->flags & SCSI_DIR_OUT) ? UFS_UTRLD_TRANSFER_TO_DEVICE
										   : UFS_UTRLD_TRANSFER_FROM_DEVICE);

	/* Initialize the overall status to 0xF */
	descriptor->reserved[2] = UFS_UTRLD_OVERALL_STATUS;

	/* Set up the address of UtpCommandDescriptor: the address must be Dword Aligned */
	descriptor->utp_cmd_descriptor_low_addr |= ucd_phys & 0xFFFFFFFF;
	descriptor->utp_cmd_descriptor_high_addr = (ucd_phys >> 32) & 0xFFFFFFFF;

	/*
	 * Set up the offset and the length of the Response UPIU
	 * The length is the count of DWORD: Already fixed at 0x80 Bytes
	 * The offset is the count of DWORD from the start of UCD : Fixed at 0x80 Bytes
	 */
	descriptor->utp_response_upiu_offset = UFS_UTRLD_COMMAND_LENGTH >> 2;
	descriptor->utp_response_upiu_length = UFS_UTRLD_RESPONSE_LENGTH >> 2;

	/*
	 * Set up the offset and the length of Prdt phys_entry
	 * The length is the number of entries
	 * The offset is the count of DWORD from the start of UCD: Fixed at 0x100 Bytes
	 */
	descriptor->prdt_offset = (UFS_UTRLD_COMMAND_LENGTH + UFS_UTRLD_RESPONSE_LENGTH) >> 2;
	descriptor->prdt_length
		= ((request->flags & SCSI_DIR_MASK) == SCSI_DIR_NONE) ? 0 : request->sg_count;

	slot->descriptor = descriptor;
	slot->ucd = ucd;

	/* Put the slot into the active Queue after acquiring the spinlock */
	{
		InterruptsSpinLocker locker(&fSlotLock);
		fActiveSlotsQueue.Add(slot);

		/* Ring the doorbell */
		fRegisters->utrl_doorbell = (1 << slotIndex);
	}

	return slot;
}


void
UfsHciController::_ProcessCompletedUtpRequest(UfsSlot* slot, scsi_ccb* request)
{

	struct UtpTransferRequestDescriptor* descriptor = slot->descriptor;
	struct UtpCommandDescriptor* ucd = slot->ucd;


	/* Check if the overall status, the response field in the response header */
	if (((descriptor->reserved[2] & 0xF) == 0) && ucd->response.reserved[3] == 0x0
		&& ucd->response.header.response == 0x0 && ucd->response.header.status == 0x0) {

		request->subsys_status = SCSI_REQ_CMP;

	} else {
		request->subsys_status = SCSI_REQ_CMP_ERR;
		uint16 senseDataLength = B_BENDIAN_TO_HOST_INT16(ucd->response.sense_data_length);
		memcpy(request->sense, ucd->response.sense_data_and_pad,
			min_c(senseDataLength, SCSI_MAX_SENSE_SIZE));
	}

	request->data_resid = ucd->response.residual_transfer_count;
	request->device_status = ucd->response.header.status;
}


void
UfsHciController::_ReleaseSlot(UfsSlot* slot)
{

	memset(slot->descriptor, 0, sizeof(struct UtpTransferRequestDescriptor));
	memset(slot->ucd, 0, sizeof(struct UtpCommandDescriptor));

	slot->ucd = NULL;
	slot->descriptor = NULL;

	AcquireFreeQueueLock();
	fFreeSlotsQueue.Add(slot);
	ReleaseFreeQueueLock();

	release_sem(fFreeSlotsSem);
}


void
UfsHciController::_SubmitRequest(scsi_ccb* request)
{

	UfsSlot* slot = _SubmitUtpTransferRequest(request);

	if (slot == NULL) {
		request->subsys_status = SCSI_REQ_ABORTED;
		request->device_status = SCSI_BUSY;
		gSCSI->finished(request, 1);
		return;
	}

	acquire_sem(slot->sem);

	_ProcessCompletedUtpRequest(slot, request);
	_ReleaseSlot(slot);

	gSCSI->finished(request, 1);
}
