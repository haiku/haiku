/*
 *	ASIX AX88172/AX88772/AX88178 USB 2.0 Ethernet Driver.
 *	Copyright (c) 2008, 2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 *	Heavily based on code of the
 *	Driver for USB Ethernet Control Model devices
 *	Copyright (C) 2008 Michael Lotz <mmlr@mlotz.ch>
 *	Distributed under the terms of the MIT license.
 *
 */


#include "MIIBus.h"

#include "ASIXVendorRequests.h"
#include "Driver.h"
#include "Settings.h"


#define MII_OUI(id1, id2)	(((id1) << 6) | ((id2) >> 10))
#define MII_MODEL(id2)		(((id2) & 0x03f0) >> 4)
#define MII_REV(id2)		 ((id2) & 0x000f)


MIIBus::MIIBus()
		:
		fStatus(B_NO_INIT),
		fDevice(0),
		fSelectedPHY(CurrentPHY)
{
	for (size_t i = 0; i < PHYsCount; i++) {
		fPHYs[i] = PHYNotInstalled;
	}
}


status_t
MIIBus::Init(usb_device device)
{
	// reset to default state
	fDevice = 0;
	fSelectedPHY = CurrentPHY;
	for (size_t i = 0; i < PHYsCount; i++) {
		fPHYs[i] = PHYNotInstalled;
	}

	size_t actual_length = 0;
	status_t result = gUSBModule->send_request(device,
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN,
			READ_PHYID, 0, 0, sizeof(fPHYs), fPHYs, &actual_length);

	if (result != B_OK) {
		TRACE_ALWAYS("Request of the PHYIDs failed:%#010x\n", result);
		return result;
	}

	if (sizeof(fPHYs) != actual_length) {
		TRACE_ALWAYS("Mismatch of reading %d PHYIDs bytes instead of %d.\n",
												actual_length, sizeof(fPHYs));
	}

	TRACE("PHYIDs are:%#02x:%#02x\n", fPHYs[0], fPHYs[1]);

	// simply tactic - we use first available PHY
	if (PHYType(PrimaryPHY) != PHYNotInstalled) {
		fSelectedPHY = PrimaryPHY;
	} else
	if (PHYType(SecondaryPHY) != PHYNotInstalled) {
		fSelectedPHY = SecondaryPHY;
	}

	TRACE("PHYs are configured: Selected:%#02x; Primary:%#02x; 2ndary:%#02x\n",
					PHYID(CurrentPHY), PHYID(PrimaryPHY), PHYID(SecondaryPHY));
	if (fSelectedPHY == CurrentPHY) {
		TRACE_ALWAYS("No PHYs found!\n");
		return B_ENTRY_NOT_FOUND;
	}

	fDevice = device;
	fStatus = result;

	return fStatus;
}


status_t
MIIBus::SetupPHY()
{
	uint16 control = 0;
	status_t result = Read(MII_BMCR, &control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of reading control word:%#010x.\n", result);
		return result;
	}

	TRACE("MII Control word is %#04x\n", control);

	control &= ~BMCR_Isolate;
	result = Write(MII_BMCR, control);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of writing control word %#04x:%#010x.\n",
				control, result);
	}

	result = Write(MII_BMCR, BMCR_Reset);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of resetting PHY:%#010x.\n", result);
	}

	uint16 id01 = 0, id02 = 0;
	result = Read(MII_PHYID0, &id01);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of reading PHY ID1:%#010x.\n", result);
	}

	result = Read(MII_PHYID1, &id02);
	if (result != B_OK) {
		TRACE_ALWAYS("Error of reading PHY ID2:%#010x.\n", result);
	}

	TRACE("MII Info: OUI:%04x; Model:%04x; rev:%02x.\n",
			MII_OUI(id01, id02), MII_MODEL(id02), MII_REV(id02));

	// Dump();

	return result;
}


status_t
MIIBus::InitCheck()
{
	if (fSelectedPHY == CurrentPHY) {
		return B_ENTRY_NOT_FOUND;
	}

	return fStatus;
}


uint8
MIIBus::PHYID(PHYIndex phyIndex /*= CurrentPHY*/)
{
	if (phyIndex == CurrentPHY) {
		return (fSelectedPHY == CurrentPHY
				? 0 : fPHYs[fSelectedPHY]) & PHYIDMask;
	}

	return fPHYs[phyIndex] & PHYIDMask;
}


uint8
MIIBus::PHYType(PHYIndex phyIndex /*= CurrentPHY*/)
{
	if (phyIndex == CurrentPHY) {
		return (fSelectedPHY == CurrentPHY
				? PHYNotInstalled : fPHYs[fSelectedPHY]) & PHYTypeMask;
	}

	return fPHYs[phyIndex] & PHYTypeMask;
}


status_t
MIIBus::Read(uint16 miiRegister, uint16 *value, PHYIndex phyIndex /*= CurrPHY*/)
{
	status_t result = InitCheck();
	if (B_OK != result) {
		TRACE_ALWAYS("Error: MII is not ready:%#010x\n", result);
		return result;
	}

	if (PHYType(phyIndex) == PHYNotInstalled) {
		TRACE_ALWAYS("Error: Invalid PHY index:%#02x.\n", phyIndex);
		return B_ENTRY_NOT_FOUND;
	}

	uint16 phyId = PHYID(phyIndex);

	size_t actual_length = 0;
	// switch to SW operation mode
	result = gUSBModule->send_request(fDevice,
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
			SW_MII_OP, 0, 0, 0, 0, &actual_length);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of switching MII to SW op.mode: %#010x\n", result);
		return result;
	}

	// read register value
	status_t op_result = gUSBModule->send_request(fDevice,
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN,
			READ_MII, phyId, miiRegister,
			sizeof(*value), value, &actual_length);

	if (op_result != B_OK) {
		TRACE_ALWAYS("Error of reading MII reg.%d at PHY%d:%#010x.\n",
										miiRegister, phyId, op_result);
	}

	if (sizeof(*value) != actual_length) {
		TRACE_ALWAYS("Mismatch of reading MII reg.%d at PHY %d. "
						"Read %d bytes instead of %d.\n",
							miiRegister, phyId, actual_length, sizeof(*value));
	}

	// switch to HW operation mode
	result = gUSBModule->send_request(fDevice,
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
			HW_MII_OP, 0, 0, 0, 0, &actual_length);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of switching MII to HW op.mode: %#010x\n", result);
	}

	return op_result;
}


status_t
MIIBus::Write(uint16 miiRegister, uint16 value, PHYIndex phyIndex /*= CurrPHY*/)
{
	size_t actual_length = 0;

	status_t result = InitCheck();
	if (B_OK != result) {
		TRACE_ALWAYS("Error: MII is not ready:%#010x\n", result);
		return result;
	}

	if (PHYType(phyIndex) == PHYNotInstalled) {
		TRACE_ALWAYS("Error: Invalid PHY index:%#02x\n", phyIndex);
		return B_ENTRY_NOT_FOUND;
	}

	uint16 phyId = PHYID(phyIndex);

	// switch to SW operation mode
	result = gUSBModule->send_request(fDevice,
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
			SW_MII_OP, 0, 0, 0, 0, &actual_length);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of switching MII to SW op.mode: %#010x\n", result);
		return result;
	}

	// write register value
	status_t op_result = gUSBModule->send_request(fDevice,
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
			WRITE_MII, phyId, miiRegister,
			sizeof(value), &value, &actual_length);

	if (op_result != B_OK) {
		TRACE_ALWAYS("Error of writing MII reg.%d at PHY %d:%#010x.\n",
										miiRegister, phyId, op_result);
	}

	if (sizeof(value) != actual_length) {
		TRACE_ALWAYS("Mismatch of writing MII reg.%d at PHY %d."
						"Write %d bytes instead of %d.\n",
							miiRegister, phyId, actual_length, sizeof(value));
	}

	// switch to HW operation mode
	result = gUSBModule->send_request(fDevice,
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
			HW_MII_OP, 0, 0, 0, 0, &actual_length);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of switching MII to HW op.mode: %#010x\n", result);
	}

	return op_result;
}


status_t
MIIBus::Status(uint16 *status, PHYIndex phyIndex /*= CurrentPHY*/)
{
	return Read(MII_BMSR, status, phyIndex);
}


status_t
MIIBus::Dump()
{
	status_t result = InitCheck();
	if (B_OK != result) {
		TRACE_ALWAYS("Error: MII is not ready:%#010x.\n", result);
		return result;
	}

	if (PHYType(CurrentPHY) == PHYNotInstalled) {
		TRACE_ALWAYS("Error: Current PHY index is invalid!\n");
		return B_ENTRY_NOT_FOUND;
	}

	uint16 phyId = PHYID(CurrentPHY);

	size_t actual_length = 0;
	// switch to SW operation mode
	result = gUSBModule->send_request(fDevice,
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
			SW_MII_OP, 0, 0, 0, 0, &actual_length);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of switching MII to SW op.mode: %#010x\n", result);
		return result;
	}

	uint8 regs[] = { MII_BMCR, MII_BMSR,
						MII_PHYID0, MII_PHYID1,
						MII_ANAR, MII_ANLPAR/*, MII_ANER*/};
	uint16 value = 0;
	for (size_t i = 0; i < sizeof(regs)/ sizeof(regs[0]); i++) {

		// read register value
		status_t op_result = gUSBModule->send_request(fDevice,
				USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN,
				READ_MII, phyId, regs[i],
				sizeof(value), &value, &actual_length);

		if (op_result != B_OK) {
			TRACE_ALWAYS("Error of reading MII reg.%d at PHY%d:%#010x.\n",
												regs[i], phyId, op_result);
		}

		if (sizeof(value) != actual_length) {
			TRACE_ALWAYS("Mismatch of reading MII reg.%d at PHY%d."
							" Read %d bytes instead of %d.\n",
							regs[i], phyId, actual_length, sizeof(value));
		}

		TRACE_ALWAYS("MII reg: %d has %#04x\n", regs[i], value);
	}

	// switch to HW operation mode
	result = gUSBModule->send_request(fDevice,
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
			HW_MII_OP, 0, 0, 0, 0, &actual_length);

	if (result != B_OK) {
		TRACE_ALWAYS("Error of switching MII to HW op.mode: %#010x\n", result);
	}

	return result;

}

