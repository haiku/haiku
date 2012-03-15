/*
 * Copyright 2006-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	  Alexander von Gluck, kallisti5@unixzen.com
 */


#include "encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "accelerant.h"
#include "accelerant_protos.h"
#include "bios.h"
#include "connector.h"
#include "display.h"
#include "displayport.h"
#include "utility.h"


#define TRACE_ENCODER
#ifdef TRACE_ENCODER
extern "C" void _sPrintf(const char* format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


void
encoder_init()
{
	radeon_shared_info &info = *gInfo->shared_info;

	for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
		if (gConnector[id]->valid == false)
			continue;

		switch (gConnector[id]->encoder.objectID) {
			case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
			case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
			case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
			case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
				transmitter_dig_setup(id, 0, 0, 0,
					ATOM_TRANSMITTER_ACTION_INIT);
				break;
			default:
				break;
		}

		if ((info.chipsetFlags & CHIP_APU) != 0) {
			if (gConnector[id]->encoderExternal.valid == true) {
				encoder_external_setup(id, 0,
					EXTERNAL_ENCODER_ACTION_V3_ENCODER_INIT);
			}
		}
	}
}


void
encoder_assign_crtc(uint8 crtcID)
{
	TRACE("%s\n", __func__);

	int index = GetIndexIntoMasterTable(COMMAND, SelectCRTC_Source);

	// Table version
	uint8 tableMajor;
	uint8 tableMinor;
	if (atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor)
		!= B_OK)
		return;

	uint16 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint16 encoderID = gConnector[connectorIndex]->encoder.objectID;
	uint16 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	// Prepare AtomBIOS command arguments
	union crtcSourceParam {
		SELECT_CRTC_SOURCE_PS_ALLOCATION v1;
		SELECT_CRTC_SOURCE_PARAMETERS_V2 v2;
	};
	union crtcSourceParam args;
	memset(&args, 0, sizeof(args));

	switch (tableMajor) {
		case 1:
			switch (tableMinor) {
				case 1:
				default:
					args.v1.ucCRTC = crtcID;
					switch (encoderID) {
						case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
							args.v1.ucDevice = ATOM_DEVICE_DFP1_INDEX;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_LVDS:
						case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
							if ((gConnector[connectorIndex]->flags
								& ATOM_DEVICE_LCD1_SUPPORT) != 0)
								args.v1.ucDevice = ATOM_DEVICE_LCD1_INDEX;
							else
								args.v1.ucDevice = ATOM_DEVICE_DFP3_INDEX;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_DVO1:
						case ENCODER_OBJECT_ID_INTERNAL_DDI:
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
							args.v1.ucDevice = ATOM_DEVICE_DFP2_INDEX;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_DAC1:
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
							if ((encoderFlags & ATOM_DEVICE_TV_SUPPORT) != 0) {
								args.v1.ucDevice = ATOM_DEVICE_TV1_INDEX;
							} else if ((encoderFlags
								& ATOM_DEVICE_CV_SUPPORT) != 0) {
								args.v1.ucDevice = ATOM_DEVICE_CV_INDEX;
							} else
								args.v1.ucDevice = ATOM_DEVICE_CRT1_INDEX;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_DAC2:
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
							if ((encoderFlags & ATOM_DEVICE_TV_SUPPORT) != 0) {
								args.v1.ucDevice = ATOM_DEVICE_TV1_INDEX;
							} else if ((encoderFlags
								& ATOM_DEVICE_CV_SUPPORT) != 0) {
								args.v1.ucDevice = ATOM_DEVICE_CV_INDEX;
							} else
								args.v1.ucDevice = ATOM_DEVICE_CRT2_INDEX;
							break;
					}
					break;
				case 2:
					args.v2.ucCRTC = crtcID;
					args.v2.ucEncodeMode
						= display_get_encoder_mode(connectorIndex);
					switch (encoderID) {
						case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
						case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
						case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
							switch (encoder_pick_dig(connectorIndex)) {
								case 0:
									args.v2.ucEncoderID
										= ASIC_INT_DIG1_ENCODER_ID;
									break;
								case 1:
									args.v2.ucEncoderID
										= ASIC_INT_DIG2_ENCODER_ID;
									break;
								case 2:
									args.v2.ucEncoderID
										= ASIC_INT_DIG3_ENCODER_ID;
									break;
								case 3:
									args.v2.ucEncoderID
										= ASIC_INT_DIG4_ENCODER_ID;
									break;
								case 4:
									args.v2.ucEncoderID
										= ASIC_INT_DIG5_ENCODER_ID;
									break;
								case 5:
									args.v2.ucEncoderID
										= ASIC_INT_DIG6_ENCODER_ID;
									break;
							}
							break;
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
							args.v2.ucEncoderID = ASIC_INT_DVO_ENCODER_ID;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
							if ((encoderFlags & ATOM_DEVICE_TV_SUPPORT) != 0) {
								args.v2.ucEncoderID = ASIC_INT_TV_ENCODER_ID;
							} else if ((encoderFlags
								& ATOM_DEVICE_CV_SUPPORT) != 0) {
								args.v2.ucEncoderID = ASIC_INT_TV_ENCODER_ID;
							} else
								args.v2.ucEncoderID = ASIC_INT_DAC1_ENCODER_ID;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
							if ((encoderFlags & ATOM_DEVICE_TV_SUPPORT) != 0) {
								args.v2.ucEncoderID = ASIC_INT_TV_ENCODER_ID;
							} else if ((encoderFlags
								& ATOM_DEVICE_CV_SUPPORT) != 0) {
								args.v2.ucEncoderID = ASIC_INT_TV_ENCODER_ID;
							} else
								args.v2.ucEncoderID = ASIC_INT_DAC2_ENCODER_ID;
							break;
					}
					break;
			}
			break;
		default:
			ERROR("%s: Unknown table version: %" B_PRIu8 ".%" B_PRIu8 "\n",
				__func__, tableMajor, tableMinor);
			return;
	}

	atom_execute_table(gAtomContext, index, (uint32*)&args);

	// update crtc encoder scratch register @ scratch 3
	encoder_crtc_scratch(crtcID);
}


uint32
encoder_pick_dig(uint32 connectorIndex)
{
	TRACE("%s\n", __func__);
	radeon_shared_info &info = *gInfo->shared_info;
	uint32 encoderID = gConnector[connectorIndex]->encoder.objectID;

	// obtain assigned CRT
	uint32 crtcID;
	for (crtcID = 0; crtcID < MAX_DISPLAY; crtcID++) {
		if (gDisplay[crtcID]->attached != true)
			continue;
		if (gDisplay[crtcID]->connectorIndex == connectorIndex)
			break;
	}

	bool linkB = gConnector[connectorIndex]->encoder.linkEnumeration
		== GRAPH_OBJECT_ENUM_ID2 ? true : false;

	uint32 dceVersion = (info.dceMajor * 100) + info.dceMinor;

	if (dceVersion >= 400) {
		// APU
		switch (info.chipsetID) {
			case RADEON_PALM:
				return linkB ? 1 : 0;
			case RADEON_SUMO:
			case RADEON_SUMO2:
				return crtcID;
		}

		switch (encoderID) {
			case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
				return linkB ? 1 : 0;
			case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
				return linkB ? 3 : 2;
			case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
				return linkB ? 5 : 4;
		}
	}

	if (dceVersion >= 302)
		return crtcID;

	if (encoderID == ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA)
		return 1;

	return 0;
}


void
encoder_apply_quirks(uint8 crtcID)
{
	TRACE("%s\n", __func__);
	radeon_shared_info &info = *gInfo->shared_info;
	register_info* regs = gDisplay[crtcID]->regs;
	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint16 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	// Setting the scaler clears this on some chips...
	if (info.dceMajor >= 3
		&& (encoderFlags & ATOM_DEVICE_TV_SUPPORT) == 0) {
		// TODO: assume non interleave mode for now
		// en: EVERGREEN_INTERLEAVE_EN : AVIVO_D1MODE_INTERLEAVE_EN
		Write32(OUT, regs->modeDataFormat, 0);
	}
}


void
encoder_mode_set(uint8 id, uint32 pixelClock)
{
	TRACE("%s\n", __func__);
	radeon_shared_info &info = *gInfo->shared_info;
	uint32 connectorIndex = gDisplay[id]->connectorIndex;
	uint16 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	switch (gConnector[connectorIndex]->encoder.objectID) {
		case ENCODER_OBJECT_ID_INTERNAL_DAC1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
		case ENCODER_OBJECT_ID_INTERNAL_DAC2:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
			encoder_analog_setup(connectorIndex, pixelClock, ATOM_ENABLE);
			if ((encoderFlags
				& (ATOM_DEVICE_TV_SUPPORT | ATOM_DEVICE_CV_SUPPORT)) != 0) {
				encoder_tv_setup(connectorIndex, pixelClock, ATOM_ENABLE);
			} else {
				encoder_tv_setup(connectorIndex, pixelClock, ATOM_DISABLE);
			}
			break;
		case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
		case ENCODER_OBJECT_ID_INTERNAL_LVDS:
		case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
			encoder_digital_setup(connectorIndex, pixelClock,
				PANEL_ENCODER_ACTION_ENABLE);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
			if ((info.chipsetFlags & CHIP_APU) != 0
				|| info.dceMajor >= 5) {
				// Setup DIG encoder
				encoder_dig_setup(connectorIndex, pixelClock,
					ATOM_ENCODER_CMD_SETUP);
				encoder_dig_setup(connectorIndex, pixelClock,
					ATOM_ENCODER_CMD_SETUP_PANEL_MODE);
			} else if (info.dceMajor >= 4) {
				// Disable DIG transmitter
				transmitter_dig_setup(connectorIndex, pixelClock, 0, 0,
					ATOM_TRANSMITTER_ACTION_DISABLE);
				// Setup DIG encoder
				encoder_dig_setup(connectorIndex, pixelClock,
					ATOM_ENCODER_CMD_SETUP);
				// Enable DIG transmitter
				transmitter_dig_setup(connectorIndex, pixelClock, 0, 0,
					ATOM_TRANSMITTER_ACTION_ENABLE);
			} else {
				// Disable DIG transmitter
				transmitter_dig_setup(connectorIndex, pixelClock, 0, 0,
					ATOM_TRANSMITTER_ACTION_DISABLE);
				// Disable DIG encoder
				encoder_dig_setup(connectorIndex, pixelClock, ATOM_DISABLE);
				// Enable the DIG encoder
				encoder_dig_setup(connectorIndex, pixelClock, ATOM_ENABLE);

				// Setup and enable DIG transmitter
				transmitter_dig_setup(connectorIndex, pixelClock, 0, 0,
					ATOM_TRANSMITTER_ACTION_SETUP);
				transmitter_dig_setup(connectorIndex, pixelClock, 0, 0,
					ATOM_TRANSMITTER_ACTION_ENABLE);
			}
			break;
		case ENCODER_OBJECT_ID_INTERNAL_DDI:
		case ENCODER_OBJECT_ID_INTERNAL_DVO1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
			TRACE("%s: TODO for DVO encoder setup\n", __func__);
			break;
	}

	if (gConnector[connectorIndex]->encoderExternal.valid == true) {
		if ((info.chipsetFlags & CHIP_APU) != 0) {
			// aka DCE 4.1
			encoder_external_setup(connectorIndex, pixelClock,
				EXTERNAL_ENCODER_ACTION_V3_ENCODER_SETUP);
		} else {
			encoder_external_setup(connectorIndex, pixelClock,
				ATOM_ENABLE);
		}
	}

	encoder_apply_quirks(id);
}


status_t
encoder_tv_setup(uint32 connectorIndex, uint32 pixelClock, int command)
{
	uint16 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	TV_ENCODER_CONTROL_PS_ALLOCATION args;
	memset(&args, 0, sizeof(args));

	int index = GetIndexIntoMasterTable(COMMAND, TVEncoderControl);

	args.sTVEncoder.ucAction = command;

	if ((encoderFlags & ATOM_DEVICE_CV_SUPPORT) != 0)
		args.sTVEncoder.ucTvStandard = ATOM_TV_CV;
	else {
		// TODO: we assume NTSC for now
		args.sTVEncoder.ucTvStandard = ATOM_TV_NTSC;
	}

	args.sTVEncoder.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);

	return atom_execute_table(gAtomContext, index, (uint32*)&args);
}


status_t
encoder_digital_setup(uint32 connectorIndex, uint32 pixelClock, int command)
{
	TRACE("%s\n", __func__);

	int index = 0;
	uint16 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	switch (gConnector[connectorIndex]->encoder.objectID) {
		case ENCODER_OBJECT_ID_INTERNAL_LVDS:
			index = GetIndexIntoMasterTable(COMMAND, LVDSEncoderControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
			index = GetIndexIntoMasterTable(COMMAND, TMDS1EncoderControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
			if ((encoderFlags & ATOM_DEVICE_LCD_SUPPORT) != 0)
				index = GetIndexIntoMasterTable(COMMAND, LVDSEncoderControl);
			else
				index = GetIndexIntoMasterTable(COMMAND, TMDS2EncoderControl);
			break;
	}

	// Table verson
	uint8 tableMajor;
	uint8 tableMinor;

	if (atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor)
		!= B_OK) {
		ERROR("%s: cannot parse command table\n", __func__);
		return B_ERROR;
	}

	uint32 lvdsFlags = gConnector[connectorIndex]->lvdsFlags;

	bool isHdmi = false;
	if (gConnector[connectorIndex]->type == VIDEO_CONNECTOR_HDMIA
		|| gConnector[connectorIndex]->type == VIDEO_CONNECTOR_HDMIB) {
		isHdmi = true;
	}

	// Prepare AtomBIOS command arguments
	union lvdsEncoderControl {
		LVDS_ENCODER_CONTROL_PS_ALLOCATION    v1;
		LVDS_ENCODER_CONTROL_PS_ALLOCATION_V2 v2;
	};
	union lvdsEncoderControl args;
	memset(&args, 0, sizeof(args));

	TRACE("%s: table %" B_PRIu8 ".%" B_PRIu8 "\n", __func__,
		tableMajor, tableMinor);

	switch (tableMajor) {
	case 1:
	case 2:
		switch (tableMinor) {
			case 1:
				args.v1.ucMisc = 0;
				args.v1.ucAction = command;
				if (isHdmi)
					args.v1.ucMisc |= PANEL_ENCODER_MISC_HDMI_TYPE;
				args.v1.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);

				if ((encoderFlags & ATOM_DEVICE_LCD_SUPPORT) != 0) {
					if ((lvdsFlags & ATOM_PANEL_MISC_DUAL) != 0)
						args.v1.ucMisc |= PANEL_ENCODER_MISC_DUAL;
					if ((lvdsFlags & ATOM_PANEL_MISC_888RGB) != 0)
						args.v1.ucMisc |= ATOM_PANEL_MISC_888RGB;
				} else {
					//if (dig->linkb)
					//	args.v1.ucMisc |= PANEL_ENCODER_MISC_TMDS_LINKB;
					if (pixelClock > 165000)
						args.v1.ucMisc |= PANEL_ENCODER_MISC_DUAL;
					/*if (pScrn->rgbBits == 8) */
					args.v1.ucMisc |= ATOM_PANEL_MISC_888RGB;
				}
				break;
			case 2:
			case 3:
				args.v2.ucMisc = 0;
				args.v2.ucAction = command;
				if (tableMinor == 3) {
					//if (dig->coherent_mode)
					//	args.v2.ucMisc |= PANEL_ENCODER_MISC_COHERENT;
				}
				if (isHdmi)
					args.v2.ucMisc |= PANEL_ENCODER_MISC_HDMI_TYPE;
				args.v2.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
				args.v2.ucTruncate = 0;
				args.v2.ucSpatial = 0;
				args.v2.ucTemporal = 0;
				args.v2.ucFRC = 0;
				if ((encoderFlags & ATOM_DEVICE_LCD_SUPPORT) != 0) {
					if ((lvdsFlags & ATOM_PANEL_MISC_DUAL) != 0)
						args.v2.ucMisc |= PANEL_ENCODER_MISC_DUAL;
					if ((lvdsFlags & ATOM_PANEL_MISC_SPATIAL) != 0) {
						args.v2.ucSpatial = PANEL_ENCODER_SPATIAL_DITHER_EN;
						if ((lvdsFlags & ATOM_PANEL_MISC_888RGB) != 0) {
							args.v2.ucSpatial
								|= PANEL_ENCODER_SPATIAL_DITHER_DEPTH;
						}
					}

					if ((lvdsFlags & ATOM_PANEL_MISC_TEMPORAL) != 0)
						args.v2.ucTemporal = PANEL_ENCODER_TEMPORAL_DITHER_EN;
						if ((lvdsFlags & ATOM_PANEL_MISC_888RGB) != 0) {
							args.v2.ucTemporal
								|= PANEL_ENCODER_TEMPORAL_DITHER_DEPTH;
						}
						if (((lvdsFlags >> ATOM_PANEL_MISC_GREY_LEVEL_SHIFT)
							& 0x3) == 2) {
							args.v2.ucTemporal
							|= PANEL_ENCODER_TEMPORAL_LEVEL_4;
						}
				} else {
					//if (dig->linkb)
					//	args.v2.ucMisc |= PANEL_ENCODER_MISC_TMDS_LINKB;
					if (pixelClock > 165000)
						args.v2.ucMisc |= PANEL_ENCODER_MISC_DUAL;
				}
				break;
			default:
				ERROR("%s: Unknown minor table version: %"
					B_PRIu8 ".%" B_PRIu8 "\n", __func__,
					tableMajor, tableMinor);
				return B_ERROR;
		}
		break;
	default:
		ERROR("%s: Unknown major table version: %" B_PRIu8 ".%" B_PRIu8 "\n",
			__func__, tableMajor, tableMinor);
		return B_ERROR;
	}
	return atom_execute_table(gAtomContext, index, (uint32*)&args);
}


status_t
encoder_dig_setup(uint32 connectorIndex, uint32 pixelClock, int command)
{
	radeon_shared_info &info = *gInfo->shared_info;

	int index = 0;
	if (info.dceMajor >= 4)
		index = GetIndexIntoMasterTable(COMMAND, DIGxEncoderControl);
	else {
		if (encoder_pick_dig(connectorIndex))
			index = GetIndexIntoMasterTable(COMMAND, DIG2EncoderControl);
		else
			index = GetIndexIntoMasterTable(COMMAND, DIG1EncoderControl);
	}

	// Table verson
	uint8 tableMajor;
	uint8 tableMinor;

	if (atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor)
		!= B_OK) {
		ERROR("%s: cannot parse command table\n", __func__);
		return B_ERROR;
	}

	// Prepare AtomBIOS command arguments
	union digEncoderControl {
		DIG_ENCODER_CONTROL_PS_ALLOCATION v1;
		DIG_ENCODER_CONTROL_PARAMETERS_V2 v2;
		DIG_ENCODER_CONTROL_PARAMETERS_V3 v3;
		DIG_ENCODER_CONTROL_PARAMETERS_V4 v4;
	};
	union digEncoderControl args;
	memset(&args, 0, sizeof(args));

	uint32 encoderID = gConnector[connectorIndex]->encoder.objectID;
	bool isDPBridge = gConnector[connectorIndex]->encoder.isDPBridge;

	bool linkB = gConnector[connectorIndex]->encoder.linkEnumeration
		== GRAPH_OBJECT_ENUM_ID2 ? true : false;

	// determine DP panel mode
	uint32 panelMode;
	if (info.dceMajor >= 4 && isDPBridge) {
		if (encoderID == ENCODER_OBJECT_ID_NUTMEG)
			panelMode = DP_PANEL_MODE_INTERNAL_DP1_MODE;
		else {
			// aka ENCODER_OBJECT_ID_TRAVIS or VIDEO_CONNECTOR_EDP
			panelMode = DP_PANEL_MODE_INTERNAL_DP2_MODE;
		}
	} else
		panelMode = DP_PANEL_MODE_EXTERNAL_DP_MODE;

	#if 0
	uint32 encoderID = gConnector[connectorIndex]->encoder.objectID;
	if (gConnector[connectorIndex]->type == VIDEO_CONNECTOR_EDP) {
		uint8 temp = dpcd_read_reg(hwPin, DP_EDP_CONFIGURATION_CAP);
		if ((temp & 1) != 0)
			panelMode = DP_PANEL_MODE_INTERNAL_DP2_MODE;
	}
	#endif

	TRACE("%s: table %" B_PRIu8 ".%" B_PRIu8 "\n", __func__,
		tableMajor, tableMinor);

	uint32 dpClock = dp_get_link_clock(connectorIndex);
	switch (tableMinor) {
		case 1:
			args.v1.ucAction = command;
			args.v1.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);

			if (command == ATOM_ENCODER_CMD_SETUP_PANEL_MODE)
				args.v3.ucPanelMode = panelMode;
			else {
				args.v1.ucEncoderMode
					= display_get_encoder_mode(connectorIndex);
			}

			if ((args.v1.ucEncoderMode == ATOM_ENCODER_MODE_DP
				|| args.v1.ucEncoderMode == ATOM_ENCODER_MODE_DP_MST)
				&& dpClock == 270000) {
				args.v1.ucConfig |= ATOM_ENCODER_CONFIG_DPLINKRATE_2_70GHZ;
			}

			switch (encoderID) {
				case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
					args.v1.ucConfig = ATOM_ENCODER_CONFIG_V2_TRANSMITTER1;
					break;
				case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
				case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
					args.v1.ucConfig = ATOM_ENCODER_CONFIG_V2_TRANSMITTER2;
					break;
				case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
					args.v1.ucConfig = ATOM_ENCODER_CONFIG_V2_TRANSMITTER3;
					break;
			}

			if (linkB)
				args.v1.ucConfig |= ATOM_ENCODER_CONFIG_LINKB;
			else
				args.v1.ucConfig |= ATOM_ENCODER_CONFIG_LINKA;
			break;
		case 2:
		case 3:
			args.v1.ucAction = command;
			args.v1.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);

			if (command == ATOM_ENCODER_CMD_SETUP_PANEL_MODE)
				args.v3.ucPanelMode = panelMode;
			else {
				args.v3.ucEncoderMode
					= display_get_encoder_mode(connectorIndex);
			}

			if (args.v1.ucEncoderMode == ATOM_ENCODER_MODE_DP
				|| args.v1.ucEncoderMode == ATOM_ENCODER_MODE_DP_MST) {
				args.v1.ucLaneNum = gDPInfo[connectorIndex]->laneCount;
			} else if (pixelClock > 165000)
				args.v1.ucLaneNum = 8;
			else
				args.v1.ucLaneNum = 4;

			if ((args.v1.ucEncoderMode == ATOM_ENCODER_MODE_DP
				|| args.v1.ucEncoderMode == ATOM_ENCODER_MODE_DP_MST)
				&& dpClock == 270000) {
				args.v1.ucConfig |= ATOM_ENCODER_CONFIG_DPLINKRATE_2_70GHZ;
			}

			args.v3.acConfig.ucDigSel = encoder_pick_dig(connectorIndex);

			// TODO: get BPC
			switch (8) {
				case 0:
					args.v4.ucBitPerColor = PANEL_BPC_UNDEFINE;
					break;
				case 6:
					args.v4.ucBitPerColor = PANEL_6BIT_PER_COLOR;
					break;
				case 8:
				default:
					args.v4.ucBitPerColor = PANEL_8BIT_PER_COLOR;
					break;
				case 10:
					args.v4.ucBitPerColor = PANEL_10BIT_PER_COLOR;
					break;
				case 12:
					args.v4.ucBitPerColor = PANEL_12BIT_PER_COLOR;
					break;
				case 16:
					args.v4.ucBitPerColor = PANEL_16BIT_PER_COLOR;
					break;
			}
			break;
		case 4:
			args.v4.ucHPD_ID = 0;
			ERROR("%s: tableMinor 4 TODO!\n", __func__);
			break;
		default:
			ERROR("%s: unknown tableMinor!\n", __func__);
	}

	status_t result = atom_execute_table(gAtomContext, index, (uint32*)&args);

	#if 0
	if (gConnector[connectorIndex]->type == VIDEO_CONNECTOR_EDP
		&& panelMode == DP_PANEL_MODE_INTERNAL_DP2_MODE) {
		dpcd_write_reg(hwPin, DP_EDP_CONFIGURATION_SET, 1);
	}
	#endif

	return result;
}


status_t
encoder_external_setup(uint32 connectorIndex, uint32 pixelClock, int command)
{
	TRACE("%s\n", __func__);

	encoder_info* encoder
		= &gConnector[connectorIndex]->encoderExternal;

	if (encoder->valid != true) {
		ERROR("%s: connector %" B_PRIu32 " doesn't have a valid "
			"external encoder!", __func__, connectorIndex);
		return B_ERROR;
	}

	uint8 tableMajor;
	uint8 tableMinor;

	int index = GetIndexIntoMasterTable(COMMAND, ExternalEncoderControl);
	if (atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor)
		!= B_OK) {
		ERROR("%s: Error parsing ExternalEncoderControl table\n", __func__);
		return B_ERROR;
	}

	// Prepare AtomBIOS command arguments
	union externalEncoderControl {
		EXTERNAL_ENCODER_CONTROL_PS_ALLOCATION v1;
		EXTERNAL_ENCODER_CONTROL_PS_ALLOCATION_V3 v3;
	};
	union externalEncoderControl args;
	memset(&args, 0, sizeof(args));

	int connectorObjectID
		= (gConnector[connectorIndex]->objectID & OBJECT_ID_MASK)
			>> OBJECT_ID_SHIFT;

	TRACE("%s: table %" B_PRIu8 ".%" B_PRIu8 "\n", __func__,
		tableMajor, tableMinor);
	switch (tableMajor) {
		case 1:
			// no options needed on table 1.x
			break;
		case 2:
			switch (tableMinor) {
				case 1:
				case 2:
					args.v1.sDigEncoder.ucAction = command;
					args.v1.sDigEncoder.usPixelClock
						= B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
					args.v1.sDigEncoder.ucEncoderMode
						= display_get_encoder_mode(connectorIndex);

					if (connector_is_dp(connectorIndex)) {
						if (gDPInfo[connectorIndex]->clock == 270000) {
							args.v1.sDigEncoder.ucConfig
								|= ATOM_ENCODER_CONFIG_DPLINKRATE_2_70GHZ;
						}
						args.v1.sDigEncoder.ucLaneNum
							= gDPInfo[connectorIndex]->laneCount;
					} else if (pixelClock > 165000) {
						args.v1.sDigEncoder.ucLaneNum = 8;
					} else {
						args.v1.sDigEncoder.ucLaneNum = 4;
					}
					break;
				case 3:
				{
					args.v3.sExtEncoder.ucAction = command;
					if (command == EXTERNAL_ENCODER_ACTION_V3_ENCODER_INIT) {
						args.v3.sExtEncoder.usConnectorId
							= B_HOST_TO_LENDIAN_INT16(connectorObjectID);
					} else {
						args.v3.sExtEncoder.usPixelClock
							= B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
					}

					args.v3.sExtEncoder.ucEncoderMode
						= display_get_encoder_mode(connectorIndex);

					if (connector_is_dp(connectorIndex)) {
						if (gDPInfo[connectorIndex]->clock == 270000) {
							args.v3.sExtEncoder.ucConfig
								|=EXTERNAL_ENCODER_CONFIG_V3_DPLINKRATE_2_70GHZ;
						} else if (gDPInfo[connectorIndex]->clock == 540000) {
							args.v3.sExtEncoder.ucConfig
								|=EXTERNAL_ENCODER_CONFIG_V3_DPLINKRATE_5_40GHZ;
						}
						args.v1.sDigEncoder.ucLaneNum
							= gDPInfo[connectorIndex]->laneCount;
					} else if (pixelClock > 165000) {
						args.v3.sExtEncoder.ucLaneNum = 8;
					} else {
						args.v3.sExtEncoder.ucLaneNum = 4;
					}

					uint16 encoderFlags = encoder->flags;

					switch ((encoderFlags & ENUM_ID_MASK) >> ENUM_ID_SHIFT) {
						case GRAPH_OBJECT_ENUM_ID1:
							TRACE("%s: external encoder 1\n", __func__);
							args.v3.sExtEncoder.ucConfig
								|= EXTERNAL_ENCODER_CONFIG_V3_ENCODER1;
							break;
						case GRAPH_OBJECT_ENUM_ID2:
							TRACE("%s: external encoder 2\n", __func__);
							args.v3.sExtEncoder.ucConfig
								|= EXTERNAL_ENCODER_CONFIG_V3_ENCODER2;
							break;
						case GRAPH_OBJECT_ENUM_ID3:
							TRACE("%s: external encoder 3\n", __func__);
							args.v3.sExtEncoder.ucConfig
								|= EXTERNAL_ENCODER_CONFIG_V3_ENCODER3;
							break;
					}

					// TODO: don't set statically
					uint32 bitsPerColor = 8;
					switch (bitsPerColor) {
						case 0:
							args.v3.sExtEncoder.ucBitPerColor
								= PANEL_BPC_UNDEFINE;
							break;
						case 6:
							args.v3.sExtEncoder.ucBitPerColor
								= PANEL_6BIT_PER_COLOR;
							break;
						case 8:
						default:
							args.v3.sExtEncoder.ucBitPerColor
								= PANEL_8BIT_PER_COLOR;
							break;
						case 10:
							args.v3.sExtEncoder.ucBitPerColor
								= PANEL_10BIT_PER_COLOR;
							break;
						case 12:
							args.v3.sExtEncoder.ucBitPerColor
								= PANEL_12BIT_PER_COLOR;
							break;
						case 16:
							args.v3.sExtEncoder.ucBitPerColor
								= PANEL_16BIT_PER_COLOR;
							break;
					}
					break;
				}
				default:
					ERROR("%s: Unknown table minor version: "
						"%" B_PRIu8 ".%" B_PRIu8 "\n", __func__,
						tableMajor, tableMinor);
					return B_ERROR;
			}
			break;
		default:
			ERROR("%s: Unknown table major version: "
				"%" B_PRIu8 ".%" B_PRIu8 "\n", __func__,
				tableMajor, tableMinor);
			return B_ERROR;
	}

	return atom_execute_table(gAtomContext, index, (uint32*)&args);
}


status_t
encoder_analog_setup(uint32 connectorIndex, uint32 pixelClock, int command)
{
	TRACE("%s\n", __func__);

	uint32 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	int index = 0;
	DAC_ENCODER_CONTROL_PS_ALLOCATION args;
	memset(&args, 0, sizeof(args));

	switch (gConnector[connectorIndex]->encoder.objectID) {
		case ENCODER_OBJECT_ID_INTERNAL_DAC1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
			index = GetIndexIntoMasterTable(COMMAND, DAC1EncoderControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_DAC2:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
			index = GetIndexIntoMasterTable(COMMAND, DAC2EncoderControl);
			break;
	}

	args.ucAction = command;

	if ((encoderFlags & ATOM_DEVICE_CRT_SUPPORT) != 0)
		args.ucDacStandard = ATOM_DAC1_PS2;
	else if ((encoderFlags & ATOM_DEVICE_CV_SUPPORT) != 0)
		args.ucDacStandard = ATOM_DAC1_CV;
	else {
		TRACE("%s: TODO, hardcoded NTSC TV support\n", __func__);
		if (1) {
			// NTSC, NTSC_J, PAL 60
			args.ucDacStandard = ATOM_DAC1_NTSC;
		} else {
			// PAL, SCART. SECAM, PAL_CN
			args.ucDacStandard = ATOM_DAC1_PAL;
		}
	}

	args.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);

	return atom_execute_table(gAtomContext, index, (uint32*)&args);
}


bool
encoder_analog_load_detect(uint32 connectorIndex)
{
	TRACE("%s\n", __func__);

	if (gConnector[connectorIndex]->encoderExternal.valid == true)
		return encoder_dig_load_detect(connectorIndex);

	return encoder_dac_load_detect(connectorIndex);
}


bool
encoder_dac_load_detect(uint32 connectorIndex)
{
	TRACE("%s\n", __func__);

	uint32 encoderFlags = gConnector[connectorIndex]->encoder.flags;
	uint32 encoderID = gConnector[connectorIndex]->encoder.objectID;

	if ((encoderFlags & ATOM_DEVICE_TV_SUPPORT) == 0
		&& (encoderFlags & ATOM_DEVICE_CV_SUPPORT) == 0
		&& (encoderFlags & ATOM_DEVICE_CRT_SUPPORT) == 0) {
		ERROR("%s: executed on non-dac device connector #%" B_PRIu8 "\n",
			__func__, connectorIndex);
		return false;
	}

	// *** tell the card we want to do a DAC detection

	DAC_LOAD_DETECTION_PS_ALLOCATION args;
	int index = GetIndexIntoMasterTable(COMMAND, DAC_LoadDetection);
	uint8 tableMajor;
	uint8 tableMinor;

	memset(&args, 0, sizeof(args));

	if (atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor)
		!= B_OK) {
		ERROR("%s: failed getting AtomBIOS header for DAC_LoadDetection\n",
			__func__);
		return false;
	}

	args.sDacload.ucMisc = 0;

	if (encoderID == ENCODER_OBJECT_ID_INTERNAL_DAC1
		|| encoderID == ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1) {
		args.sDacload.ucDacType = ATOM_DAC_A;
	} else {
		args.sDacload.ucDacType = ATOM_DAC_B;
	}

	if ((encoderFlags & ATOM_DEVICE_CRT1_SUPPORT) != 0) {
		args.sDacload.usDeviceID
			= B_HOST_TO_LENDIAN_INT16(ATOM_DEVICE_CRT1_SUPPORT);
		atom_execute_table(gAtomContext, index, (uint32*)&args);

		uint32 biosScratch0 = Read32(OUT, R600_BIOS_0_SCRATCH);

		if ((biosScratch0 & ATOM_S0_CRT1_MASK) != 0)
			return true;

	} else if ((encoderFlags & ATOM_DEVICE_CRT2_SUPPORT) != 0) {
		args.sDacload.usDeviceID
			= B_HOST_TO_LENDIAN_INT16(ATOM_DEVICE_CRT2_SUPPORT);
		atom_execute_table(gAtomContext, index, (uint32*)&args);

		uint32 biosScratch0 = Read32(OUT, R600_BIOS_0_SCRATCH);

		if ((biosScratch0 & ATOM_S0_CRT2_MASK) != 0)
			return true;

	} else if ((encoderFlags & ATOM_DEVICE_CV_SUPPORT) != 0) {
		args.sDacload.usDeviceID
			= B_HOST_TO_LENDIAN_INT16(ATOM_DEVICE_CV_SUPPORT);
		if (tableMinor >= 3)
			args.sDacload.ucMisc = DAC_LOAD_MISC_YPrPb;
		atom_execute_table(gAtomContext, index, (uint32*)&args);

		uint32 biosScratch0 = Read32(OUT, R600_BIOS_0_SCRATCH);

		if ((biosScratch0 & (ATOM_S0_CV_MASK | ATOM_S0_CV_MASK_A)) != 0)
			return true;

	} else if ((encoderFlags & ATOM_DEVICE_TV1_SUPPORT) != 0) {
		args.sDacload.usDeviceID
			= B_HOST_TO_LENDIAN_INT16(ATOM_DEVICE_TV1_SUPPORT);
		if (tableMinor >= 3)
			args.sDacload.ucMisc = DAC_LOAD_MISC_YPrPb;
		atom_execute_table(gAtomContext, index, (uint32*)&args);

		uint32 biosScratch0 = Read32(OUT, R600_BIOS_0_SCRATCH);

		if ((biosScratch0
			& (ATOM_S0_TV1_COMPOSITE | ATOM_S0_TV1_COMPOSITE_A)) != 0) {
			return true; /* Composite connected */
		} else if ((biosScratch0
			& (ATOM_S0_TV1_SVIDEO | ATOM_S0_TV1_SVIDEO_A)) != 0) {
			return true; /* S-Video connected */
		}

	}
	return false;
}


bool
encoder_dig_load_detect(uint32 connectorIndex)
{
	TRACE("%s\n", __func__);
	radeon_shared_info &info = *gInfo->shared_info;

	if (info.dceMajor < 4) {
		ERROR("%s: Strange: External DIG encoder on DCE < 4?\n", __func__);
		return false;
	}

	encoder_external_setup(connectorIndex, 0,
		EXTERNAL_ENCODER_ACTION_V3_DACLOAD_DETECTION);

	uint32 biosScratch0 = Read32(OUT, R600_BIOS_0_SCRATCH);

	uint32 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	if ((encoderFlags & ATOM_DEVICE_CRT1_SUPPORT) != 0)
		if ((biosScratch0 & ATOM_S0_CRT1_MASK) != 0)
			return true;
	if ((encoderFlags & ATOM_DEVICE_CRT2_SUPPORT) != 0)
		if ((biosScratch0 & ATOM_S0_CRT2_MASK) != 0)
			return true;
	if ((encoderFlags & ATOM_DEVICE_CV_SUPPORT) != 0)
		if ((biosScratch0 & (ATOM_S0_CV_MASK | ATOM_S0_CV_MASK_A)) != 0)
			return true;
	if ((encoderFlags & ATOM_DEVICE_TV1_SUPPORT) != 0) {
		if ((biosScratch0
			& (ATOM_S0_TV1_COMPOSITE | ATOM_S0_TV1_COMPOSITE_A)) != 0)
			return true; /* Composite connected */
		else if ((biosScratch0
			& (ATOM_S0_TV1_SVIDEO | ATOM_S0_TV1_SVIDEO_A)) != 0)
			return true; /* S-Video connected */
	}

	return false;
}


status_t
transmitter_dig_setup(uint32 connectorIndex, uint32 pixelClock,
	uint8 laneNumber, uint8 laneSet, int command)
{
	TRACE("%s\n", __func__);

	uint16 encoderID = gConnector[connectorIndex]->encoder.objectID;
	int index;
	switch (encoderID) {
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
			index = GetIndexIntoMasterTable(COMMAND, DVOOutputControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
			index = GetIndexIntoMasterTable(COMMAND, UNIPHYTransmitterControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
			index = GetIndexIntoMasterTable(COMMAND, LVTMATransmitterControl);
			break;
		default:
			ERROR("%s: BUG: dig setup run on non-dig encoder!\n", __func__);
			return B_ERROR;
	}

	if (index < 0) {
		ERROR("%s: GetIndexIntoMasterTable failed!\n", __func__);
		return B_ERROR;
	}

	uint8 tableMajor;
	uint8 tableMinor;

	if (atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor)
		!= B_OK)
		return B_ERROR;

	// Prepare AtomBIOS arguments
	union digTransmitterControl {
		DIG_TRANSMITTER_CONTROL_PS_ALLOCATION v1;
		DIG_TRANSMITTER_CONTROL_PARAMETERS_V2 v2;
		DIG_TRANSMITTER_CONTROL_PARAMETERS_V3 v3;
		DIG_TRANSMITTER_CONTROL_PARAMETERS_V4 v4;
	};
	union digTransmitterControl args;
	memset(&args, 0, sizeof(args));

	TRACE("%s: table %" B_PRIu8 ".%" B_PRIu8 "\n", __func__,
		tableMajor, tableMinor);

	int connectorObjectID
		= (gConnector[connectorIndex]->objectID & OBJECT_ID_MASK)
			>> OBJECT_ID_SHIFT;
	uint32 encoderObjectID = gConnector[connectorIndex]->encoder.objectID;
	uint32 digEncoderID = encoder_pick_dig(connectorIndex);

	pll_info* pll = &gConnector[connectorIndex]->encoder.pll;

	bool isDP = connector_is_dp(connectorIndex);
	bool linkB = gConnector[connectorIndex]->encoderExternal.linkEnumeration
		== GRAPH_OBJECT_ENUM_ID2 ? true : false;

	uint8 dpClock = 0;
	int dpLaneCount = 0;
	if (gDPInfo[connectorIndex]->valid == true) {
		dpClock = gDPInfo[connectorIndex]->clock;
		dpLaneCount = gDPInfo[connectorIndex]->laneCount;
	}

	switch (tableMajor) {
		case 1:
			switch (tableMinor) {
				case 1:
					args.v1.ucAction = command;
					if (command == ATOM_TRANSMITTER_ACTION_INIT) {
						args.v1.usInitInfo
							= B_HOST_TO_LENDIAN_INT16(connectorObjectID);
					} else if (command
						== ATOM_TRANSMITTER_ACTION_SETUP_VSEMPH) {
						args.v1.asMode.ucLaneSel = laneNumber;
						args.v1.asMode.ucLaneSet = laneSet;
					} else {
						if (isDP) {
							args.v1.usPixelClock
								= B_HOST_TO_LENDIAN_INT16(dpClock / 10);
						} else if (pixelClock > 165000) {
							args.v1.usPixelClock = B_HOST_TO_LENDIAN_INT16(
								(pixelClock / 2) / 10);
						} else {
							args.v1.usPixelClock
								= B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
						}
					}

					args.v1.ucConfig = ATOM_TRANSMITTER_CONFIG_CLKSRC_PPLL;

					if (digEncoderID > 0) {
						args.v1.ucConfig
							|= ATOM_TRANSMITTER_CONFIG_DIG2_ENCODER;
					} else {
						args.v1.ucConfig
							|= ATOM_TRANSMITTER_CONFIG_DIG1_ENCODER;
					}

					// TODO: IGP DIG Transmitter setup
					#if 0
					if ((rdev->flags & RADEON_IS_IGP) && (encoderObjectID
						== ENCODER_OBJECT_ID_INTERNAL_UNIPHY)) {
						if (is_dp || (radeon_encoder->pixel_clock <= 165000)) {
							if (igp_lane_info & 0x1)
								args.v1.ucConfig
									|= ATOM_TRANSMITTER_CONFIG_LANE_0_3;
							else if (igp_lane_info & 0x2)
								args.v1.ucConfig
									|= ATOM_TRANSMITTER_CONFIG_LANE_4_7;
							else if (igp_lane_info & 0x4)
								args.v1.ucConfig
									|= ATOM_TRANSMITTER_CONFIG_LANE_8_11;
							else if (igp_lane_info & 0x8)
								args.v1.ucConfig
									|= ATOM_TRANSMITTER_CONFIG_LANE_12_15;
						} else {
							if (igp_lane_info & 0x3)
								args.v1.ucConfig
									|= ATOM_TRANSMITTER_CONFIG_LANE_0_7;
							else if (igp_lane_info & 0xc)
								args.v1.ucConfig
									|= ATOM_TRANSMITTER_CONFIG_LANE_8_15;
						}
					}
					#endif

					if (linkB == true)
						args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_LINKB;
					else
						args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_LINKA;

					if (isDP)
						args.v1.ucConfig |= ATOM_TRANSMITTER_CONFIG_COHERENT;
					else if ((gConnector[connectorIndex]->encoder.flags
						& ATOM_DEVICE_DFP_SUPPORT) != 0) {
						if (1) {
							// if coherentMode, i've only ever seen it true
							args.v1.ucConfig
								|= ATOM_TRANSMITTER_CONFIG_COHERENT;
						}
						if (pixelClock > 165000) {
							args.v1.ucConfig
								|= ATOM_TRANSMITTER_CONFIG_8LANE_LINK;
						}
					}
					break;
				case 2:
					args.v2.ucAction = command;
					if (command == ATOM_TRANSMITTER_ACTION_INIT) {
						args.v2.usInitInfo
							= B_HOST_TO_LENDIAN_INT16(connectorObjectID);
					} else if (command
						== ATOM_TRANSMITTER_ACTION_SETUP_VSEMPH) {
						args.v2.asMode.ucLaneSel = laneNumber;
						args.v2.asMode.ucLaneSet = laneSet;
					} else {
						if (isDP) {
							args.v2.usPixelClock
								= B_HOST_TO_LENDIAN_INT16(dpClock / 10);
						} else if (pixelClock > 165000) {
							args.v2.usPixelClock = B_HOST_TO_LENDIAN_INT16(
								(pixelClock / 2) / 10);
						} else {
							args.v2.usPixelClock
								= B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
						}
					}
					args.v2.acConfig.ucEncoderSel = digEncoderID;
					if (linkB)
						args.v2.acConfig.ucLinkSel = 1;

					switch (encoderObjectID) {
						case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
							args.v2.acConfig.ucTransmitterSel = 0;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
							args.v2.acConfig.ucTransmitterSel = 1;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
							args.v2.acConfig.ucTransmitterSel = 2;
							break;
					}

					if (isDP) {
						args.v2.acConfig.fCoherentMode = 1;
						args.v2.acConfig.fDPConnector = 1;
					} else if ((gConnector[connectorIndex]->encoder.flags
						& ATOM_DEVICE_DFP_SUPPORT) != 0) {
						if (1) {
							// if coherentMode, i've only ever seen it true
							args.v2.acConfig.fCoherentMode = 1;
						}

						if (pixelClock > 165000)
							args.v2.acConfig.fDualLinkConnector = 1;
					}
					break;
				case 3:
					args.v3.ucAction = command;
					if (command == ATOM_TRANSMITTER_ACTION_INIT) {
						args.v3.usInitInfo
							= B_HOST_TO_LENDIAN_INT16(connectorObjectID);
					} else if (command
						== ATOM_TRANSMITTER_ACTION_SETUP_VSEMPH) {
						args.v3.asMode.ucLaneSel = laneNumber;
						args.v3.asMode.ucLaneSet = laneSet;
					} else {
						if (isDP) {
							args.v3.usPixelClock
								= B_HOST_TO_LENDIAN_INT16(dpClock / 10);
						} else if (pixelClock > 165000) {
							args.v3.usPixelClock = B_HOST_TO_LENDIAN_INT16(
								(pixelClock / 2) / 10);
						} else {
							args.v3.usPixelClock
								= B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
						}
					}

					if (isDP)
						args.v3.ucLaneNum = dpLaneCount;
					else if (pixelClock > 165000)
						args.v3.ucLaneNum = 8;
					else
						args.v3.ucLaneNum = 4;

					if (linkB == true)
						args.v3.acConfig.ucLinkSel = 1;
					if (digEncoderID & 1)
						args.v3.acConfig.ucEncoderSel = 1;

					// Select the PLL for the PHY
					// DP PHY to be clocked from external src if possible
					if (isDP && pll->dpExternalClock) {
						// use external clock source
						args.v3.acConfig.ucRefClkSource = 2;
					} else
						args.v3.acConfig.ucRefClkSource = pll->id;

					switch (encoderObjectID) {
						case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
							args.v3.acConfig.ucTransmitterSel = 0;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
							args.v3.acConfig.ucTransmitterSel = 1;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
							args.v3.acConfig.ucTransmitterSel = 2;
							break;
					}

					if (isDP)
						args.v3.acConfig.fCoherentMode = 1;
					else if ((gConnector[connectorIndex]->encoder.flags
						& ATOM_DEVICE_DFP_SUPPORT) != 0) {
						if (1) {
							// if coherentMode, i've only ever seen it true
							args.v3.acConfig.fCoherentMode = 1;
						}
						if (pixelClock > 165000)
							args.v3.acConfig.fDualLinkConnector = 1;
					}
					break;
				case 4:
					args.v4.ucAction = command;
					if (command == ATOM_TRANSMITTER_ACTION_INIT) {
						args.v4.usInitInfo
							= B_HOST_TO_LENDIAN_INT16(connectorObjectID);
					} else if (command
						== ATOM_TRANSMITTER_ACTION_SETUP_VSEMPH) {
						args.v4.asMode.ucLaneSel = laneNumber;
						args.v4.asMode.ucLaneSet = laneSet;
					} else {
						if (isDP) {
							args.v4.usPixelClock
								= B_HOST_TO_LENDIAN_INT16(dpClock / 10);
						} else if (pixelClock > 165000) {
							args.v4.usPixelClock = B_HOST_TO_LENDIAN_INT16(
								(pixelClock / 2) / 10);
						} else {
							args.v4.usPixelClock
								= B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
						}
					}

					if (isDP)
						args.v4.ucLaneNum = dpLaneCount;
					else if (pixelClock > 165000)
						args.v4.ucLaneNum = 8;
					else
						args.v4.ucLaneNum = 4;

					if (linkB == true)
						args.v4.acConfig.ucLinkSel = 1;
					if (digEncoderID & 1)
						args.v4.acConfig.ucEncoderSel = 1;

					// Select the PLL for the PHY
					// DP PHY to be clocked from external src if possible
					if (isDP) {
						if (pll->dpExternalClock > 0) {
							args.v4.acConfig.ucRefClkSource
								= ENCODER_REFCLK_SRC_EXTCLK;
						} else {
							args.v4.acConfig.ucRefClkSource
								= ENCODER_REFCLK_SRC_DCPLL;
						}
					} else
						args.v4.acConfig.ucRefClkSource = pll->id;

					switch (encoderObjectID) {
						case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
							args.v4.acConfig.ucTransmitterSel = 0;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
							args.v4.acConfig.ucTransmitterSel = 1;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
							args.v4.acConfig.ucTransmitterSel = 2;
							break;
					}

					if (isDP)
						args.v4.acConfig.fCoherentMode = 1;
					else if ((gConnector[connectorIndex]->encoder.flags
						& ATOM_DEVICE_DFP_SUPPORT) != 0) {
						if (1) {
							// if coherentMode, i've only ever seen it true
							args.v4.acConfig.fCoherentMode = 1;
						}
						if (pixelClock > 165000)
							args.v4.acConfig.fDualLinkConnector = 1;
					}
					break;
				default:
					ERROR("%s: unknown table version\n", __func__);
			}
			break;
		default:
			ERROR("%s: unknown table version\n", __func__);
	}

	return atom_execute_table(gAtomContext, index, (uint32*)&args);
}


void
encoder_crtc_scratch(uint8 crtcID)
{
	TRACE("%s\n", __func__);

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint32 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	// TODO: r500
	uint32 biosScratch3 = Read32(OUT, R600_BIOS_3_SCRATCH);

	if ((encoderFlags & ATOM_DEVICE_TV1_SUPPORT) != 0) {
		biosScratch3 &= ~ATOM_S3_TV1_CRTC_ACTIVE;
		biosScratch3 |= (crtcID << 18);
	}
	if ((encoderFlags & ATOM_DEVICE_CV_SUPPORT) != 0) {
		biosScratch3 &= ~ATOM_S3_CV_CRTC_ACTIVE;
		biosScratch3 |= (crtcID << 24);
	}
	if ((encoderFlags & ATOM_DEVICE_CRT1_SUPPORT) != 0) {
		biosScratch3 &= ~ATOM_S3_CRT1_CRTC_ACTIVE;
		biosScratch3 |= (crtcID << 16);
	}
	if ((encoderFlags & ATOM_DEVICE_CRT2_SUPPORT) != 0) {
		biosScratch3 &= ~ATOM_S3_CRT2_CRTC_ACTIVE;
		biosScratch3 |= (crtcID << 20);
	}
	if ((encoderFlags & ATOM_DEVICE_LCD1_SUPPORT) != 0) {
		biosScratch3 &= ~ATOM_S3_LCD1_CRTC_ACTIVE;
		biosScratch3 |= (crtcID << 17);
	}
	if ((encoderFlags & ATOM_DEVICE_DFP1_SUPPORT) != 0) {
		biosScratch3 &= ~ATOM_S3_DFP1_CRTC_ACTIVE;
		biosScratch3 |= (crtcID << 19);
	}
	if ((encoderFlags & ATOM_DEVICE_DFP2_SUPPORT) != 0) {
		biosScratch3 &= ~ATOM_S3_DFP2_CRTC_ACTIVE;
		biosScratch3 |= (crtcID << 23);
	}
	if ((encoderFlags & ATOM_DEVICE_DFP3_SUPPORT) != 0) {
		biosScratch3 &= ~ATOM_S3_DFP3_CRTC_ACTIVE;
		biosScratch3 |= (crtcID << 25);
	}

	// TODO: r500
	Write32(OUT, R600_BIOS_3_SCRATCH, biosScratch3);
}


void
encoder_dpms_scratch(uint8 crtcID, bool power)
{
	TRACE("%s\n", __func__);

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint32 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	// TODO: r500
	uint32 biosScratch2 = Read32(OUT, R600_BIOS_2_SCRATCH);

	if ((encoderFlags & ATOM_DEVICE_TV1_SUPPORT) != 0) {
		if (power == true)
			biosScratch2 &= ~ATOM_S2_TV1_DPMS_STATE;
		else
			biosScratch2 |= ATOM_S2_TV1_DPMS_STATE;
	}
	if ((encoderFlags & ATOM_DEVICE_CV_SUPPORT) != 0) {
		if (power == true)
			biosScratch2 &= ~ATOM_S2_CV_DPMS_STATE;
		else
			biosScratch2 |= ATOM_S2_CV_DPMS_STATE;
	}
	if ((encoderFlags & ATOM_DEVICE_CRT1_SUPPORT) != 0) {
		if (power == true)
			biosScratch2 &= ~ATOM_S2_CRT1_DPMS_STATE;
		else
			biosScratch2 |= ATOM_S2_CRT1_DPMS_STATE;
	}
	if ((encoderFlags & ATOM_DEVICE_CRT2_SUPPORT) != 0) {
		if (power == true)
			biosScratch2 &= ~ATOM_S2_CRT2_DPMS_STATE;
		else
			biosScratch2 |= ATOM_S2_CRT2_DPMS_STATE;
	}
	if ((encoderFlags & ATOM_DEVICE_LCD1_SUPPORT) != 0) {
		if (power == true)
			biosScratch2 &= ~ATOM_S2_LCD1_DPMS_STATE;
		else
			biosScratch2 |= ATOM_S2_LCD1_DPMS_STATE;
	}
	if ((encoderFlags & ATOM_DEVICE_DFP1_SUPPORT) != 0) {
		if (power == true)
			biosScratch2 &= ~ATOM_S2_DFP1_DPMS_STATE;
		else
			biosScratch2 |= ATOM_S2_DFP1_DPMS_STATE;
	}
	if ((encoderFlags & ATOM_DEVICE_DFP2_SUPPORT) != 0) {
		if (power == true)
			biosScratch2 &= ~ATOM_S2_DFP2_DPMS_STATE;
		else
			biosScratch2 |= ATOM_S2_DFP2_DPMS_STATE;
	}
	if ((encoderFlags & ATOM_DEVICE_DFP3_SUPPORT) != 0) {
		if (power == true)
			biosScratch2 &= ~ATOM_S2_DFP3_DPMS_STATE;
		else
			biosScratch2 |= ATOM_S2_DFP3_DPMS_STATE;
	}
	if ((encoderFlags & ATOM_DEVICE_DFP4_SUPPORT) != 0) {
		if (power == true)
			biosScratch2 &= ~ATOM_S2_DFP4_DPMS_STATE;
		else
			biosScratch2 |= ATOM_S2_DFP4_DPMS_STATE;
	}
	if ((encoderFlags & ATOM_DEVICE_DFP5_SUPPORT) != 0) {
		if (power == true)
			biosScratch2 &= ~ATOM_S2_DFP5_DPMS_STATE;
		else
			biosScratch2 |= ATOM_S2_DFP5_DPMS_STATE;
	}
	Write32(OUT, R600_BIOS_2_SCRATCH, biosScratch2);
}


void
encoder_dpms_set(uint8 crtcID, int mode)
{
	TRACE("%s: power: %s\n", __func__, mode == B_DPMS_ON ? "true" : "false");

	int index = -1;
	radeon_shared_info &info = *gInfo->shared_info;

	DISPLAY_DEVICE_OUTPUT_CONTROL_PS_ALLOCATION args;
	memset(&args, 0, sizeof(args));

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint32 encoderFlags = gConnector[connectorIndex]->encoder.flags;
	uint16 encoderID = gConnector[connectorIndex]->encoder.objectID;

	switch (encoderID) {
		case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
			index = GetIndexIntoMasterTable(COMMAND, TMDSAOutputControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
			encoder_dpms_set_dig(crtcID, mode);
			return;
		case ENCODER_OBJECT_ID_INTERNAL_DVO1:
		case ENCODER_OBJECT_ID_INTERNAL_DDI:
			index = GetIndexIntoMasterTable(COMMAND, DVOOutputControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
			if (info.dceMajor >= 5)
				encoder_dpms_set_dvo(crtcID, mode);
			else if (info.dceMajor >= 3)
				encoder_dpms_set_dig(crtcID, mode);
			else
				index = GetIndexIntoMasterTable(COMMAND, DVOOutputControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_LVDS:
			index = GetIndexIntoMasterTable(COMMAND, LCD1OutputControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
			if ((encoderFlags & ATOM_DEVICE_LCD_SUPPORT) != 0)
				index = GetIndexIntoMasterTable(COMMAND, LCD1OutputControl);
			else
				index = GetIndexIntoMasterTable(COMMAND, LVTMAOutputControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_DAC1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
			if ((encoderFlags & ATOM_DEVICE_TV_SUPPORT) != 0)
				index = GetIndexIntoMasterTable(COMMAND, TV1OutputControl);
			else if ((encoderFlags & ATOM_DEVICE_CV_SUPPORT) != 0)
				index = GetIndexIntoMasterTable(COMMAND, CV1OutputControl);
			else
				index = GetIndexIntoMasterTable(COMMAND, DAC1OutputControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_DAC2:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
			if ((encoderFlags & ATOM_DEVICE_TV_SUPPORT) != 0)
				index = GetIndexIntoMasterTable(COMMAND, TV1OutputControl);
			else if ((encoderFlags & ATOM_DEVICE_CV_SUPPORT) != 0)
				index = GetIndexIntoMasterTable(COMMAND, CV1OutputControl);
			else
				index = GetIndexIntoMasterTable(COMMAND, DAC2OutputControl);
			break;
		// default, none on purpose
	}

	switch (mode) {
		case B_DPMS_ON:
			args.ucAction = ATOM_ENABLE;
			break;
		case B_DPMS_STAND_BY:
		case B_DPMS_SUSPEND:
		case B_DPMS_OFF:
			args.ucAction = ATOM_DISABLE;
			break;
	}

	if (index >= 0) {
		atom_execute_table(gAtomContext, index, (uint32*)&args);
		if (info.dceMajor < 5) {
			if ((encoderFlags & ATOM_DEVICE_LCD_SUPPORT) != 0) {
				args.ucAction = args.ucAction == ATOM_DISABLE
					? ATOM_LCD_BLOFF : ATOM_LCD_BLON;
				atom_execute_table(gAtomContext, index, (uint32*)&args);
			}
			encoder_dpms_scratch(crtcID, true);
		}
	}

	/* TODO: I feel as though what is below may be incorrect...
	 * AMD: the ext encoder will only show up in conjunction with an internal
	 * encoder, the pipeline generally looks like crtc -> dvo -> ext encoder
	 * or crtc -> uniphy -> ext encoder
	 */

	if (encoder_is_external(encoderID))
		encoder_dpms_set_external(crtcID, mode);
}


void
encoder_dpms_set_dig(uint8 crtcID, int mode)
{
	TRACE("%s: power: %s\n", __func__, mode == B_DPMS_ON ? "true" : "false");

	radeon_shared_info &info = *gInfo->shared_info;
	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint32 encoderFlags = gConnector[connectorIndex]->encoder.flags;
	pll_info* pll = &gConnector[connectorIndex]->encoder.pll;

	switch (mode) {
		case B_DPMS_ON:
			if (info.chipsetID == RADEON_RV710
				|| info.chipsetID == RADEON_RV730
				|| (info.chipsetFlags & CHIP_APU) != 0
				|| info.dceMajor >= 5) {
				transmitter_dig_setup(connectorIndex, pll->pixelClock, 0, 0,
					ATOM_TRANSMITTER_ACTION_ENABLE);
			} else {
				transmitter_dig_setup(connectorIndex, pll->pixelClock, 0, 0,
					ATOM_TRANSMITTER_ACTION_ENABLE_OUTPUT);
			}
			if (connector_is_dp(connectorIndex)) {
				if (gConnector[connectorIndex]->type == VIDEO_CONNECTOR_EDP) {
					ERROR("%s: TODO, edp_panel_power for this card!\n",
						__func__);
					// atombios_set_edp_panel_power(connector,
					//	ATOM_TRANSMITTER_ACTION_POWER_ON);
				}
				if (info.dceMajor >= 4) {
					encoder_dig_setup(connectorIndex, pll->pixelClock,
						ATOM_ENCODER_CMD_DP_VIDEO_OFF);
				}
				// TODO: dp link train here
				//radeon_dp_link_train(encoder, connector);
				if (info.dceMajor >= 4) {
					encoder_dig_setup(connectorIndex, pll->pixelClock,
						ATOM_ENCODER_CMD_DP_VIDEO_ON);
				}
			}
			if ((encoderFlags & ATOM_DEVICE_LCD_SUPPORT) != 0) {
				transmitter_dig_setup(connectorIndex, pll->pixelClock,
					0, 0, ATOM_TRANSMITTER_ACTION_LCD_BLON);
			}
			break;
		case B_DPMS_STAND_BY:
		case B_DPMS_SUSPEND:
		case B_DPMS_OFF:
			if ((info.chipsetFlags & CHIP_APU) != 0 || info.dceMajor >= 5) {
				transmitter_dig_setup(connectorIndex, pll->pixelClock, 0, 0,
					ATOM_TRANSMITTER_ACTION_DISABLE);
			} else {
				transmitter_dig_setup(connectorIndex, pll->pixelClock, 0, 0,
					ATOM_TRANSMITTER_ACTION_DISABLE_OUTPUT);
			}
			if (connector_is_dp(connectorIndex)) {
				if (info.dceMajor >= 4) {
					encoder_dig_setup(connectorIndex, pll->pixelClock,
						ATOM_ENCODER_CMD_DP_VIDEO_OFF);
					#if 0
					if (connector->connector_type == DRM_MODE_CONNECTOR_eDP) {
						atombios_set_edp_panel_power(connector,
							ATOM_TRANSMITTER_ACTION_POWER_OFF);
						radeon_dig_connector->edp_on = false;
					#endif
				}
			}
			if ((encoderFlags & ATOM_DEVICE_LCD_SUPPORT) != 0) {
				transmitter_dig_setup(connectorIndex, pll->pixelClock,
					0, 0, ATOM_TRANSMITTER_ACTION_LCD_BLOFF);
			}
			break;
	}
}


void
encoder_dpms_set_external(uint8 crtcID, int mode)
{
	TRACE("%s: power: %s\n", __func__, mode == B_DPMS_ON ? "true" : "false");

	radeon_shared_info &info = *gInfo->shared_info;
	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	pll_info* pll = &gConnector[connectorIndex]->encoder.pll;

	switch (mode) {
		case B_DPMS_ON:
			if ((info.chipsetFlags & CHIP_APU) != 0) {
				encoder_external_setup(connectorIndex, pll->pixelClock,
					EXTERNAL_ENCODER_ACTION_V3_ENABLE_OUTPUT);
				encoder_external_setup(connectorIndex, pll->pixelClock,
					EXTERNAL_ENCODER_ACTION_V3_ENCODER_BLANKING_OFF);
			} else {
				encoder_external_setup(connectorIndex, pll->pixelClock,
					ATOM_ENABLE);
			}
			break;
		case B_DPMS_STAND_BY:
		case B_DPMS_SUSPEND:
		case B_DPMS_OFF:
			if ((info.chipsetFlags & CHIP_APU) != 0) {
				encoder_external_setup(connectorIndex, pll->pixelClock,
					EXTERNAL_ENCODER_ACTION_V3_ENCODER_BLANKING);
				encoder_external_setup(connectorIndex, pll->pixelClock,
					EXTERNAL_ENCODER_ACTION_V3_DISABLE_OUTPUT);
			} else {
				encoder_external_setup(connectorIndex, pll->pixelClock,
					ATOM_DISABLE);
			}
			break;
	}
}


void
encoder_dpms_set_dvo(uint8 crtcID, int mode)
{
	ERROR("%s: TODO, dvo encoder dpms stub\n", __func__);
}


void
encoder_output_lock(bool lock)
{
	TRACE("%s: %s\n", __func__, lock ? "true" : "false");
	uint32 biosScratch6 = Read32(OUT, R600_BIOS_6_SCRATCH);

	if (lock) {
		biosScratch6 |= ATOM_S6_CRITICAL_STATE;
		biosScratch6 &= ~ATOM_S6_ACC_MODE;
	} else {
		biosScratch6 &= ~ATOM_S6_CRITICAL_STATE;
		biosScratch6 |= ATOM_S6_ACC_MODE;
	}

	Write32(OUT, R600_BIOS_6_SCRATCH, biosScratch6);
}


static const char* encoder_name_matrix[36] = {
	"NONE",
	"Internal Radeon LVDS",
	"Internal Radeon TMDS1",
	"Internal Radeon TMDS2",
	"Internal Radeon DAC1",
	"Internal Radeon DAC2 (TV)",
	"Internal Radeon SDVOA",
	"Internal Radeon SDVOB",
	"External 3rd party SI170B",
	"External 3rd party CH7303",
	"External 3rd party CH7301",
	"Internal Radeon DVO1",
	"External 3rd party SDVOA",
	"External 3rd party SDVOB",
	"External 3rd party TITFP513",
	"Internal LVTM1",
	"External 3rd party VT1623",
	"External HDMI SI1930",
	"Internal HDMI",
	"Internal Kaleidoscope TMDS1",
	"Internal Kaleidoscope DVO1",
	"Internal Kaleidoscope DAC1",
	"Internal Kaleidoscope DAC2",
	"External Kaleidoscope SI178",
	"MVPU FPGA",
	"Internal Kaleidoscope DDI",
	"External Kaleidoscope VT1625",
	"External Kaleidoscope HDMI SI1932",
	"External Kaleidoscope DP AN9801",
	"External Kaleidoscope DP DP501",
	"Internal Kaleidoscope UNIPHY",
	"Internal Kaleidoscope LVTMA",
	"Internal Kaleidoscope UNIPHY1",
	"Internal Kaleidoscope UNIPHY2",
	"External Travis Bridge",
	"External Nutmeg Bridge"
};


const char*
encoder_name_lookup(uint32 encoderID) {
	if (encoderID <= sizeof(encoder_name_matrix))
		return encoder_name_matrix[encoderID];
	else
		return "Unknown";
}


uint32
encoder_object_lookup(uint32 encoderFlags, uint8 dacID)
{
	// used on older cards to take a guess at the encoder
	// object

	radeon_shared_info &info = *gInfo->shared_info;

	uint32 ret = 0;

	switch (encoderFlags) {
		case ATOM_DEVICE_CRT1_SUPPORT:
		case ATOM_DEVICE_TV1_SUPPORT:
		case ATOM_DEVICE_TV2_SUPPORT:
		case ATOM_DEVICE_CRT2_SUPPORT:
		case ATOM_DEVICE_CV_SUPPORT:
			switch (dacID) {
				case 1:
					if ((info.chipsetID == RADEON_RS400)
						|| (info.chipsetID == RADEON_RS480))
						ret = ENCODER_INTERNAL_DAC2_ENUM_ID1;
					else if (info.chipsetID >= RADEON_RS600)
						ret = ENCODER_INTERNAL_KLDSCP_DAC1_ENUM_ID1;
					else
						ret = ENCODER_INTERNAL_DAC1_ENUM_ID1;
					break;
				case 2:
					if (info.chipsetID >= RADEON_RS600)
						ret = ENCODER_INTERNAL_KLDSCP_DAC2_ENUM_ID1;
					else {
						ret = ENCODER_INTERNAL_DAC2_ENUM_ID1;
					}
					break;
				case 3: // external dac
					if (info.chipsetID >= RADEON_RS600)
						ret = ENCODER_INTERNAL_KLDSCP_DVO1_ENUM_ID1;
					else
						ret = ENCODER_INTERNAL_DVO1_ENUM_ID1;
					break;
			}
			break;
		case ATOM_DEVICE_LCD1_SUPPORT:
			if (info.chipsetID >= RADEON_RS600)
				ret = ENCODER_INTERNAL_LVTM1_ENUM_ID1;
			else
				ret = ENCODER_INTERNAL_LVDS_ENUM_ID1;
			break;
		case ATOM_DEVICE_DFP1_SUPPORT:
			if ((info.chipsetID == RADEON_RS400)
				|| (info.chipsetID == RADEON_RS480))
				ret = ENCODER_INTERNAL_DVO1_ENUM_ID1;
			else if (info.chipsetID >= RADEON_RS600)
				ret = ENCODER_INTERNAL_KLDSCP_TMDS1_ENUM_ID1;
			else
				ret = ENCODER_INTERNAL_TMDS1_ENUM_ID1;
			break;
		case ATOM_DEVICE_LCD2_SUPPORT:
		case ATOM_DEVICE_DFP2_SUPPORT:
			if ((info.chipsetID == RADEON_RS600)
				|| (info.chipsetID == RADEON_RS690)
				|| (info.chipsetID == RADEON_RS740))
				ret = ENCODER_INTERNAL_DDI_ENUM_ID1;
			else if (info.chipsetID >= RADEON_RS600)
				ret = ENCODER_INTERNAL_KLDSCP_DVO1_ENUM_ID1;
			else
				ret = ENCODER_INTERNAL_DVO1_ENUM_ID1;
			break;
		case ATOM_DEVICE_DFP3_SUPPORT:
			ret = ENCODER_INTERNAL_LVTM1_ENUM_ID1;
			break;
	}

	return ret;
}


uint32
encoder_type_lookup(uint32 encoderID, uint32 connectorFlags)
{
	switch (encoderID) {
		case ENCODER_OBJECT_ID_INTERNAL_LVDS:
		case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
		case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
			if ((connectorFlags & ATOM_DEVICE_LCD_SUPPORT) != 0)
				return VIDEO_ENCODER_LVDS;
			else
				return VIDEO_ENCODER_TMDS;
			break;
		case ENCODER_OBJECT_ID_INTERNAL_DAC1:
			return VIDEO_ENCODER_DAC;
		case ENCODER_OBJECT_ID_INTERNAL_DAC2:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
			return VIDEO_ENCODER_TVDAC;
		case ENCODER_OBJECT_ID_INTERNAL_DVO1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
		case ENCODER_OBJECT_ID_INTERNAL_DDI:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
			if ((connectorFlags & ATOM_DEVICE_LCD_SUPPORT) != 0)
				return VIDEO_ENCODER_LVDS;
			else if ((connectorFlags & ATOM_DEVICE_CRT_SUPPORT) != 0)
				return VIDEO_ENCODER_DAC;
			else
				return VIDEO_ENCODER_TMDS;
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
			if ((connectorFlags & ATOM_DEVICE_LCD_SUPPORT) != 0)
				return VIDEO_ENCODER_LVDS;
			else if ((connectorFlags & ATOM_DEVICE_CRT_SUPPORT) != 0)
				return VIDEO_ENCODER_DAC;
			else
				return VIDEO_ENCODER_TMDS;
			break;
	}

	return VIDEO_ENCODER_NONE;
}


bool
encoder_is_external(uint32 encoderID)
{
	switch (encoderID) {
		case ENCODER_OBJECT_ID_SI170B:
		case ENCODER_OBJECT_ID_CH7303:
		case ENCODER_OBJECT_ID_EXTERNAL_SDVOA:
		case ENCODER_OBJECT_ID_EXTERNAL_SDVOB:
		case ENCODER_OBJECT_ID_TITFP513:
		case ENCODER_OBJECT_ID_VT1623:
		case ENCODER_OBJECT_ID_HDMI_SI1930:
		case ENCODER_OBJECT_ID_TRAVIS:
		case ENCODER_OBJECT_ID_NUTMEG:
			return true;
	}

	return false;
}


bool
encoder_is_dp_bridge(uint32 encoderID)
{
	switch (encoderID) {
		case ENCODER_OBJECT_ID_TRAVIS:
		case ENCODER_OBJECT_ID_NUTMEG:
			return true;
	}
	return false;
}
