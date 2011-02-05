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

#include "Settings.h"
#include "BeceemCPU.h"

#include "Driver.h"


BeceemCPU::BeceemCPU()
{
	TRACE("Debug: Load CPU handler\n");
}


status_t
BeceemCPU::CPUInit(WIMAX_DEVICE* swmxdevice)
{
	TRACE("Debug: Init CPU handler\n");
	pwmxdevice = swmxdevice;
	return B_OK;
}


status_t
BeceemCPU::CPURun()
{
	unsigned int clkReg = 0;

	if (BizarroReadRegister(CLOCK_RESET_CNTRL_REG_1, sizeof(clkReg), &clkReg)
			!= B_OK) {

		TRACE_ALWAYS("Error: Read of clock reset reg failure\n");
		return B_ERROR;
	}

	if (pwmxdevice->CPUFlashBoot) {
		clkReg &= (~(1<<30));
	} else {
		clkReg |= (1<<30);
	}

	if (BizarroWriteRegister(CLOCK_RESET_CNTRL_REG_1, sizeof(clkReg), &clkReg)
			!= B_OK) {

		TRACE_ALWAYS("Error: Write of clock reset reg failure\n");
		return B_ERROR;
	}

	return B_OK;
}


status_t
BeceemCPU::CPUReset()
{
	status_t	retval = B_OK;
	unsigned int value = 0;
	unsigned int uiResetValue = 0;

	if (pwmxdevice->deviceChipID >= T3LPB)
	{
		BizarroReadRegister(SYS_CFG, sizeof(value), &value);
		BizarroReadRegister(SYS_CFG, sizeof(value), &value);
			// SYS_CFG register is write protected hence for modifying
			// this reg value, it should be read twice before writing.

		value = value | (pwmxdevice->syscfgBefFw & 0x00000060) ;
			// making bit[6...5] same as was before f/w download. this
			// setting forces the h/w to re-populated the SP RAM area
			// with the string descriptor .

		BizarroWriteRegister(SYS_CFG, sizeof(value), &value);
	}

	/* Reset the UMA-B Device */
	if (pwmxdevice->deviceChipID >= T3LPB)
	{
		// TRACE("Debug: Resetting UMA-B\n");
		// retval = usb_reset_device(psIntfAdapter->udev);

		if (retval != B_OK)
		{
			TRACE_ALWAYS("Error: Reset failed\n");
			goto err_exit;
		}
		if (pwmxdevice->deviceChipID == BCS220_2 ||
			pwmxdevice->deviceChipID == BCS220_2BC ||
			pwmxdevice->deviceChipID == BCS250_BC ||
			pwmxdevice->deviceChipID == BCS220_3)
		{
			retval = BizarroReadRegister(HPM_CONFIG_LDO145,
						sizeof(value), &value);

			if ( retval < 0)
			{
				TRACE_ALWAYS("Error: read failed with status: %d\n", retval);
				goto err_exit;
			}
			// setting 0th bit
			value |= (1<<0);
			retval = BizarroWriteRegister(HPM_CONFIG_LDO145,
				sizeof(value), &value);

			if ( retval < 0)
			{
				TRACE_ALWAYS("Error: write failed with status: %d\n", retval);
				goto err_exit;
			}
		}

	}
	// TODO : ELSE OLDER CHIP ID's < T3LP see Misc.c:1048

	if (pwmxdevice->CPUFlashBoot)
	{
		// In flash boot mode MIPS state register has reverse polarity.
		// So just or with setting bit 30.
		// Make the MIPS in Reset state.
		BizarroReadRegister(CLOCK_RESET_CNTRL_REG_1, sizeof(uiResetValue),
			&uiResetValue);

		uiResetValue |=(1<<30);
		BizarroWriteRegister(CLOCK_RESET_CNTRL_REG_1, sizeof(uiResetValue),
			&uiResetValue);
	}

	if (pwmxdevice->deviceChipID >= T3LPB)
	{
		uiResetValue = 0;
			// WA for SYSConfig Issue.
		BizarroReadRegister(SYS_CFG, sizeof(uiResetValue), &uiResetValue);
		if (uiResetValue & (1<<4))
		{
			uiResetValue = 0;
			BizarroReadRegister(SYS_CFG, sizeof(uiResetValue), &uiResetValue);
				// Read SYSCFG Twice to make it writable.
			uiResetValue &= (~(1<<4));
			BizarroWriteRegister(SYS_CFG, sizeof(uiResetValue), &uiResetValue);
		}

	}
	uiResetValue = 0;
	BizarroWriteRegister(0x0f01186c, sizeof(uiResetValue), &uiResetValue);

	err_exit :
		return retval;
}

