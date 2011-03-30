/*
	Driver for Intel(R) PRO/Wireless 2100 devices.
	Copyright (C) 2006 Michael Lotz <mmlr@mlotz.ch>
	Released under the terms of the MIT license.
*/

#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <driver_settings.h>
#include <sys/stat.h>

#include "kernel_cpp.h"
#include "ipw2100.h"
#include "ipw2100_hw.h"
#include "driver.h"
#include "ethernet.h"


IPW2100::IPW2100(int32 deviceID, pci_info *info, pci_module_info *module)
	:	fStatus(B_NO_INIT),
		fDeviceID(deviceID),
		fPCIInfo(info),
		fPCIModule(module),
		fRegisterBase(0),
		fRegisters(NULL),
		fRegistersArea(-1),
		fTXRingArea(-1),
		fTXRingLog(NULL),
		fTXRingPhy(0),
		fTXPacketsArea(-1),
		fTXPacketsLog(NULL),
		fTXPacketsPhy(0),
		fRXRingArea(-1),
		fRXRingLog(NULL),
		fRXRingPhy(0),
		fRXPacketsArea(-1),
		fRXPacketsLog(NULL),
		fRXPacketsPhy(0),
		fStatusRingArea(-1),
		fStatusRingLog(NULL),
		fStatusRingPhy(0),
		fCommandArea(-1),
		fCommandLog(NULL),
		fCommandPhy(0),
		fInterruptMask(0),
		fTXPosition(0),
		fRXPosition(IPW_RX_BUFFER_COUNT - 1),
		fAssociated(false),
		fOrdinalTable1Base(0),
		fOrdinalTable2Base(0),
		fOrdinalTable1Length(0),
		fOrdinalTable2Length(0),

		// settings
		fInterruptLine(-1),
		fMode(IPW_MODE_BSS),
		fChannel(1),
		fTXRates(IPW_TX_RATE_ALL),
		fPowerMode(IPW_POWER_MODE_CAM),
		fRTSThreshold(IPW_RTS_THRESHOLD_DEFAULT),
		fScanOptions(IPW_SCAN_MIXED_CELL),
		fAuthMode(IPW_AUTH_MODE_OPEN),
		fCiphers(IPW_CIPHER_NONE),
		fESSID(NULL),
		fWEPKeyIndex(0),
		fWEPFlags(0),
		fDumpTXPackets(false),
		fDumpRXPackets(false)
{
	const char *parameter;
	void *settings = load_driver_settings("ipw2100");

	parameter = get_driver_parameter(settings, "interrupt", NULL, NULL);
	if (parameter)
		fInterruptLine = atoi(parameter);

	parameter = get_driver_parameter(settings, "mode", NULL, NULL);
	if (parameter) {
		int mode = atoi(parameter);
		switch (mode) {
			case 0: fMode = IPW_MODE_IBSS; break;
			case 1: fMode = IPW_MODE_BSS; break;
			case 2: fMode = IPW_MODE_MONITOR; break;
		}
	}

	parameter = get_driver_parameter(settings, "channel", NULL, NULL);
	if (parameter) {
		int channel = atoi(parameter);
		if (channel >= 1 && channel <= 14)
			fChannel = channel;
	}

	parameter = get_driver_parameter(settings, "essid", NULL, NULL);
	if (parameter)
		fESSID = strdup(parameter);

	uint32 keyLength = 0;
	parameter = get_driver_parameter(settings, "privacy", NULL, NULL);
	if (parameter) {
		int privacy = atoi(parameter);
		switch (privacy) {
			case 0: {
				fCiphers = IPW_CIPHER_NONE;
				keyLength = 0;
				break;
			}
			case 1: {
				fCiphers = IPW_CIPHER_WEP40;
				fWEPFlags = IPW_WEP_FLAGS_HW_ENCRYPT | IPW_WEP_FLAGS_HW_DECRYPT;
				keyLength = 5;
				break;
			}
			case 2: {
				fCiphers = IPW_CIPHER_WEP104;
				fWEPFlags = IPW_WEP_FLAGS_HW_ENCRYPT | IPW_WEP_FLAGS_HW_DECRYPT;
				keyLength = 13;
				break;
			}
		}
	}

	parameter = get_driver_parameter(settings, "authmode", NULL, NULL);
	if (parameter) {
		int auth = atoi(parameter);
		switch (auth) {
			case 0: fAuthMode = IPW_AUTH_MODE_OPEN; break;
			case 1: fAuthMode = IPW_AUTH_MODE_SHARED; break;
		}
	}

	parameter = get_driver_parameter(settings, "keyindex", NULL, NULL);
	if (parameter) {
		int index = atoi(parameter);
		if (index >= 0 && index <= 3)
			fWEPKeyIndex = index;
	}

	for (uint8 i = 0; i < 4; i++) {
		fWEPKeys[i].index = i;
		fWEPKeys[i].length = keyLength;

		char keyName[10];
		sprintf(keyName, "key%d", i);
		parameter = get_driver_parameter(settings, keyName, NULL, NULL);
		if (parameter) {
			int32 keyFormat = -1;
			switch (strlen(parameter)) {
				case 5:
					if (keyLength == 5)
						keyFormat = 0;
					break;
				case 13:
					if (keyLength == 13)
						keyFormat = 0;
					break;
				case 10:
					if (keyLength == 5)
						keyFormat = 1;
					break;
				case 26:
					if (keyLength == 13)
						keyFormat = 1;
					break;
			}

			if (keyFormat == 0)
				strncpy((char *)fWEPKeys[i].key, parameter, keyLength);
			else if (keyFormat == 1) {
				uint8 buffer[30];
				for (uint8 j = 0; j < keyLength * 2; j++) {
					if (parameter[j] >= '0' && parameter[j] <= '9')
						buffer[j] = parameter[j] - '0';
					else if (parameter[j] >= 'a' && parameter[j] <= 'f')
						buffer[j] = parameter[j] - 'a' + 10;
					else if (parameter[j] >= 'A' && parameter[j] <= 'F')
						buffer[j] = parameter[j] - 'A' + 10;
					else
						break;
				}

				for (uint8 j = 0; j < keyLength; j++)
					fWEPKeys[i].key[j] = (buffer[j * 2] << 4) + buffer[j * 2 + 1];
			} else {
				TRACE_ALWAYS(("IPW2100: the wep key specified in %s is invalid, it will not be used\n", keyName));
			}
		}
	}

	parameter = get_driver_parameter(settings, "dumptx", NULL, NULL);
	if (parameter) {
		int value = atoi(parameter);
		if (value == 1)
			fDumpTXPackets = true;
	}

	parameter = get_driver_parameter(settings, "dumprx", NULL, NULL);
	if (parameter) {
		int value = atoi(parameter);
		if (value == 1)
			fDumpRXPackets = true;
	}

	unload_driver_settings(settings);

	if (fDumpRXPackets || fDumpTXPackets)
		mkdir("/var/tmp/ipwlog", 0x0755);

	fRegisterBase = fPCIInfo->u.h0.base_registers[0];
	TRACE(("IPW2100: register base: 0x%08lx\n", fRegisterBase));

	// enable addressing
	uint16 command = fPCIModule->read_pci_config(fPCIInfo->bus,
		fPCIInfo->device, fPCIInfo->function, PCI_command, 2);
	command |= PCI_command_io | PCI_command_master | PCI_command_memory;
	fPCIModule->write_pci_config(fPCIInfo->bus, fPCIInfo->device,
		fPCIInfo->function, PCI_command, 2, command);

	// map physical register memory
	fRegistersArea = MapPhysicalMemory("ipw2100 registers",
		fRegisterBase, (void **)&fRegisters, 128 * 1024);
	if (!fRegisters) {
		TRACE_ALWAYS(("IPW2100: couldn't map registers\n"));
		return;
	}

	// disable interrupts
	SetInterrupts(0);

	// synchronisation semaphores
	fFWInitDoneSem = create_sem(0, "IPW2100 init done sem");
	fAdapterDisabledSem = create_sem(0, "IPW2100 adapter disabled sem");
	fTXTransferSem = create_sem(0, "IPW2100 tx transfer sem");
	fRXTransferSem = create_sem(0, "IPW2100 rx transfer sem");
	fCommandDoneSem = create_sem(0, "IPW2100 command done sem");

	if (fInterruptLine < 0)
		fInterruptLine = fPCIInfo->u.h0.interrupt_line;

	// install the interrupt handler
	TRACE_ALWAYS(("IPW2100: installing interrupt handler on interrupt line %ld\n", fInterruptLine));
	install_io_interrupt_handler(fInterruptLine, InterruptHandler, (void *)this, 0);

	TRACE(("IPW2100: device class created (device: %ld)\n", fDeviceID));
	fStatus = B_OK;
}


IPW2100::~IPW2100()
{
	fStatus = B_NO_INIT;

	remove_io_interrupt_handler(fInterruptLine, InterruptHandler, (void *)this);

	fRegisters = NULL;
	fTXRingLog = NULL;
	fRXPacketsLog = NULL;
	fRXRingLog = NULL;
	fRXPacketsLog = NULL;
	fStatusRingLog = NULL;
	fCommandLog = NULL;

	delete_area(fRegistersArea);
	delete_area(fTXRingArea);
	delete_area(fTXPacketsArea);
	delete_area(fRXRingArea);
	delete_area(fRXPacketsArea);
	delete_area(fStatusRingArea);
	delete_area(fCommandArea);

	delete_sem(fFWInitDoneSem);
	delete_sem(fAdapterDisabledSem);
	delete_sem(fTXTransferSem);
	delete_sem(fRXTransferSem);
	delete_sem(fCommandDoneSem);

	free(fESSID);
}


status_t
IPW2100::InitCheck()
{
	return fStatus;
}


status_t
IPW2100::Open(uint32 flags)
{
	TRACE(("IPW2100: open\n"));
	fClosing = false;

	status_t status = CheckAdapterAccess();
	if (status < B_OK) {
		TRACE_ALWAYS(("IPW2100: adapter not accessible via mapped registers\n"));
		return status;
	}

	status = SoftResetAdapter();
	if (status < B_OK) {
		TRACE_ALWAYS(("IPW2100: couldn't softreset the adapter\n"));
		return status;
	}

	status = LoadMicrocodeAndFirmware();
	if (status < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to load microcode and firmware\n"));
		return status;
	}

	status = ClearSharedMemory();
	if (status < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to clear shared memory\n"));
		return status;
	}

	status = SetupBuffers();
	if (status < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to setup buffers\n"));
		return status;
	}

	status = StartFirmware();
	if (status < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to start firmware\n"));
		return status;
	}

	status = ReadMACAddress();
	if (status < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to get MAC address\n"));
		return status;
	}

	// enable all interrupts
	SetInterrupts(IPW_INTERRUPT_TX_TRANSFER | IPW_INTERRUPT_RX_TRANSFER
		| IPW_INTERRUPT_STATUS_CHANGE | IPW_INTERRUPT_COMMAND_DONE
		| IPW_INTERRUPT_FW_INIT_DONE | IPW_INTERRUPT_FATAL_ERROR
		| IPW_INTERRUPT_PARITY_ERROR);

	TRACE(("IPW2100: open was successful\n"));
	return B_OK;
}


status_t
IPW2100::Close()
{
	TRACE(("IPW2100: close\n"));
	fClosing = true;

	DisableRadio();
	PowerDown();

	SetInterrupts(0);
	release_sem(fRXTransferSem);
	release_sem(fTXTransferSem);
	return B_OK;
}


status_t
IPW2100::Free()
{
	TRACE(("IPW2100: free\n"));
	return B_OK;
}


status_t
IPW2100::Read(off_t position, void *buffer, size_t *numBytes)
{
	TRACE(("IPW2100: read at most %ld bytes\n", *numBytes));
	status_t result = acquire_sem(fRXTransferSem);
	if (result < B_OK || fClosing) {
		TRACE_ALWAYS(("IPW2100: read failed: %s\n", fClosing ? "closing" : "error"));
		*numBytes = 0;
		return (fClosing ? B_INTERRUPTED : result);
	}

	uint32 index = fReadIndex;
	for (int32 i = 0; i < IPW_RX_BUFFER_COUNT; i++) {
		if (fReadQueue[index]) {
			ipw_rx *packet = &fRXPacketsLog[index];
			ipw_status *status = &fStatusRingLog[index];

			if (fDumpRXPackets) {
				char outName[100];
				static uint32 packetIndex = 0;
				sprintf(outName, "/var/tmp/ipwlog/ipwrx-%06ld", packetIndex++);
				int fd = open(outName, O_CREAT | O_WRONLY | O_TRUNC);
				write(fd, packet->data, status->length);
				close(fd);
			}

			ieee80211_header *frame = (ieee80211_header *)packet->data;
			if (fMode == IPW_MODE_MONITOR
				|| (frame->frame_control[0] & IEEE80211_DATA_FRAME) == 0) {
				// just wanted to log this, but it's not interesting
				fReadQueue[index] = false;
				fReadIndex = index;
				return B_INTERRUPTED;
			}

			size_t payloadLength = status->length - sizeof(ieee80211_header);
			ethernet_header *ether = (ethernet_header *)buffer;
			memcpy(ether->dest_address, frame->address1, ETHER_ADDRESS_SIZE);
			memcpy(ether->source_address, frame->address3, ETHER_ADDRESS_SIZE);
			llc_header *llc = (llc_header *)frame->data;
			ether->ether_type = llc->ether_type;

			payloadLength -= sizeof(llc_header);
			memcpy(ether->data, frame->data + sizeof(llc_header), payloadLength);

			*numBytes = sizeof(ethernet_header) + payloadLength;
			fReadQueue[index] = false;
			fReadIndex = index;
			return B_OK;
		}

		index = (index + 1) % IPW_RX_BUFFER_COUNT;
	}

	return B_INTERRUPTED;
}


status_t
IPW2100::Write(off_t position, const void *buffer, size_t *numBytes)
{
	TRACE(("IPW2100: write %ld bytes\n", *numBytes));
	if (*numBytes > sizeof(ipw_tx)) {
		TRACE_ALWAYS(("IPW2100: packet too big, discarding\n"));
		*numBytes = 0;
		return B_ERROR;
	}

	if (fMode == IPW_MODE_MONITOR) {
		*numBytes = 0;
		return B_ERROR;
	}

	// wait for association
	uint32 tries = 1000;
	while (!fAssociated) {
		snooze(5000);
		if (--tries == 0) {
			TRACE_ALWAYS(("IPW2100: writing failed due to not beeing associated\n"));
			return B_INTERRUPTED;
		}
	}

	ipw_bd *descriptor = &fTXRingLog[fTXPosition];
	descriptor->physical_address = fTXPacketsPhy + fTXPosition * sizeof(ipw_tx);
	descriptor->length = sizeof(ipw_data);
	descriptor->flags = IPW_BD_FLAG_TYPE_802_3 | IPW_BD_FLAG_NOT_LAST_FRAGMENT;
	descriptor->fragment_count = 2;

	ipw_data *data = (ipw_data *)&(fTXPacketsLog[fTXPosition].data);
	memset(data, 0, sizeof(ipw_data));
	data->command = IPW_COMMAND_SEND_DATA;
	if (fCiphers != IPW_CIPHER_NONE) {
		data->needs_encryption = 1;
		data->key_index = fWEPKeyIndex + 1;
	}

	ethernet_header *ether = (ethernet_header *)buffer;
	memcpy(data->dest_address, ether->dest_address, ETHER_ADDRESS_SIZE);
	memcpy(data->source_address, ether->source_address, ETHER_ADDRESS_SIZE);

	fTXPosition = (fTXPosition + 1) % IPW_TX_BUFFER_COUNT;
	descriptor = &fTXRingLog[fTXPosition];
	descriptor->physical_address = fTXPacketsPhy + fTXPosition * sizeof(ipw_tx);
	descriptor->length = *numBytes - sizeof(ethernet_header) + sizeof(llc_header);
	descriptor->flags = IPW_BD_FLAG_TYPE_802_3 | IPW_BD_FLAG_INTERRUPT;
	descriptor->fragment_count = 0;

	llc_header llc;
	llc.dsap = llc.ssap = LLC_LSAP_SNAP;
	llc.control = LLC_CONTROL_UI;
	llc.org_code[0] = llc.org_code[1] = llc.org_code[2] = 0;
	llc.ether_type = ether->ether_type;

	memcpy(fTXPacketsLog[fTXPosition].data, &llc, sizeof(llc_header));
	memcpy(fTXPacketsLog[fTXPosition].data + sizeof(llc_header), ether->data, *numBytes - sizeof(ethernet_header));

	if (fDumpTXPackets) {
		char outName[100];
		static uint32 packetIndex = 0;
		sprintf(outName, "/var/tmp/ipwlog/ipwtx-%06ld", packetIndex++);
		int fd = open(outName, O_CREAT | O_WRONLY | O_TRUNC);
		write(fd, fTXPacketsLog[fTXPosition].data, descriptor->length);
		close(fd);
	}

	// tell the firmware that there is data to process
	fTXPosition = (fTXPosition + 1) % IPW_TX_BUFFER_COUNT;
	WriteReg(IPW_REG_TX_WRITE, fTXPosition);

	status_t result = acquire_sem_etc(fTXTransferSem, 1, B_RELATIVE_TIMEOUT, 200000);
	if (result < B_OK || fClosing) {
		TRACE_ALWAYS(("IPW2100: error waiting for tx transfer completion: %s\n", fClosing ? "closing" : "timeout"));
		*numBytes = 0;
		return (fClosing ? B_INTERRUPTED : result);
	}

	return B_OK;
}


status_t
IPW2100::Control(uint32 op, void *buffer, size_t length)
{
	if (fStatus < B_OK)
		return fStatus;

	switch (op) {
		case ETHER_INIT: {
			TRACE(("IPW2100: ETHER_INIT\n"));
			return EtherInit();
		}

		case ETHER_GETADDR: {
			TRACE(("IPW2100: ETHER_GETADDR\n"));
			memcpy(buffer, &fMACAddress, sizeof(fMACAddress));
			return B_OK;
		}

		case ETHER_GETFRAMESIZE: {
			TRACE(("IPW2100: ETHER_GETFRAMESIZE\n"));
			*(uint32 *)buffer = 1500;
			return B_OK;
		}

		default:
			TRACE_ALWAYS(("IPW2100: unsupported ioctl 0x%08lx\n", op));
	}

	return B_DEV_INVALID_IOCTL;
}


//#pragma mark -


int32
IPW2100::InterruptHandler(void *data)
{
	return ((IPW2100 *)data)->Interrupt();
}


int32
IPW2100::Interrupt()
{
	spinlock lock = 0;
	acquire_spinlock(&lock);

	uint32 status = ReadReg(IPW_REG_INTERRUPT);
	status &= fInterruptMask;
	if (status == 0) {
		release_spinlock(&lock);
		return B_UNHANDLED_INTERRUPT;
	}

	uint32 acknowledge = 0;
	int32 result = B_HANDLED_INTERRUPT;

	if (status & IPW_INTERRUPT_TX_TRANSFER) {
		acknowledge |= IPW_INTERRUPT_TX_TRANSFER;
		result = B_INVOKE_SCHEDULER;
	}

	if (status & IPW_INTERRUPT_RX_TRANSFER) {
		acknowledge |= IPW_INTERRUPT_RX_TRANSFER;
		result = B_INVOKE_SCHEDULER;
	}

	if (status & IPW_INTERRUPT_FW_INIT_DONE) {
		acknowledge |= IPW_INTERRUPT_FW_INIT_DONE;
		result = B_INVOKE_SCHEDULER;
	}

	if (status & IPW_INTERRUPT_FATAL_ERROR) {
		TRACE_ALWAYS(("IPW2100: interrupt fatal error\n"));
		acknowledge |= IPW_INTERRUPT_FATAL_ERROR;
		// ToDo: do something...
	}

	if (status & IPW_INTERRUPT_PARITY_ERROR) {
		TRACE_ALWAYS(("IPW2100: interrupt parity error\n"));
		acknowledge |= IPW_INTERRUPT_PARITY_ERROR;
		// ToDo: do something...
	}

	if (acknowledge)
		WriteReg(IPW_REG_INTERRUPT, acknowledge);

	release_spinlock(&lock);

	if (status & IPW_INTERRUPT_TX_TRANSFER)
		HandleTXInterrupt();
	if (status & IPW_INTERRUPT_RX_TRANSFER)
		HandleRXInterrupt();
	if (status & IPW_INTERRUPT_FW_INIT_DONE)
		release_sem_etc(fFWInitDoneSem, 1, B_DO_NOT_RESCHEDULE);

	return result;
}


void
IPW2100::HandleTXInterrupt()
{
	release_sem_etc(fTXTransferSem, 1, B_DO_NOT_RESCHEDULE);
}


void
IPW2100::HandleRXInterrupt()
{
	uint32 readIndex = ReadReg(IPW_REG_RX_READ);
	uint32 writeIndex = (fRXPosition + 1) % IPW_RX_BUFFER_COUNT;

	while (writeIndex != readIndex) {
		ipw_status *status = &fStatusRingLog[writeIndex];
		ipw_rx *packet = &fRXPacketsLog[writeIndex];

		switch (status->code & IPW_STATUS_CODE_MASK) {
			case IPW_STATUS_CODE_COMMAND: {
#ifdef TRACE_IPW2100
				ipw_command *command = (ipw_command *)packet->data;
				TRACE(("IPW2100: command done received for command %ld\n", command->command));
#endif
				release_sem_etc(fCommandDoneSem, 1, B_DO_NOT_RESCHEDULE);
				break;
			}

			case IPW_STATUS_CODE_STATUS: {
				TRACE(("IPW2100: status change received\n"));
				uint32 statusCode = *(uint32 *)packet->data;
				if (statusCode & IPW_STATE_INITIALIZED)
					TRACE(("         initialized\n"));
				if (statusCode & IPW_STATE_COUNTRY_FOUND)
					TRACE(("         country found\n"));
				if (statusCode & IPW_STATE_ASSOCIATED) {
					TRACE(("         associated\n"));
					TRACE_ALWAYS(("IPW2100: associated\n"));
					fAssociated = true;
				}
				if (statusCode & IPW_STATE_ASSOCIATION_LOST) {
					TRACE(("         association lost\n"));
					TRACE_ALWAYS(("IPW2100: association lost\n"));
					fAssociated = false;
				}
				if (statusCode & IPW_STATE_ASSOCIATION_CHANGED)
					TRACE(("         association changed\n"));
				if (statusCode & IPW_STATE_SCAN_COMPLETE)
					TRACE(("         scan complete\n"));
				if (statusCode & IPW_STATE_PSP_ENTERED)
					TRACE(("         psp entered\n"));
				if (statusCode & IPW_STATE_PSP_LEFT)
					TRACE(("         psp left\n"));
				if (statusCode & IPW_STATE_RF_KILL) {
					TRACE(("         rf kill\n"));
					TRACE_ALWAYS(("IPW2100: rf kill switch enabled\n"));
				}
				if (statusCode & IPW_STATE_DISABLED) {
					TRACE(("         disabled\n"));
					release_sem_etc(fAdapterDisabledSem, 1, B_DO_NOT_RESCHEDULE);
				}
				if (statusCode & IPW_STATE_POWER_DOWN)
					TRACE(("         power down\n"));
				if (statusCode & IPW_STATE_SCANNING)
					TRACE(("         scanning\n"));
				break;
			}

			case IPW_STATUS_CODE_DATA_802_11:
			case IPW_STATUS_CODE_DATA_802_3: {
				TRACE(("IPW2100: %s data received at index %ld with %ld bytes\n", (status->code & IPW_STATUS_CODE_MASK) == IPW_STATUS_CODE_DATA_802_11 ? "802.11" : "802.3", writeIndex, status->length));
				fReadQueue[writeIndex] = true;
				release_sem_etc(fRXTransferSem, 1, B_DO_NOT_RESCHEDULE);
				break;
			}

			case IPW_STATUS_CODE_NOTIFICATION: {
				TRACE(("IPW2100: notification received\n"));
				break;
			}

			default: {
				TRACE_ALWAYS(("IPW2100: unknown status code received 0x%04x\n", status->code));
				break;
			}
		}

		writeIndex = (writeIndex + 1) % IPW_RX_BUFFER_COUNT;
		fRXPosition = (writeIndex > 0 ? writeIndex - 1 : IPW_RX_BUFFER_COUNT - 1);
		WriteReg(IPW_REG_RX_WRITE, fRXPosition);
		TRACE(("IPW2100: new rx write index %ld\n", fRXPosition));
	}
}


inline uint32
IPW2100::ReadReg(uint32 offset)
{
	return *(uint32 *)(fRegisters + offset);
}


inline void
IPW2100::WriteReg(uint32 offset, uint32 value)
{
	*(uint32 *)(fRegisters + offset) = value;
}


inline uint8
IPW2100::ReadMem8(uint32 address)
{
	WriteReg(IPW_REG_INDIRECT_ADDRESS, address & IPW_INDIRECT_ADDRESS_MASK);
	return *(uint8 *)(fRegisters + IPW_REG_INDIRECT_DATA + (address & ~IPW_INDIRECT_ADDRESS_MASK));
}


inline uint16
IPW2100::ReadMem16(uint32 address)
{
	WriteReg(IPW_REG_INDIRECT_ADDRESS, address & IPW_INDIRECT_ADDRESS_MASK);
	return *(uint16 *)(fRegisters + IPW_REG_INDIRECT_DATA + (address & ~IPW_INDIRECT_ADDRESS_MASK));
}


inline uint32
IPW2100::ReadMem32(uint32 address)
{
	WriteReg(IPW_REG_INDIRECT_ADDRESS, address & IPW_INDIRECT_ADDRESS_MASK);
	return *(uint32 *)(fRegisters + IPW_REG_INDIRECT_DATA);
}


inline void
IPW2100::WriteMem8(uint32 address, uint8 value)
{
	WriteReg(IPW_REG_INDIRECT_ADDRESS, address & IPW_INDIRECT_ADDRESS_MASK);
	*(uint8 *)(fRegisters + IPW_REG_INDIRECT_DATA + (address & ~IPW_INDIRECT_ADDRESS_MASK)) = value;
}


inline void
IPW2100::WriteMem16(uint32 address, uint16 value)
{
	WriteReg(IPW_REG_INDIRECT_ADDRESS, address & IPW_INDIRECT_ADDRESS_MASK);
	*(uint16 *)(fRegisters + IPW_REG_INDIRECT_DATA + (address & ~IPW_INDIRECT_ADDRESS_MASK)) = value;
}


inline void
IPW2100::WriteMem32(uint32 address, uint32 value)
{
	WriteReg(IPW_REG_INDIRECT_ADDRESS, address & IPW_INDIRECT_ADDRESS_MASK);
	*(uint32 *)(fRegisters + IPW_REG_INDIRECT_DATA) = value;
}


status_t
IPW2100::ReadOrdinal(uint32 index, uint8 *buffer, size_t *size)
{
	if (index >= IPW_ORDINAL_TABLE_1_START
		&& index < IPW_ORDINAL_TABLE_1_START + fOrdinalTable1Length) {
		TRACE(("IPW2100: reading ordinal from table 1 at index %ld\n", index));
		if (*size < sizeof(uint32)) {
			TRACE_ALWAYS(("IPW2100: not enough space to read ordinal\n"));
			return B_BAD_VALUE;
		}

		uint32 address = ReadMem32(fOrdinalTable1Base + index * 4);
		*(uint32 *)buffer = ReadMem32(address);
		*size = sizeof(uint32);
		return B_OK;
	}

	if (index >= IPW_ORDINAL_TABLE_2_START
		&& index < IPW_ORDINAL_TABLE_2_START + fOrdinalTable2Length) {
		TRACE(("IPW2100: reading ordinal from table 2 at index %ld\n", index));

		index -= IPW_ORDINAL_TABLE_2_START;
		uint32 address = ReadMem32(fOrdinalTable2Base + index * 8);
		uint16 fieldLength = ReadMem16(fOrdinalTable2Base + index * 8 + sizeof(address));
		uint16 fieldCount = ReadMem16(fOrdinalTable2Base + index * 8 + sizeof(address) + sizeof(fieldLength));
		TRACE(("IPW2100: reading field at 0x%08lx with length %d and count %d\n", address, fieldLength, fieldCount));
		uint32 length = fieldLength * fieldCount;

		if (*size < length) {
			TRACE_ALWAYS(("IPW2100: not enough memory to read ordinal (length is %ld)\n", length));
			return B_BAD_VALUE;
		}

		for (uint32 i = 0; i < length; i++)
			buffer[i] = ReadMem8(address + i);

		*size = length;
		return B_OK;
	}

	TRACE_ALWAYS(("IPW2100: invalid ordinal index (%ld)\n", index));
	return B_BAD_INDEX;
}


status_t
IPW2100::WriteOrdinal(uint32 index, const uint8 *buffer, size_t *size)
{
	if (index >= IPW_ORDINAL_TABLE_1_START
		&& index < IPW_ORDINAL_TABLE_1_START + fOrdinalTable1Length) {
		TRACE(("IPW2100: writing ordinal in table 1 at index %ld\n", index));
		if (*size < sizeof(uint32)) {
			TRACE_ALWAYS(("IPW2100: not enough data to write ordinal\n"));
			return B_BAD_VALUE;
		}

		uint32 address = ReadMem32(fOrdinalTable1Base + index * 4);
		WriteMem32(address, *(uint32 *)buffer);
		*size = sizeof(uint32);
		return B_OK;
	}

	TRACE_ALWAYS(("IPW2100: only ordinals in table 1 can be written\n"));
	return B_BAD_INDEX;
}


void
IPW2100::SetInterrupts(uint32 interrupts)
{
	fInterruptMask = interrupts;
	WriteReg(IPW_REG_INTERRUPT_MASK, fInterruptMask);
}


uint32
IPW2100::Interrupts()
{
	return fInterruptMask;
}


status_t
IPW2100::SendCommand(uint32 commandId, const uint8 *buffer, size_t length)
{
	ipw_command *command = fCommandLog;
	memset(command, 0, sizeof(ipw_command));
	command->command = commandId;
	command->length = length;
	if (buffer && length > 0)
		memcpy(command->data, buffer, length);

	ipw_bd *descriptor = &fTXRingLog[fTXPosition];
	descriptor->physical_address = fCommandPhy;
	descriptor->length = sizeof(ipw_command);
	descriptor->flags = IPW_BD_FLAG_COMMAND;
	descriptor->fragment_count = 1;

	// tell the firmware that there is data to process
	fTXPosition = (fTXPosition + 1) % IPW_TX_BUFFER_COUNT;
	WriteReg(IPW_REG_TX_WRITE, fTXPosition);

	status_t result = acquire_sem_etc(fCommandDoneSem, 1, B_RELATIVE_TIMEOUT, 200000);
	if (result < B_OK) {
		TRACE_ALWAYS(("IPW2100: timeout at waiting for command completion\n"));
		return result;
	}

	// wait some, the firmware needs this
	snooze(10000);
	return B_OK;
}


//#pragma mark -


area_id
IPW2100::MapPhysicalMemory(const char *name, uint32 physicalAddress,
	void **logicalAddress, size_t size)
{
	TRACE(("IPW2100: mapping physical address 0x%08lx with size %ld\n", physicalAddress, size));
	uint32 offset = physicalAddress & (B_PAGE_SIZE - 1);
	physicalAddress = physicalAddress - offset;
	size = (size + offset + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	void *virtualAddress;
	area_id area = map_physical_memory(name, physicalAddress, size,
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA, &virtualAddress);
	if (area < B_OK) {
		TRACE_ALWAYS(("IPW2100: mapping physical address failed\n"));
		return area;
	}

	virtualAddress = (uint8 *)virtualAddress + offset;
	if (logicalAddress)
		*logicalAddress = virtualAddress;

	TRACE(("IPW2100: mapped physical address 0x%08lx to 0x%08lx\n", physicalAddress, (uint32)virtualAddress));
	TRACE(("IPW2100: with offset 0x%08lx and size %ld\n", offset, size));
	return area;
}


area_id
IPW2100::AllocateContiguous(const char *name, void **logicalAddress,
	uint32 *physicalAddress, size_t size)
{
// TODO: physicalAddress should be phys_addr_t*!
	size = (size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);
	void *virtualAddress = NULL;
	area_id area = create_area(name, &virtualAddress, B_ANY_KERNEL_ADDRESS,
		size, B_32_BIT_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (area < B_OK) {
		TRACE_ALWAYS(("IPW2100: allocating contiguous area failed\n"));
		return area;
	}

	if (logicalAddress)
		*logicalAddress = virtualAddress;

	if (physicalAddress) {
		physical_entry physicalEntry;
		get_memory_map(virtualAddress, size, &physicalEntry, 1);
		*physicalAddress = physicalEntry.address;
	}

	return area;
}


//#pragma mark -


status_t
IPW2100::CheckAdapterAccess()
{
	TRACE(("IPW2100: debug register 0x%08lx\n", ReadReg(IPW_REG_DEBUG)));
	return (ReadReg(IPW_REG_DEBUG) == IPW_DEBUG_DATA ? B_OK : B_ERROR);
}


status_t
IPW2100::SoftResetAdapter()
{
	WriteReg(IPW_REG_RESET, IPW_RESET_SW_RESET);

	// wait for reset
	int32 tries;
	for (tries = 1000; tries > 0; tries--) {
		if (ReadReg(IPW_REG_RESET) & IPW_RESET_PRINCETON_RESET)
			break;

		snooze(10);
	}

	if (tries == 0) {
		TRACE_ALWAYS(("IPW2100: reset wasn't asserted\n"));
		return B_ERROR;
	}

	TRACE(("IPW2100: reset asserted with %ld tries left\n", tries));
	WriteReg(IPW_REG_CONTROL, IPW_CONTROL_INIT_COMPLETE);

	// wait for clock stabilisation
	for (tries = 1000; tries > 0; tries--) {
		if (ReadReg(IPW_REG_CONTROL) & IPW_CONTROL_CLOCK_READY)
			break;

		snooze(200);
	}

	if (tries == 0) {
		TRACE_ALWAYS(("IPW2100: clock didn't stabilise\n"));
		return B_ERROR;
	}

	TRACE(("IPW2100: clock ready with %ld tries left\n", tries));

	// allow standby
	WriteReg(IPW_REG_CONTROL, ReadReg(IPW_REG_CONTROL)
		| IPW_CONTROL_ALLOW_STANDBY);

	return B_OK;
}


status_t
IPW2100::ResetAdapter()
{
	status_t status = StopMaster();
	if (status < B_OK) {
		TRACE_ALWAYS(("IPW2100: master stop failed\n"));
		return status;
	}

	uint32 reg = ReadReg(IPW_REG_CONTROL);
	WriteReg(IPW_REG_CONTROL, reg | IPW_CONTROL_INIT_COMPLETE);

	// wait for clock stabilisation
	int32 tries;
	for (tries = 1000; tries > 0; tries--) {
		if (ReadReg(IPW_REG_CONTROL) & IPW_CONTROL_CLOCK_READY)
			break;

		snooze(200);
	}

	if (tries == 0) {
		TRACE_ALWAYS(("IPW2100: clock didn't stabilise\n"));
		return B_ERROR;
	}

	TRACE(("IPW2100: clock ready with %ld tries left\n", tries));

	reg = ReadReg(IPW_REG_RESET);
	WriteReg(IPW_REG_RESET, reg | IPW_RESET_SW_RESET);
	snooze(10);

	reg = ReadReg(IPW_REG_CONTROL);
	WriteReg(IPW_REG_RESET, reg | IPW_CONTROL_INIT_COMPLETE);
	return B_OK;
}


status_t
IPW2100::StopMaster()
{
	// disable interrupts
	SetInterrupts(0);

	uint32 tries;
	WriteReg(IPW_REG_RESET, IPW_RESET_STOP_MASTER);
	for (tries = 50; tries > 0; tries--) {
		if (ReadReg(IPW_REG_RESET) & IPW_RESET_MASTER_DISABLED)
			break;

		snooze(10);
	}

	if (tries == 0) {
		TRACE_ALWAYS(("IPW2100: timeout waiting for master to stop\n"));
		return B_ERROR;
	}

	TRACE(("IPW2100: master stopped with %ld tries left\n", tries));
	uint32 reg = ReadReg(IPW_REG_RESET);
	WriteReg(IPW_REG_RESET, reg | IPW_RESET_PRINCETON_RESET);
	return B_OK;
}


status_t
IPW2100::SetupBuffers()
{
	fTXRingArea = AllocateContiguous("ipw2100 tx ring", (void **)&fTXRingLog,
		&fTXRingPhy, IPW_TX_BUFFER_SIZE);
	if (fTXRingArea < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to create tx ring\n"));
		return fTXRingArea;
	}

	fRXRingArea = AllocateContiguous("ipw2100 rx ring", (void **)&fRXRingLog,
		&fRXRingPhy, IPW_RX_BUFFER_SIZE);
	if (fRXRingArea < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to create rx ring\n"));
		return fRXRingArea;
	}

	fStatusRingArea = AllocateContiguous("ipw2100 status ring",
		(void **)&fStatusRingLog, &fStatusRingPhy, IPW_STATUS_BUFFER_SIZE);
	if (fStatusRingArea < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to create status ring\n"));
		return fStatusRingArea;
	}

	fCommandArea = AllocateContiguous("ipw2100 command", (void **)&fCommandLog,
		&fCommandPhy, sizeof(ipw_command));
	if (fCommandArea < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to create command area\n"));
		return fCommandArea;
	}

	// these will be filled later
	memset(fTXRingLog, 0, IPW_TX_BUFFER_SIZE);
	memset(fRXRingLog, 0, IPW_TX_BUFFER_SIZE);
	memset(fStatusRingLog, 0, IPW_STATUS_BUFFER_SIZE);
	memset(fCommandLog, 0, sizeof(ipw_command));

	// allocate and fill tx packet infos
	fTXPacketsArea = AllocateContiguous("ipw2100 tx packets",
		(void **)&fTXPacketsLog, &fTXPacketsPhy, IPW_TX_PACKET_SIZE);
	if (fTXPacketsArea < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to create tx packet area\n"));
		return fTXPacketsArea;
	}

	// allocate and fill rx packet infos
	fRXPacketsArea = AllocateContiguous("ipw2100 rx packets",
		(void **)&fRXPacketsLog, &fRXPacketsPhy, IPW_RX_PACKET_SIZE);
	if (fRXPacketsArea < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to create rx packet area\n"));
		return fRXPacketsArea;
	}

	for (uint32 i = 0; i < IPW_RX_BUFFER_COUNT; i++) {
		fRXRingLog[i].physical_address = fRXPacketsPhy + (i * sizeof(ipw_rx));
		fRXRingLog[i].length = sizeof(ipw_rx);
	}

	fTXPosition = 0;
	fRXPosition = IPW_RX_BUFFER_COUNT - 1;

	fReadQueue = (bool *)malloc(IPW_RX_BUFFER_COUNT * sizeof(bool));
	for (uint32 i = 0; i < IPW_RX_BUFFER_COUNT; i++)
		fReadQueue[i] = false;
	fReadIndex = fRXPosition;

	WriteReg(IPW_REG_TX_BASE, (uint32)fTXRingPhy);
	WriteReg(IPW_REG_TX_SIZE, IPW_TX_BUFFER_COUNT);
	WriteReg(IPW_REG_TX_READ, 0);
	WriteReg(IPW_REG_TX_WRITE, fTXPosition);

	WriteReg(IPW_REG_RX_BASE, (uint32)fRXRingPhy);
	WriteReg(IPW_REG_RX_SIZE, IPW_RX_BUFFER_COUNT);
	WriteReg(IPW_REG_RX_READ, 0);
	WriteReg(IPW_REG_RX_WRITE, fRXPosition);

	WriteReg(IPW_REG_STATUS_BASE, (uint32)fStatusRingPhy);
	return B_OK;
}


status_t
IPW2100::ReadMACAddress()
{
	size_t size = sizeof(fMACAddress);
	status_t result = ReadOrdinal(1001, fMACAddress, &size);
	if (result < B_OK || size != sizeof(fMACAddress)) {
		TRACE_ALWAYS(("IPW2100: failed to read mac address\n"));
		return result;
	}
	return B_OK;
}


status_t
IPW2100::LoadMicrocodeAndFirmware()
{
	const char *firmware;
	switch (fMode) {
		case IPW_MODE_BSS:
			firmware = "/system/data/firmware/ipw2100/ipw2100-1.3.fw";
			break;

		case IPW_MODE_IBSS:
			firmware = "/system/data/firmware/ipw2100/ipw2100-1.3-i.fw";
			break;

		case IPW_MODE_MONITOR:
			firmware = "/system/data/firmware/ipw2100/ipw2100-1.3-p.fw";
			break;

		default:
			return B_BAD_VALUE;
	}

	TRACE_ALWAYS(("IPW2100: loading firmware %s\n", firmware));
	int fd = open(firmware, B_READ_ONLY);
	if (fd < 0) {
		TRACE_ALWAYS(("IPW2100: firmware file unavailable\n"));
		return B_ERROR;
	}

	int32 fileSize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	if (fileSize <= 0) {
		TRACE_ALWAYS(("IPW2100: firmware file seems empty\n"));
		return B_ERROR;
	}

	uint8 *buffer = (uint8 *)malloc(fileSize);
	if (buffer == NULL) {
		TRACE_ALWAYS(("IPW2100: no memory for firmware buffer\n"));
		return B_NO_MEMORY;
	}

	ssize_t readCount = read(fd, buffer, fileSize);
	TRACE(("IPW2100: read %ld bytes\n", readCount));
	close(fd);

	if (readCount < (ssize_t)sizeof(ipw_firmware_header)) {
		TRACE_ALWAYS(("IPW2100: firmware too short for header\n"));
		free(buffer);
		return B_ERROR;
	}

	ipw_firmware_header *header = (ipw_firmware_header *)buffer;
	if ((uint32)readCount < sizeof(ipw_firmware_header) + header->main_size
		+ header->ucode_size) {
		TRACE_ALWAYS(("IPW2100: firmware too short for data\n"));
		free(buffer);
		return B_ERROR;
	}

	uint8 *code = buffer + sizeof(ipw_firmware_header) + header->main_size;
	status_t status = LoadMicrocode(code, header->ucode_size);
	if (status < B_OK) {
		TRACE_ALWAYS(("IPW2100: loading microcode failed\n"));
		free(buffer);
		return status;
	}

	status = SoftResetAdapter();
	if (status < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to reset before firmware upload\n"));
		free(buffer);
		return status;
	}

	code = buffer + sizeof(ipw_firmware_header);
	status = LoadFirmware(code, header->main_size);
	if (status < B_OK) {
		TRACE_ALWAYS(("IPW2100: loading firmware failed\n"));
		free(buffer);
		return status;
	}

	free(buffer);
	return B_OK;
}


status_t
IPW2100::LoadMicrocode(const uint8 *buffer, uint32 size)
{
	TRACE(("IPW2100: loading microcode (%ld bytes)\n", size));
	WriteMem32(0x003000e0, 0x80000000);
	WriteReg(IPW_REG_RESET, 0);

	WriteMem16(0x00220000, 0x0703);
	WriteMem16(0x00220000, 0x0707);

	WriteMem8(0x00210014, 0x72);
	WriteMem8(0x00210014, 0x72);

	WriteMem8(0x00210000, 0x40);
	WriteMem8(0x00210000, 0x00);
	WriteMem8(0x00210000, 0x40);

	uint32 dataLeft = size;
	while (dataLeft > 0) {
		WriteMem8(0x00210010, *buffer++);
		WriteMem8(0x00210010, *buffer++);
		dataLeft -= 2;
	}

	WriteMem8(0x00210000, 0x00);
	WriteMem8(0x00210000, 0x00);
	WriteMem8(0x00210000, 0x80);

	WriteMem16(0x00220000, 0x0703);
	WriteMem16(0x00220000, 0x0707);

	WriteMem8(0x00210014, 0x72);
	WriteMem8(0x00210014, 0x72);

	WriteMem8(0x00210000, 0x00);
	WriteMem8(0x00210000, 0x80);

	uint32 tries;
	for (tries = 10; tries > 0; tries--) {
		if (ReadMem8(0x00210000) & 1)
			break;
		snooze(10);
	}

	if (tries == 0) {
		TRACE_ALWAYS(("IPW2100: timeout waiting for microcode to initialize\n"));
		return B_ERROR;
	}

	symbol_alive_response response;
	for (tries = 30; tries > 0; tries--) {
		for (uint32 i = 0; i < sizeof(symbol_alive_response) / 2; i++)
			((uint16 *)&response)[i] = ReadMem16(0x00210004);

		TRACE(("IPW2100: symbol alive response:\n"));
		TRACE(("             command_id: 0x%02x\n", response.command_id));
		TRACE(("             sequence_number: 0x%02x\n", response.sequence_number));
		TRACE(("             microcode_revision: 0x%02x\n", response.microcode_revision));
		TRACE(("             eeprom_valid: 0x%02x\n", response.eeprom_valid));
		TRACE(("             valid_flags: 0x%04x\n", response.valid_flags));
		TRACE(("             ieee_address: %02x:%02x:%02x:%02x:%02x:%02x\n", response.ieee_address[0], response.ieee_address[1], response.ieee_address[2], response.ieee_address[3], response.ieee_address[4], response.ieee_address[5]));
		TRACE(("             flags: 0x%04x\n", response.flags));
		TRACE(("             pcb_revision: 0x%04x\n", response.pcb_revision));
		TRACE(("             clock_settle_time: %d\n", response.clock_settle_time));
		TRACE(("             powerup_settle_time: %d\n", response.powerup_settle_time));
		TRACE(("             hop_settle_time: %d\n", response.hop_settle_time));
		TRACE(("             date: %d-%d-%d\n", response.date[2], response.date[0], response.date[1]));
		TRACE(("             time: %d:%d\n", response.time[0], response.time[1]));
		TRACE(("             microcode_valid: 0x%02x\n", response.microcode_valid));
		if (response.command_id == 0x01 && response.microcode_valid == 0x01)
			break;

		snooze(10);
	}

	if (tries == 0) {
		TRACE_ALWAYS(("IPW2100: microcode did not validate itself\n"));
		return B_ERROR;
	}

	TRACE(("IPW2100: microcode loaded and initialized with %ld tries left\n", tries));
	WriteMem32(0x003000e0, 0);
	return B_OK;
}


status_t
IPW2100::LoadFirmware(const uint8 *buffer, uint32 size)
{
	TRACE(("IPW2100: loading firmware (%ld bytes)\n", size));
	for (uint32 i = 0; i < size;) {
		uint32 address = buffer[0] | buffer[1] << 8 | buffer[2] << 16 | buffer[3] << 24;
		uint16 length = buffer[4] | buffer[5] << 8;
		i += 6 + length;
		buffer += 6;

		while (length > 0) {
			WriteMem32(address, *(uint32 *)buffer);
			length -= 4;
			address += 4;
			buffer += 4;
		}
	}

	TRACE(("IPW2100: firmware loaded\n"));
	return B_OK;
}


status_t
IPW2100::ClearSharedMemory()
{
	for (uint32 i = 0; i < IPW_SHARED_MEMORY_LENGTH_0; i++)
		WriteMem8(IPW_SHARED_MEMORY_BASE_0 + i, 0);
	for (uint32 i = 0; i < IPW_SHARED_MEMORY_LENGTH_1; i++)
		WriteMem8(IPW_SHARED_MEMORY_BASE_1 + i, 0);
	for (uint32 i = 0; i < IPW_SHARED_MEMORY_LENGTH_2; i++)
		WriteMem8(IPW_SHARED_MEMORY_BASE_2 + i, 0);
	for (uint32 i = 0; i < IPW_SHARED_MEMORY_LENGTH_3; i++)
		WriteMem8(IPW_SHARED_MEMORY_BASE_3 + i, 0);
	for (uint32 i = 0; i < IPW_INTERRUPT_MEMORY_LENGTH; i++)
		WriteMem8(IPW_INTERRUPT_MEMORY_BASE + i, 0);
	return B_OK;
}


status_t
IPW2100::StartFirmware()
{
	TRACE(("IPW2100: initializing firmware\n"));
	// grant firmware access
	WriteReg(IPW_REG_IO, IPW_IO_GPIO1_ENABLE | IPW_IO_GPIO3_MASK
		| IPW_IO_LED_OFF);

	SetInterrupts(IPW_INTERRUPT_FW_INIT_DONE);

	// start the firmware
	WriteReg(IPW_REG_RESET, 0);

	// wait for the fw init done interrupt
	status_t result = acquire_sem_etc(fFWInitDoneSem, 1, B_RELATIVE_TIMEOUT, 1000000);
	if (result < B_OK) {
		TRACE_ALWAYS(("IPW2100: firmware initialization timed out\n"));
		return result;
	}

	// discard fatal errors while init
	uint32 interrupts = ReadReg(IPW_REG_INTERRUPT);
	if (interrupts & (IPW_INTERRUPT_FATAL_ERROR | IPW_INTERRUPT_PARITY_ERROR)) {
		TRACE(("IPW2100: discarding error interrupt while firmware initialization\n"));
		WriteReg(IPW_REG_INTERRUPT, IPW_INTERRUPT_FATAL_ERROR
			| IPW_INTERRUPT_PARITY_ERROR);
	}

	TRACE(("IPW2100: firmware initialized\n"));

	WriteReg(IPW_REG_IO, ReadReg(IPW_REG_IO) | IPW_IO_GPIO1_MASK
		 | IPW_IO_GPIO3_MASK);

	// initialize ordinal table values
	fOrdinalTable1Base = ReadReg(IPW_ORDINAL_TABLE_1_ADDRESS);
	fOrdinalTable2Base = ReadReg(IPW_ORDINAL_TABLE_2_ADDRESS);
	fOrdinalTable1Length = ReadMem32(fOrdinalTable1Base);
	fOrdinalTable2Length = ReadMem32(fOrdinalTable2Base) & 0x0000ffff;

	TRACE(("IPW2100: ordinal tables initialized\n"));
	TRACE(("         	table 1 base: 0x%08lx\n", fOrdinalTable1Base));
	TRACE(("         	table 1 length: %ld\n", fOrdinalTable1Length));
	TRACE(("         	table 2 base: 0x%08lx\n", fOrdinalTable2Base));
	TRACE(("         	table 2 length: %ld\n", fOrdinalTable2Length));

	uint32 value = IPW_FIRMWARE_LOCK_NONE;
	size_t length = sizeof(value);
	result = WriteOrdinal(IPW_ORD_FIRMWARE_DB_LOCK, (uint8 *)&value, &length);
	if (result < B_OK || length != sizeof(value)) {
		TRACE_ALWAYS(("IPW2100: unable to set ordinal lock\n"));
		return result;
	}

	return B_OK;
}


status_t
IPW2100::EtherInit()
{
	if (SendCommand(IPW_COMMAND_SET_MODE, (uint8 *)&fMode, sizeof(fMode)) < B_OK) {
		TRACE_ALWAYS(("IPW2100: failed to set the desired operation mode\n"));
		return B_ERROR;
	}

	if (fMode == IPW_MODE_MONITOR) {
		// only initialize the bare minimum in monitor mode
		SendCommand(IPW_COMMAND_SET_CHANNEL, (uint8 *)&fChannel, sizeof(fChannel));
		EnableAdapter();
		return B_OK;
	}

	uint32 beacon = IPW_DEFAULT_BEACON_INTERVAL;
	uint32 power = IPW_DEFAULT_TX_POWER;

	ipw_configuration configuration;
	configuration.flags = IPW_CONFIG_802_1X_ENABLE | IPW_CONFIG_BSS_MASK
		| IPW_CONFIG_IBSS_MASK;
	if (fMode == IPW_MODE_IBSS)
		configuration.flags |= IPW_CONFIG_IBSS_AUTO_START;
	configuration.bss_channel_mask = IPW_BSS_CHANNEL_MASK;
	configuration.ibss_channel_mask = IPW_IBSS_CHANNEL_MASK;

	ipw_security security;
	security.ciphers = fCiphers;
	security.auth_mode = fAuthMode;

	if (SendCommand(IPW_COMMAND_SET_MAC_ADDRESS, fMACAddress, sizeof(fMACAddress)) < B_OK
		|| (fMode == IPW_MODE_IBSS
		&& (SendCommand(IPW_COMMAND_SET_CHANNEL, (uint8 *)&fChannel, sizeof(fChannel)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_BEACON_INTERVAL, (uint8 *)&beacon, sizeof(beacon)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_TX_POWER_INDEX, (uint8 *)&power, sizeof(power)) < B_OK))
		|| SendCommand(IPW_COMMAND_SET_CONFIGURATION, (uint8 *)&configuration, sizeof(configuration)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_BASIC_TX_RATES, (uint8 *)&fTXRates, sizeof(fTXRates)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_TX_RATES, (uint8 *)&fTXRates, sizeof(fTXRates)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_MSDU_TX_RATES, (uint8 *)&fTXRates, sizeof(fTXRates)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_POWER_MODE, (uint8 *)&fPowerMode, sizeof(fPowerMode)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_RTS_THRESHOLD, (uint8 *)&fRTSThreshold, sizeof(fRTSThreshold)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_MANDATORY_BSSID, NULL, 0) < B_OK
		|| SendCommand(IPW_COMMAND_SET_ESSID, (uint8 *)fESSID, (fESSID ? strlen(fESSID) + 1 : 0)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_SECURITY_INFO, (uint8 *)&security, sizeof(ipw_security)) < B_OK
		|| (fCiphers != IPW_CIPHER_NONE
		&& (SendCommand(IPW_COMMAND_SET_WEP_KEY, (uint8 *)&fWEPKeys[0], sizeof(ipw_wep_key)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_WEP_KEY, (uint8 *)&fWEPKeys[1], sizeof(ipw_wep_key)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_WEP_KEY, (uint8 *)&fWEPKeys[2], sizeof(ipw_wep_key)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_WEP_KEY, (uint8 *)&fWEPKeys[3], sizeof(ipw_wep_key)) < B_OK
		|| SendCommand(IPW_COMMAND_SET_WEP_KEY_INDEX, (uint8 *)&fWEPKeyIndex, sizeof(fWEPKeyIndex)) < B_OK))
		|| SendCommand(IPW_COMMAND_SET_WEP_FLAGS, (uint8 *)&fWEPFlags, sizeof(fWEPFlags)) < B_OK) {
		TRACE_ALWAYS(("IPW2100: setting up the adapter configuration failed\n"));
		return B_ERROR;
	}

	status_t result = EnableAdapter();
	if (result < B_OK) {
		TRACE_ALWAYS(("IPW2100: unable to enable the adapter\n"));
		return result;
	}

	result = ScanForNetworks();
	if (result < B_OK) {
		TRACE_ALWAYS(("IPW2100: scan for networks failed\n"));
		return result;
	}

	return B_OK;
}


status_t
IPW2100::DisableAdapter()
{
	status_t result = SendCommand(IPW_COMMAND_DISABLE, NULL, 0);
	if (result < B_OK) {
		TRACE_ALWAYS(("IPW2100: the disable command failed\n"));
		return result;
	}

	// wait for the disabled state change
	result = acquire_sem_etc(fAdapterDisabledSem, 1, B_RELATIVE_TIMEOUT, 1000000);
	if (result < B_OK) {
		TRACE_ALWAYS(("IPW2100: timeout when waiting for adapter to become disabled\n"));
		return result;
	}

	TRACE(("IPW2100: adapter disabled\n"));
	return B_OK;
}


status_t
IPW2100::EnableAdapter()
{
	status_t result = SendCommand(IPW_COMMAND_ENABLE, NULL, 0);
	if (result < B_OK) {
		TRACE_ALWAYS(("IPW2100: the enable command failed\n"));
		return result;
	}

	TRACE(("IPW2100: adapter enabled\n"));
	return B_OK;
}


status_t
IPW2100::ScanForNetworks()
{
	TRACE(("IPW2100: scanning for networks\n"));

	ipw_scan_options options;
	options.flags = fScanOptions;
	options.channels = fChannel;
	SendCommand(IPW_COMMAND_SET_SCAN_OPTIONS, (uint8 *)&options, sizeof(options));

	uint32 parameters = 0;
	SendCommand(IPW_COMMAND_BROADCAST_SCAN, (uint8 *)&parameters, sizeof(parameters));
	return B_OK;
}


status_t
IPW2100::DisableRadio()
{
	SendCommand(IPW_COMMAND_DISABLE_PHY, NULL, 0);

	uint32 tries;
	for (tries = 1000; tries > 0; tries--) {
		if ((ReadMem32(0x00220000) & 0x00000008) > 0
			&& (ReadMem32(0x00300004) & 0x00000001) > 0) {
			TRACE(("IPW2100: radio disabled\n"));
			break;
		}

		snooze(10000);
	}

	if (tries == 0) {
		TRACE_ALWAYS(("IPW2100: timeout waiting for radio to get disabled\n"));
		return B_TIMED_OUT;
	}

	return B_OK;
}


status_t
IPW2100::PowerDown()
{
	SendCommand(IPW_COMMAND_PREPARE_POWER_DOWN, NULL, 0);
	WriteReg(IPW_REG_IO, IPW_IO_GPIO1_ENABLE | IPW_IO_GPIO3_MASK
		| IPW_IO_LED_OFF);
	StopMaster();
	WriteReg(IPW_REG_RESET, IPW_RESET_SW_RESET);
	return B_OK;
}
