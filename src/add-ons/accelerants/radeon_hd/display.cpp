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


#include "display.h"

#include <stdlib.h>
#include <string.h>

#include "accelerant.h"
#include "accelerant_protos.h"
#include "bios.h"
#include "connector.h"
#include "encoder.h"


#define TRACE_DISPLAY
#ifdef TRACE_DISPLAY
extern "C" void _sPrintf(const char* format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


/*! Populate regs with device dependant register locations */
status_t
init_registers(register_info* regs, uint8 crtcID)
{
	memset(regs, 0, sizeof(register_info));

	radeon_shared_info &info = *gInfo->shared_info;

	if (info.chipsetID >= RADEON_CEDAR) {
		// Evergreen
		uint32 offset = 0;

		switch (crtcID) {
			case 0:
				offset = EVERGREEN_CRTC0_REGISTER_OFFSET;
				regs->vgaControl = AVIVO_D1VGA_CONTROL;
				break;
			case 1:
				offset = EVERGREEN_CRTC1_REGISTER_OFFSET;
				regs->vgaControl = AVIVO_D2VGA_CONTROL;
				break;
			case 2:
				offset = EVERGREEN_CRTC2_REGISTER_OFFSET;
				regs->vgaControl = EVERGREEN_D3VGA_CONTROL;
				break;
			case 3:
				offset = EVERGREEN_CRTC3_REGISTER_OFFSET;
				regs->vgaControl = EVERGREEN_D4VGA_CONTROL;
				break;
			case 4:
				offset = EVERGREEN_CRTC4_REGISTER_OFFSET;
				regs->vgaControl = EVERGREEN_D5VGA_CONTROL;
				break;
			case 5:
				offset = EVERGREEN_CRTC5_REGISTER_OFFSET;
				regs->vgaControl = EVERGREEN_D6VGA_CONTROL;
				break;
			default:
				ERROR("%s: Unknown CRTC %" B_PRIu32 "\n",
					__func__, crtcID);
				return B_ERROR;
		}

		regs->crtcOffset = offset;

		regs->grphEnable = EVERGREEN_GRPH_ENABLE + offset;
		regs->grphControl = EVERGREEN_GRPH_CONTROL + offset;
		regs->grphSwapControl = EVERGREEN_GRPH_SWAP_CONTROL + offset;

		regs->grphPrimarySurfaceAddr
			= EVERGREEN_GRPH_PRIMARY_SURFACE_ADDRESS + offset;
		regs->grphSecondarySurfaceAddr
			= EVERGREEN_GRPH_SECONDARY_SURFACE_ADDRESS + offset;
		regs->grphPrimarySurfaceAddrHigh
			= EVERGREEN_GRPH_PRIMARY_SURFACE_ADDRESS_HIGH + offset;
		regs->grphSecondarySurfaceAddrHigh
			= EVERGREEN_GRPH_SECONDARY_SURFACE_ADDRESS_HIGH + offset;

		regs->grphPitch = EVERGREEN_GRPH_PITCH + offset;
		regs->grphSurfaceOffsetX
			= EVERGREEN_GRPH_SURFACE_OFFSET_X + offset;
		regs->grphSurfaceOffsetY
			= EVERGREEN_GRPH_SURFACE_OFFSET_Y + offset;
		regs->grphXStart = EVERGREEN_GRPH_X_START + offset;
		regs->grphYStart = EVERGREEN_GRPH_Y_START + offset;
		regs->grphXEnd = EVERGREEN_GRPH_X_END + offset;
		regs->grphYEnd = EVERGREEN_GRPH_Y_END + offset;
		regs->modeDesktopHeight = EVERGREEN_DESKTOP_HEIGHT + offset;
		regs->modeDataFormat = EVERGREEN_DATA_FORMAT + offset;
		regs->viewportStart = EVERGREEN_VIEWPORT_START + offset;
		regs->viewportSize = EVERGREEN_VIEWPORT_SIZE + offset;

	} else if (info.chipsetID >= RADEON_RV770) {
		// R700 series
		uint32 offset = 0;

		switch (crtcID) {
			case 0:
				offset = R600_CRTC0_REGISTER_OFFSET;
				regs->vgaControl = AVIVO_D1VGA_CONTROL;
				regs->grphPrimarySurfaceAddrHigh
					= R700_D1GRPH_PRIMARY_SURFACE_ADDRESS_HIGH;
				break;
			case 1:
				offset = R600_CRTC1_REGISTER_OFFSET;
				regs->vgaControl = AVIVO_D2VGA_CONTROL;
				regs->grphPrimarySurfaceAddrHigh
					= R700_D2GRPH_PRIMARY_SURFACE_ADDRESS_HIGH;
				break;
			default:
				ERROR("%s: Unknown CRTC %" B_PRIu32 "\n",
					__func__, crtcID);
				return B_ERROR;
		}

		regs->crtcOffset = offset;

		regs->grphEnable = AVIVO_D1GRPH_ENABLE + offset;
		regs->grphControl = AVIVO_D1GRPH_CONTROL + offset;
		regs->grphSwapControl = D1GRPH_SWAP_CNTL + offset;

		regs->grphPrimarySurfaceAddr
			= R700_D1GRPH_PRIMARY_SURFACE_ADDRESS + offset;
		regs->grphSecondarySurfaceAddr
			= R700_D1GRPH_SECONDARY_SURFACE_ADDRESS + offset;

		regs->grphPitch = AVIVO_D1GRPH_PITCH + offset;
		regs->grphSurfaceOffsetX = AVIVO_D1GRPH_SURFACE_OFFSET_X + offset;
		regs->grphSurfaceOffsetY = AVIVO_D1GRPH_SURFACE_OFFSET_Y + offset;
		regs->grphXStart = AVIVO_D1GRPH_X_START + offset;
		regs->grphYStart = AVIVO_D1GRPH_Y_START + offset;
		regs->grphXEnd = AVIVO_D1GRPH_X_END + offset;
		regs->grphYEnd = AVIVO_D1GRPH_Y_END + offset;

		regs->modeDesktopHeight = AVIVO_D1MODE_DESKTOP_HEIGHT + offset;
		regs->modeDataFormat = AVIVO_D1MODE_DATA_FORMAT + offset;
		regs->viewportStart = AVIVO_D1MODE_VIEWPORT_START + offset;
		regs->viewportSize = AVIVO_D1MODE_VIEWPORT_SIZE + offset;

	} else if (info.chipsetID >= RADEON_RS600) {
		// Avivo+
		uint32 offset = 0;

		switch (crtcID) {
			case 0:
				offset = R600_CRTC0_REGISTER_OFFSET;
				regs->vgaControl = AVIVO_D1VGA_CONTROL;
				break;
			case 1:
				offset = R600_CRTC1_REGISTER_OFFSET;
				regs->vgaControl = AVIVO_D2VGA_CONTROL;
				break;
			default:
				ERROR("%s: Unknown CRTC %" B_PRIu32 "\n",
					__func__, crtcID);
				return B_ERROR;
		}

		regs->crtcOffset = offset;

		regs->grphEnable = AVIVO_D1GRPH_ENABLE + offset;
		regs->grphControl = AVIVO_D1GRPH_CONTROL + offset;
		regs->grphSwapControl = D1GRPH_SWAP_CNTL + offset;

		regs->grphPrimarySurfaceAddr
			= D1GRPH_PRIMARY_SURFACE_ADDRESS + offset;
		regs->grphSecondarySurfaceAddr
			= D1GRPH_SECONDARY_SURFACE_ADDRESS + offset;

		// Surface Address high only used on r700 and higher
		regs->grphPrimarySurfaceAddrHigh = 0xDEAD;
		regs->grphSecondarySurfaceAddrHigh = 0xDEAD;

		regs->grphPitch = AVIVO_D1GRPH_PITCH + offset;
		regs->grphSurfaceOffsetX = AVIVO_D1GRPH_SURFACE_OFFSET_X + offset;
		regs->grphSurfaceOffsetY = AVIVO_D1GRPH_SURFACE_OFFSET_Y + offset;
		regs->grphXStart = AVIVO_D1GRPH_X_START + offset;
		regs->grphYStart = AVIVO_D1GRPH_Y_START + offset;
		regs->grphXEnd = AVIVO_D1GRPH_X_END + offset;
		regs->grphYEnd = AVIVO_D1GRPH_Y_END + offset;

		regs->modeDesktopHeight = AVIVO_D1MODE_DESKTOP_HEIGHT + offset;
		regs->modeDataFormat = AVIVO_D1MODE_DATA_FORMAT + offset;
		regs->viewportStart = AVIVO_D1MODE_VIEWPORT_START + offset;
		regs->viewportSize = AVIVO_D1MODE_VIEWPORT_SIZE + offset;
	} else {
		// this really shouldn't happen unless a driver PCIID chipset is wrong
		TRACE("%s, unknown Radeon chipset: %s\n", __func__,
			info.chipsetName);
		return B_ERROR;
	}

	TRACE("%s, registers for ATI chipset %s crt #%d loaded\n", __func__,
		info.chipsetName, crtcID);

	return B_OK;
}


status_t
detect_crt_ranges(uint32 crtid)
{
	edid1_info* edid = &gDisplay[crtid]->edidData;

	// Scan each display EDID description for monitor ranges
	for (uint32 index = 0; index < EDID1_NUM_DETAILED_MONITOR_DESC; index++) {

		edid1_detailed_monitor* monitor
			= &edid->detailed_monitor[index];

		if (monitor->monitor_desc_type
			== EDID1_MONITOR_RANGES) {
			edid1_monitor_range range = monitor->data.monitor_range;
			gDisplay[crtid]->vfreqMin = range.min_v;   /* in Hz */
			gDisplay[crtid]->vfreqMax = range.max_v;
			gDisplay[crtid]->hfreqMin = range.min_h;   /* in kHz */
			gDisplay[crtid]->hfreqMax = range.max_h;
			return B_OK;
		}
	}

	return B_ERROR;
}


status_t
detect_displays()
{
	// reset known displays
	for (uint32 id = 0; id < MAX_DISPLAY; id++) {
		gDisplay[id]->attached = false;
		gDisplay[id]->powered = false;
		gDisplay[id]->foundRanges = false;
	}

	uint32 displayIndex = 0;
	for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
		if (gConnector[id]->valid == false)
			continue;
		if (displayIndex >= MAX_DISPLAY)
			continue;

		if (gConnector[id]->type == VIDEO_CONNECTOR_9DIN) {
			TRACE("%s: Skipping 9DIN connector (not yet supported)\n",
				__func__);
			continue;
		}

		// TODO: As DP aux transactions don't work yet, just use LVDS as a hack
		#if 0
		if (gConnector[id]->encoderExternal.isDPBridge == true) {
			// If this is a DisplayPort Bridge, setup ddc on bus
			// TRAVIS (LVDS) or NUTMEG (VGA)
			TRACE("%s: is bridge, performing bridge DDC setup\n", __func__);
			encoder_external_setup(id, 23860,
				EXTERNAL_ENCODER_ACTION_V3_DDC_SETUP);
			gDisplay[displayIndex]->attached = true;
		} else if (gConnector[id]->type == VIDEO_CONNECTOR_LVDS) {
		#endif
		if (gConnector[id]->type == VIDEO_CONNECTOR_LVDS) {
			// If plain (non-DP) laptop LVDS, read mode info from AtomBIOS
			//TRACE("%s: non-DP laptop LVDS detected\n", __func__);
			gDisplay[displayIndex]->attached
				= connector_read_mode_lvds(id,
					&gDisplay[displayIndex]->preferredMode);
		}

		// If no display found yet, try more standard detection methods
		if (gDisplay[displayIndex]->attached == false) {
			TRACE("%s: bit-banging ddc for EDID on connector %" B_PRIu32 "\n",
				__func__, id);

			// Lets try bit-banging edid from connector
			gDisplay[displayIndex]->attached
				= connector_read_edid(id, &gDisplay[displayIndex]->edidData);

			// Since DVI-I shows up as two connectors, and there is only one
			// edid channel, we have to make *sure* the edid data received is
			// valid for te connector.

			// Found EDID data?
			if (gDisplay[displayIndex]->attached) {
				TRACE("%s: found EDID data on connector %" B_PRIu32 "\n",
					__func__, id);

				bool analogEncoder
					= gConnector[id]->encoder.type == VIDEO_ENCODER_TVDAC
					|| gConnector[id]->encoder.type == VIDEO_ENCODER_DAC;

				edid1_info* edid = &gDisplay[displayIndex]->edidData;
				if (!edid->display.input_type && analogEncoder) {
					// If non-digital EDID + the encoder is analog...
					TRACE("%s: connector %" B_PRIu32 " has non-digital EDID "
						"and a analog encoder.\n", __func__, id);
					gDisplay[displayIndex]->attached
						= encoder_analog_load_detect(id);
				} else if (edid->display.input_type && !analogEncoder) {
					// If EDID is digital, we make an assumption here.
					TRACE("%s: connector %" B_PRIu32 " has digital EDID "
						"and is not a analog encoder.\n", __func__, id);
				} else {
					// This generally means the monitor is of poor design
					// Since we *know* there is no load on the analog encoder
					// we assume that it is a digital display.
					TRACE("%s: Warning: monitor on connector %" B_PRIu32 " has "
						"false digital EDID flag and unloaded analog encoder!\n",
						__func__, id);
				}
			}
		}

		if (gDisplay[displayIndex]->attached != true) {
			// Nothing interesting here, move along
			continue;
		}

		// We found a valid / attached display

		gDisplay[displayIndex]->connectorIndex = id;
			// Populate physical connector index from gConnector

		init_registers(gDisplay[displayIndex]->regs, displayIndex);

		if (gDisplay[displayIndex]->preferredMode.virtual_width > 0) {
			// Found a single preferred mode
			gDisplay[displayIndex]->foundRanges = false;
		} else {
			// Use edid data and pull ranges
			if (detect_crt_ranges(displayIndex) == B_OK)
				gDisplay[displayIndex]->foundRanges = true;
		}

		displayIndex++;
	}

	// fallback if no attached monitors were found
	if (displayIndex == 0) {
		// This is a hack, however as we don't support HPD just yet,
		// it tries to prevent a "no displays" situation.
		ERROR("%s: ERROR: 0 attached monitors were found on display connectors."
			" Injecting first connector as a last resort.\n", __func__);
		for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
			// skip TV DAC connectors as likely fallback isn't for TV
			if (gConnector[id]->encoder.type == VIDEO_ENCODER_TVDAC)
				continue;
			gDisplay[0]->attached = true;
			gDisplay[0]->connectorIndex = id;
			init_registers(gDisplay[0]->regs, 0);
			if (detect_crt_ranges(0) == B_OK)
				gDisplay[0]->foundRanges = true;
			break;
		}
	}

	// Initial boot state is the first two crtc's powered
	if (gDisplay[0]->attached == true)
		gDisplay[0]->powered = true;
	if (gDisplay[1]->attached == true)
		gDisplay[1]->powered = true;

	return B_OK;
}


void
debug_displays()
{
	TRACE("Currently detected monitors===============\n");
	for (uint32 id = 0; id < MAX_DISPLAY; id++) {
		ERROR("Display #%" B_PRIu32 " attached = %s\n",
			id, gDisplay[id]->attached ? "true" : "false");

		uint32 connectorIndex = gDisplay[id]->connectorIndex;

		if (gDisplay[id]->attached) {
			uint32 connectorType = gConnector[connectorIndex]->type;
			uint32 encoderType = gConnector[connectorIndex]->encoder.type;
			ERROR(" + connector ID:   %" B_PRIu32 "\n", connectorIndex);
			ERROR(" + connector type: %s\n", get_connector_name(connectorType));
			ERROR(" + encoder type:   %s\n", get_encoder_name(encoderType));
			ERROR(" + limits: Vert Min/Max: %" B_PRIu32 "/%" B_PRIu32"\n",
				gDisplay[id]->vfreqMin, gDisplay[id]->vfreqMax);
			ERROR(" + limits: Horz Min/Max: %" B_PRIu32 "/%" B_PRIu32"\n",
				gDisplay[id]->hfreqMin, gDisplay[id]->hfreqMax);
		}
	}
	TRACE("==========================================\n");
}


uint32
display_get_encoder_mode(uint32 connectorIndex)
{
	// Is external DisplayPort Bridge?
	if (gConnector[connectorIndex]->encoderExternal.valid == true
		&& gConnector[connectorIndex]->encoderExternal.isDPBridge == true) {
		return ATOM_ENCODER_MODE_DP;
	}

	// DVO Encoders (should be bridges)
	switch (gConnector[connectorIndex]->encoder.objectID) {
		case ENCODER_OBJECT_ID_INTERNAL_DVO1:
		case ENCODER_OBJECT_ID_INTERNAL_DDI:
		case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
			return ATOM_ENCODER_MODE_DVO;
	}

	// Find crtc for connector so we can identify source of edid data
	int32 crtc = -1;
	for (int32 id = 0; id < MAX_DISPLAY; id++) {
		if (gDisplay[id]->connectorIndex == connectorIndex) {
			crtc = id;
			break;
		}
	}
	bool edidDigital = false;
	if (crtc == -1) {
		ERROR("%s: BUG: executed on connector without crtc!\n", __func__);
	} else {
		edid1_info* edid = &gDisplay[crtc]->edidData;
		edidDigital = edid->display.input_type ? true : false;
	}

	// Normal encoder situations
	switch (gConnector[connectorIndex]->type) {
		case VIDEO_CONNECTOR_DVII:
		case VIDEO_CONNECTOR_HDMIB: /* HDMI-B is DL-DVI; analog works fine */
			// TODO: if audio detected on edid and DCE4, ATOM_ENCODER_MODE_DVI
			//        if audio detected on edid not DCE4, ATOM_ENCODER_MODE_HDMI
			if (edidDigital)
				return ATOM_ENCODER_MODE_DVI;
			else
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
			return ATOM_ENCODER_MODE_DP;
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

	args.usBlackColorRCr = 0;
	args.usBlackColorGY = 0;
	args.usBlackColorBCb = 0;

	atom_execute_table(gAtomContext, index, (uint32*)&args);
}


void
display_crtc_scale(uint8 crtcID, display_mode* mode)
{
	TRACE("%s\n", __func__);
	ENABLE_SCALER_PS_ALLOCATION args;
	int index = GetIndexIntoMasterTable(COMMAND, EnableScaler);

	memset(&args, 0, sizeof(args));

	args.ucScaler = crtcID;
	args.ucEnable = ATOM_SCALER_DISABLE;

	atom_execute_table(gAtomContext, index, (uint32*)&args);
}


void
display_crtc_dpms(uint8 crtcID, int mode)
{

	radeon_shared_info &info = *gInfo->shared_info;

	switch (mode) {
		case B_DPMS_ON:
			TRACE("%s: crtc %" B_PRIu8 " dpms powerup\n", __func__, crtcID);
			if (gDisplay[crtcID]->attached == false)
				return;
			gDisplay[crtcID]->powered = true;
			display_crtc_power(crtcID, ATOM_ENABLE);
			if (info.dceMajor >= 3)
				display_crtc_memreq(crtcID, ATOM_ENABLE);
			display_crtc_blank(crtcID, ATOM_BLANKING_OFF);
			break;
		case B_DPMS_STAND_BY:
		case B_DPMS_SUSPEND:
		case B_DPMS_OFF:
			TRACE("%s: crtc %" B_PRIu8 " dpms powerdown\n", __func__, crtcID);
			if (gDisplay[crtcID]->attached == false)
				return;
			if (gDisplay[crtcID]->powered == true)
				display_crtc_blank(crtcID, ATOM_BLANKING);
			if (info.dceMajor >= 3)
				display_crtc_memreq(crtcID, ATOM_DISABLE);
			display_crtc_power(crtcID, ATOM_DISABLE);
			gDisplay[crtcID]->powered = false;
	}
}


void
display_crtc_fb_set(uint8 crtcID, display_mode* mode)
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

	Write32(OUT, regs->vgaControl, 0);

	uint64 fbAddress = gInfo->fb.vramStart;

	TRACE("%s: Framebuffer at: 0x%" B_PRIX64 "\n", __func__, fbAddress);

	if (info.chipsetID >= RADEON_RV770) {
		TRACE("%s: Set SurfaceAddress High: 0x%" B_PRIX32 "\n",
			__func__, (fbAddress >> 32) & 0xf);

		Write32(OUT, regs->grphPrimarySurfaceAddrHigh,
			(fbAddress >> 32) & 0xf);
		Write32(OUT, regs->grphSecondarySurfaceAddrHigh,
			(fbAddress >> 32) & 0xf);
	}

	TRACE("%s: Set SurfaceAddress: 0x%" B_PRIX32 "\n",
		__func__, (fbAddress & 0xFFFFFFFF));

	Write32(OUT, regs->grphPrimarySurfaceAddr, (fbAddress & 0xFFFFFFFF));
	Write32(OUT, regs->grphSecondarySurfaceAddr, (fbAddress & 0xFFFFFFFF));

	if (info.chipsetID >= RADEON_R600) {
		Write32(CRT, regs->grphControl, fbFormat);
		Write32(CRT, regs->grphSwapControl, fbSwap);
	}

	// Align our framebuffer width
	uint32 widthAligned = mode->virtual_width;
	uint32 pitchMask = 0;

	switch (bytesPerPixel) {
		case 1:
			pitchMask = 255;
			break;
		case 2:
			pitchMask = 127;
			break;
		case 3:
		case 4:
			pitchMask = 63;
			break;
	}
	widthAligned += pitchMask;
	widthAligned &= ~pitchMask;

	TRACE("%s: fb: %" B_PRIu32 "x%" B_PRIu32 " (%" B_PRIu32 " bpp)\n", __func__,
		mode->virtual_width, mode->virtual_height, bitsPerPixel);
	TRACE("%s: fb pitch: %" B_PRIu32 " \n", __func__,
		widthAligned * bytesPerPixel / 4);
	TRACE("%s: fb width aligned: %" B_PRIu32 "\n", __func__,
		widthAligned);

	Write32(CRT, regs->grphSurfaceOffsetX, 0);
	Write32(CRT, regs->grphSurfaceOffsetY, 0);
	Write32(CRT, regs->grphXStart, 0);
	Write32(CRT, regs->grphYStart, 0);
	Write32(CRT, regs->grphXEnd, mode->virtual_width);
	Write32(CRT, regs->grphYEnd, mode->virtual_height);
	Write32(CRT, regs->grphPitch, widthAligned * bytesPerPixel / 4);

	Write32(CRT, regs->grphEnable, 1);
		// Enable Frame buffer

	Write32(CRT, regs->modeDesktopHeight, mode->virtual_height);

	uint32 viewportWidth = mode->timing.h_display;
	uint32 viewportHeight = (mode->timing.v_display + 1) & ~1;

	Write32(CRT, regs->viewportStart, 0);
	Write32(CRT, regs->viewportSize,
		(viewportWidth << 16) | viewportHeight);

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
	gInfo->shared_info->bytes_per_row = widthAligned * bytesPerPixel;
	gInfo->shared_info->current_mode = *mode;
	gInfo->shared_info->bits_per_pixel = bitsPerPixel;
}


void
display_crtc_set(uint8 crtcID, display_mode* mode)
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

	atom_execute_table(gAtomContext, index, (uint32*)&args);
}


void
display_crtc_set_dtd(uint8 crtcID, display_mode* mode)
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
display_crtc_ss(uint8 crtcID, int command)
{
	TRACE("%s\n", __func__);
	radeon_shared_info &info = *gInfo->shared_info;

	int index = GetIndexIntoMasterTable(COMMAND, EnableSpreadSpectrumOnPPLL);

	if (command != ATOM_DISABLE) {
		ERROR("%s: TODO: SS was enabled, however functionality incomplete\n",
			__func__);
		command = ATOM_DISABLE;
	}

	union enableSS {
		ENABLE_LVDS_SS_PARAMETERS lvds_ss;
		ENABLE_LVDS_SS_PARAMETERS_V2 lvds_ss_2;
		ENABLE_SPREAD_SPECTRUM_ON_PPLL_PS_ALLOCATION v1;
		ENABLE_SPREAD_SPECTRUM_ON_PPLL_V2 v2;
		ENABLE_SPREAD_SPECTRUM_ON_PPLL_V3 v3;
	};

	union enableSS args;
	memset(&args, 0, sizeof(args));

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	pll_info* pll = &gConnector[connectorIndex]->encoder.pll;

	if (info.dceMajor >= 5) {
		args.v3.usSpreadSpectrumAmountFrac = B_HOST_TO_LENDIAN_INT16(0);
		args.v3.ucSpreadSpectrumType
			= pll->ssType & ATOM_SS_CENTRE_SPREAD_MODE_MASK;
		switch (pll->id) {
			case ATOM_PPLL1:
				args.v3.ucSpreadSpectrumType |= ATOM_PPLL_SS_TYPE_V3_P1PLL;
				args.v3.usSpreadSpectrumAmount
					= B_HOST_TO_LENDIAN_INT16(pll->ssAmount);
				args.v3.usSpreadSpectrumStep
					= B_HOST_TO_LENDIAN_INT16(pll->ssStep);
				break;
			case ATOM_PPLL2:
				args.v3.ucSpreadSpectrumType |= ATOM_PPLL_SS_TYPE_V3_P2PLL;
				args.v3.usSpreadSpectrumAmount
					= B_HOST_TO_LENDIAN_INT16(pll->ssAmount);
				args.v3.usSpreadSpectrumStep
					= B_HOST_TO_LENDIAN_INT16(pll->ssStep);
				break;
			case ATOM_DCPLL:
				args.v3.ucSpreadSpectrumType |= ATOM_PPLL_SS_TYPE_V3_DCPLL;
				args.v3.usSpreadSpectrumAmount = B_HOST_TO_LENDIAN_INT16(0);
				args.v3.usSpreadSpectrumStep = B_HOST_TO_LENDIAN_INT16(0);
				break;
			default:
				ERROR("%s: BUG: Invalid PLL ID!\n", __func__);
				return;
		}
		if (pll->ssPercentage == 0
			|| ((pll->ssType & ATOM_EXTERNAL_SS_MASK) != 0)) {
			command = ATOM_DISABLE;
		}
		args.v3.ucEnable = command;
	} else if (info.dceMajor >= 4) {
		args.v2.usSpreadSpectrumPercentage
			= B_HOST_TO_LENDIAN_INT16(pll->ssPercentage);
		args.v2.ucSpreadSpectrumType
			= pll->ssType & ATOM_SS_CENTRE_SPREAD_MODE_MASK;
		switch (pll->id) {
			case ATOM_PPLL1:
				args.v2.ucSpreadSpectrumType |= ATOM_PPLL_SS_TYPE_V2_P1PLL;
				args.v2.usSpreadSpectrumAmount
					= B_HOST_TO_LENDIAN_INT16(pll->ssAmount);
				args.v2.usSpreadSpectrumStep
					= B_HOST_TO_LENDIAN_INT16(pll->ssStep);
				break;
			case ATOM_PPLL2:
				args.v2.ucSpreadSpectrumType |= ATOM_PPLL_SS_TYPE_V3_P2PLL;
				args.v2.usSpreadSpectrumAmount
					= B_HOST_TO_LENDIAN_INT16(pll->ssAmount);
				args.v2.usSpreadSpectrumStep
					= B_HOST_TO_LENDIAN_INT16(pll->ssStep);
				break;
			case ATOM_DCPLL:
				args.v2.ucSpreadSpectrumType |= ATOM_PPLL_SS_TYPE_V3_DCPLL;
				args.v2.usSpreadSpectrumAmount = B_HOST_TO_LENDIAN_INT16(0);
				args.v2.usSpreadSpectrumStep = B_HOST_TO_LENDIAN_INT16(0);
				break;
			default:
				ERROR("%s: BUG: Invalid PLL ID!\n", __func__);
				return;
		}
		if (pll->ssPercentage == 0
			|| ((pll->ssType & ATOM_EXTERNAL_SS_MASK) != 0)
			|| (info.chipsetFlags & CHIP_APU) != 0 ) {
			command = ATOM_DISABLE;
		}
		args.v2.ucEnable = command;
	} else if (info.dceMajor >= 3) {
		args.v1.usSpreadSpectrumPercentage
			= B_HOST_TO_LENDIAN_INT16(pll->ssPercentage);
		args.v1.ucSpreadSpectrumType
			= pll->ssType & ATOM_SS_CENTRE_SPREAD_MODE_MASK;
		args.v1.ucSpreadSpectrumStep = pll->ssStep;
		args.v1.ucSpreadSpectrumDelay = pll->ssDelay;
		args.v1.ucSpreadSpectrumRange = pll->ssRange;
		args.v1.ucPpll = pll->id;
		args.v1.ucEnable = command;
	} else if (info.dceMajor >= 2) {
		if ((command == ATOM_DISABLE) || (pll->ssPercentage == 0)
			|| (pll->ssType & ATOM_EXTERNAL_SS_MASK)) {
			// TODO: gpu_ss_disable needs pll id
			radeon_gpu_ss_disable();
			return;
		}
		args.lvds_ss_2.usSpreadSpectrumPercentage
			= B_HOST_TO_LENDIAN_INT16(pll->ssPercentage);
		args.lvds_ss_2.ucSpreadSpectrumType
			= pll->ssType & ATOM_SS_CENTRE_SPREAD_MODE_MASK;
		args.lvds_ss_2.ucSpreadSpectrumStep = pll->ssStep;
		args.lvds_ss_2.ucSpreadSpectrumDelay = pll->ssDelay;
		args.lvds_ss_2.ucSpreadSpectrumRange = pll->ssRange;
		args.lvds_ss_2.ucEnable = command;
	} else {
		ERROR("%s: TODO: Old card SS control\n", __func__);
		return;
	}

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
