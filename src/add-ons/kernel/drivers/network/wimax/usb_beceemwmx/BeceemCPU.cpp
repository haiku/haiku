/*
 *	Beceem WiMax USB Driver.
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the GNU General Public License.
 *
 *	Based on GPL code developed by: Beceem Communications Pvt. Ltd
 *
 *	Control the MIPS cpu within the Beceem chipset.
 *
 */


#include "BeceemCPU.h"
#include "Driver.h"
#include "Settings.h"


BeceemCPU::BeceemCPU()
{
	TRACE("Debug: Load CPU handler\n");
}


status_t
BeceemCPU::CPUInit(WIMAX_DEVICE* swmxdevice)
{
	TRACE("Debug: Init CPU handler\n");
	fWmxDevice = swmxdevice;
	return B_OK;
}


status_t
BeceemCPU::CPURun()
{
	unsigned int clockRegister = 0;

	// Read current clock register contents
	if (BizarroReadRegister(CLOCK_RESET_CNTRL_REG_1,
		sizeof(clockRegister), &clockRegister) != B_OK) {
		TRACE_ALWAYS("Error: Read of clock reset reg failure\n");
		return B_ERROR;
	}

	// Adjust clock register contents to start cpu
	if (fWmxDevice->CPUFlashBoot)
		clockRegister &= ~(1 << 30);
	else
		clockRegister |= (1 << 30);

	// Write new clock register contents
	if (BizarroWriteRegister(CLOCK_RESET_CNTRL_REG_1,
		sizeof(clockRegister), &clockRegister) != B_OK) {
		TRACE_ALWAYS("Error: Write of clock reset reg failure\n");
		return B_ERROR;
	}

	return B_OK;
}


status_t
BeceemCPU::CPUReset()
{
	unsigned int value = 0;

	if (fWmxDevice->deviceChipID >= T3LPB) {
		BizarroReadRegister(SYS_CFG, sizeof(value), &value);
		BizarroReadRegister(SYS_CFG, sizeof(value), &value);
			// SYS_CFG register is write protected hence for modifying
			// this reg value, it should be read twice before writing.

		value = value | (fWmxDevice->syscfgBefFw & 0x00000060);
			// making bit[6...5] same as was before f/w download. this
			// setting forces the h/w to re-populated the SP RAM area
			// with the string descriptor .

		if (BizarroWriteRegister(SYS_CFG, sizeof(value), &value) != B_OK) {
			TRACE_ALWAYS("Error: unable to write SYS_CFG during reset\n");
			return B_ERROR;
		}
	}

	/* Reset the UMA-B Device */
	if (fWmxDevice->deviceChipID >= T3LPB) {
		// Reset UMA-B
		// TODO : USB reset needs implimented
		/*
		if (usb_reset_device(psIntfAdapter->udev) != B_OK) {
			TRACE_ALWAYS("Error: USB Reset failed\n");
			retrun B_ERROR;
		}
		*/

		if (fWmxDevice->deviceChipID == BCS220_2
			|| fWmxDevice->deviceChipID == BCS220_2BC
			|| fWmxDevice->deviceChipID == BCS250_BC
			|| fWmxDevice->deviceChipID == BCS220_3) {
			if (BizarroReadRegister(HPM_CONFIG_LDO145,
				sizeof(value), &value) != B_OK) {
				TRACE_ALWAYS("Error: USB read failed during reset\n");
				return B_ERROR;
			}
			// set 0th bit
			value |= (1<<0);

			if (BizarroWriteRegister(HPM_CONFIG_LDO145,
				sizeof(value), &value) != B_OK) {
				TRACE_ALWAYS("Error: USB write failed during reset\n");
				return B_ERROR;
			}
		}

	}
	// TODO : ELSE OLDER CHIP ID's < T3LP see Misc.c:1048

	unsigned int uiResetValue = 0;

	if (fWmxDevice->CPUFlashBoot) {
		// In flash boot mode MIPS state register has reverse polarity.
		// So just or with setting bit 30.
		// Make the MIPS in Reset state.
		if (BizarroReadRegister(CLOCK_RESET_CNTRL_REG_1,
			sizeof(uiResetValue), &uiResetValue) != B_OK) {
			TRACE_ALWAYS("Error: read failed during FlashBoot device reset\n");
			return B_ERROR;
		}
		// set 30th bit
		uiResetValue |= (1 << 30);

		if (BizarroWriteRegister(CLOCK_RESET_CNTRL_REG_1,
			sizeof(uiResetValue), &uiResetValue) != B_OK) {
			TRACE_ALWAYS("Error: write failed during FlashBoot device reset\n");
			return B_ERROR;
		}
	}

	if (fWmxDevice->deviceChipID >= T3LPB) {
		uiResetValue = 0;
			// WA for SYSConfig Issue.
		BizarroReadRegister(SYS_CFG, sizeof(uiResetValue), &uiResetValue);
		if (uiResetValue & (1 << 4)) {
			uiResetValue = 0;
			BizarroReadRegister(SYS_CFG, sizeof(uiResetValue), &uiResetValue);
				// Read SYSCFG Twice to make it writable.
			uiResetValue &= ~(1 << 4);

			if (BizarroWriteRegister(SYS_CFG,
				sizeof(uiResetValue), &uiResetValue) != B_OK) {
				TRACE_ALWAYS("Error: unable to write SYS_CFG during reset\n");
				return B_ERROR;
			}
		}

	}

	uiResetValue = 0;
	if (BizarroWriteRegister(0x0f01186c,
		sizeof(uiResetValue), &uiResetValue) != B_OK) {
		TRACE_ALWAYS("Error: unable to write reset to 0x0f01186c\n");
		return B_ERROR;
	}

	return B_OK;
}

