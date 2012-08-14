/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "connector.h"

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
		if (info->hwCapable == true && gInfo->shared_info->dceMajor >= 3) {
			// Switch GPIO pads to ddc mode
			buffer = Read32(OUT, info->sclMaskReg);
			buffer &= ~(1 << 16);
			Write32(OUT, info->sclMaskReg, buffer);
		}

		// Clear pins
		buffer = Read32(OUT, info->sclAReg) & ~info->sclAMask;
		Write32(OUT, info->sclAReg, buffer);
		buffer = Read32(OUT, info->sdaAReg) & ~info->sdaAMask;
		Write32(OUT, info->sdaAReg, buffer);
	}

	// Set pins to input
	buffer = Read32(OUT, info->sclEnReg) & ~info->sclEnMask;
	Write32(OUT, info->sclEnReg, buffer);
	buffer = Read32(OUT, info->sdaEnReg) & ~info->sdaEnMask;
	Write32(OUT, info->sdaEnReg, buffer);

	// mask clock GPIO pins for software use
	buffer = Read32(OUT, info->sclMaskReg);
	if (lock == true)
		buffer |= info->sclMask;
	else
		buffer &= ~info->sclMask;

	Write32(OUT, info->sclMaskReg, buffer);
	Read32(OUT, info->sclMaskReg);

	// mask data GPIO pins for software use
	buffer = Read32(OUT, info->sdaMaskReg);
	if (lock == true)
		buffer |= info->sdaMask;
	else
		buffer &= ~info->sdaMask;

	Write32(OUT, info->sdaMaskReg, buffer);
	Read32(OUT, info->sdaMaskReg);
}


static status_t
gpio_get_i2c_bit(void* cookie, int* _clock, int* _data)
{
	gpio_info* info = (gpio_info*)cookie;

	uint32 scl = Read32(OUT, info->sclYReg) & info->sclYMask;
	uint32 sda = Read32(OUT, info->sdaYReg) & info->sdaYMask;

	*_clock = scl != 0;
	*_data = sda != 0;

	return B_OK;
}


static status_t
gpio_set_i2c_bit(void* cookie, int clock, int data)
{
	gpio_info* info = (gpio_info*)cookie;

	uint32 scl = Read32(OUT, info->sclEnReg) & ~info->sclEnMask;
	scl |= clock ? 0 : info->sclEnMask;
	Write32(OUT, info->sclEnReg, scl);
	Read32(OUT, info->sclEnReg);

	uint32 sda = Read32(OUT, info->sdaEnReg) & ~info->sdaEnMask;
	sda |= data ? 0 : info->sdaEnMask;
	Write32(OUT, info->sdaEnReg, sda);
	Read32(OUT, info->sdaEnReg);

	return B_OK;
}


bool
connector_read_edid(uint32 connectorIndex, edid1_info* edid)
{
	// ensure things are sane
	uint32 gpioID = gConnector[connectorIndex]->gpioID;
	if (gGPIOInfo[gpioID]->valid == false) {
		ERROR("%s: invalid gpio %" B_PRIu32 " for connector %" B_PRIu32 "\n",
			__func__, gpioID, connectorIndex);
		return false;
	}

	i2c_bus bus;

	ddc2_init_timing(&bus);
	bus.cookie = (void*)gGPIOInfo[gpioID];
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
	uint8 dceMajor;
	uint8 dceMinor;
	int index = GetIndexIntoMasterTable(DATA, LVDS_Info);
	uint16 offset;

	union atomLVDSInfo {
		struct _ATOM_LVDS_INFO info;
		struct _ATOM_LVDS_INFO_V12 info_12;
	};

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


status_t
connector_attach_gpio(uint32 connectorIndex, uint8 hwPin)
{
	gConnector[connectorIndex]->gpioID = 0;
	for (uint32 i = 0; i < ATOM_MAX_SUPPORTED_DEVICE; i++) {
		if (gGPIOInfo[i]->hwPin != hwPin)
			continue;
		gConnector[connectorIndex]->gpioID = i;
		return B_OK;
	}

	TRACE("%s: couldn't find GPIO for connector %" B_PRIu32 "\n",
		__func__, connectorIndex);
	return B_ERROR;
}


status_t
gpio_probe()
{
	radeon_shared_info &info = *gInfo->shared_info;

	int index = GetIndexIntoMasterTable(DATA, GPIO_I2C_Info);

	uint8 tableMajor;
	uint8 tableMinor;
	uint16 tableOffset;
	uint16 tableSize;

	if (atom_parse_data_header(gAtomContext, index, &tableSize,
		&tableMajor, &tableMinor, &tableOffset) != B_OK) {
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

	for (uint32 i = 0; i < numIndices; i++) {
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
		gGPIOInfo[i]->hwPin = gpio->sucI2cId.ucAccess;
		gGPIOInfo[i]->hwCapable
			= (gpio->sucI2cId.sbfAccess.bfHW_Capable) ? true : false;

		// GPIO mask (Allows software to control the GPIO pad)
		// 0 = chip access; 1 = only software;
		gGPIOInfo[i]->sclMaskReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usClkMaskRegisterIndex) * 4;
		gGPIOInfo[i]->sdaMaskReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usDataMaskRegisterIndex) * 4;
		gGPIOInfo[i]->sclMask = 1 << gpio->ucClkMaskShift;
		gGPIOInfo[i]->sdaMask = 1 << gpio->ucDataMaskShift;

		// GPIO output / write (A) enable
		// 0 = GPIO input (Y); 1 = GPIO output (A);
		gGPIOInfo[i]->sclEnReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usClkEnRegisterIndex) * 4;
		gGPIOInfo[i]->sdaEnReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usDataEnRegisterIndex) * 4;
		gGPIOInfo[i]->sclEnMask = 1 << gpio->ucClkEnShift;
		gGPIOInfo[i]->sdaEnMask = 1 << gpio->ucDataEnShift;

		// GPIO output / write (A)
		gGPIOInfo[i]->sclAReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usClkA_RegisterIndex) * 4;
		gGPIOInfo[i]->sdaAReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usDataA_RegisterIndex) * 4;
		gGPIOInfo[i]->sclAMask = 1 << gpio->ucClkA_Shift;
		gGPIOInfo[i]->sdaAMask = 1 << gpio->ucDataA_Shift;

		// GPIO input / read (Y)
		gGPIOInfo[i]->sclYReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usClkY_RegisterIndex) * 4;
		gGPIOInfo[i]->sdaYReg
			= B_LENDIAN_TO_HOST_INT16(gpio->usDataY_RegisterIndex) * 4;
		gGPIOInfo[i]->sclYMask = 1 << gpio->ucClkY_Shift;
		gGPIOInfo[i]->sdaYMask = 1 << gpio->ucDataY_Shift;

		// ensure data is valid
		gGPIOInfo[i]->valid = gGPIOInfo[i]->sclMaskReg ? true : false;

		TRACE("%s: GPIO @ %" B_PRIu32 ", valid: %s, hwPin: 0x%" B_PRIX32 "\n",
			__func__, i, gGPIOInfo[i]->valid ? "true" : "false",
			gGPIOInfo[i]->hwPin);
	}

	return B_OK;
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
		gConnector[connectorIndex]->encoder.flags = (1 << i);
		gConnector[connectorIndex]->encoder.valid = true;
		gConnector[connectorIndex]->encoder.objectID = encoderID;
		gConnector[connectorIndex]->encoder.type
			= encoder_type_lookup(encoderID, (1 << i));

		// TODO: Eval external encoders on legacy connector probe
		gConnector[connectorIndex]->encoderExternal.valid = false;
		// encoder_is_external(encoderID);

		connector_attach_gpio(connectorIndex, ci.sucI2cId.ucAccess);

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

							// External encoders are behind DVO or UNIPHY
							if (encoder_is_external(encoderID)) {
								encoder_info* encoder
									= &connector->encoderExternal;
								encoder->isExternal = true;

								// Set up found connector
								encoder->valid = true;
								encoder->flags = connectorFlags;
								encoder->objectID = encoderID;
								encoder->type = encoderType;
								encoder->linkEnumeration
									= (encoderObjectRaw & ENUM_ID_MASK)
										>> ENUM_ID_SHIFT;
								encoder->isDPBridge
									= encoder_is_dp_bridge(encoderID);

								pll_limit_probe(&encoder->pll);
							} else {
								encoder_info* encoder
									= &connector->encoder;
								encoder->isExternal = false;

								// Set up found connector
								encoder->valid = true;
								encoder->flags = connectorFlags;
								encoder->objectID = encoderID;
								encoder->type = encoderType;
								encoder->linkEnumeration
									= (encoderObjectRaw & ENUM_ID_MASK)
										>> ENUM_ID_SHIFT;
								encoder->isDPBridge = false;

								pll_limit_probe(&encoder->pll);
							}
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
							//ATOM_HPD_INT_RECORD* hpd_record;

							switch (record->ucRecordType) {
								case ATOM_I2C_RECORD_TYPE:
									i2cRecord
										= (ATOM_I2C_RECORD*)record;
									i2cConfig
										= (ATOM_I2C_ID_CONFIG_ACCESS*)
										&i2cRecord->sucI2cId;
									// attach i2c gpio information for connector
									connector_attach_gpio(connectorIndex,
										i2cConfig->ucAccess);
									break;
								case ATOM_HPD_INT_RECORD_TYPE:
									// TODO: HPD (Hot Plug)
									break;
							}

							// move to next record
							record = (ATOM_COMMON_RECORD_HEADER*)
								((char*)record + record->ucRecordSize);
						}
					}
				}
			}

			// TODO: aux chan transactions

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
			uint16 gpioID = gConnector[id]->gpioID;

			ERROR("Connector #%" B_PRIu32 ")\n", id);
			ERROR(" + connector:          %s\n",
				get_connector_name(connectorType));
			ERROR(" + gpio table id:      %" B_PRIu16 "\n", gpioID);
			ERROR(" + gpio hw pin:        0x%" B_PRIX32 "\n",
				gGPIOInfo[gpioID]->hwPin);
			ERROR(" + gpio valid:         %s\n",
				gGPIOInfo[gpioID]->valid ? "true" : "false");

			encoder_info* encoder = &gConnector[id]->encoder;
			ERROR(" + encoder:            %s\n",
				get_encoder_name(encoder->type));
			ERROR("   - id:               %" B_PRIu16 "\n", encoder->objectID);
			ERROR("   - type:             %s\n",
				encoder_name_lookup(encoder->objectID));
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

			uint32 encoderFlags = gConnector[id]->encoder.flags;
			bool flags = false;
			ERROR(" + flags:\n");
			if ((encoderFlags & ATOM_DEVICE_CRT1_SUPPORT) != 0) {
				ERROR("   * device CRT1 support\n");
				flags = true;
			}
			if ((encoderFlags & ATOM_DEVICE_CRT2_SUPPORT) != 0) {
				ERROR("   * device CRT2 support\n");
				flags = true;
			}
			if ((encoderFlags & ATOM_DEVICE_LCD1_SUPPORT) != 0) {
				ERROR("   * device LCD1 support\n");
				flags = true;
			}
			if ((encoderFlags & ATOM_DEVICE_LCD2_SUPPORT) != 0) {
				ERROR("   * device LCD2 support\n");
				flags = true;
			}
			if ((encoderFlags & ATOM_DEVICE_TV1_SUPPORT) != 0) {
				ERROR("   * device TV1 support\n");
				flags = true;
			}
			if ((encoderFlags & ATOM_DEVICE_CV_SUPPORT) != 0) {
				ERROR("   * device CV support\n");
				flags = true;
			}
			if ((encoderFlags & ATOM_DEVICE_DFP1_SUPPORT) != 0) {
				ERROR("   * device DFP1 support\n");
				flags = true;
			}
			if ((encoderFlags & ATOM_DEVICE_DFP2_SUPPORT) != 0) {
				ERROR("   * device DFP2 support\n");
				flags = true;
			}
			if ((encoderFlags & ATOM_DEVICE_DFP3_SUPPORT) != 0) {
				ERROR("   * device DFP3 support\n");
				flags = true;
			}
			if ((encoderFlags & ATOM_DEVICE_DFP4_SUPPORT) != 0) {
				ERROR("   * device DFP4 support\n");
				flags = true;
			}
			if ((encoderFlags & ATOM_DEVICE_DFP5_SUPPORT) != 0) {
				ERROR("   * device DFP5 support\n");
				flags = true;
			}
			if ((encoderFlags & ATOM_DEVICE_DFP6_SUPPORT) != 0) {
				ERROR("   * device DFP6 support\n");
				flags = true;
			}
			if (flags == false)
				ERROR("   * no known flags\n");
		}
	}
	ERROR("==========================================\n");
}
