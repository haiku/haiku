/*
 * Copyright 2018-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 */
#include "mmc_bus.h"

#include <Errors.h>

#include <stdint.h>


MMCBus::MMCBus(device_node* node)
	:
	fNode(node),
	fController(NULL),
	fCookie(NULL),
	fStatus(B_OK),
	fWorkerThread(-1),
	fActiveDevice(0)
{
	CALLED();

	// Get the parent info, it includes the API to send commands to the hardware
	device_node* parent = gDeviceManager->get_parent_node(node);
	fStatus = gDeviceManager->get_driver(parent,
		(driver_module_info**)&fController, &fCookie);
	gDeviceManager->put_node(parent);

	if (fStatus != B_OK) {
		ERROR("Not able to establish the bus %s\n",
			strerror(fStatus));
		return;
	}

	fScanSemaphore = create_sem(0, "MMC bus scan");
	fLockSemaphore = create_sem(1, "MMC bus lock");
	fWorkerThread = spawn_kernel_thread(_WorkerThread, "SD bus controller",
		B_NORMAL_PRIORITY, this);
	resume_thread(fWorkerThread);
}


MMCBus::~MMCBus()
{
	CALLED();

	// stop worker thread
	fStatus = B_SHUTTING_DOWN;

	status_t result;
	if (fWorkerThread != 0)
		wait_for_thread(fWorkerThread, &result);
	// TODO power off cards, stop clock, etc if needed.

	delete_sem(fLockSemaphore);
	delete_sem(fScanSemaphore);
}


status_t
MMCBus::InitCheck()
{
	return fStatus;
}


void
MMCBus::Rescan()
{
	// Just wake up the thread for a scan
	release_sem(fScanSemaphore);
}


status_t
MMCBus::ExecuteCommand(uint8_t command, uint32_t argument, uint32_t* response)
{
	status_t status = _ActivateDevice(0);
	if (status != B_OK)
		return status;
	return fController->execute_command(fCookie, command, argument, response);
}


status_t
MMCBus::Read(uint16_t rca, off_t position, void* buffer, size_t* length)
{
	status_t status = _ActivateDevice(rca);
	if (status != B_OK)
		return status;
	return fController->read_naive(fCookie, position, buffer, length);
}


status_t
MMCBus::_ActivateDevice(uint16_t rca)
{
	// Do nothing if the device is already activated
	if (fActiveDevice == rca)
		return B_OK;

	uint32_t response;
	status_t result;
	result = fController->execute_command(fCookie, SD_SELECT_DESELECT_CARD,
		((uint32)rca) << 16, &response);

	if (result == B_OK)
		fActiveDevice = rca;

	return result;
}


status_t
MMCBus::_WorkerThread(void* cookie)
{
	MMCBus* bus = (MMCBus*)cookie;
	uint32_t response;

	acquire_sem(bus->fLockSemaphore);

	// We assume the bus defaults to 400kHz clock and has already powered on
	// cards.

	// Reset all cards on the bus
	bus->ExecuteCommand(SD_GO_IDLE_STATE, 0, NULL);

	while (bus->fStatus != B_SHUTTING_DOWN) {
		release_sem(bus->fLockSemaphore);
		// wait for bus to signal a card is inserted
		// Most of the time the thread will be waiting here, with
		// fLockSemaphore released
		acquire_sem(bus->fScanSemaphore);
		acquire_sem(bus->fLockSemaphore);

		TRACE("Scanning the bus\n");

		// Probe the voltage range
		enum {
			// Table 4-40 in physical layer specification v8.00
			// All other values are currently reserved
			HOST_27_36V = 1, //Host supplied voltage 2.7-3.6V
		};

		// An arbitrary value, we just need to check that the response
		// containts the same.
		static const uint8 kVoltageCheckPattern = 0xAA;

		// FIXME MMC cards will not reply to this! They expect CMD1 instead
		// SD v1 cards will also not reply, but we can proceed to ACMD41
		// If ACMD41 also does not work, it may be an SDIO card, too
		uint32_t probe = (HOST_27_36V << 8) | kVoltageCheckPattern;
		uint32_t hcs = 1 << 30;
		if (bus->ExecuteCommand(SD_SEND_IF_COND, probe, &response) != B_OK) {
			TRACE("Card does not implement CMD8, may be a V1 SD card\n");
			// Do not check for SDHC support in this case
			hcs = 0;
		} else if (response != probe) {
			ERROR("Card does not support voltage range (expected %x, "
				"reply %x)\n", probe, response);
			// TODO what now?
		}

		// Probe OCR, waiting for card to become ready
		uint32_t ocr;
		do {
			uint32_t cardStatus;
			while (bus->ExecuteCommand(SD_APP_CMD, 0, &cardStatus)
					== B_BUSY) {
				ERROR("Card locked after CMD8...\n");
				snooze(1000000);
			}
			if ((cardStatus & 0xFFFF8000) != 0)
				ERROR("SD card reports error %x\n", cardStatus);
			if ((cardStatus & (1 << 5)) == 0)
				ERROR("Card did not enter ACMD mode\n");

			bus->ExecuteCommand(SD_SEND_OP_COND, hcs | 0xFF8000, &ocr);

			if ((ocr & (1 << 31)) == 0) {
				TRACE("Card is busy\n");
				snooze(100000);
			}
		} while (((ocr & (1 << 31)) == 0));

		// FIXME this should be asked to each card, when there are multiple
		// ones. So ACMD41 should be moved inside the probing loop below?
		uint8_t cardType = CARD_TYPE_SD;

		if ((ocr & hcs) != 0)
			cardType = CARD_TYPE_SDHC;
		if ((ocr & (1 << 29)) != 0)
			cardType = CARD_TYPE_UHS2;
		if ((ocr & (1 << 24)) != 0)
			TRACE("Card supports 1.8v");
		TRACE("Voltage range: %x\n", ocr & 0xFFFFFF);

		// TODO send CMD11 to switch to low voltage mode if card supports it?

		// We use CMD2 (ALL_SEND_CID) and CMD3 (SEND_RELATIVE_ADDR) to assign
		// an RCA to all cards. Initially all cards have an RCA of 0 and will
		// all receive CMD2. But only ne of them will reply (they do collision
		// detection while sending the CID in reply). We assign a new RCA to
		// that first card, and repeat the process with the remaining ones
		// until no one answers to CMD2. Then we know all cards have an RCA
		// (and a matching published device on our side).
		uint32_t cid[4];
		
		while (bus->ExecuteCommand(SD_ALL_SEND_CID, 0, cid) == B_OK) {
			bus->ExecuteCommand(SD_SEND_RELATIVE_ADDR, 0, &response);

			TRACE("RCA: %x Status: %x\n", response >> 16, response & 0xFFFF);

			if ((response & 0xFF00) != 0x500) {
				TRACE("Card did not enter data state\n");
				// This probably means there are no more cards to scan on the
				// bus, so exit the loop.
				break;
			}

			// The card now has an RCA and it entered the data phase, which
			// means our initializing job is over, we can pass it on to the
			// mmc_disk driver.
			
			uint32_t vendor = cid[3] & 0xFFFFFF;
			char name[6] = {(char)(cid[2] >> 24), (char)(cid[2] >> 16),
				(char)(cid[2] >> 8), (char)cid[2], (char)(cid[1] >> 24), 0};
			uint32_t serial = (cid[1] << 16) | (cid[0] >> 16);
			uint16_t revision = (cid[1] >> 20) & 0xF;
			revision *= 100;
			revision += (cid[1] >> 16) & 0xF;
			uint8_t month = cid[0] & 0xF;
			uint16_t year = 2000 + ((cid[0] >> 4) & 0xFF);
			uint16_t rca = response >> 16;
				
			device_attr attrs[] = {
				{ B_DEVICE_BUS, B_STRING_TYPE, {string: "mmc" }},
				{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "mmc device" }},
				{ B_DEVICE_VENDOR_ID, B_UINT32_TYPE, {ui32: vendor}},
				{ B_DEVICE_ID, B_STRING_TYPE, {string: name}},
				{ B_DEVICE_UNIQUE_ID, B_UINT32_TYPE, {ui32: serial}},
				{ "mmc/revision", B_UINT16_TYPE, {ui16: revision}},
				{ "mmc/month", B_UINT8_TYPE, {ui8: month}},
				{ "mmc/year", B_UINT16_TYPE, {ui16: year}},
				{ kMmcRcaAttribute, B_UINT16_TYPE, {ui16: rca}},
				{ kMmcTypeAttribute, B_UINT8_TYPE, {ui8: cardType}},
				{}
			};

			// publish child device for the card
			gDeviceManager->register_node(bus->fNode, MMC_BUS_MODULE_NAME,
				attrs, NULL, NULL);
		}

		// FIXME we also need to unpublish devices that are gone. Probably need
		// to "ping" all RCAs somehow? Or is there an interrupt we can look for
		// to detect added/removed cards?
	}

	release_sem(bus->fLockSemaphore);

	TRACE("poller thread terminating");
	return B_OK;
}
