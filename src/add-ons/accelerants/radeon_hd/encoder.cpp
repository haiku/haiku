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
encoder_assign_crtc(uint8 id)
{
	int index = GetIndexIntoMasterTable(COMMAND, SelectCRTC_Source);
	union crtc_source_param args;
	uint8 frev;
	uint8 crev;

	memset(&args, 0, sizeof(args));

	if (atom_parse_cmd_header(gAtomContext, index, &frev, &crev)
		!= B_OK)
		return;

	uint16 connector_index = gDisplay[id]->connector_index;
	uint16 encoder_id = gConnector[connector_index]->encoder.object_id;

	switch (frev) {
		case 1:
			switch (crev) {
				case 1:
				default:
					args.v1.ucCRTC = id;
					switch (encoder_id) {
						case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
						case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
							args.v1.ucDevice = ATOM_DEVICE_DFP1_INDEX;
							break;
						case ENCODER_OBJECT_ID_INTERNAL_LVDS:
						case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
							if (gConnector[connector_index]->flags
								& ATOM_DEVICE_LCD1_SUPPORT)
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
					args.v2.ucCRTC = id;
					args.v2.ucEncodeMode
						= display_get_encoder_mode(connector_index);
					switch (encoder_id) {
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
			ERROR("%s: Unknown table version: %d, %d\n", __func__, frev, crev);
			return;
	}

	atom_execute_table(gAtomContext, index, (uint32*)&args);

	// TODO : encoder_crtc_scratch_regs?
}


void
encoder_mode_set(uint8 id, uint32 pixelClock)
{
	uint32 connector_index = gDisplay[id]->connector_index;

	switch (gConnector[connector_index]->encoder.object_id) {
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

	uint32 connector_index = gDisplay[id]->connector_index;

	union lvds_encoder_control args;
	memset(&args, 0, sizeof(args));

	int index = 0;
	uint16 connector_flags = gConnector[connector_index]->encoder.flags;

	switch (gConnector[connector_index]->encoder.object_id) {
		case ENCODER_OBJECT_ID_INTERNAL_LVDS:
			index = GetIndexIntoMasterTable(COMMAND, LVDSEncoderControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
			index = GetIndexIntoMasterTable(COMMAND, TMDS1EncoderControl);
			break;
		case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
			if (connector_flags & ATOM_DEVICE_LCD_SUPPORT)
				index = GetIndexIntoMasterTable(COMMAND, LVDSEncoderControl);
			else
				index = GetIndexIntoMasterTable(COMMAND, TMDS2EncoderControl);
			break;
	}

	uint8 frev;
	uint8 crev;
	if (atom_parse_cmd_header(gAtomContext, index, &frev, &crev) != B_OK)
		return B_ERROR;

	switch (frev) {
	case 1:
	case 2:
		switch (crev) {
			case 1:
				args.v1.ucMisc = 0;
				args.v1.ucAction = command;
				if (0)	// TODO : HDMI?
					args.v1.ucMisc |= PANEL_ENCODER_MISC_HDMI_TYPE;
				args.v1.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);

				if (connector_flags & (ATOM_DEVICE_LCD_SUPPORT)) {
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
				if (crev == 3) {
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
				if (connector_flags & ATOM_DEVICE_LCD_SUPPORT) {
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
				ERROR("%s: Unknown minor table version: %d.%d\n", __func__,
					frev, crev);
				return B_ERROR;
		}
		break;
	default:
		ERROR("%s: Unknown major table version: %d.%d\n", __func__,
			frev, crev);
		return B_ERROR;
	}
	return atom_execute_table(gAtomContext, index, (uint32*)&args);
}


status_t
encoder_analog_setup(uint8 id, uint32 pixelClock, int command)
{
	TRACE("%s\n", __func__);

	uint32 connector_index = gDisplay[id]->connector_index;

	int index = 0;
	DAC_ENCODER_CONTROL_PS_ALLOCATION args;
	memset(&args, 0, sizeof(args));

	switch (gConnector[connector_index]->encoder.object_id) {
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


void
encoder_dpms_set(uint8 encoder_id, int mode)
{
	int index = 0;
	DISPLAY_DEVICE_OUTPUT_CONTROL_PS_ALLOCATION args;

	memset(&args, 0, sizeof(args));

	switch (encoder_id) {
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
			// TODO : Laptop LCD special cases dpms set
			// if ATOM_DEVICE_LCD_SUPPORT, LCD1OutputControl
			// else...
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
			// TODO : ATOM_DEVICE_LCD_SUPPORT : args.ucAction = ATOM_LCD_BLON;
			//		  execute again
			break;
		case B_DPMS_STAND_BY:
		case B_DPMS_SUSPEND:
		case B_DPMS_OFF:
			args.ucAction = ATOM_DISABLE;
			atom_execute_table(gAtomContext, index, (uint32*)&args);
			// TODO : ATOM_DEVICE_LCD_SUPPORT : args.ucAction = ATOM_LCD_BLOFF;
			//		  execute again
			break;
	}
}


void
encoder_output_lock(bool lock)
{
	TRACE("%s: %s\n", __func__, lock ? "true" : "false");
	uint32 bios_6_scratch = Read32(OUT, R600_BIOS_6_SCRATCH);

	if (lock) {
		bios_6_scratch |= ATOM_S6_CRITICAL_STATE;
		bios_6_scratch &= ~ATOM_S6_ACC_MODE;
	} else {
		bios_6_scratch &= ~ATOM_S6_CRITICAL_STATE;
		bios_6_scratch |= ATOM_S6_ACC_MODE;
	}

	Write32(OUT, R600_BIOS_6_SCRATCH, bios_6_scratch);
}
