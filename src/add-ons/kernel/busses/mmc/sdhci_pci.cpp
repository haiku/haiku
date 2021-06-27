/*
 * Copyright 2018-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */
#include <algorithm>
#include <new>
#include <stdio.h>
#include <string.h>

#include <bus/PCI.h>
#include <PCI_x86.h>

#include <KernelExport.h>

#include "IOSchedulerSimple.h"
#include "mmc.h"
#include "sdhci_pci.h"


#define TRACE_SDHCI
#ifdef TRACE_SDHCI
#	define TRACE(x...) dprintf("\33[33msdhci_pci:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33msdhci_pci:\33[0m " x)
#define ERROR(x...)			dprintf("\33[33msdhci_pci:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define SDHCI_PCI_DEVICE_MODULE_NAME "busses/mmc/sdhci_pci/driver_v1"
#define SDHCI_PCI_MMC_BUS_MODULE_NAME "busses/mmc/sdhci_pci/device/v1"

#define SLOT_NUMBER				"device/slot"
#define BAR_INDEX				"device/bar"


class SdhciBus {
	public:
							SdhciBus(struct registers* registers, uint8_t irq);
							~SdhciBus();

		void				EnableInterrupts(uint32_t mask);
		void				DisableInterrupts();
		status_t			ExecuteCommand(uint8_t command, uint32_t argument,
								uint32_t* response);
		int32				HandleInterrupt();
		status_t			InitCheck();
		void				Reset();
		void				SetClock(int kilohertz);
		status_t			DoIO(uint8_t command, IOOperation* operation,
								bool offsetAsSectors);
		void				SetScanSemaphore(sem_id sem);
		void				SetBusWidth(int width);

	private:
		bool				PowerOn();
		void				RecoverError();

	private:
		struct registers*	fRegisters;
		uint32_t			fCommandResult;
		uint8_t				fIrq;
		sem_id				fSemaphore;
		sem_id				fScanSemaphore;
		status_t			fStatus;
};


class SdhciDevice {
	public:
		device_node* fNode;
		uint8_t fRicohOriginalMode;
};


device_manager_info* gDeviceManager;
device_module_info* gMMCBusController;
static pci_x86_module_info* sPCIx86Module;


static int32
sdhci_generic_interrupt(void* data)
{
	SdhciBus* bus = (SdhciBus*)data;
	return bus->HandleInterrupt();
}


SdhciBus::SdhciBus(struct registers* registers, uint8_t irq)
	:
	fRegisters(registers),
	fIrq(irq),
	fSemaphore(0)
{
	if (irq == 0 || irq == 0xff) {
		ERROR("PCI IRQ not assigned\n");
		fStatus = B_BAD_DATA;
		return;
	}

	fSemaphore = create_sem(0, "SDHCI interrupts");
	
	fStatus = install_io_interrupt_handler(fIrq,
		sdhci_generic_interrupt, this, 0);

	if (fStatus != B_OK) {
		ERROR("can't install interrupt handler\n");
		return;
	}

	// First of all, we have to make sure we are in a sane state. The easiest
	// way is to reset everything.
	Reset();

	// Turn on the power supply to the card, if there is a card inserted
	if (PowerOn()) {
		// Then we configure the clock to the frequency needed for
		// initialization
		SetClock(400);
	}

	// Finally, configure some useful interrupts
	EnableInterrupts(SDHCI_INT_CMD_CMP | SDHCI_INT_CARD_REM
		| SDHCI_INT_TRANS_CMP);

	// We want to see the error bits in the status register, but not have an
	// interrupt trigger on them (we get a "command complete" interrupt on
	// errors already)
	fRegisters->interrupt_status_enable |= SDHCI_INT_ERROR
		| SDHCI_INT_TIMEOUT | SDHCI_INT_CRC | SDHCI_INT_INDEX
		| SDHCI_INT_BUS_POWER | SDHCI_INT_END_BIT;
}


SdhciBus::~SdhciBus()
{
	DisableInterrupts();

	if (fSemaphore != 0)
		delete_sem(fSemaphore);

	if (fIrq != 0)
		remove_io_interrupt_handler(fIrq, sdhci_generic_interrupt, this);

	area_id regs_area = area_for(fRegisters);
	delete_area(regs_area);
}


void
SdhciBus::EnableInterrupts(uint32_t mask)
{
	fRegisters->interrupt_status_enable |= mask;
	fRegisters->interrupt_signal_enable |= mask;
}


void
SdhciBus::DisableInterrupts()
{
	fRegisters->interrupt_status_enable = 0;
	fRegisters->interrupt_signal_enable = 0;
}


// #pragma mark -
/*
PartA2, SD Host Controller Simplified Specification, Version 4.20
§3.7.1.1 The sequence to issue an SD Command
*/
status_t
SdhciBus::ExecuteCommand(uint8_t command, uint32_t argument, uint32_t* response)
{
	TRACE("ExecuteCommand(%d, %x)\n", command, argument);

	// First of all clear the result
	fCommandResult = 0;

	// Check if it's possible to send a command right now.
	// It is not possible to send a command as long as the command line is busy.
	// The spec says we should wait, but we can't do that on kernel side, since
	// it leaves no chance for the upper layers to handle the problem. So we
	// just say we're busy and the caller can retry later.
	// Note that this should normally never happen: the command line is busy
	// only during command execution, and we don't leave this function with ac
	// command running.
	if (fRegisters->present_state.CommandInhibit()) {
		panic("Command execution impossible, command inhibit\n");
		return B_BUSY;
	}
	if (fRegisters->present_state.DataInhibit()) {
		panic("Command execution unwise, data inhibit\n");
		return B_BUSY;
	}

	uint32_t replyType;

	switch(command) {
		case SD_GO_IDLE_STATE:
			replyType = Command::kNoReplyType;
			break;
		case SD_ALL_SEND_CID:
		case SD_SEND_CSD:
			replyType = Command::kR2Type;
			break;
		case SD_SEND_RELATIVE_ADDR:
			replyType = Command::kR6Type;
			break;
		case SD_SELECT_DESELECT_CARD:
		case SD_ERASE:
			replyType = Command::kR1bType;
			break;
		case SD_SEND_IF_COND:
			replyType = Command::kR7Type;
			break;
		case SD_READ_SINGLE_BLOCK:
		case SD_READ_MULTIPLE_BLOCKS:
		case SD_WRITE_SINGLE_BLOCK:
		case SD_WRITE_MULTIPLE_BLOCKS:
			replyType = Command::kR1Type | Command::kDataPresent;
			break;
		case SD_APP_CMD:
		case SD_ERASE_WR_BLK_START:
		case SD_ERASE_WR_BLK_END:
		case SD_SET_BUS_WIDTH: // SD Application command
			replyType = Command::kR1Type;
			break;
		case SD_SEND_OP_COND: // SD Application command
			replyType = Command::kR3Type;
			break;
		default:
			ERROR("Unknown command %x\n", command);
			return B_BAD_DATA;
	}

	// Check if DATA line is available (if needed)
	if ((replyType & Command::k32BitResponseCheckBusy) != 0
		&& command != SD_STOP_TRANSMISSION && command != SD_IO_ABORT) {
		if (fRegisters->present_state.DataInhibit()) {
			ERROR("Execution aborted, data inhibit\n");
			return B_BUSY;
		}
	}

	if (fRegisters->present_state.CommandInhibit())
		panic("Command line busy at start of execute command\n");

	if (replyType == Command::kR1bType)
		fRegisters->transfer_mode = 0;
	
	fRegisters->argument = argument;
	fRegisters->command.SendCommand(command, replyType);

	// Wait for command response to be available ("command complete" interrupt)
	TRACE("Wait for command complete...");
	while (fCommandResult == 0) {
		acquire_sem(fSemaphore);
		TRACE("command complete sem acquired, status: %x\n", fCommandResult);
		TRACE("real status = %x command line busy: %d\n",
			fRegisters->interrupt_status,
			fRegisters->present_state.CommandInhibit());
	}

	TRACE("Command response available\n");

	if (fCommandResult & SDHCI_INT_ERROR) {
		fRegisters->interrupt_status |= fCommandResult;
		if (fCommandResult & SDHCI_INT_TIMEOUT) {
			ERROR("Command execution timed out\n");
			if (fRegisters->present_state.CommandInhibit()) {
				TRACE("Command line is still busy, clearing it\n");
				// Clear the stall
				fRegisters->software_reset.ResetCommandLine();
			}
			return B_TIMED_OUT;
		}
		if (fCommandResult & SDHCI_INT_CRC) {
			ERROR("CRC error\n");
			return B_BAD_VALUE;
		}
		ERROR("Command execution failed %x\n", fCommandResult);
		// TODO look at errors in interrupt_status register for more details
		// and return a more appropriate error code
		return B_ERROR;
	}

	if (fRegisters->present_state.CommandInhibit()) {
		TRACE("Command execution failed, card stalled\n");
		// Clear the stall
		fRegisters->software_reset.ResetCommandLine();
		return B_ERROR;
	}

	switch (replyType & Command::kReplySizeMask) {
		case Command::k32BitResponse:
			*response = fRegisters->response[0];
			break;
		case Command::k128BitResponse:
			response[0] = fRegisters->response[0];
			response[1] = fRegisters->response[1];
			response[2] = fRegisters->response[2];
			response[3] = fRegisters->response[3];
			break;

		default:
			// No response
			break;
	}

	if (replyType == Command::kR1bType
			&& (fCommandResult & SDHCI_INT_TRANS_CMP) == 0) {
		// R1b commands may use the data line so we must wait for the
		// "transfer complete" interrupt here.
		TRACE("Waiting for data line...\n");
		do {
			acquire_sem(fSemaphore);
		} while (fRegisters->present_state.DataInhibit());
		TRACE("Dataline is released.\n");
	}

	ERROR("Command execution %d complete\n", command);
	return B_OK;
}


status_t
SdhciBus::InitCheck()
{
	return fStatus;
}


void
SdhciBus::Reset()
{
	fRegisters->software_reset.ResetAll();
}


void
SdhciBus::SetClock(int kilohertz)
{
	int base_clock = fRegisters->capabilities.BaseClockFrequency();
	// Try to get as close to 400kHz as possible, but not faster
	int divider = base_clock * 1000 / kilohertz;

	if (fRegisters->host_controller_version.specVersion <= 1) {
		// Old controller only support power of two dividers up to 256,
		// round to next power of two up to 256
		if (divider > 256)
			divider = 256;

		divider--;
		divider |= divider >> 1;
		divider |= divider >> 2;
		divider |= divider >> 4;
		divider++;
	}

	divider = fRegisters->clock_control.SetDivider(divider);

	// Log the value after possible rounding by SetDivider (only even values
	// are allowed).
	TRACE("SDCLK frequency: %dMHz / %d = %dkHz\n", base_clock, divider,
		base_clock * 1000 / divider);

	// We have set the divider, now we can enable the internal clock.
	fRegisters->clock_control.EnableInternal();

	// wait until internal clock is stabilized
	while (!(fRegisters->clock_control.InternalStable()));

	fRegisters->clock_control.EnablePLL();
	while (!(fRegisters->clock_control.InternalStable()));

	// Finally, route the clock to the SD card
	fRegisters->clock_control.EnableSD();
}


status_t
SdhciBus::DoIO(uint8_t command, IOOperation* operation, bool offsetAsSectors)
{
	bool isWrite = operation->IsWrite();

	static const uint32 kBlockSize = 512;
	off_t offset = operation->Offset();
	generic_size_t length = operation->Length();

	TRACE("%s %"B_PRIu64 " bytes at %" B_PRIdOFF "\n",
		isWrite ? "Write" : "Read", length, offset);

	// Check that the IO scheduler did its job in following our DMA restrictions
	// We can start a read only at a sector boundary
	ASSERT(offset % kBlockSize == 0);
	// We can only read complete sectors
	ASSERT(length % kBlockSize == 0);

	const generic_io_vec* vecs = operation->Vecs();
	generic_size_t vecOffset = 0;

	// FIXME can this be moved to the init function instead?
	//
	// For simplicity we use a transfer size equal to the sector size. We could
	// go up to 2K here if the length to read in each individual vec is a
	// multiple of 2K, but we have no easy way to know this (we would need to
	// iterate through the IOOperation vecs and check the size of each of them).
	// We could also do smaller transfers, but it is not possible to start a
	// transfer anywhere else than the start of a sector, so it's a lot simpler
	// to always work in complete sectors. We set the B_DMA_ALIGNMENT device
	// node property accordingly, making sure that we don't get asked to do
	// transfers that are not aligned with sectors.
	//
	// Additionnally, set SDMA buffer boundary aligment to 512K. This is the
	// largest possible size. We also set the B_DMA_BOUNDARY property on the
	// published device node, so that the DMA resource manager knows that it
	// must respect this boundary. As a result, we will never be asked to
	// do a transfer that crosses this boundary, and we don't need to handle
	// the DMA boundary interrupt (the transfer will be split in two at an
	// upper layer).
	fRegisters->block_size.ConfigureTransfer(kBlockSize,
		BlockSize::kDmaBoundary512K);
	status_t result = B_OK;

	while (length > 0) {
		size_t toCopy = std::min((generic_size_t)length,
			vecs->length - vecOffset);

		// If the current vec is empty, we can move to the next
		if (toCopy == 0) {
			vecs++;
			vecOffset = 0;
			continue;
		}

		// With SDMA we can only transfer multiples of 1 sector
		ASSERT(toCopy % kBlockSize == 0);

		fRegisters->system_address = vecs->base + vecOffset;
		// fRegisters->adma_system_address = fDmaMemory;

		fRegisters->block_count = toCopy / kBlockSize;

		uint16 direction;
		if (isWrite)
			direction = TransferMode::kWrite;
		else
			direction = TransferMode::kRead;
		fRegisters->transfer_mode = TransferMode::kMulti | direction
			| TransferMode::kAutoCmd12Enable
			| TransferMode::kBlockCountEnable | TransferMode::kDmaEnable;

		uint32_t response;
		result = ExecuteCommand(command,
			offset / (offsetAsSectors ? kBlockSize : 1), &response);
		if (result != B_OK)
			break;

		// Wait for DMA transfer to complete
		// In theory we could go on and send other commands as long as they
		// don't need the DAT lines, but it's overcomplicating things.
		TRACE("Wait for transfer complete...");
		acquire_sem(fSemaphore);
		TRACE("transfer complete OK.\n");

		length -= toCopy;
		vecOffset += toCopy;
		offset += toCopy;
	}

	return result;
}


void
SdhciBus::SetScanSemaphore(sem_id sem)
{
	fScanSemaphore = sem;

	// If there is already a card in, start a scan immediately
	if (fRegisters->present_state.IsCardInserted())
		release_sem(fScanSemaphore);

	// We can now enable the card insertion interrupt for next time a card
	// is inserted
	EnableInterrupts(SDHCI_INT_CARD_INS);
}


void
SdhciBus::SetBusWidth(int width)
{
	uint8_t widthBits;
	switch(width) {
		case 1:
			widthBits = HostControl::kDataTransfer1Bit;
			break;
		case 4:
			widthBits = HostControl::kDataTransfer4Bit;
			break;
		case 8:
			widthBits = HostControl::kDataTransfer8Bit;
			break;
		default:
			panic("Incorrect bitwidth value");
			return;
	}
	fRegisters->host_control.SetDataTransferWidth(widthBits);
}


bool
SdhciBus::PowerOn()
{
	if (!fRegisters->present_state.IsCardInserted()) {
		TRACE("Card not inserted, not powering on for now\n");
		return false;
	}

	uint8_t supportedVoltages = fRegisters->capabilities.SupportedVoltages();
	if ((supportedVoltages & Capabilities::k3v3) != 0)
		fRegisters->power_control.SetVoltage(PowerControl::k3v3);
	else if ((supportedVoltages & Capabilities::k3v0) != 0)
		fRegisters->power_control.SetVoltage(PowerControl::k3v0);
	else if ((supportedVoltages & Capabilities::k1v8) != 0)
		fRegisters->power_control.SetVoltage(PowerControl::k1v8);
	else {
		fRegisters->power_control.PowerOff();
		ERROR("No voltage is supported\n");
		return false;
	}

	return true;
}


void
SdhciBus::RecoverError()
{
	fRegisters->interrupt_signal_enable &= ~(SDHCI_INT_CMD_CMP
		| SDHCI_INT_TRANS_CMP | SDHCI_INT_CARD_INS | SDHCI_INT_CARD_REM);

	if (fRegisters->interrupt_status & 7)
		fRegisters->software_reset.ResetCommandLine();

	int16_t error_status = fRegisters->interrupt_status;
	fRegisters->interrupt_status &= ~(error_status);
}


int32
SdhciBus::HandleInterrupt()
{
#if 0
	// We could use the slot register to quickly see for which slot the
	// interrupt is. But since we have an interrupt handler call for each slot
	// anyway, it's just as simple to let each of them scan its own interrupt
	// status register.
	if ( !(fRegisters->slot_interrupt_status & (1 << fSlot)) ) {
		TRACE("interrupt not for me.\n");
		return B_UNHANDLED_INTERRUPT;
	}
#endif
	
	uint32_t intmask = fRegisters->interrupt_status;

	// Shortcut: exit early if there is no interrupt or if the register is
	// clearly invalid.
	if ((intmask == 0) || (intmask == 0xffffffff)) {
		return B_UNHANDLED_INTERRUPT;
	}

	TRACE("interrupt function called %x\n", intmask);

	// handling card presence interrupts
	if ((intmask & SDHCI_INT_CARD_REM) != 0) {
		// We can get spurious interrupts as the card is inserted or removed,
		// so check the actual state before acting
		if (!fRegisters->present_state.IsCardInserted())
			fRegisters->power_control.PowerOff();
		else
			TRACE("Card removed interrupt, but card is inserted\n");

		fRegisters->interrupt_status |= SDHCI_INT_CARD_REM;
		TRACE("Card removal interrupt handled\n");
	}

	if ((intmask & SDHCI_INT_CARD_INS) != 0) {
		// We can get spurious interrupts as the card is inserted or removed,
		// so check the actual state before acting
		if (fRegisters->present_state.IsCardInserted()) {
			if (PowerOn())
				SetClock(400);
			release_sem_etc(fScanSemaphore, 1, B_DO_NOT_RESCHEDULE);
		} else
			TRACE("Card insertion interrupt, but card is removed\n");

		fRegisters->interrupt_status |= SDHCI_INT_CARD_INS;
		TRACE("Card presence interrupt handled\n");
	}

	// handling command interrupt
	if (intmask & SDHCI_INT_CMD_MASK) {
		fCommandResult = intmask;
			// Save the status before clearing so the thread can handle it
		fRegisters->interrupt_status |= (intmask & SDHCI_INT_CMD_MASK);

		// Notify the thread
		release_sem_etc(fSemaphore, 1, B_DO_NOT_RESCHEDULE);
		TRACE("Command complete interrupt handled\n");
	}

	if (intmask & SDHCI_INT_TRANS_CMP) {
		fCommandResult = intmask;
		fRegisters->interrupt_status |= SDHCI_INT_TRANS_CMP;
		release_sem_etc(fSemaphore, 1, B_DO_NOT_RESCHEDULE);
		TRACE("Transfer complete interrupt handled\n");
	}

	// handling bus power interrupt
	if (intmask & SDHCI_INT_BUS_POWER) {
		fRegisters->interrupt_status |= SDHCI_INT_BUS_POWER;
		TRACE("card is consuming too much power\n");
	}

	// Check that all interrupts have been cleared (we check all the ones we
	// enabled, so that should always be the case)
	intmask = fRegisters->interrupt_status;
	if (intmask != 0) {
		ERROR("Remaining interrupts at end of handler: %x\n", intmask);
	}

	return B_HANDLED_INTERRUPT;
}
// #pragma mark -


static status_t
init_bus(device_node* node, void** bus_cookie)
{
	CALLED();

	// Get the PCI driver and device
	pci_device_module_info* pci;
	pci_device* device;

	device_node* parent = gDeviceManager->get_parent_node(node);
	device_node* pciParent = gDeviceManager->get_parent_node(parent);
	gDeviceManager->get_driver(pciParent, (driver_module_info**)&pci,
	        (void**)&device);
	gDeviceManager->put_node(pciParent);
	gDeviceManager->put_node(parent);

	if (get_module(B_PCI_X86_MODULE_NAME, (module_info**)&sPCIx86Module)
	    != B_OK) {
	    sPCIx86Module = NULL;
		ERROR("PCIx86Module not loaded\n");
		// FIXME try probing FDT as well
		return B_NO_MEMORY;
	}

	uint8_t bar, slot;
	if (gDeviceManager->get_attr_uint8(node, SLOT_NUMBER, &slot, false) < B_OK
		|| gDeviceManager->get_attr_uint8(node, BAR_INDEX, &bar, false) < B_OK)
		return B_BAD_TYPE;

	// Ignore invalid bars
	TRACE("Register SD bus at slot %d, using bar %d\n", slot + 1, bar);

	pci_info pciInfo;
	pci->get_pci_info(device, &pciInfo);

	if (pciInfo.u.h0.base_register_sizes[bar] == 0) {
		ERROR("No registers to map\n");
		return B_IO_ERROR;
	}

	int msiCount = sPCIx86Module->get_msi_count(pciInfo.bus,
		pciInfo.device, pciInfo.function);
	TRACE("interrupts count: %d\n",msiCount);
	// FIXME if available, use MSI rather than good old IRQ...

	// enable bus master and io
	uint16 pcicmd = pci->read_pci_config(device, PCI_command, 2);
	pcicmd &= ~(PCI_command_int_disable | PCI_command_io);
	pcicmd |= PCI_command_master | PCI_command_memory;
	pci->write_pci_config(device, PCI_command, 2, pcicmd);

	// map the slot registers
	area_id	regs_area;
	struct registers* _regs;
	regs_area = map_physical_memory("sdhc_regs_map",
		pciInfo.u.h0.base_registers[bar],
		pciInfo.u.h0.base_register_sizes[bar], B_ANY_KERNEL_BLOCK_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&_regs);

	if (regs_area < B_OK) {
		ERROR("Could not map registers\n");
		return B_BAD_VALUE;
	}

	// the interrupt is shared between all busses in an SDHC controller, but
	// they each register an handler. Not a problem, we will just test the
	// interrupt registers for all busses one after the other and find no
	// interrupts on the idle busses.
	uint8_t irq = pciInfo.u.h0.interrupt_line;
	TRACE("irq interrupt line: %d\n", irq);

	SdhciBus* bus = new(std::nothrow) SdhciBus(_regs, irq);

	status_t status = B_NO_MEMORY;
	if (bus != NULL)
		status = bus->InitCheck();

	if (status != B_OK) {
		if (sPCIx86Module != NULL) {
			put_module(B_PCI_X86_MODULE_NAME);
			sPCIx86Module = NULL;
		}

		if (bus != NULL)
			delete bus;
		else
			delete_area(regs_area);
		return status;
	}

	// Store the created object as a cookie, allowing users of the bus to
	// locate it.
	*bus_cookie = bus;

	return status;
}


static void
uninit_bus(void* bus_cookie)
{
	SdhciBus* bus = (SdhciBus*)bus_cookie;
	delete bus;

	// FIXME do we need to put() the PCI module here?
}


static void
bus_removed(void* bus_cookie)
{
	return;
}


static status_t
register_child_devices(void* cookie)
{
	CALLED();
	SdhciDevice* context = (SdhciDevice*)cookie;
	device_node* parent = gDeviceManager->get_parent_node(context->fNode);
	pci_device_module_info* pci;
	pci_device* device;
	uint8 slots_count, bar, slotsInfo;

	gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
		(void**)&device);
	slotsInfo = pci->read_pci_config(device, SDHCI_PCI_SLOT_INFO, 1);
	bar = SDHCI_PCI_SLOT_INFO_FIRST_BASE_INDEX(slotsInfo);
	slots_count = SDHCI_PCI_SLOTS(slotsInfo);

	char prettyName[25];

	if (slots_count > 6 || bar > 5) {
		ERROR("Invalid slots count: %d or BAR count: %d \n", slots_count, bar);
		return B_BAD_VALUE;
	}

	for (uint8_t slot = 0; slot <= slots_count; slot++) {
		sprintf(prettyName, "SDHC bus %" B_PRIu8, slot);
		device_attr attrs[] = {
			// properties of this controller for mmc bus manager
			{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: prettyName } },
			{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
				{string: MMC_BUS_MODULE_NAME} },
			{ B_DEVICE_BUS, B_STRING_TYPE, {string: "mmc"} },

			// DMA properties
			// The high alignment is to force access only to complete sectors
			// These constraints could be removed by using ADMA which allows
			// use of the full 64bit address space and can do scatter-gather.
			{ B_DMA_ALIGNMENT, B_UINT32_TYPE, { ui32: 511 }},
			{ B_DMA_HIGH_ADDRESS, B_UINT64_TYPE, { ui64: 0x100000000LL }},
			{ B_DMA_BOUNDARY, B_UINT32_TYPE, { ui32: (1 << 19) - 1 }},
			{ B_DMA_MAX_SEGMENT_COUNT, B_UINT32_TYPE, { ui32: 1 }},
			{ B_DMA_MAX_SEGMENT_BLOCKS, B_UINT32_TYPE, { ui32: (1 << 10) - 1 }},

			// private data to identify device
			{ SLOT_NUMBER, B_UINT8_TYPE, { ui8: slot} },
			{ BAR_INDEX, B_UINT8_TYPE, { ui8: bar + slot} },
			{ NULL }
		};
		device_node* node;
		if (gDeviceManager->register_node(context->fNode,
				SDHCI_PCI_MMC_BUS_MODULE_NAME, attrs, NULL,
				&node) != B_OK)
			return B_BAD_VALUE;
	}
	return B_OK;
}


static status_t
init_device(device_node* node, void** device_cookie)
{
	CALLED();

	// Get the PCI driver and device
	pci_device_module_info* pci;
	pci_device* device;
	uint16 vendorId, deviceId;

	device_node* pciParent = gDeviceManager->get_parent_node(node);
	gDeviceManager->get_driver(pciParent, (driver_module_info**)&pci,
	        (void**)&device);
	gDeviceManager->put_node(pciParent);

	SdhciDevice* context = new(std::nothrow)SdhciDevice;
	if (context == NULL)
		return B_NO_MEMORY;
	context->fNode = node;
	*device_cookie = context;

	if (gDeviceManager->get_attr_uint16(node, B_DEVICE_VENDOR_ID,
			&vendorId, true) != B_OK
		|| gDeviceManager->get_attr_uint16(node, B_DEVICE_ID, &deviceId,
			true) != B_OK) {
		panic("No vendor or device id attribute\n");
		return B_OK; // Let's hope it didn't need the quirk?
	}

	if (vendorId == 0x1180 && deviceId == 0xe823) {
		// Switch the device to SD2.0 mode
		// This should change its device id to 0xe822 if succesful.
		// The device then remains in SD2.0 mode even after reboot.
		pci->write_pci_config(device, SDHCI_PCI_RICOH_MODE_KEY, 1, 0xfc);
		context->fRicohOriginalMode = pci->read_pci_config(device,
			SDHCI_PCI_RICOH_MODE, 1);
		pci->write_pci_config(device, SDHCI_PCI_RICOH_MODE, 1,
			SDHCI_PCI_RICOH_MODE_SD20);
		pci->write_pci_config(device, SDHCI_PCI_RICOH_MODE_KEY, 1, 0);

		deviceId = pci->read_pci_config(device, 2, 2);
		TRACE("Device ID after Ricoh quirk: %x\n", deviceId);
	}

	return B_OK;
}


static void
uninit_device(void* device_cookie)
{
	// Get the PCI driver and device
	pci_device_module_info* pci;
	pci_device* device;
	uint16 vendorId, deviceId;

	SdhciDevice* context = (SdhciDevice*)device_cookie;
	device_node* pciParent = gDeviceManager->get_parent_node(context->fNode);
	gDeviceManager->get_driver(pciParent, (driver_module_info**)&pci,
	        (void**)&device);

	if (gDeviceManager->get_attr_uint16(context->fNode, B_DEVICE_VENDOR_ID,
			&vendorId, true) != B_OK
		|| gDeviceManager->get_attr_uint16(context->fNode, B_DEVICE_ID,
			&deviceId, false) != B_OK) {
		ERROR("No vendor or device id attribute\n");
	} else if (vendorId == 0x1180 && deviceId == 0xe823) {
		pci->write_pci_config(device, SDHCI_PCI_RICOH_MODE_KEY, 1, 0xfc);
		pci->write_pci_config(device, SDHCI_PCI_RICOH_MODE, 1,
			context->fRicohOriginalMode);
		pci->write_pci_config(device, SDHCI_PCI_RICOH_MODE_KEY, 1, 0);
	}

	gDeviceManager->put_node(pciParent);

	delete context;
}


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "SD Host Controller"}},
		{}
	};

	return gDeviceManager->register_node(parent, SDHCI_PCI_DEVICE_MODULE_NAME,
		attrs, NULL, NULL);
}


static float
supports_device(device_node* parent)
{
	const char* bus;
	uint16 type, subType;
	uint16 vendorId, deviceId;

	// make sure parent is a PCI SDHCI device node
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
		!= B_OK) {
		TRACE("Could not find required attribute device/bus\n");
		return -1;
	}

	if (strcmp(bus, "pci") != 0)
		return 0.0f;

	if (gDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID, &vendorId,
			false) != B_OK
		|| gDeviceManager->get_attr_uint16(parent, B_DEVICE_ID, &deviceId,
			false) != B_OK) {
		TRACE("No vendor or device id attribute\n");
		return 0.0f;
	}

	TRACE("supports_device(vid:%04x pid:%04x)\n", vendorId, deviceId);

	if (gDeviceManager->get_attr_uint16(parent, B_DEVICE_SUB_TYPE, &subType,
			false) < B_OK
		|| gDeviceManager->get_attr_uint16(parent, B_DEVICE_TYPE, &type,
			false) < B_OK) {
		TRACE("Could not find type/subtype attributes\n");
		return -1;
	}

	if (type == PCI_base_peripheral) {
		if (subType != PCI_sd_host) {
			// Also accept some compliant devices that do not advertise
			// themselves as such.
			if (vendorId != 0x1180
				|| (deviceId != 0xe823 && deviceId != 0xe822)) {
				TRACE("Not the right subclass, and not a Ricoh device\n");
				return 0.0f;
			}
		}

		pci_device_module_info* pci;
		pci_device* device;
		gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
			(void**)&device);
		TRACE("SDHCI Device found! Subtype: 0x%04x, type: 0x%04x\n",
			subType, type);

		return 0.8f;
	}

	return 0.0f;
}


static status_t
set_clock(void* controller, uint32_t kilohertz)
{
	SdhciBus* bus = (SdhciBus*)controller;
	bus->SetClock(kilohertz);
	return B_OK;
}


static status_t
execute_command(void* controller, uint8_t command, uint32_t argument,
	uint32_t* response)
{
	SdhciBus* bus = (SdhciBus*)controller;
	return bus->ExecuteCommand(command, argument, response);
}


static status_t
do_io(void* controller, uint8_t command, IOOperation* operation,
	bool offsetAsSectors)
{
	CALLED();

	SdhciBus* bus = (SdhciBus*)controller;
	return bus->DoIO(command, operation, offsetAsSectors);
}


static void
set_scan_semaphore(void* controller, sem_id sem)
{
	CALLED();

	SdhciBus* bus = (SdhciBus*)controller;
	return bus->SetScanSemaphore(sem);
}


static void
set_bus_width(void* controller, int width)
{
	CALLED();

	SdhciBus* bus = (SdhciBus*)controller;
	return bus->SetBusWidth(width);
}


module_dependency module_dependencies[] = {
	{ MMC_BUS_MODULE_NAME, (module_info**)&gMMCBusController},
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{}
};


// Device node registered for each SD slot. It implements the MMC operations so
// the bus manager can use it to communicate with SD cards.
static mmc_bus_interface gSDHCIPCIDeviceModule = {
	{
		{
			SDHCI_PCI_MMC_BUS_MODULE_NAME,
			0,
			NULL
		},
		NULL,	// supports device
		NULL,	// register device
		init_bus,
		uninit_bus,
		NULL,	// register child devices
		NULL,	// rescan
		bus_removed,
	},

	set_clock,
	execute_command,
	do_io,
	set_scan_semaphore,
	set_bus_width
};


// Root device that binds to the PCI bus. It will register an mmc_bus_interface
// node for each SD slot in the device.
static driver_module_info sSDHCIDevice = {
	{
		SDHCI_PCI_DEVICE_MODULE_NAME,
		0,
		NULL
	},
	supports_device,
	register_device,
	init_device,
	uninit_device,
	register_child_devices,
	NULL,	// rescan
	NULL,	// device removed
};


module_info* modules[] = {
	(module_info* )&sSDHCIDevice,
	(module_info* )&gSDHCIPCIDeviceModule,
	NULL
};
