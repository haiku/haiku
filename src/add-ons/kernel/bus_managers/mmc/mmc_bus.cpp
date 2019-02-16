/*
 * Copyright 2018 Haiku, Inc. All rights reserved.
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
	fWorkerThread(0)
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

	// TODO enumerate the bus (in a separate thread?)
	fWorkerThread = spawn_kernel_thread(WorkerThread, "SD bus controller",
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
}


status_t
MMCBus::InitCheck()
{
	return fStatus;
}


status_t
MMCBus::ExecuteCommand(uint8_t command, uint32_t argument, uint32_t* response)
{
	return fController->execute_command(fCookie, command, argument, response);
}


status_t
MMCBus::WorkerThread(void* cookie)
{
	TRACE("worker thread spawned.\n");

	MMCBus* bus = (MMCBus*)cookie;
	uint32_t response;

	// FIXME wait for bus to signal a card is inserted
	// We assume the bus defaults to 400kHz clock and has already powered on
	// cards.
	
	// Reset all cards on the bus
	bus->ExecuteCommand(0, 0, NULL);

	// Probe the voltage range
	// FIXME MMC cards will not reply to this! They expect CMD1 instead
	// SD v1 cards will also not reply, but we can proceed to ACMD41
	// If ACMD41 also does not work, it may be an SDIO card, too
	uint32_t probe = (1 << 8) | 0x55;
	uint32_t hcs = 1 << 30;
	if (bus->ExecuteCommand(8, probe, &response) != B_OK) {
		TRACE("Card does not implement CMD8, may be a V1 SD card\n");
		// Do not check for SDHC support in this case
		hcs = 0;
	} else if (response != probe) {
		ERROR("Card does not support voltage range (expected %x, reply %x)\n",
			probe, response);
		// TODO what now?
	}

	// Probe OCR, waiting for card to become ready
	uint32_t ocr;
	do {
		uint32_t cardStatus;
		while (bus->ExecuteCommand(55, 0, &cardStatus)
			== B_BUSY) {
			// FIXME we shouldn't get here if we handle CMD8 failure properly
			ERROR("Card locked after CMD8...\n");
			snooze(1000000);
		}
		if ((cardStatus & 0xFFFF8000) != 0)
			ERROR("SD card reports error\n");
		if ((cardStatus & (1 << 5)) == 0)
			ERROR("Card did not enter ACMD mode");

		bus->ExecuteCommand(41, hcs | 0xFF8000, &ocr);

		if ((ocr & (1 << 31)) == 0) {
			TRACE("Card is busy\n");
			snooze(100000);
		}
	} while (((ocr & (1 << 31)) == 0));

	if (ocr & hcs != 0)
		TRACE("Card is SDHC");
	if (ocr & (1 << 29) != 0)
		TRACE("Card supports UHS-II");
	if (ocr & (1 << 24) != 0)
		TRACE("Card supports 1.8v");
	TRACE("Voltage range: %x\n", ocr & 0xFFFFFF);

	// TODO send CMD11 to switch to low voltage mode if card supports it?

	uint32_t cid[4];
	bus->ExecuteCommand(2, 0, cid);

	TRACE("Manufacturer: %02x%c%c\n", cid[3] >> 16, cid[3] >> 8, cid[3]);
	TRACE("Name: %c%c%c%c%c\n", cid[2] >> 24, cid[2] >> 16, cid[2] >> 8,
		cid[2], cid[1] >> 24);
	TRACE("Revision: %d.%d\n", (cid[1] >> 20) & 0xF, (cid[1] >> 16) & 0xF);
	TRACE("Serial number: %x\n", (cid[1] << 16) | (cid[0] >> 16));
	TRACE("Date: %d/%d\n", cid[0] & 0xF, 2000 + ((cid[0] >> 4) & 0xFF));

	bus->ExecuteCommand(3, 0, &response);

	TRACE("RCA: %x Status: %x\n", response >> 16, response & 0xFFFF);

	if ((response & 0xFF00) != 0x5000)
		ERROR("Card did not enter data state\n");

	// The card now has an RCA and it entered the data phase, which means our
	// initializing job is over, we can pass it on to the mmc_disk driver.

	// TODO publish child device for the card
	// TODO fill it with attributes from the CID
	
	// TODO iterate CMD2/CMD3 to assign an RCA to all cards (and publish devices
	// for each of them)
}
