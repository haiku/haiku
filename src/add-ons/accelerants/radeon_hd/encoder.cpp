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
	uint16 encoder_id = gConnector[connector_index]->encoder_object_id;

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
							//if (radeon_encoder->devices
							//	& ATOM_DEVICE_LCD1_SUPPORT)
							//	args.v1.ucDevice = ATOM_DEVICE_LCD1_INDEX;
							//else
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

	switch (gConnector[connector_index]->encoder_object_id) {
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
			TRACE("%s: TODO for digital encoder setup\n", __func__);
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


void
encoder_analog_setup(uint8 id, uint32 pixelClock, int command)
{
	TRACE("%s\n", __func__);

	uint32 connector_index = gDisplay[id]->connector_index;

	int index = 0;
	DAC_ENCODER_CONTROL_PS_ALLOCATION args;
	memset(&args, 0, sizeof(args));

	switch (gConnector[connector_index]->encoder_object_id) {
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

	atom_execute_table(gAtomContext, index, (uint32*)&args);
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
