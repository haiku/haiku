/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	  Alexander von Gluck, kallisti5@unixzen.com
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "bios.h"
#include "display.h"
#include "utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define TRACE_ENCODER
#ifdef TRACE_ENCODER
extern "C" void _sPrintf(const char *format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


union crtc_source_param {
	SELECT_CRTC_SOURCE_PS_ALLOCATION v1;
	SELECT_CRTC_SOURCE_PARAMETERS_V2 v2;
};


void
encoder_assign_crtc(uint8 crtcID)
{
	int index = GetIndexIntoMasterTable(COMMAND, SelectCRTC_Source);
	union crtc_source_param args;

	// Table version
	uint8 tableMajor;
	uint8 tableMinor;

	memset(&args, 0, sizeof(args));

	if (atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor)
		!= B_OK)
		return;

	uint16 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint16 encoderID = gConnector[connectorIndex]->encoder.objectID;

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
							//if (radeon_encoder->active_device
							//	& (ATOM_DEVICE_TV_SUPPORT))
							//	args.v1.ucDevice = ATOM_DEVICE_TV1_INDEX;
							//else if (radeon_encoder->active_device
							//	& (ATOM_DEVICE_CV_SUPPORT))
							//	args.v1.ucDevice = ATOM_DEVICE_CV_INDEX;
							//else
								args.v1.ucDevice = ATOM_DEVICE_CRT1_INDEX;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_DAC2:
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
							//if (radeon_encoder->active_device
							//	& (ATOM_DEVICE_TV_SUPPORT))
							//	args.v1.ucDevice = ATOM_DEVICE_TV1_INDEX;
							//else if (radeon_encoder->active_device
							//	& (ATOM_DEVICE_CV_SUPPORT))
							//	args.v1.ucDevice = ATOM_DEVICE_CV_INDEX;
							//else
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
							ERROR("%s: DIG encoder not yet supported!\n",
								__func__);
							//dig = radeon_encoder->enc_priv;
							//switch (dig->dig_encoder) {
							//	case 0:
							//		args.v2.ucEncoderID = ASIC_INT_DIG1_ENCODER_ID;
							//		break;
							//	case 1:
							//		args.v2.ucEncoderID = ASIC_INT_DIG2_ENCODER_ID;
							//		break;
							//	case 2:
							//		args.v2.ucEncoderID = ASIC_INT_DIG3_ENCODER_ID;
							//		break;
							//	case 3:
							//		args.v2.ucEncoderID = ASIC_INT_DIG4_ENCODER_ID;
							//		break;
							//	case 4:
							//		args.v2.ucEncoderID = ASIC_INT_DIG5_ENCODER_ID;
							//		break;
							//	case 5:
							//		args.v2.ucEncoderID = ASIC_INT_DIG6_ENCODER_ID;
							//		break;
							//}
							break;
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
							args.v2.ucEncoderID = ASIC_INT_DVO_ENCODER_ID;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
							//if (radeon_encoder->active_device
							//	& (ATOM_DEVICE_TV_SUPPORT))
							//	args.v2.ucEncoderID = ASIC_INT_TV_ENCODER_ID;
							//else if (radeon_encoder->active_device
							//	& (ATOM_DEVICE_CV_SUPPORT))
							//	args.v2.ucEncoderID = ASIC_INT_TV_ENCODER_ID;
							//else
								args.v2.ucEncoderID = ASIC_INT_DAC1_ENCODER_ID;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
							//if (radeon_encoder->active_device
							//	& (ATOM_DEVICE_TV_SUPPORT))
							//	args.v2.ucEncoderID = ASIC_INT_TV_ENCODER_ID;
							//else if (radeon_encoder->active_device
							//	& (ATOM_DEVICE_CV_SUPPORT))
							//	args.v2.ucEncoderID = ASIC_INT_TV_ENCODER_ID;
							//else
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


void
encoder_mode_set(uint8 id, uint32 pixelClock)
{
	uint32 connectorIndex = gDisplay[id]->connectorIndex;

	switch (gConnector[connectorIndex]->encoder.objectID) {
		case ENCODER_OBJECT_ID_INTERNAL_DAC1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
		case ENCODER_OBJECT_ID_INTERNAL_DAC2:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
			encoder_analog_setup(id, pixelClock, ATOM_ENABLE);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
		case ENCODER_OBJECT_ID_INTERNAL_LVDS:
		case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
			encoder_digital_setup(id, pixelClock, ATOM_ENABLE);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
			TRACE("%s: TODO for DIG encoder setup\n", __func__);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_DDI:
		case ENCODER_OBJECT_ID_INTERNAL_DVO1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
			TRACE("%s: TODO for DVO encoder setup\n", __func__);
			break;
		default:
			TRACE("%s: TODO for unknown encoder setup!\n", __func__);
	}

}


union lvds_encoder_control {
	LVDS_ENCODER_CONTROL_PS_ALLOCATION    v1;
	LVDS_ENCODER_CONTROL_PS_ALLOCATION_V2 v2;
};


status_t
encoder_digital_setup(uint8 id, uint32 pixelClock, int command)
{
	TRACE("%s\n", __func__);

	uint32 connectorIndex = gDisplay[id]->connectorIndex;

	union lvds_encoder_control args;
	memset(&args, 0, sizeof(args));

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
		!= B_OK)
		return B_ERROR;

	switch (tableMajor) {
	case 1:
	case 2:
		switch (tableMinor) {
			case 1:
				args.v1.ucMisc = 0;
				args.v1.ucAction = command;
				if (0)	// TODO : HDMI?
					args.v1.ucMisc |= PANEL_ENCODER_MISC_HDMI_TYPE;
				args.v1.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);

				if ((encoderFlags & ATOM_DEVICE_LCD_SUPPORT) != 0) {
					// TODO : laptop display support
					//if (dig->lcd_misc & ATOM_PANEL_MISC_DUAL)
					//	args.v1.ucMisc |= PANEL_ENCODER_MISC_DUAL;
					//if (dig->lcd_misc & ATOM_PANEL_MISC_888RGB)
					//	args.v1.ucMisc |= ATOM_PANEL_MISC_888RGB;
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
				if (0) // TODO : HDMI?
					args.v2.ucMisc |= PANEL_ENCODER_MISC_HDMI_TYPE;
				args.v2.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
				args.v2.ucTruncate = 0;
				args.v2.ucSpatial = 0;
				args.v2.ucTemporal = 0;
				args.v2.ucFRC = 0;
				if ((encoderFlags & ATOM_DEVICE_LCD_SUPPORT) != 0) {
					// TODO : laptop display support
					//if (dig->lcd_misc & ATOM_PANEL_MISC_DUAL)
					//	args.v2.ucMisc |= PANEL_ENCODER_MISC_DUAL;
					//if (dig->lcd_misc & ATOM_PANEL_MISC_SPATIAL) {
					args.v2.ucSpatial = PANEL_ENCODER_SPATIAL_DITHER_EN;
					//if (dig->lcd_misc & ATOM_PANEL_MISC_888RGB)
					//	args.v2.ucSpatial |= PANEL_ENCODER_SPATIAL_DITHER_DEPTH;
					//}
					//if (dig->lcd_misc & ATOM_PANEL_MISC_TEMPORAL) {
					//	args.v2.ucTemporal = PANEL_ENCODER_TEMPORAL_DITHER_EN;
					//	if (dig->lcd_misc & ATOM_PANEL_MISC_888RGB) {
					//		args.v2.ucTemporal
					//			|= PANEL_ENCODER_TEMPORAL_DITHER_DEPTH;
					//	}
					//	if (((dig->lcd_misc >> ATOM_PANEL_MISC_GREY_LEVEL_SHIFT)
					//		& 0x3) == 2) {
					//		args.v2.ucTemporal
					//			|= PANEL_ENCODER_TEMPORAL_LEVEL_4;
					//	}
					//}
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
encoder_analog_setup(uint8 id, uint32 pixelClock, int command)
{
	TRACE("%s\n", __func__);

	uint32 connectorIndex = gDisplay[id]->connectorIndex;

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
	args.ucDacStandard = ATOM_DAC1_PS2;
		// TODO : or ATOM_DAC1_CV if ATOM_DEVICE_CV_SUPPORT
		// TODO : or ATOM_DAC1_PAL or ATOM_DAC1_NTSC if else

	args.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);

	return atom_execute_table(gAtomContext, index, (uint32*)&args);
}


bool
encoder_analog_load_detect(uint8 connectorIndex)
{
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


void
encoder_crtc_scratch(uint8 crtcID)
{
	TRACE("%s\n", __func__);

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint32 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	// TODO : r500
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

	// TODO : r500
	Write32(OUT, R600_BIOS_3_SCRATCH, biosScratch3);
}


void
encoder_dpms_scratch(uint8 crtcID, bool power)
{
	TRACE("%s: power: %s\n", __func__, power ? "true" : "false");

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint32 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	// TODO : r500
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
encoder_dpms_set(uint8 crtcID, uint8 encoderID, int mode)
{
	int index = 0;
	DISPLAY_DEVICE_OUTPUT_CONTROL_PS_ALLOCATION args;

	memset(&args, 0, sizeof(args));

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint32 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	switch (encoderID) {
		case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
			index = GetIndexIntoMasterTable(COMMAND, TMDSAOutputControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
		case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
			ERROR("%s: TODO DIG DPMS set\n", __func__);
			return;
		case ENCODER_OBJECT_ID_INTERNAL_DVO1:
		case ENCODER_OBJECT_ID_INTERNAL_DDI:
			index = GetIndexIntoMasterTable(COMMAND, DVOOutputControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
			// TODO : encoder dpms set newer cards
			// If DCE5, dvo true
			// If DCE3, dig true
			// else...
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
			// TODO : encoder dpms dce5 dac
			// else...
			/*
			if (radeon_encoder->active_device & (ATOM_DEVICE_TV_SUPPORT))
				index = GetIndexIntoMasterTable(COMMAND, TV1OutputControl);
			else if (radeon_encoder->active_device & (ATOM_DEVICE_CV_SUPPORT))
				index = GetIndexIntoMasterTable(COMMAND, CV1OutputControl);
			else
			*/
			index = GetIndexIntoMasterTable(COMMAND, DAC1OutputControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_DAC2:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
			// TODO : tv or CV encoder on DAC2
			index = GetIndexIntoMasterTable(COMMAND, DAC2OutputControl);
			break;
	}

	switch (mode) {
		case B_DPMS_ON:
			args.ucAction = ATOM_ENABLE;
			atom_execute_table(gAtomContext, index, (uint32*)&args);
			if ((encoderFlags & ATOM_DEVICE_LCD_SUPPORT) != 0) {
				args.ucAction = ATOM_LCD_BLON;
				atom_execute_table(gAtomContext, index, (uint32*)&args);
			}
			encoder_dpms_scratch(crtcID, true);
			break;
		case B_DPMS_STAND_BY:
		case B_DPMS_SUSPEND:
		case B_DPMS_OFF:
			args.ucAction = ATOM_DISABLE;
			atom_execute_table(gAtomContext, index, (uint32*)&args);
			if ((encoderFlags & ATOM_DEVICE_LCD_SUPPORT) != 0) {
				args.ucAction = ATOM_LCD_BLOFF;
				atom_execute_table(gAtomContext, index, (uint32*)&args);
			}
			encoder_dpms_scratch(crtcID, false);
			break;
	}
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
