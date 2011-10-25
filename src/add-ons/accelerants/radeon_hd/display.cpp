/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */

/*
 * It's dangerous to go alone, take this!
 *	framebuffer -> crtc -> encoder -> transmitter -> connector -> monitor
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "bios.h"
#include "display.h"

#include <stdlib.h>
#include <string.h>


#define TRACE_DISPLAY
#ifdef TRACE_DISPLAY
extern "C" void _sPrintf(const char *format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


/*! Populate regs with device dependant register locations */
status_t
init_registers(register_info* regs, uint8 crtid)
{
	memset(regs, 0, sizeof(register_info));

	radeon_shared_info &info = *gInfo->shared_info;

	if (info.device_chipset >= RADEON_R1000) {
		uint32 offset = 0;

		// AMD Eyefinity on Evergreen GPUs
		if (crtid == 1) {
			offset = EVERGREEN_CRTC1_REGISTER_OFFSET;
			regs->vgaControl = D2VGA_CONTROL;
		} else if (crtid == 2) {
			offset = EVERGREEN_CRTC2_REGISTER_OFFSET;
			regs->vgaControl = EVERGREEN_D3VGA_CONTROL;
		} else if (crtid == 3) {
			offset = EVERGREEN_CRTC3_REGISTER_OFFSET;
			regs->vgaControl = EVERGREEN_D4VGA_CONTROL;
		} else if (crtid == 4) {
			offset = EVERGREEN_CRTC4_REGISTER_OFFSET;
			regs->vgaControl = EVERGREEN_D5VGA_CONTROL;
		} else if (crtid == 5) {
			offset = EVERGREEN_CRTC5_REGISTER_OFFSET;
			regs->vgaControl = EVERGREEN_D6VGA_CONTROL;
		} else {
			offset = EVERGREEN_CRTC0_REGISTER_OFFSET;
			regs->vgaControl = D1VGA_CONTROL;
		}

		regs->crtcOffset = offset;

		// Evergreen+ is crtoffset + register
		regs->grphEnable = offset + EVERGREEN_GRPH_ENABLE;
		regs->grphControl = offset + EVERGREEN_GRPH_CONTROL;
		regs->grphSwapControl = offset + EVERGREEN_GRPH_SWAP_CONTROL;

		regs->grphPrimarySurfaceAddr
			= offset + EVERGREEN_GRPH_PRIMARY_SURFACE_ADDRESS;
		regs->grphSecondarySurfaceAddr
			= offset + EVERGREEN_GRPH_SECONDARY_SURFACE_ADDRESS;
		regs->grphPrimarySurfaceAddrHigh
			= offset + EVERGREEN_GRPH_PRIMARY_SURFACE_ADDRESS_HIGH;
		regs->grphSecondarySurfaceAddrHigh
			= offset + EVERGREEN_GRPH_SECONDARY_SURFACE_ADDRESS_HIGH;

		regs->grphPitch = offset + EVERGREEN_GRPH_PITCH;
		regs->grphSurfaceOffsetX
			= offset + EVERGREEN_GRPH_SURFACE_OFFSET_X;
		regs->grphSurfaceOffsetY
			= offset + EVERGREEN_GRPH_SURFACE_OFFSET_Y;
		regs->grphXStart = offset + EVERGREEN_GRPH_X_START;
		regs->grphYStart = offset + EVERGREEN_GRPH_Y_START;
		regs->grphXEnd = offset + EVERGREEN_GRPH_X_END;
		regs->grphYEnd = offset + EVERGREEN_GRPH_Y_END;
		regs->crtControl = offset + EVERGREEN_CRTC_CONTROL;
		regs->modeDesktopHeight = offset + EVERGREEN_DESKTOP_HEIGHT;
		regs->modeDataFormat = offset + EVERGREEN_DATA_FORMAT;
		regs->viewportStart = offset + EVERGREEN_VIEWPORT_START;
		regs->viewportSize = offset + EVERGREEN_VIEWPORT_SIZE;

	} else if (info.device_chipset >= RADEON_R600
		&& info.device_chipset < RADEON_R1000) {

		// r600 - r700 are D1 or D2 based on primary / secondary crt
		regs->vgaControl
			= crtid == 1 ? D2VGA_CONTROL : D1VGA_CONTROL;
		regs->grphEnable
			= crtid == 1 ? D2GRPH_ENABLE : D1GRPH_ENABLE;
		regs->grphControl
			= crtid == 1 ? D2GRPH_CONTROL : D1GRPH_CONTROL;
		regs->grphSwapControl
			= crtid == 1 ? D2GRPH_SWAP_CNTL : D1GRPH_SWAP_CNTL;
		regs->grphPrimarySurfaceAddr
			= crtid == 1 ? D2GRPH_PRIMARY_SURFACE_ADDRESS
				: D1GRPH_PRIMARY_SURFACE_ADDRESS;
		regs->grphSecondarySurfaceAddr
			= crtid == 1 ? D2GRPH_SECONDARY_SURFACE_ADDRESS
				: D1GRPH_SECONDARY_SURFACE_ADDRESS;

		regs->crtcOffset
			= crtid == 1 ? (D2GRPH_X_END - D1GRPH_X_END) : 0;

		// Surface Address high only used on r770+
		regs->grphPrimarySurfaceAddrHigh
			= crtid == 1 ? D2GRPH_PRIMARY_SURFACE_ADDRESS_HIGH
				: D1GRPH_PRIMARY_SURFACE_ADDRESS_HIGH;
		regs->grphSecondarySurfaceAddrHigh
			= crtid == 1 ? D2GRPH_SECONDARY_SURFACE_ADDRESS_HIGH
				: D1GRPH_SECONDARY_SURFACE_ADDRESS_HIGH;

		regs->grphPitch
			= crtid == 1 ? D2GRPH_PITCH : D1GRPH_PITCH;
		regs->grphSurfaceOffsetX
			= crtid == 1 ? D2GRPH_SURFACE_OFFSET_X : D1GRPH_SURFACE_OFFSET_X;
		regs->grphSurfaceOffsetY
			= crtid == 1 ? D2GRPH_SURFACE_OFFSET_Y : D1GRPH_SURFACE_OFFSET_Y;
		regs->grphXStart
			= crtid == 1 ? D2GRPH_X_START : D1GRPH_X_START;
		regs->grphYStart
			= crtid == 1 ? D2GRPH_Y_START : D1GRPH_Y_START;
		regs->grphXEnd
			= crtid == 1 ? D2GRPH_X_END : D1GRPH_X_END;
		regs->grphYEnd
			= crtid == 1 ? D2GRPH_Y_END : D1GRPH_Y_END;
		regs->crtControl
			= crtid == 1 ? D2CRTC_CONTROL : D1CRTC_CONTROL;
		regs->modeDesktopHeight
			= crtid == 1 ? D2MODE_DESKTOP_HEIGHT : D1MODE_DESKTOP_HEIGHT;
		regs->modeDataFormat
			= crtid == 1 ? D2MODE_DATA_FORMAT : D1MODE_DATA_FORMAT;
		regs->viewportStart
			= crtid == 1 ? D2MODE_VIEWPORT_START : D1MODE_VIEWPORT_START;
		regs->viewportSize
			= crtid == 1 ? D2MODE_VIEWPORT_SIZE : D1MODE_VIEWPORT_SIZE;
	} else {
		// this really shouldn't happen unless a driver PCIID chipset is wrong
		TRACE("%s, unknown Radeon chipset: r%X\n", __func__,
			info.device_chipset);
		return B_ERROR;
	}

	// Populate common registers
	// TODO: Wait.. this doesn't work with Eyefinity > crt 1.

	regs->modeCenter
		= crtid == 1 ? D2MODE_CENTER : D1MODE_CENTER;
	regs->grphUpdate
		= crtid == 1 ? D2GRPH_UPDATE : D1GRPH_UPDATE;
	regs->crtHPolarity
		= crtid == 1 ? D2CRTC_H_SYNC_A_CNTL : D1CRTC_H_SYNC_A_CNTL;
	regs->crtVPolarity
		= crtid == 1 ? D2CRTC_V_SYNC_A_CNTL : D1CRTC_V_SYNC_A_CNTL;
	regs->crtHTotal
		= crtid == 1 ? D2CRTC_H_TOTAL : D1CRTC_H_TOTAL;
	regs->crtVTotal
		= crtid == 1 ? D2CRTC_V_TOTAL : D1CRTC_V_TOTAL;
	regs->crtHSync
		= crtid == 1 ? D2CRTC_H_SYNC_A : D1CRTC_H_SYNC_A;
	regs->crtVSync
		= crtid == 1 ? D2CRTC_V_SYNC_A : D1CRTC_V_SYNC_A;
	regs->crtHBlank
		= crtid == 1 ? D2CRTC_H_BLANK_START_END : D1CRTC_H_BLANK_START_END;
	regs->crtVBlank
		= crtid == 1 ? D2CRTC_V_BLANK_START_END : D1CRTC_V_BLANK_START_END;
	regs->crtInterlace
		= crtid == 1 ? D2CRTC_INTERLACE_CONTROL : D1CRTC_INTERLACE_CONTROL;
	regs->crtCountControl
		= crtid == 1 ? D2CRTC_COUNT_CONTROL : D1CRTC_COUNT_CONTROL;
	regs->sclUpdate
		= crtid == 1 ? D2SCL_UPDATE : D1SCL_UPDATE;
	regs->sclEnable
		= crtid == 1 ? D2SCL_ENABLE : D1SCL_ENABLE;
	regs->sclTapControl
		= crtid == 1 ? D2SCL_TAP_CONTROL : D1SCL_TAP_CONTROL;

	TRACE("%s, registers for ATI chipset r%X crt #%d loaded\n", __func__,
		info.device_chipset, crtid);

	return B_OK;
}


status_t
detect_crt_ranges(uint32 crtid)
{
	edid1_info *edid = &gDisplay[crtid]->edid_info;

	// Scan each display EDID description for monitor ranges
	for (uint32 index = 0; index < EDID1_NUM_DETAILED_MONITOR_DESC; index++) {

		edid1_detailed_monitor *monitor
			= &edid->detailed_monitor[index];

		if (monitor->monitor_desc_type
			== EDID1_MONITOR_RANGES) {
			edid1_monitor_range range = monitor->data.monitor_range;
			gDisplay[crtid]->vfreq_min = range.min_v;   /* in Hz */
			gDisplay[crtid]->vfreq_max = range.max_v;
			gDisplay[crtid]->hfreq_min = range.min_h;   /* in kHz */
			gDisplay[crtid]->hfreq_max = range.max_h;
			return B_OK;
		}
	}

	return B_ERROR;
}


// TODO: only used on r4xx, r5xx, and rs600/rs690/rs740
#if 0
union atom_supported_devices {
	struct _ATOM_SUPPORTED_DEVICES_INFO info;
	struct _ATOM_SUPPORTED_DEVICES_INFO_2 info_2;
	struct _ATOM_SUPPORTED_DEVICES_INFO_2d1 info_2d1;
};


status_t
detect_connectors_legacy()
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

	union atom_supported_devices *supported_devices;
	supported_devices
		= (union atom_supported_devices *)
		(gAtomContext->bios + tableOffset);

	uint16 device_support
		= B_LENDIAN_TO_HOST_INT16(supported_devices->info.usDeviceSupport);

	int32 i;
	for (i = 0; i < ATOM_MAX_SUPPORTED_DEVICE; i++) {

		gConnector[i]->valid = false;

		// check if this connector is used
		if ((device_support & (1 << i)) == 0)
			continue;

		if (i == ATOM_DEVICE_CV_INDEX) {
			TRACE("%s: skipping component video\n",
				__func__);
			continue;
		}

		ATOM_CONNECTOR_INFO_I2C ci
			= supported_devices->info.asConnInfo[i];

		gConnector[i]->type
			= connector_convert_legacy[
				ci.sucConnectorInfo.sbfAccess.bfConnectorType];

		if (gConnector[i]->type == VIDEO_CONNECTOR_UNKNOWN) {
			TRACE("%s: skipping unknown connector at %" B_PRId32
				" of 0x%" B_PRIX8 "\n", __func__, i,
				ci.sucConnectorInfo.sbfAccess.bfConnectorType);
			continue;
		}

		// uint8 dac = ci.sucConnectorInfo.sbfAccess.bfAssociatedDAC;
		// gConnector[i]->line_mux = ci.sucI2cId.ucAccess;

		// TODO: give tv unique connector ids

		// Always set CRT1 and CRT2 as VGA, some cards incorrectly set
		// VGA ports as DVI
		if (i == ATOM_DEVICE_CRT1_INDEX || i == ATOM_DEVICE_CRT2_INDEX)
			gConnector[i]->type = VIDEO_CONNECTOR_VGA;

		gConnector[i]->valid = true;
		gConnector[i]->encoder.flags = (1 << i);

		// TODO: add the encoder
		#if 0
		radeon_add_atom_encoder(dev,
			radeon_get_encoder_enum(dev,
				(1 << i),
				dac),
			(1 << i),
			0);
		#endif
	}

	// TODO: combine shared connectors

	// TODO: add connectors

	for (i = 0; i < ATOM_MAX_SUPPORTED_DEVICE_INFO; i++) {
		if (gConnector[i]->valid == true) {
			TRACE("%s: connector #%" B_PRId32 " is %s\n", __func__, i,
				get_connector_name(gConnector[i]->type));
		}
	}

	return B_OK;
}
#endif


// r600+
status_t
detect_connectors()
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

	ATOM_CONNECTOR_OBJECT_TABLE *con_obj;
	ATOM_ENCODER_OBJECT_TABLE *enc_obj;
	ATOM_OBJECT_TABLE *router_obj;
	ATOM_DISPLAY_OBJECT_PATH_TABLE *path_obj;
	ATOM_OBJECT_HEADER *obj_header;

	obj_header = (ATOM_OBJECT_HEADER *)(gAtomContext->bios + tableOffset);
	path_obj = (ATOM_DISPLAY_OBJECT_PATH_TABLE *)
		(gAtomContext->bios + tableOffset
		+ B_LENDIAN_TO_HOST_INT16(obj_header->usDisplayPathTableOffset));
	con_obj = (ATOM_CONNECTOR_OBJECT_TABLE *)
		(gAtomContext->bios + tableOffset
		+ B_LENDIAN_TO_HOST_INT16(obj_header->usConnectorObjectTableOffset));
	enc_obj = (ATOM_ENCODER_OBJECT_TABLE *)
		(gAtomContext->bios + tableOffset
		+ B_LENDIAN_TO_HOST_INT16(obj_header->usEncoderObjectTableOffset));
	router_obj = (ATOM_OBJECT_TABLE *)
		(gAtomContext->bios + tableOffset
		+ B_LENDIAN_TO_HOST_INT16(obj_header->usRouterObjectTableOffset));
	int deviceSupport = B_LENDIAN_TO_HOST_INT16(obj_header->usDeviceSupport);

	int pathSize = 0;
	int32 i = 0;

	TRACE("%s: found %" B_PRIu8 " potential display paths.\n", __func__,
		path_obj->ucNumOfDispPath);

	uint32 connectorIndex = 0;
	for (i = 0; i < path_obj->ucNumOfDispPath; i++) {

		if (connectorIndex >= ATOM_MAX_SUPPORTED_DEVICE)
			continue;

		uint8 *addr = (uint8*)path_obj->asDispPath;
		ATOM_DISPLAY_OBJECT_PATH *path;
		addr += pathSize;
		path = (ATOM_DISPLAY_OBJECT_PATH *)addr;
		pathSize += B_LENDIAN_TO_HOST_INT16(path->usSize);

		uint32 connectorType;
		uint16 connectorObjectID;
		uint16 connectorFlags = B_LENDIAN_TO_HOST_INT16(path->usDeviceTag);

		if ((deviceSupport & connectorFlags) != 0) {
			uint8 con_obj_id = (B_LENDIAN_TO_HOST_INT16(path->usConnObjectId)
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

			uint16 igp_lane_info;
			if (0)
				ERROR("%s: TODO: IGP chip connector detection\n", __func__);
			else {
				igp_lane_info = 0;
				connectorType = connector_convert[con_obj_id];
				connectorObjectID = con_obj_id;
			}

			if (connectorType == VIDEO_CONNECTOR_UNKNOWN) {
				ERROR("%s: Path #%" B_PRId32 ": skipping unknown connector.\n",
					__func__, i);
				continue;
			}

			int32 j;
			for (j = 0; j < ((B_LENDIAN_TO_HOST_INT16(path->usSize) - 8) / 2);
				j++) {
				//uint16 grph_obj_id
				//	= (B_LENDIAN_TO_HOST_INT16(path->usGraphicObjIds[j])
				//	& OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;
				//uint8 grph_obj_num
				//	= (B_LENDIAN_TO_HOST_INT16(path->usGraphicObjIds[j]) &
				//	ENUM_ID_MASK) >> ENUM_ID_SHIFT;
				uint8 grph_obj_type
					= (B_LENDIAN_TO_HOST_INT16(path->usGraphicObjIds[j]) &
					OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT;

				if (grph_obj_type == GRAPH_OBJECT_TYPE_ENCODER) {
					// Found an encoder
					// TODO: it may be possible to have more then one encoder
					int32 k;
					for (k = 0; k < enc_obj->ucNumberOfObjects; k++) {
						uint16 encoder_obj
							= B_LENDIAN_TO_HOST_INT16(
							enc_obj->asObjects[k].usObjectID);
						if (B_LENDIAN_TO_HOST_INT16(path->usGraphicObjIds[j])
							== encoder_obj) {
							ATOM_COMMON_RECORD_HEADER *record
								= (ATOM_COMMON_RECORD_HEADER *)
								((uint16 *)gAtomContext->bios + tableOffset
								+ B_LENDIAN_TO_HOST_INT16(
								enc_obj->asObjects[k].usRecordOffset));
							ATOM_ENCODER_CAP_RECORD *cap_record;
							uint16 caps = 0;
							while (record->ucRecordSize > 0
								&& record->ucRecordType > 0
								&& record->ucRecordType
								<= ATOM_MAX_OBJECT_RECORD_NUMBER) {
								switch (record->ucRecordType) {
									case ATOM_ENCODER_CAP_RECORD_TYPE:
										cap_record = (ATOM_ENCODER_CAP_RECORD *)
											record;
										caps = B_LENDIAN_TO_HOST_INT16(
											cap_record->usEncoderCap);
										break;
								}
								record = (ATOM_COMMON_RECORD_HEADER *)
									((char *)record + record->ucRecordSize);
							}
							uint32 encoderID = (encoder_obj & OBJECT_ID_MASK)
								>> OBJECT_ID_SHIFT;

							uint32 encoderType = VIDEO_ENCODER_NONE;
							bool encoderExternal = false;

							switch(encoderID) {
								case ENCODER_OBJECT_ID_INTERNAL_LVDS:
								case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
								case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
								case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
									if ((connectorFlags
										& ATOM_DEVICE_LCD_SUPPORT) != 0) {
										encoderType = VIDEO_ENCODER_LVDS;
										// radeon_atombios_get_lvds_info
									} else {
										encoderType = VIDEO_ENCODER_TMDS;
										// radeon_atombios_set_dig_info
									}
									break;
								case ENCODER_OBJECT_ID_INTERNAL_DAC1:
									encoderType = VIDEO_ENCODER_DAC;
									break;
								case ENCODER_OBJECT_ID_INTERNAL_DAC2:
								case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
								case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
									encoderType = VIDEO_ENCODER_TVDAC;
									break;
								case ENCODER_OBJECT_ID_INTERNAL_DVO1:
								case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
								case ENCODER_OBJECT_ID_INTERNAL_DDI:
								case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
								case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
								case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
								case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
									if ((connectorFlags
										& ATOM_DEVICE_LCD_SUPPORT) != 0) {
										encoderType = VIDEO_ENCODER_LVDS;
									} else if ((connectorFlags
										& ATOM_DEVICE_CRT_SUPPORT) != 0) {
										encoderType = VIDEO_ENCODER_DAC;
									} else {
										encoderType = VIDEO_ENCODER_TMDS;
									}
									// drm_encoder_helper_add
									break;
								case ENCODER_OBJECT_ID_SI170B:
								case ENCODER_OBJECT_ID_CH7303:
								case ENCODER_OBJECT_ID_EXTERNAL_SDVOA:
								case ENCODER_OBJECT_ID_EXTERNAL_SDVOB:
								case ENCODER_OBJECT_ID_TITFP513:
								case ENCODER_OBJECT_ID_VT1623:
								case ENCODER_OBJECT_ID_HDMI_SI1930:
								case ENCODER_OBJECT_ID_TRAVIS:
								case ENCODER_OBJECT_ID_NUTMEG:
									encoderExternal = true;
									if ((connectorFlags
										& ATOM_DEVICE_LCD_SUPPORT) != 0) {
										encoderType = VIDEO_ENCODER_LVDS;
									} else if ((connectorFlags
										& ATOM_DEVICE_CRT_SUPPORT) != 0) {
										encoderType = VIDEO_ENCODER_DAC;
									} else {
										encoderType = VIDEO_ENCODER_TMDS;
									}
									// drm_encoder_helper_add
									break;
							}

							if (encoderType == VIDEO_ENCODER_NONE) {
								ERROR("%s: Path #%" B_PRId32 ":"
									"skipping unknown encoder.\n",
									__func__, i);
									continue;
							}

							// Set up encoder on connector if valid
							TRACE("%s: Path #%" B_PRId32 ": Found encoder "
								"%s\n", __func__, i,
								get_encoder_name(encoderType));

							gConnector[connectorIndex]->encoder.flags
								= connectorFlags;
							gConnector[connectorIndex]->encoder.valid
								= true;
							gConnector[connectorIndex]->encoder.objectID
								= encoderID;
							gConnector[connectorIndex]->encoder.type
								= encoderType;
							gConnector[connectorIndex]->encoder.isExternal
								= encoderExternal;

							pll_limit_probe(
								&gConnector[connectorIndex]->encoder.pll);
						}
					}
					// END if object is encoder
				} else if (grph_obj_type == GRAPH_OBJECT_TYPE_ROUTER) {
					ERROR("%s: TODO: Found router object?\n", __func__);
				} // END if object is router
			}

			// Set up information buses such as ddc
			if ((connectorFlags
				& (ATOM_DEVICE_TV_SUPPORT | ATOM_DEVICE_CV_SUPPORT)) == 0) {
				for (j = 0; j < con_obj->ucNumberOfObjects; j++) {
					if (B_LENDIAN_TO_HOST_INT16(path->usConnObjectId)
						== B_LENDIAN_TO_HOST_INT16(
						con_obj->asObjects[j].usObjectID)) {
						ATOM_COMMON_RECORD_HEADER *record
							= (ATOM_COMMON_RECORD_HEADER*)(gAtomContext->bios
							+ tableOffset + B_LENDIAN_TO_HOST_INT16(
							con_obj->asObjects[j].usRecordOffset));
						while (record->ucRecordSize > 0
							&& record->ucRecordType > 0
							&& record->ucRecordType
								<= ATOM_MAX_OBJECT_RECORD_NUMBER) {
							ATOM_I2C_RECORD *i2c_record;
							ATOM_I2C_ID_CONFIG_ACCESS *i2c_config;
							//ATOM_HPD_INT_RECORD *hpd_record;

							switch (record->ucRecordType) {
								case ATOM_I2C_RECORD_TYPE:
									i2c_record
										= (ATOM_I2C_RECORD *)record;
									i2c_config
										= (ATOM_I2C_ID_CONFIG_ACCESS *)
										&i2c_record->sucI2cId;
									// attach i2c gpio information for connector
									radeon_gpu_i2c_attach(connectorIndex,
										i2c_config->ucAccess);
									break;
								case ATOM_HPD_INT_RECORD_TYPE:
									// TODO: HPD (Hot Plug)
									break;
							}

							// move to next record
							record = (ATOM_COMMON_RECORD_HEADER *)
								((char *)record + record->ucRecordSize);
						}
					}
				}
			}

			// TODO: aux chan transactions

			// record connector information
			TRACE("%s: Path #%" B_PRId32 ": Found %s (0x%" B_PRIX32 ")\n",
				__func__, i, get_connector_name(connectorType),
				connectorType);

			gConnector[connectorIndex]->valid = true;
			gConnector[connectorIndex]->flags = connectorFlags;
			gConnector[connectorIndex]->type = connectorType;
			gConnector[connectorIndex]->objectID = connectorObjectID;

			gConnector[connectorIndex]->encoder.isTV = false;
			gConnector[connectorIndex]->encoder.isHDMI = false;

			switch(connectorType) {
				case VIDEO_CONNECTOR_COMPOSITE:
				case VIDEO_CONNECTOR_SVIDEO:
				case VIDEO_CONNECTOR_9DIN:
					gConnector[connectorIndex]->encoder.isTV = true;
					break;
				case VIDEO_CONNECTOR_HDMIA:
				case VIDEO_CONNECTOR_HDMIB:
					gConnector[connectorIndex]->encoder.isHDMI = true;
					break;
			}

			connectorIndex++;
		} // END for each valid connector
	} // end for each display path

	return B_OK;
}


status_t
detect_displays()
{
	// reset known displays
	for (uint32 id = 0; id < MAX_DISPLAY; id++) {
		gDisplay[id]->active = false;
		gDisplay[id]->found_ranges = false;
	}

	uint32 displayIndex = 0;
	for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
		if (gConnector[id]->valid == false)
			continue;
		if (displayIndex >= MAX_DISPLAY)
			continue;

		if (radeon_gpu_read_edid(id, &gDisplay[displayIndex]->edid_info)) {

			if (gConnector[id]->encoder.type == VIDEO_ENCODER_TVDAC
				|| gConnector[id]->encoder.type == VIDEO_ENCODER_DAC) {
				// analog? with valid EDID? lets make sure there is load.
				// There is only one ddc communications path on DVI-I
				if (encoder_analog_load_detect(id) != true) {
					TRACE("%s: no analog load on EDID valid connector "
						"#%" B_PRIu32 "\n", __func__, id);
					continue;
				}
			}

			gDisplay[displayIndex]->active = true;
			gDisplay[displayIndex]->connectorIndex = id;
				// set physical connector index from gConnector

			init_registers(gDisplay[displayIndex]->regs, displayIndex);

			if (detect_crt_ranges(displayIndex) == B_OK)
				gDisplay[displayIndex]->found_ranges = true;
			displayIndex++;
		}
	}

	// fallback if no edid monitors were found
	if (displayIndex == 0) {
		ERROR("%s: ERROR: 0 attached monitors were found on display connectors."
			" Injecting first connector as a last resort.\n", __func__);
		for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
			// skip TV DAC connectors as likely fallback isn't for TV
			if (gConnector[id]->encoder.type == VIDEO_ENCODER_TVDAC)
				continue;
			gDisplay[0]->active = true;
			gDisplay[0]->connectorIndex = id;
			init_registers(gDisplay[0]->regs, 0);
			if (detect_crt_ranges(0) == B_OK)
				gDisplay[0]->found_ranges = true;
			break;
		}
	}

	return B_OK;
}


void
debug_displays()
{
	TRACE("Currently detected monitors===============\n");
	for (uint32 id = 0; id < MAX_DISPLAY; id++) {
		ERROR("Display #%" B_PRIu32 " active = %s\n",
			id, gDisplay[id]->active ? "true" : "false");

		uint32 connectorIndex = gDisplay[id]->connectorIndex;

		if (gDisplay[id]->active) {
			uint32 connectorType = gConnector[connectorIndex]->type;
			uint32 encoderType = gConnector[connectorIndex]->encoder.type;
			ERROR(" + connector: %s\n", get_connector_name(connectorType));
			ERROR(" + encoder:   %s\n", get_encoder_name(encoderType));

			ERROR(" + limits: Vert Min/Max: %" B_PRIu32 "/%" B_PRIu32"\n",
				gDisplay[id]->vfreq_min, gDisplay[id]->vfreq_max);
			ERROR(" + limits: Horz Min/Max: %" B_PRIu32 "/%" B_PRIu32"\n",
				gDisplay[id]->hfreq_min, gDisplay[id]->hfreq_max);
		}
	}
	TRACE("==========================================\n");
}


void
debug_connectors()
{
	ERROR("Currently detected connectors=============\n");
	for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
		if (gConnector[id]->valid == true) {
			uint32 connectorType = gConnector[id]->type;
			uint32 encoderType = gConnector[id]->encoder.type;
			uint16 encoderID = gConnector[id]->encoder.objectID;
			uint16 gpioID = gConnector[id]->gpioID;
			ERROR("Connector #%" B_PRIu32 ")\n", id);
			ERROR(" + connector:  %s\n", get_connector_name(connectorType));
			ERROR(" + encoder:    %s\n", get_encoder_name(encoderType));
			ERROR(" + encoder id: %" B_PRIu16 "\n", encoderID);
			ERROR(" + gpio id:    %" B_PRIu16 "\n", gpioID);
			ERROR(" + gpio valid: %s\n",
				gGPIOInfo[gpioID]->valid ? "true" : "false");
			ERROR(" + hw line:    0x%" B_PRIX32 "\n",
				gGPIOInfo[gpioID]->hw_line);
		}
	}
	ERROR("==========================================\n");
}


uint32
display_get_encoder_mode(uint32 connectorIndex)
{
	switch (gConnector[connectorIndex]->type) {
		case VIDEO_CONNECTOR_DVII:
		case VIDEO_CONNECTOR_HDMIB: /* HDMI-B is DL-DVI; analog works fine */
			// TODO: if audio detected on edid and DCE4, ATOM_ENCODER_MODE_DVI
			//        if audio detected on edid not DCE4, ATOM_ENCODER_MODE_HDMI
			// if (gConnector[connectorIndex]->use_digital)
			//	return ATOM_ENCODER_MODE_DVI;
			// else
				return ATOM_ENCODER_MODE_CRT;
			break;
		case VIDEO_CONNECTOR_DVID:
		case VIDEO_CONNECTOR_HDMIA:
		default:
			// TODO: if audio detected on edid and DCE4, ATOM_ENCODER_MODE_DVI
			//        if audio detected on edid not DCE4, ATOM_ENCODER_MODE_HDMI
			return ATOM_ENCODER_MODE_DVI;
		case VIDEO_CONNECTOR_LVDS:
			return ATOM_ENCODER_MODE_LVDS;
		case VIDEO_CONNECTOR_DP:
			// dig_connector = radeon_connector->con_priv;
			// if ((dig_connector->dp_sink_type
			//	== CONNECTOR_OBJECT_ID_DISPLAYPORT)
			// 	|| (dig_connector->dp_sink_type == CONNECTOR_OBJECT_ID_eDP)) {
			// 	return ATOM_ENCODER_MODE_DP;
			// }
			// TODO: if audio detected on edid and DCE4, ATOM_ENCODER_MODE_DVI
			//        if audio detected on edid not DCE4, ATOM_ENCODER_MODE_HDMI
			return ATOM_ENCODER_MODE_DVI;
		case VIDEO_CONNECTOR_EDP:
			return ATOM_ENCODER_MODE_DP;
		case VIDEO_CONNECTOR_DVIA:
		case VIDEO_CONNECTOR_VGA:
			return ATOM_ENCODER_MODE_CRT;
		case VIDEO_CONNECTOR_COMPOSITE:
		case VIDEO_CONNECTOR_SVIDEO:
		case VIDEO_CONNECTOR_9DIN:
			return ATOM_ENCODER_MODE_TV;
	}
}


void
display_crtc_lock(uint8 crtcID, int command)
{
	TRACE("%s\n", __func__);
	ENABLE_CRTC_PS_ALLOCATION args;
	int index
		= GetIndexIntoMasterTable(COMMAND, UpdateCRTC_DoubleBufferRegisters);

	memset(&args, 0, sizeof(args));

	args.ucCRTC = crtcID;
	args.ucEnable = command;

	atom_execute_table(gAtomContext, index, (uint32*)&args);
}


void
display_crtc_blank(uint8 crtcID, int command)
{
	TRACE("%s\n", __func__);
	BLANK_CRTC_PS_ALLOCATION args;
	int index = GetIndexIntoMasterTable(COMMAND, BlankCRTC);

	memset(&args, 0, sizeof(args));

	args.ucCRTC = crtcID;
	args.ucBlanking = command;

	atom_execute_table(gAtomContext, index, (uint32*)&args);
}


void
display_crtc_scale(uint8 crtcID, display_mode *mode)
{
	TRACE("%s\n", __func__);
	ENABLE_SCALER_PS_ALLOCATION args;
	int index = GetIndexIntoMasterTable(COMMAND, EnableScaler);

	memset(&args, 0, sizeof(args));

	args.ucScaler = crtcID;
	args.ucEnable = ATOM_SCALER_EXPANSION;

	atom_execute_table(gAtomContext, index, (uint32*)&args);
}


void
display_crtc_fb_set(uint8 crtcID, display_mode *mode)
{
	radeon_shared_info &info = *gInfo->shared_info;
	register_info* regs = gDisplay[crtcID]->regs;

	uint32 fbSwap;
	if (info.dceMajor >= 4)
		fbSwap = EVERGREEN_GRPH_ENDIAN_SWAP(EVERGREEN_GRPH_ENDIAN_NONE);
	else
		fbSwap = R600_D1GRPH_SWAP_ENDIAN_NONE;

	uint32 fbFormat;

	uint32 bytesPerPixel;
	uint32 bitsPerPixel;

	switch (mode->space) {
		case B_CMAP8:
			bytesPerPixel = 1;
			bitsPerPixel = 8;
			if (info.dceMajor >= 4) {
				fbFormat = (EVERGREEN_GRPH_DEPTH(EVERGREEN_GRPH_DEPTH_8BPP)
					| EVERGREEN_GRPH_FORMAT(EVERGREEN_GRPH_FORMAT_INDEXED));
			} else {
				fbFormat = AVIVO_D1GRPH_CONTROL_DEPTH_8BPP
					| AVIVO_D1GRPH_CONTROL_8BPP_INDEXED;
			}
			break;
		case B_RGB15_LITTLE:
			bytesPerPixel = 2;
			bitsPerPixel = 15;
			if (info.dceMajor >= 4) {
				fbFormat = (EVERGREEN_GRPH_DEPTH(EVERGREEN_GRPH_DEPTH_16BPP)
					| EVERGREEN_GRPH_FORMAT(EVERGREEN_GRPH_FORMAT_ARGB1555));
			} else {
				fbFormat = AVIVO_D1GRPH_CONTROL_DEPTH_16BPP
					| AVIVO_D1GRPH_CONTROL_16BPP_ARGB1555;
			}
			break;
		case B_RGB16_LITTLE:
			bytesPerPixel = 2;
			bitsPerPixel = 16;

			if (info.dceMajor >= 4) {
				fbFormat = (EVERGREEN_GRPH_DEPTH(EVERGREEN_GRPH_DEPTH_16BPP)
					| EVERGREEN_GRPH_FORMAT(EVERGREEN_GRPH_FORMAT_ARGB565));
				#ifdef __POWERPC__
				fbSwap
					= EVERGREEN_GRPH_ENDIAN_SWAP(EVERGREEN_GRPH_ENDIAN_8IN16);
				#endif
			} else {
				fbFormat = AVIVO_D1GRPH_CONTROL_DEPTH_16BPP
					| AVIVO_D1GRPH_CONTROL_16BPP_RGB565;
				#ifdef __POWERPC__
				fbSwap = R600_D1GRPH_SWAP_ENDIAN_16BIT;
				#endif
			}
			break;
		case B_RGB24_LITTLE:
		case B_RGB32_LITTLE:
		default:
			bytesPerPixel = 4;
			bitsPerPixel = 32;
			if (info.dceMajor >= 4) {
				fbFormat = (EVERGREEN_GRPH_DEPTH(EVERGREEN_GRPH_DEPTH_32BPP)
					| EVERGREEN_GRPH_FORMAT(EVERGREEN_GRPH_FORMAT_ARGB8888));
				#ifdef __POWERPC__
				fbSwap
					= EVERGREEN_GRPH_ENDIAN_SWAP(EVERGREEN_GRPH_ENDIAN_8IN32);
				#endif
			} else {
				fbFormat = AVIVO_D1GRPH_CONTROL_DEPTH_32BPP
					| AVIVO_D1GRPH_CONTROL_32BPP_ARGB8888;
				#ifdef __POWERPC__
				fbSwap = R600_D1GRPH_SWAP_ENDIAN_32BIT;
				#endif
			}
			break;
	}

	uint32 bytesPerRow = mode->virtual_width * bytesPerPixel;

	Write32(OUT, regs->vgaControl, 0);

	uint64 fbAddressInt = gInfo->shared_info->frame_buffer_int;
	TRACE("%s: Framebuffer at: 0x%" B_PRIX64 "\n", __func__, fbAddressInt);


	if (info.device_chipset >= (RADEON_R700 | 0x70)) {
		TRACE("%s: Set SurfaceAddress High: 0x%" B_PRIX32 "\n",
			__func__, (fbAddressInt >> 32) & 0xf);

		Write32(OUT, regs->grphPrimarySurfaceAddrHigh,
			(fbAddressInt >> 32) & 0xf);
		Write32(OUT, regs->grphSecondarySurfaceAddrHigh,
			(fbAddressInt >> 32) & 0xf);
	}

	TRACE("%s: Set SurfaceAddress: 0x%" B_PRIX32 "\n",
		__func__, (fbAddressInt & 0xFFFFFFFF));

	Write32(OUT, regs->grphPrimarySurfaceAddr, (fbAddressInt & 0xFFFFFFFF));
	Write32(OUT, regs->grphSecondarySurfaceAddr, (fbAddressInt & 0xFFFFFFFF));

	if (info.device_chipset >= RADEON_R600) {
		Write32(CRT, regs->grphControl, fbFormat);
		Write32(CRT, regs->grphSwapControl, fbSwap);
	}

	Write32(CRT, regs->grphSurfaceOffsetX, 0);
	Write32(CRT, regs->grphSurfaceOffsetY, 0);
	Write32(CRT, regs->grphXStart, 0);
	Write32(CRT, regs->grphYStart, 0);
	Write32(CRT, regs->grphXEnd, mode->virtual_width);
	Write32(CRT, regs->grphYEnd, mode->virtual_height);
	Write32(CRT, regs->grphPitch, (bytesPerRow / 4));

	Write32(CRT, regs->grphEnable, 1);
		// Enable Frame buffer

	Write32(CRT, regs->modeDesktopHeight, mode->virtual_height);

	uint32 viewport_w = mode->timing.h_display;
	uint32 viewport_h = (mode->timing.v_display + 1) & ~1;

	Write32(CRT, regs->viewportStart, 0);
	Write32(CRT, regs->viewportSize,
		(viewport_w << 16) | viewport_h);

	// Pageflip setup
	if (info.dceMajor >= 4) {
		uint32 tmp
			= Read32(OUT, EVERGREEN_GRPH_FLIP_CONTROL + regs->crtcOffset);
		tmp &= ~EVERGREEN_GRPH_SURFACE_UPDATE_H_RETRACE_EN;
		Write32(OUT, EVERGREEN_GRPH_FLIP_CONTROL + regs->crtcOffset, tmp);

		Write32(OUT, EVERGREEN_MASTER_UPDATE_MODE + regs->crtcOffset, 0);
			// Pageflip to happen anywhere in vblank

	} else {
		uint32 tmp = Read32(OUT, AVIVO_D1GRPH_FLIP_CONTROL + regs->crtcOffset);
		tmp &= ~AVIVO_D1GRPH_SURFACE_UPDATE_H_RETRACE_EN;
		Write32(OUT, AVIVO_D1GRPH_FLIP_CONTROL + regs->crtcOffset, tmp);

		Write32(OUT, AVIVO_D1MODE_MASTER_UPDATE_MODE + regs->crtcOffset, 0);
			// Pageflip to happen anywhere in vblank
	}

	// update shared info
	gInfo->shared_info->bytes_per_row = bytesPerRow;
	gInfo->shared_info->current_mode = *mode;
	gInfo->shared_info->bits_per_pixel = bitsPerPixel;
}


void
display_crtc_set(uint8 crtcID, display_mode *mode)
{
	display_timing& displayTiming = mode->timing;

	TRACE("%s called to do %dx%d\n",
		__func__, displayTiming.h_display, displayTiming.v_display);

	SET_CRTC_TIMING_PARAMETERS_PS_ALLOCATION args;
	int index = GetIndexIntoMasterTable(COMMAND, SetCRTC_Timing);
	uint16 misc = 0;

	memset(&args, 0, sizeof(args));

	args.usH_Total = B_HOST_TO_LENDIAN_INT16(displayTiming.h_total);
	args.usH_Disp = B_HOST_TO_LENDIAN_INT16(displayTiming.h_display);
	args.usH_SyncStart = B_HOST_TO_LENDIAN_INT16(displayTiming.h_sync_start);
	args.usH_SyncWidth = B_HOST_TO_LENDIAN_INT16(displayTiming.h_sync_end
		- displayTiming.h_sync_start);

	args.usV_Total = B_HOST_TO_LENDIAN_INT16(displayTiming.v_total);
	args.usV_Disp = B_HOST_TO_LENDIAN_INT16(displayTiming.v_display);
	args.usV_SyncStart = B_HOST_TO_LENDIAN_INT16(displayTiming.v_sync_start);
	args.usV_SyncWidth = B_HOST_TO_LENDIAN_INT16(displayTiming.v_sync_end
		- displayTiming.v_sync_start);

	args.ucOverscanRight = 0;
	args.ucOverscanLeft = 0;
	args.ucOverscanBottom = 0;
	args.ucOverscanTop = 0;

	if ((displayTiming.flags & B_POSITIVE_HSYNC) == 0)
		misc |= ATOM_HSYNC_POLARITY;
	if ((displayTiming.flags & B_POSITIVE_VSYNC) == 0)
		misc |= ATOM_VSYNC_POLARITY;

	args.susModeMiscInfo.usAccess = B_HOST_TO_LENDIAN_INT16(misc);
	args.ucCRTC = crtcID;

	atom_execute_table(gAtomContext, index, (uint32 *)&args);
}


void
display_crtc_set_dtd(uint8 crtcID, display_mode *mode)
{
	display_timing& displayTiming = mode->timing;

	TRACE("%s called to do %dx%d\n",
		__func__, displayTiming.h_display, displayTiming.v_display);

	SET_CRTC_USING_DTD_TIMING_PARAMETERS args;
	int index = GetIndexIntoMasterTable(COMMAND, SetCRTC_UsingDTDTiming);
	uint16 misc = 0;

	memset(&args, 0, sizeof(args));

	uint16 blankStart
		= MIN(displayTiming.h_sync_start, displayTiming.h_display);
	uint16 blankEnd
		= MAX(displayTiming.h_sync_end, displayTiming.h_total);
	args.usH_Size = B_HOST_TO_LENDIAN_INT16(displayTiming.h_display);
	args.usH_Blanking_Time = B_HOST_TO_LENDIAN_INT16(blankEnd - blankStart);

	blankStart = MIN(displayTiming.v_sync_start, displayTiming.v_display);
	blankEnd = MAX(displayTiming.v_sync_end, displayTiming.v_total);
	args.usV_Size = B_HOST_TO_LENDIAN_INT16(displayTiming.v_display);
	args.usV_Blanking_Time = B_HOST_TO_LENDIAN_INT16(blankEnd - blankStart);

	args.usH_SyncOffset = B_HOST_TO_LENDIAN_INT16(displayTiming.h_sync_start
		- displayTiming.h_display);
	args.usH_SyncWidth = B_HOST_TO_LENDIAN_INT16(displayTiming.h_sync_end
		- displayTiming.h_sync_start);

	args.usV_SyncOffset = B_HOST_TO_LENDIAN_INT16(displayTiming.v_sync_start
		- displayTiming.v_display);
	args.usV_SyncWidth = B_HOST_TO_LENDIAN_INT16(displayTiming.v_sync_end
		- displayTiming.v_sync_start);

	args.ucH_Border = 0;
	args.ucV_Border = 0;

	if ((displayTiming.flags & B_POSITIVE_HSYNC) == 0)
		misc |= ATOM_HSYNC_POLARITY;
	if ((displayTiming.flags & B_POSITIVE_VSYNC) == 0)
		misc |= ATOM_VSYNC_POLARITY;

	args.susModeMiscInfo.usAccess = B_HOST_TO_LENDIAN_INT16(misc);
	args.ucCRTC = crtcID;

	atom_execute_table(gAtomContext, index, (uint32*)&args);
}


void
display_crtc_power(uint8 crtcID, int command)
{
	TRACE("%s\n", __func__);
	int index = GetIndexIntoMasterTable(COMMAND, EnableCRTC);
	ENABLE_CRTC_PS_ALLOCATION args;

	memset(&args, 0, sizeof(args));

	args.ucCRTC = crtcID;
	args.ucEnable = command;

	atom_execute_table(gAtomContext, index, (uint32*)&args);
}


void
display_crtc_memreq(uint8 crtcID, int command)
{
	TRACE("%s\n", __func__);
	int index = GetIndexIntoMasterTable(COMMAND, EnableCRTCMemReq);
	ENABLE_CRTC_PS_ALLOCATION args;

	memset(&args, 0, sizeof(args));

	args.ucCRTC = crtcID;
	args.ucEnable = command;

	atom_execute_table(gAtomContext, index, (uint32*)&args);
}
