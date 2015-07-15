/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "connector.h"

#include <assert.h>
#include <Debug.h>

#include "accelerant_protos.h"
#include "accelerant.h"
#include "bios.h"
#include "encoder.h"
#include "gpu.h"
#include "utility.h"


#undef TRACE

#define TRACE_CONNECTOR
#ifdef TRACE_CONNECTOR
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


static void
gpio_lock_i2c(void* cookie, bool lock)
{
	gpio_info* info = (gpio_info*)cookie;

	uint32 buffer = 0;

	if (lock == true) {
		// hwCapable and > DCE3
		if (info->i2c.hwCapable == true && gInfo->shared_info->dceMajor >= 3) {
			// Switch GPIO pads to ddc mode
			buffer = Read32(OUT, info->i2c.sclMaskReg);
			buffer &= ~(1 << 16);
			Write32(OUT, info->i2c.sclMaskReg, buffer);
		}

		// Clear pins
		buffer = Read32(OUT, info->i2c.sclAReg) & ~info->i2c.sclAMask;
		Write32(OUT, info->i2c.sclAReg, buffer);
		buffer = Read32(OUT, info->i2c.sdaAReg) & ~info->i2c.sdaAMask;
		Write32(OUT, info->i2c.sdaAReg, buffer);
	}

	// Set pins to input
	buffer = Read32(OUT, info->i2c.sclEnReg) & ~info->i2c.sclEnMask;
	Write32(OUT, info->i2c.sclEnReg, buffer);
	buffer = Read32(OUT, info->i2c.sdaEnReg) & ~info->i2c.sdaEnMask;
	Write32(OUT, info->i2c.sdaEnReg, buffer);

	// mask clock GPIO pins for software use
	buffer = Read32(OUT, info->i2c.sclMaskReg);
	if (lock == true)
		buffer |= info->i2c.sclMask;
	else
		buffer &= ~info->i2c.sclMask;

	Write32(OUT, info->i2c.sclMaskReg, buffer);
	Read32(OUT, info->i2c.sclMaskReg);

	// mask data GPIO pins for software use
	buffer = Read32(OUT, info->i2c.sdaMaskReg);
	if (lock == true)
		buffer |= info->i2c.sdaMask;
	else
		buffer &= ~info->i2c.sdaMask;

	Write32(OUT, info->i2c.sdaMaskReg, buffer);
	Read32(OUT, info->i2c.sdaMaskReg);
}


static status_t
gpio_get_i2c_bit(void* cookie, int* _clock, int* _data)
{
	gpio_info* info = (gpio_info*)cookie;

	uint32 scl = Read32(OUT, info->i2c.sclYReg) & info->i2c.sclYMask;
	uint32 sda = Read32(OUT, info->i2c.sdaYReg) & info->i2c.sdaYMask;

	*_clock = scl != 0;
	*_data = sda != 0;

	return B_OK;
}


static status_t
gpio_set_i2c_bit(void* cookie, int clock, int data)
{
	gpio_info* info = (gpio_info*)cookie;

	uint32 scl = Read32(OUT, info->i2c.sclEnReg) & ~info->i2c.sclEnMask;
	scl |= clock ? 0 : info->i2c.sclEnMask;
	Write32(OUT, info->i2c.sclEnReg, scl);
	Read32(OUT, info->i2c.sclEnReg);

	uint32 sda = Read32(OUT, info->i2c.sdaEnReg) & ~info->i2c.sdaEnMask;
	sda |= data ? 0 : info->i2c.sdaEnMask;
	Write32(OUT, info->i2c.sdaEnReg, sda);
	Read32(OUT, info->i2c.sdaEnReg);

	return B_OK;
}


uint16
connector_pick_atom_hpdid(uint32 connectorIndex)
{
	radeon_shared_info &info = *gInfo->shared_info;

	uint16 atomHPDID = 0xff;
	uint16 hpdPinIndex = gConnector[connectorIndex]->hpdPinIndex;
	if (info.dceMajor >= 4
		&& gGPIOInfo[hpdPinIndex]->valid) {

		// See mmDC_GPIO_HPD_A in drm for register value
		uint32 targetReg = AVIVO_DC_GPIO_HPD_A;
		if (info.dceMajor >= 12) {
			ERROR("WARNING: CHECK NEW DCE mmDC_GPIO_HPD_A value!\n");
			targetReg = CAR_mmDC_GPIO_HPD_A;
		} else if (info.dceMajor >= 11)
			targetReg = CAR_mmDC_GPIO_HPD_A;
		else if (info.dceMajor >= 10)
			targetReg = VOL_mmDC_GPIO_HPD_A;
		else if (info.dceMajor >= 8)
			targetReg = SEA_mmDC_GPIO_HPD_A;
		else if (info.dceMajor >= 6)
			targetReg = SI_DC_GPIO_HPD_A;
		else if (info.dceMajor >= 4)
			targetReg = EVERGREEN_DC_GPIO_HPD_A;

		// You're drunk AMD, go home. (this makes no sense)
		if (gGPIOInfo[hpdPinIndex]->hwReg == targetReg) {
			switch(gGPIOInfo[hpdPinIndex]->hwMask) {
				case (1 << 0):
					atomHPDID = 0;
					break;
				case (1 << 8):
					atomHPDID = 1;
					break;
				case (1 << 16):
					atomHPDID = 2;
					break;
				case (1 << 24):
					atomHPDID = 3;
					break;
				case (1 << 26):
					atomHPDID = 4;
					break;
				case (1 << 28):
					atomHPDID = 5;
					break;
			}
		}
	}
	return atomHPDID;
}


bool
connector_read_edid(uint32 connectorIndex, edid1_info* edid)
{
	// ensure things are sane
	uint32 i2cPinIndex = gConnector[connectorIndex]->i2cPinIndex;
	if (gGPIOInfo[i2cPinIndex]->valid == false
		|| gGPIOInfo[i2cPinIndex]->i2c.valid == false) {
		ERROR("%s: invalid gpio %" B_PRIu32 " for connector %" B_PRIu32 "\n",
			__func__, i2cPinIndex, connectorIndex);
		return false;
	}

	i2c_bus bus;

	ddc2_init_timing(&bus);
	bus.cookie = (void*)gGPIOInfo[i2cPinIndex];
	bus.set_signals = &gpio_set_i2c_bit;
	bus.get_signals = &gpio_get_i2c_bit;

	gpio_lock_i2c(bus.cookie, true);
	status_t edid_result = ddc2_read_edid1(&bus, edid, NULL, NULL);
	gpio_lock_i2c(bus.cookie, false);

	if (edid_result != B_OK)
		return false;

	TRACE("%s: found edid monitor on connector #%" B_PRId32 "\n",
		__func__, connectorIndex);

	return true;
}


bool
connector_read_mode_lvds(uint32 connectorIndex, display_mode* mode)
{
	assert(mode);

	uint8 dceMajor;
	uint8 dceMinor;
	int index = GetIndexIntoMasterTable(DATA, LVDS_Info);
	uint16 offset;

	union atomLVDSInfo {
		struct _ATOM_LVDS_INFO info;
		struct _ATOM_LVDS_INFO_V12 info_12;
	};

	// Wipe out display_mode
	memset(mode, 0, sizeof(display_mode));

	if (atom_parse_data_header(gAtomContext, index, NULL,
		&dceMajor, &dceMinor, &offset) == B_OK) {

		union atomLVDSInfo* lvdsInfo
			= (union atomLVDSInfo*)(gAtomContext->bios + offset);

		display_timing* timing = &mode->timing;

		// Pixel Clock
		timing->pixel_clock
			= B_LENDIAN_TO_HOST_INT16(lvdsInfo->info.sLCDTiming.usPixClk) * 10;
		// Horizontal
		timing->h_display
			= B_LENDIAN_TO_HOST_INT16(lvdsInfo->info.sLCDTiming.usHActive);
		timing->h_total = timing->h_display + B_LENDIAN_TO_HOST_INT16(
			lvdsInfo->info.sLCDTiming.usHBlanking_Time);
		timing->h_sync_start = timing->h_display
			+ B_LENDIAN_TO_HOST_INT16(lvdsInfo->info.sLCDTiming.usHSyncOffset);
		timing->h_sync_end = timing->h_sync_start
			+ B_LENDIAN_TO_HOST_INT16(lvdsInfo->info.sLCDTiming.usHSyncWidth);
		// Vertical
		timing->v_display
			= B_LENDIAN_TO_HOST_INT16(lvdsInfo->info.sLCDTiming.usVActive);
		timing->v_total = timing->v_display + B_LENDIAN_TO_HOST_INT16(
			lvdsInfo->info.sLCDTiming.usVBlanking_Time);
		timing->v_sync_start = timing->v_display
			+ B_LENDIAN_TO_HOST_INT16(lvdsInfo->info.sLCDTiming.usVSyncOffset);
		timing->v_sync_end = timing->v_sync_start
			+ B_LENDIAN_TO_HOST_INT16(lvdsInfo->info.sLCDTiming.usVSyncWidth);

		#if 0
		// Who cares.
		uint32 powerDelay
			= B_LENDIAN_TO_HOST_INT16(lvdsInfo->info.usOffDelayInMs);
		#endif

		// Store special lvds flags the encoder setup needs
		gConnector[connectorIndex]->lvdsFlags = lvdsInfo->info.ucLVDS_Misc;

		// Spread Spectrum ID (in SS table)
		gInfo->lvdsSpreadSpectrumID = lvdsInfo->info.ucSS_Id;

		uint16 flags = B_LENDIAN_TO_HOST_INT16(
			lvdsInfo->info.sLCDTiming.susModeMiscInfo.usAccess);

		if ((flags & ATOM_VSYNC_POLARITY) == 0)
			timing->flags |= B_POSITIVE_VSYNC;
		if ((flags & ATOM_HSYNC_POLARITY) == 0)
			timing->flags |= B_POSITIVE_HSYNC;

		// Extra flags
		if ((flags & ATOM_INTERLACE) != 0)
			timing->flags |= B_TIMING_INTERLACED;

		#if 0
		// We don't use these timing flags at the moment
		if ((flags & ATOM_COMPOSITESYNC) != 0)
			timing->flags |= MODE_FLAG_CSYNC;
		if ((flags & ATOM_DOUBLE_CLOCK_MODE) != 0)
			timing->flags |= MODE_FLAG_DBLSCAN;
		#endif

		mode->h_display_start = 0;
		mode->v_display_start = 0;
		mode->virtual_width = timing->h_display;
		mode->virtual_height = timing->v_display;

		// Assume 32-bit color
		mode->space = B_RGB32_LITTLE;

		TRACE("%s: %" B_PRIu32 " %" B_PRIu16 " %" B_PRIu16 " %" B_PRIu16
			" %" B_PRIu16  " %" B_PRIu16 " %" B_PRIu16 " %" B_PRIu16
			" %" B_PRIu16 "\n", __func__, timing->pixel_clock,
			timing->h_display, timing->h_sync_start, timing->h_sync_end,
			timing->h_total, timing->v_display, timing->v_sync_start,
			timing->v_sync_end, timing->v_total);

		return true;
	}
	return false;
}


static status_t
connector_attach_gpio_i2c(uint32 connectorIndex, uint8 hwPin)
{
	gConnector[connectorIndex]->i2cPinIndex = 0;
	for (uint32 i = 0; i < MAX_GPIO_PINS; i++) {
		if (gGPIOInfo[i]->hwPin != hwPin)
			continue;
		gConnector[connectorIndex]->i2cPinIndex = i;
		return B_OK;
	}

	// We couldnt find the GPIO pin in the known GPIO pins.
    TRACE("%s: can't find GPIO pin 0x%" B_PRIX8 " for connector %" B_PRIu32 "\n",
		__func__, hwPin, connectorIndex);
	return B_ERROR;
}


static status_t
connector_attach_gpio_hpd(uint32 connectorIndex, uint8 hwPin)
{
    gConnector[connectorIndex]->hpdPinIndex = 0;

    for (uint32 i = 0; i < MAX_GPIO_PINS; i++) {
        if (gGPIOInfo[i]->hwPin != hwPin)
            continue;
        gConnector[connectorIndex]->hpdPinIndex = i;
        return B_OK;
    }

	// We couldnt find the GPIO pin in the known GPIO pins.
    TRACE("%s: can't find GPIO pin 0x%" B_PRIX8 " for connector %" B_PRIu32 "\n",
        __func__, hwPin, connectorIndex);
    return B_ERROR;
}


static status_t
gpio_general_populate()
{
	int index = GetIndexIntoMasterTable(DATA, GPIO_Pin_LUT);
	uint16 tableOffset;
	uint16 tableSize;

	struct _ATOM_GPIO_PIN_LUT* gpioInfo;

	if (atom_parse_data_header(gAtomContext, index, &tableSize, NULL, NULL,
		&tableOffset)) {
		ERROR("%s: could't read GPIO_Pin_LUT table from AtomBIOS index %d!\n",
			__func__, index);
	}
	gpioInfo = (struct _ATOM_GPIO_PIN_LUT*)(gAtomContext->bios + tableOffset);

	int numIndices = (tableSize - sizeof(ATOM_COMMON_TABLE_HEADER)) /
		sizeof(ATOM_GPIO_PIN_ASSIGNMENT);
	
	// Find the next available GPIO pin index
	int32 gpioIndex = -1;
	for(int32 pin = 0; pin < MAX_GPIO_PINS; pin++) {
		if (!gGPIOInfo[pin]->valid) {
			gpioIndex = pin;
			break;
		}
	}
	if (gpioIndex < 0) {
		ERROR("%s: ERROR: Out of space for additional GPIO pins!\n", __func__);
		return B_ERROR;
	}

	ATOM_GPIO_PIN_ASSIGNMENT* pin = gpioInfo->asGPIO_Pin;
	for (int i = 0; i < numIndices; i++) {
		if (gGPIOInfo[gpioIndex]->valid) {
			ERROR("%s: BUG: Attempting to fill already populated gpio pin!\n",
				__func__);
			return B_ERROR;
		}
		gGPIOInfo[gpioIndex]->valid = true;
		gGPIOInfo[gpioIndex]->i2c.valid = false;
		gGPIOInfo[gpioIndex]->hwPin = pin->ucGPIO_ID;
		gGPIOInfo[gpioIndex]->hwReg
			= B_LENDIAN_TO_HOST_INT16(pin->usGpioPin_AIndex) * 4;
		gGPIOInfo[gpioIndex]->hwMask
			= (1 << pin->ucGpioPinBitShift);
		pin = (ATOM_GPIO_PIN_ASSIGNMENT*)((uint8*)pin
			+ sizeof(ATOM_GPIO_PIN_ASSIGNMENT));

		TRACE("%s: general GPIO @ %" B_PRId32 ", valid: %s, "
			"hwPin: 0x%" B_PRIX32 "\n", __func__, gpioIndex,
			gGPIOInfo[gpioIndex]->valid ? "true" : "false",
			gGPIOInfo[gpioIndex]->hwPin);

		gpioIndex++;
	}
	return B_OK;
}


static status_t
gpio_i2c_populate()
{
	radeon_shared_info &info = *gInfo->shared_info;

	int index = GetIndexIntoMasterTable(DATA, GPIO_I2C_Info);
	uint16 tableOffset;
	uint16 tableSize;

	if (atom_parse_data_header(gAtomContext, index, &tableSize,
		NULL, NULL, &tableOffset) != B_OK) {
		ERROR("%s: could't read GPIO_I2C_Info table from AtomBIOS index %d!\n",
			__func__, index);
		return B_ERROR;
	}

	struct _ATOM_GPIO_I2C_INFO* i2cInfo
		= (struct _ATOM_GPIO_I2C_INFO*)(gAtomContext->bios + tableOffset);

	uint32 numIndices = (tableSize - sizeof(ATOM_COMMON_TABLE_HEADER))
		/ sizeof(ATOM_GPIO_I2C_ASSIGMENT);

	if (numIndices > ATOM_MAX_SUPPORTED_DEVICE) {
		ERROR("%s: ERROR: AtomBIOS contains more GPIO_Info items then I"
			"was prepared for! (seen: %" B_PRIu32 "; max: %" B_PRIu32 ")\n",
			__func__, numIndices, (uint32)ATOM_MAX_SUPPORTED_DEVICE);
		return B_ERROR;
	}

	// Find the next available GPIO pin index
	int32 gpioIndex = -1;
	for(int32 pin = 0; pin < MAX_GPIO_PINS; pin++) {
		if (!gGPIOInfo[pin]->valid) {
			gpioIndex = pin;
			break;
		}
	}
	if (gpioIndex < 0) {
		ERROR("%s: ERROR: Out of space for additional GPIO pins!\n", __func__);
		return B_ERROR;
	}

	for (uint32 i = 0; i < numIndices; i++) {
		if (gGPIOInfo[gpioIndex]->valid) {
			ERROR("%s: BUG: Attempting to fill already populated gpio pin!\n",
				__func__);
			return B_ERROR;
		}
		ATOM_GPIO_I2C_ASSIGMENT* gpio = &i2cInfo->asGPIO_Info[i];

		if (info.dceMajor >= 3) {
			if (i == 4 && B_LENDIAN_TO_HOST_INT16(gpio->usClkMaskRegisterIndex)
				== 0x1fda && gpio->sucI2cId.ucAccess == 0x94) {
				gpio->sucI2cId.ucAccess = 0x14;
				TRACE("%s: BUG: GPIO override for DCE 3 occured\n", __func__);
			}
		}

		if (info.dceMajor >= 4) {
			if (i == 7 && B_LENDIAN_TO_HOST_INT16(gpio->usClkMaskRegisterIndex)
				== 0x1936 && gpio->sucI2cId.ucAccess == 0) {
				gpio->sucI2cId.ucAccess = 0x97;
				gpio->ucDataMaskShift = 8;
				gpio->ucDataEnShift = 8;
				gpio->ucDataY_Shift = 8;
				gpio->ucDataA_Shift = 8;
				TRACE("%s: BUG: GPIO override for DCE 4 occured\n", __func__);
			}
		}

		// populate gpio information
		gGPIOInfo[gpioIndex]->hwPin = gpio->sucI2cId.ucAccess;
		gGPIOInfo[gpioIndex]->i2c.hwCapable
			= (gpio->sucI2cId.sbfAccess.bfHW_Capable) ? true : false;

		// GPIO mask (Allows software to control the GPIO pad)
		// 0 = chip access; 1 = only software;
		gGPIOInfo[gpioIndex]->i2c.sclMaskReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usClkMaskRegisterIndex) * 4;
		gGPIOInfo[gpioIndex]->i2c.sdaMaskReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usDataMaskRegisterIndex) * 4;
		gGPIOInfo[gpioIndex]->i2c.sclMask = 1 << gpio->ucClkMaskShift;
		gGPIOInfo[gpioIndex]->i2c.sdaMask = 1 << gpio->ucDataMaskShift;

		// GPIO output / write (A) enable
		// 0 = GPIO input (Y); 1 = GPIO output (A);
		gGPIOInfo[gpioIndex]->i2c.sclEnReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usClkEnRegisterIndex) * 4;
		gGPIOInfo[gpioIndex]->i2c.sdaEnReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usDataEnRegisterIndex) * 4;
		gGPIOInfo[gpioIndex]->i2c.sclEnMask = 1 << gpio->ucClkEnShift;
		gGPIOInfo[gpioIndex]->i2c.sdaEnMask = 1 << gpio->ucDataEnShift;

		// GPIO output / write (A)
		gGPIOInfo[gpioIndex]->i2c.sclAReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usClkA_RegisterIndex) * 4;
		gGPIOInfo[gpioIndex]->i2c.sdaAReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usDataA_RegisterIndex) * 4;
		gGPIOInfo[gpioIndex]->i2c.sclAMask = 1 << gpio->ucClkA_Shift;
		gGPIOInfo[gpioIndex]->i2c.sdaAMask = 1 << gpio->ucDataA_Shift;

		// GPIO input / read (Y)
		gGPIOInfo[gpioIndex]->i2c.sclYReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usClkY_RegisterIndex) * 4;
		gGPIOInfo[gpioIndex]->i2c.sdaYReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usDataY_RegisterIndex) * 4;
		gGPIOInfo[gpioIndex]->i2c.sclYMask = 1 << gpio->ucClkY_Shift;
		gGPIOInfo[gpioIndex]->i2c.sdaYMask = 1 << gpio->ucDataY_Shift;

		// ensure data is valid
		gGPIOInfo[gpioIndex]->i2c.valid
			= gGPIOInfo[gpioIndex]->i2c.sclMaskReg ? true : false;
		gGPIOInfo[gpioIndex]->valid = gGPIOInfo[gpioIndex]->i2c.valid;

		TRACE("%s: i2c GPIO @ %" B_PRIu32 ", valid: %s, hwPin: 0x%" B_PRIX32 "\n",
			__func__, gpioIndex, gGPIOInfo[gpioIndex]->valid ? "true" : "false",
			gGPIOInfo[gpioIndex]->hwPin);

		gpioIndex++;
	}

	return B_OK;
}


status_t
gpio_populate()
{
	status_t result = gpio_general_populate();
	if (result != B_OK)
		return result;

	result = gpio_i2c_populate();
	return result;
}


status_t
connector_probe_legacy()
{
	int index = GetIndexIntoMasterTable(DATA, SupportedDevicesInfo);
	uint8 tableMajor;
	uint8 tableMinor;
	uint16 tableSize;
	uint16 tableOffset;

	if (atom_parse_data_header(gAtomContext, index, &tableSize,
		&tableMajor, &tableMinor, &tableOffset) != B_OK) {
		ERROR("%s: unable to parse data header!\n", __func__);
		return B_ERROR;
	}

	union atomSupportedDevices {
		struct _ATOM_SUPPORTED_DEVICES_INFO info;
		struct _ATOM_SUPPORTED_DEVICES_INFO_2 info_2;
		struct _ATOM_SUPPORTED_DEVICES_INFO_2d1 info_2d1;
	};
	union atomSupportedDevices* supportedDevices;
	supportedDevices = (union atomSupportedDevices*)
		(gAtomContext->bios + tableOffset);

	uint16 deviceSupport
		= B_LENDIAN_TO_HOST_INT16(supportedDevices->info.usDeviceSupport);

	uint32 maxDevice;

	if (tableMajor > 1)
		maxDevice = ATOM_MAX_SUPPORTED_DEVICE;
	else
		maxDevice = ATOM_MAX_SUPPORTED_DEVICE_INFO;

	uint32 i;
	uint32 connectorIndex = 0;
	for (i = 0; i < maxDevice; i++) {

		gConnector[connectorIndex]->valid = false;

		// check if this connector is used
		if ((deviceSupport & (1 << i)) == 0)
			continue;

		if (i == ATOM_DEVICE_CV_INDEX) {
			TRACE("%s: skipping component video\n",
				__func__);
			continue;
		}

		ATOM_CONNECTOR_INFO_I2C ci
			= supportedDevices->info.asConnInfo[i];

		gConnector[connectorIndex]->type = kConnectorConvertLegacy[
			ci.sucConnectorInfo.sbfAccess.bfConnectorType];

		if (gConnector[connectorIndex]->type == VIDEO_CONNECTOR_UNKNOWN) {
			TRACE("%s: skipping unknown connector at %" B_PRId32
				" of 0x%" B_PRIX8 "\n", __func__, i,
				ci.sucConnectorInfo.sbfAccess.bfConnectorType);
			continue;
		}

		// TODO: give tv unique connector ids

		// Always set CRT1 and CRT2 as VGA, some cards incorrectly set
		// VGA ports as DVI
		if (i == ATOM_DEVICE_CRT1_INDEX || i == ATOM_DEVICE_CRT2_INDEX)
			gConnector[connectorIndex]->type = VIDEO_CONNECTOR_VGA;

		uint8 dac = ci.sucConnectorInfo.sbfAccess.bfAssociatedDAC;
		uint32 encoderObject = encoder_object_lookup((1 << i), dac);
		uint32 encoderID = (encoderObject & OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;

		gConnector[connectorIndex]->valid = true;
		gConnector[connectorIndex]->flags = (1 << i);
		gConnector[connectorIndex]->encoder.valid = true;
		gConnector[connectorIndex]->encoder.objectID = encoderID;
		gConnector[connectorIndex]->encoder.type
			= encoder_type_lookup(encoderID, (1 << i));

		// TODO: Eval external encoders on legacy connector probe
		gConnector[connectorIndex]->encoderExternal.valid = false;
		// encoder_is_external(encoderID);

		connector_attach_gpio_i2c(connectorIndex, ci.sucI2cId.ucAccess);

		pll_limit_probe(&gConnector[connectorIndex]->encoder.pll);

		connectorIndex++;
	}

	// TODO: combine shared connectors

	if (connectorIndex == 0) {
		TRACE("%s: zero connectors found using legacy detection\n", __func__);
		return B_ERROR;
	}

	return B_OK;
}


// r600+
status_t
connector_probe()
{
	int index = GetIndexIntoMasterTable(DATA, Object_Header);
	uint8 tableMajor;
	uint8 tableMinor;
	uint16 tableSize;
	uint16 tableOffset;

	if (atom_parse_data_header(gAtomContext, index, &tableSize,
		&tableMajor, &tableMinor, &tableOffset) != B_OK) {
		ERROR("%s: ERROR: parsing data header failed!\n", __func__);
		return B_ERROR;
	}

	if (tableMinor < 2) {
		ERROR("%s: ERROR: table minor version unknown! "
			"(%" B_PRIu8 ".%" B_PRIu8 ")\n", __func__, tableMajor, tableMinor);
		return B_ERROR;
	}

	ATOM_CONNECTOR_OBJECT_TABLE* connectorObject;
	ATOM_ENCODER_OBJECT_TABLE* encoderObject;
	ATOM_OBJECT_TABLE* routerObject;
	ATOM_DISPLAY_OBJECT_PATH_TABLE* pathObject;
	ATOM_OBJECT_HEADER* objectHeader;

	objectHeader = (ATOM_OBJECT_HEADER*)(gAtomContext->bios + tableOffset);
	pathObject = (ATOM_DISPLAY_OBJECT_PATH_TABLE*)
		(gAtomContext->bios + tableOffset
		+ B_LENDIAN_TO_HOST_INT16(objectHeader->usDisplayPathTableOffset));
	connectorObject = (ATOM_CONNECTOR_OBJECT_TABLE*)
		(gAtomContext->bios + tableOffset
		+ B_LENDIAN_TO_HOST_INT16(objectHeader->usConnectorObjectTableOffset));
	encoderObject = (ATOM_ENCODER_OBJECT_TABLE*)
		(gAtomContext->bios + tableOffset
		+ B_LENDIAN_TO_HOST_INT16(objectHeader->usEncoderObjectTableOffset));
	routerObject = (ATOM_OBJECT_TABLE*)
		(gAtomContext->bios + tableOffset
		+ B_LENDIAN_TO_HOST_INT16(objectHeader->usRouterObjectTableOffset));
	int deviceSupport = B_LENDIAN_TO_HOST_INT16(objectHeader->usDeviceSupport);

	int pathSize = 0;
	int32 i = 0;

	TRACE("%s: found %" B_PRIu8 " potential display paths.\n", __func__,
		pathObject->ucNumOfDispPath);

	uint32 connectorIndex = 0;
	for (i = 0; i < pathObject->ucNumOfDispPath; i++) {

		if (connectorIndex >= ATOM_MAX_SUPPORTED_DEVICE)
			continue;

		uint8* address = (uint8*)pathObject->asDispPath;
		ATOM_DISPLAY_OBJECT_PATH* path;
		address += pathSize;
		path = (ATOM_DISPLAY_OBJECT_PATH*)address;
		pathSize += B_LENDIAN_TO_HOST_INT16(path->usSize);

		uint32 connectorType;
		uint16 connectorFlags = B_LENDIAN_TO_HOST_INT16(path->usDeviceTag);

		if ((deviceSupport & connectorFlags) != 0) {

			uint16 connectorObjectID
				= (B_LENDIAN_TO_HOST_INT16(path->usConnObjectId)
					& OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;
			//uint8 con_obj_num
			//	= (B_LENDIAN_TO_HOST_INT16(path->usConnObjectId)
			//	& ENUM_ID_MASK) >> ENUM_ID_SHIFT;
			//uint8 con_obj_type
			//	= (B_LENDIAN_TO_HOST_INT16(path->usConnObjectId)
			//	& OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT;

			if (connectorFlags == ATOM_DEVICE_CV_SUPPORT) {
				TRACE("%s: Path #%" B_PRId32 ": skipping component video.\n",
					__func__, i);
				continue;
			}

			radeon_shared_info &info = *gInfo->shared_info;

			uint16 igpLaneInfo;
			if ((info.chipsetFlags & CHIP_IGP) != 0) {
				ERROR("%s: TODO: IGP chip connector detection\n", __func__);
				// try non-IGP method for now
				igpLaneInfo = 0;
				connectorType = kConnectorConvert[connectorObjectID];
			} else {
				igpLaneInfo = 0;
				connectorType = kConnectorConvert[connectorObjectID];
			}

			if (connectorType == VIDEO_CONNECTOR_UNKNOWN) {
				ERROR("%s: Path #%" B_PRId32 ": skipping unknown connector.\n",
					__func__, i);
				continue;
			}

			connector_info* connector = gConnector[connectorIndex];

			int32 j;
			for (j = 0; j < ((B_LENDIAN_TO_HOST_INT16(path->usSize) - 8) / 2);
				j++) {
				//uint16 grph_obj_id
				//	= (B_LENDIAN_TO_HOST_INT16(path->usGraphicObjIds[j])
				//	& OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;
				//uint8 grph_obj_num
				//	= (B_LENDIAN_TO_HOST_INT16(path->usGraphicObjIds[j]) &
				//	ENUM_ID_MASK) >> ENUM_ID_SHIFT;
				uint8 graphicObjectType
					= (B_LENDIAN_TO_HOST_INT16(path->usGraphicObjIds[j]) &
					OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT;

				if (graphicObjectType == GRAPH_OBJECT_TYPE_ENCODER) {
					// Found an encoder
					int32 k;
					for (k = 0; k < encoderObject->ucNumberOfObjects; k++) {
						uint16 encoderObjectRaw
							= B_LENDIAN_TO_HOST_INT16(
							encoderObject->asObjects[k].usObjectID);
						if (B_LENDIAN_TO_HOST_INT16(path->usGraphicObjIds[j])
							== encoderObjectRaw) {
							ATOM_COMMON_RECORD_HEADER* record
								= (ATOM_COMMON_RECORD_HEADER*)
								((uint16*)gAtomContext->bios + tableOffset
								+ B_LENDIAN_TO_HOST_INT16(
								encoderObject->asObjects[k].usRecordOffset));
							ATOM_ENCODER_CAP_RECORD* capRecord;
							uint16 caps = 0;
							while (record->ucRecordSize > 0
								&& record->ucRecordType > 0
								&& record->ucRecordType
								<= ATOM_MAX_OBJECT_RECORD_NUMBER) {
								switch (record->ucRecordType) {
									case ATOM_ENCODER_CAP_RECORD_TYPE:
										capRecord = (ATOM_ENCODER_CAP_RECORD*)
											record;
										caps = B_LENDIAN_TO_HOST_INT16(
											capRecord->usEncoderCap);
										break;
								}
								record = (ATOM_COMMON_RECORD_HEADER*)
									((char*)record + record->ucRecordSize);
							}

							uint32 encoderID
								= (encoderObjectRaw & OBJECT_ID_MASK)
									>> OBJECT_ID_SHIFT;

							uint32 encoderType = encoder_type_lookup(encoderID,
								connectorFlags);

							if (encoderType == VIDEO_ENCODER_NONE) {
								ERROR("%s: Path #%" B_PRId32 ":"
									"skipping unknown encoder.\n",
									__func__, i);
								continue;
							}

							encoder_info* encoder;

							// External encoders are behind DVO or UNIPHY
							if (encoder_is_external(encoderID)) {
								encoder = &connector->encoderExternal;
								encoder->isExternal = true;
								encoder->isDPBridge
									= encoder_is_dp_bridge(encoderID);
							} else {
								encoder = &connector->encoder;
								encoder->isExternal = false;
								encoder->isDPBridge = false;
							}

							// Set up found connector encoder generics
							encoder->valid = true;
							encoder->capabilities = caps;
							encoder->objectID = encoderID;
							encoder->type = encoderType;
							encoder->linkEnumeration
								= (encoderObjectRaw & ENUM_ID_MASK)
									>> ENUM_ID_SHIFT;
							pll_limit_probe(&encoder->pll);
						}
					}
					// END if object is encoder
				} else if (graphicObjectType == GRAPH_OBJECT_TYPE_ROUTER) {
					ERROR("%s: TODO: Found router object?\n", __func__);
				} // END if object is router
			}

			// Set up information buses such as ddc
			if (((connectorFlags & ATOM_DEVICE_TV_SUPPORT) == 0)
				&& (connectorFlags & ATOM_DEVICE_CV_SUPPORT) == 0) {
				for (j = 0; j < connectorObject->ucNumberOfObjects; j++) {
					if (B_LENDIAN_TO_HOST_INT16(path->usConnObjectId)
						== B_LENDIAN_TO_HOST_INT16(
						connectorObject->asObjects[j].usObjectID)) {
						ATOM_COMMON_RECORD_HEADER* record
							= (ATOM_COMMON_RECORD_HEADER*)(gAtomContext->bios
							+ tableOffset + B_LENDIAN_TO_HOST_INT16(
							connectorObject->asObjects[j].usRecordOffset));
						while (record->ucRecordSize > 0
							&& record->ucRecordType > 0
							&& record->ucRecordType
								<= ATOM_MAX_OBJECT_RECORD_NUMBER) {
							ATOM_I2C_RECORD* i2cRecord;
							ATOM_I2C_ID_CONFIG_ACCESS* i2cConfig;
							ATOM_HPD_INT_RECORD* hpdRecord;

							switch (record->ucRecordType) {
								case ATOM_I2C_RECORD_TYPE:
									i2cRecord
										= (ATOM_I2C_RECORD*)record;
									i2cConfig
										= (ATOM_I2C_ID_CONFIG_ACCESS*)
										&i2cRecord->sucI2cId;
									connector_attach_gpio_i2c(connectorIndex,
										i2cConfig->ucAccess);
									break;
								case ATOM_HPD_INT_RECORD_TYPE:
									hpdRecord = (ATOM_HPD_INT_RECORD*)record;
									connector_attach_gpio_hpd(connectorIndex,
										hpdRecord->ucHPDIntGPIOID);
									break;
							}

							// move to next record
							record = (ATOM_COMMON_RECORD_HEADER*)
								((char*)record + record->ucRecordSize);
						}
					}
				}
			}

			connector->valid = true;
			connector->flags = connectorFlags;
			connector->type = connectorType;
			connector->objectID = connectorObjectID;

			connectorIndex++;
		} // END for each valid connector
	} // end for each display path

	return B_OK;
}


bool
connector_is_dp(uint32 connectorIndex)
{
	connector_info* connector = gConnector[connectorIndex];

	// Traditional DisplayPort connector
	if (connector->type == VIDEO_CONNECTOR_DP
		|| connector->type == VIDEO_CONNECTOR_EDP) {
		return true;
	}

	// DisplayPort bridge on external encoder
	if (connector->encoderExternal.valid == true
		&& connector->encoderExternal.isDPBridge == true) {
		return true;
	}

	return false;
}


void
debug_connectors()
{
	ERROR("Currently detected connectors=============\n");
	for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
		if (gConnector[id]->valid == true) {
			uint32 connectorType = gConnector[id]->type;
			uint16 i2cPinIndex = gConnector[id]->i2cPinIndex;
			uint16 hpdPinIndex = gConnector[id]->hpdPinIndex;

			ERROR("Connector #%" B_PRIu32 ")\n", id);
			ERROR(" + connector:          %s\n",
				get_connector_name(connectorType));
			ERROR(" + i2c gpio table id:  %" B_PRIu16 "\n", i2cPinIndex);
			ERROR("   - gpio hw pin:      0x%" B_PRIX32 "\n",
				gGPIOInfo[i2cPinIndex]->hwPin);
			ERROR("   - gpio valid:       %s\n",
				gGPIOInfo[i2cPinIndex]->valid ? "true" : "false");
			ERROR("   - i2c valid:        %s\n",
				gGPIOInfo[i2cPinIndex]->i2c.valid ? "true" : "false");
			ERROR(" + hpd gpio table id:  %" B_PRIu16 "\n", hpdPinIndex);
			ERROR("   - gpio hw pin:      0x%" B_PRIX32 "\n",
				 gGPIOInfo[hpdPinIndex]->hwPin);
			ERROR("   - gpio valid:       %s\n",
				gGPIOInfo[hpdPinIndex]->valid ? "true" : "false");
			encoder_info* encoder = &gConnector[id]->encoder;
			ERROR(" + encoder:            %s\n",
				get_encoder_name(encoder->type));
			ERROR("   - id:               %" B_PRIu16 "\n", encoder->objectID);
			ERROR("   - type:             %s\n",
				encoder_name_lookup(encoder->objectID));
			ERROR("   - capabilities:     0x%" B_PRIX32 "\n",
				encoder->capabilities);
			ERROR("   - enumeration:      %" B_PRIu32 "\n",
				encoder->linkEnumeration);

			encoder = &gConnector[id]->encoderExternal;

			ERROR("   - is bridge:        %s\n",
				encoder->valid ? "true" : "false");

			if (!encoder->valid)
				ERROR("   + external encoder: none\n");
			else {
			ERROR("   + external encoder: %s\n",
					get_encoder_name(encoder->type));
				ERROR("     - valid:          true\n");
				ERROR("     - id:             %" B_PRIu16 "\n",
					encoder->objectID);
				ERROR("     - type:           %s\n",
					encoder_name_lookup(encoder->objectID));
				ERROR("     - enumeration:    %" B_PRIu32 "\n",
					encoder->linkEnumeration);
			}

			uint32 connectorFlags = gConnector[id]->flags;
			bool flags = false;
			ERROR(" + flags:\n");
			if ((connectorFlags & ATOM_DEVICE_CRT1_SUPPORT) != 0) {
				ERROR("   * device CRT1 support\n");
				flags = true;
			}
			if ((connectorFlags & ATOM_DEVICE_CRT2_SUPPORT) != 0) {
				ERROR("   * device CRT2 support\n");
				flags = true;
			}
			if ((connectorFlags & ATOM_DEVICE_LCD1_SUPPORT) != 0) {
				ERROR("   * device LCD1 support\n");
				flags = true;
			}
			if ((connectorFlags & ATOM_DEVICE_LCD2_SUPPORT) != 0) {
				ERROR("   * device LCD2 support\n");
				flags = true;
			}
			if ((connectorFlags & ATOM_DEVICE_TV1_SUPPORT) != 0) {
				ERROR("   * device TV1 support\n");
				flags = true;
			}
			if ((connectorFlags & ATOM_DEVICE_CV_SUPPORT) != 0) {
				ERROR("   * device CV support\n");
				flags = true;
			}
			if ((connectorFlags & ATOM_DEVICE_DFP1_SUPPORT) != 0) {
				ERROR("   * device DFP1 support\n");
				flags = true;
			}
			if ((connectorFlags & ATOM_DEVICE_DFP2_SUPPORT) != 0) {
				ERROR("   * device DFP2 support\n");
				flags = true;
			}
			if ((connectorFlags & ATOM_DEVICE_DFP3_SUPPORT) != 0) {
				ERROR("   * device DFP3 support\n");
				flags = true;
			}
			if ((connectorFlags & ATOM_DEVICE_DFP4_SUPPORT) != 0) {
				ERROR("   * device DFP4 support\n");
				flags = true;
			}
			if ((connectorFlags & ATOM_DEVICE_DFP5_SUPPORT) != 0) {
				ERROR("   * device DFP5 support\n");
				flags = true;
			}
			if ((connectorFlags & ATOM_DEVICE_DFP6_SUPPORT) != 0) {
				ERROR("   * device DFP6 support\n");
				flags = true;
			}
			if (flags == false)
				ERROR("   * no known flags\n");
		}
	}
	ERROR("==========================================\n");
}
