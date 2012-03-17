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
					= D1GRPH_PRIMARY_SURFACE_ADDRESS_HIGH;
				break;
			case 1:
				offset = R600_CRTC1_REGISTER_OFFSET;
				regs->vgaControl = AVIVO_D2VGA_CONTROL;
				regs->grphPrimarySurfaceAddrHigh
					= D2GRPH_PRIMARY_SURFACE_ADDRESS_HIGH;
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
	edid1_info* edid = &gDisplay[crtid]->edid_info;

	// Scan each display EDID description for monitor ranges
	for (uint32 index = 0; index < EDID1_NUM_DETAILED_MONITOR_DESC; index++) {

		edid1_detailed_monitor* monitor
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


status_t
detect_displays()
{
	// reset known displays
	for (uint32 id = 0; id < MAX_DISPLAY; id++) {
		gDisplay[id]->attached = false;
		gDisplay[id]->powered = false;
		gDisplay[id]->found_ranges = false;
	}

	uint32 displayIndex = 0;
	for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
		if (gConnector[id]->valid == false)
			continue;
		if (displayIndex >= MAX_DISPLAY)
			continue;

		// TODO: As DP aux transactions don't work yet, just use LVDS as a hack
		#if 0
		if (gConnector[id]->encoderExternal.isDPBridge == true) {
			// If this is a DisplayPort Bridge, setup ddc on bus
			// TRAVIS (LVDS) or NUTMEG (VGA)
			TRACE("%s: is bridge, performing bridge DDC setup\n", __func__);
			encoder_external_setup(id, 23860,
				EXTERNAL_ENCODER_ACTION_V3_DDC_SETUP);
		} else if (gConnector[id]->type == VIDEO_CONNECTOR_LVDS) {
		#endif
		if (gConnector[id]->type == VIDEO_CONNECTOR_LVDS) {
			// If plain (non-DP) laptop LVDS, read mode info from AtomBIOS
			//TRACE("%s: non-DP laptop LVDS detected\n", __func__);
			gDisplay[displayIndex]->attached
				= connector_read_mode_lvds(id,
					&gDisplay[displayIndex]->preferredMode);
		}

		if (gDisplay[displayIndex]->attached == false) {
			TRACE("%s: bit-banging ddc for edid on connector %" B_PRIu32 "\n",
				__func__, id);
			// Lets try bit-banging edid from connector
			gDisplay[displayIndex]->attached =
				connector_read_edid(id, &gDisplay[displayIndex]->edid_info);

			if (gConnector[id]->encoder.type == VIDEO_ENCODER_TVDAC
				|| gConnector[id]->encoder.type == VIDEO_ENCODER_DAC) {
				// analog? with valid EDID? lets make sure there is load.
				// There is only one ddc communications path on DVI-I
				if (encoder_analog_load_detect(id) != true) {
					TRACE("%s: no analog load on EDID valid connector "
						"#%" B_PRIu32 "\n", __func__, id);
					gDisplay[displayIndex]->attached = false;
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
			gDisplay[displayIndex]->found_ranges = false;
		} else {
			// Use edid data and pull ranges
			if (detect_crt_ranges(displayIndex) == B_OK)
				gDisplay[displayIndex]->found_ranges = true;
		}

		displayIndex++;
	}

	// fallback if no attached monitors were found
	if (displayIndex == 0) {
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
				gDisplay[0]->found_ranges = true;
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
				gDisplay[id]->vfreq_min, gDisplay[id]->vfreq_max);
			ERROR(" + limits: Horz Min/Max: %" B_PRIu32 "/%" B_PRIu32"\n",
				gDisplay[id]->hfreq_min, gDisplay[id]->hfreq_max);
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

	// Normal encoder situations
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

	uint32 bytesPerRow = mode->virtual_width * bytesPerPixel;

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
