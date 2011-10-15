/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "accelerant.h"


//#define TRACE_DPMS
#ifdef TRACE_DPMS
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


void
enable_display_plane(bool enable)
{
	uint32 planeAControl = read32(INTEL_DISPLAY_A_CONTROL);
	uint32 planeBControl = read32(INTEL_DISPLAY_B_CONTROL);

	if (enable) {
		// when enabling the display, the register values are updated automatically
		if (gInfo->head_mode & HEAD_MODE_A_ANALOG)
			write32(INTEL_DISPLAY_A_CONTROL, planeAControl | DISPLAY_CONTROL_ENABLED);
		if (gInfo->head_mode & HEAD_MODE_B_DIGITAL)
			write32(INTEL_DISPLAY_B_CONTROL, planeBControl | DISPLAY_CONTROL_ENABLED);

		read32(INTEL_DISPLAY_A_BASE);
			// flush the eventually cached PCI bus writes
	} else {
		// when disabling it, we have to trigger the update using a write to
		// the display base address
		if (gInfo->head_mode & HEAD_MODE_A_ANALOG)
			write32(INTEL_DISPLAY_A_CONTROL, planeAControl & ~DISPLAY_CONTROL_ENABLED);
		if (gInfo->head_mode & HEAD_MODE_B_DIGITAL)
			write32(INTEL_DISPLAY_B_CONTROL, planeBControl & ~DISPLAY_CONTROL_ENABLED);

		set_frame_buffer_base();
	}
}


static void
enable_display_pipe(bool enable)
{
	uint32 pipeAControl = read32(INTEL_DISPLAY_A_PIPE_CONTROL);
	uint32 pipeBControl = read32(INTEL_DISPLAY_B_PIPE_CONTROL);

	if (enable) {
		if (gInfo->head_mode & HEAD_MODE_A_ANALOG)
			write32(INTEL_DISPLAY_A_PIPE_CONTROL, pipeAControl | DISPLAY_PIPE_ENABLED);
		if (gInfo->head_mode & HEAD_MODE_B_DIGITAL)
			write32(INTEL_DISPLAY_B_PIPE_CONTROL, pipeBControl | DISPLAY_PIPE_ENABLED);
	} else {
		if (gInfo->head_mode & HEAD_MODE_A_ANALOG)
			write32(INTEL_DISPLAY_A_PIPE_CONTROL, pipeAControl & ~DISPLAY_PIPE_ENABLED);
		if (gInfo->head_mode & HEAD_MODE_B_DIGITAL)
			write32(INTEL_DISPLAY_B_PIPE_CONTROL, pipeBControl & ~DISPLAY_PIPE_ENABLED);
	}

	read32(INTEL_DISPLAY_A_BASE);
		// flush the eventually cached PCI bus writes
}


static void
enable_lvds_panel(bool enable)
{
	bool isSNB = gInfo->shared_info->device_type.InGroup(INTEL_TYPE_SNB);
	if (isSNB) {
		// TODO: fix for SNB
		return;
	}

	int controlRegister = isSNB ? PCH_PANEL_CONTROL : INTEL_PANEL_CONTROL;
	int statusRegister = isSNB ? PCH_PANEL_STATUS : INTEL_PANEL_STATUS;

	uint32 control = read32(controlRegister);
	uint32 panelStatus;

	if (enable) {
		if ((control & PANEL_CONTROL_POWER_TARGET_ON) == 0) {
			write32(controlRegister, control | PANEL_CONTROL_POWER_TARGET_ON
				| (isSNB ? PANEL_REGISTER_UNLOCK : 0));
		}

		do {
			panelStatus = read32(statusRegister);
		} while ((panelStatus & PANEL_STATUS_POWER_ON) == 0);
	} else {
		if ((control & PANEL_CONTROL_POWER_TARGET_ON) != 0) {
			write32(controlRegister, (control & ~PANEL_CONTROL_POWER_TARGET_ON)
				| (isSNB ? PANEL_REGISTER_UNLOCK : 0));
		}

		do {
			panelStatus = read32(statusRegister);
		} while ((panelStatus & PANEL_STATUS_POWER_ON) != 0);
	}
}


void
set_display_power_mode(uint32 mode)
{
	uint32 monitorMode = 0;

	if (mode == B_DPMS_ON) {
		uint32 pll = read32(INTEL_DISPLAY_A_PLL);
		if ((pll & DISPLAY_PLL_ENABLED) == 0) {
			// reactivate PLL
			write32(INTEL_DISPLAY_A_PLL, pll);
			read32(INTEL_DISPLAY_A_PLL);
			spin(150);
			write32(INTEL_DISPLAY_A_PLL, pll | DISPLAY_PLL_ENABLED);
			read32(INTEL_DISPLAY_A_PLL);
			spin(150);
			write32(INTEL_DISPLAY_A_PLL, pll | DISPLAY_PLL_ENABLED);
			read32(INTEL_DISPLAY_A_PLL);
			spin(150);
		}

		pll = read32(INTEL_DISPLAY_B_PLL);
		if ((pll & DISPLAY_PLL_ENABLED) == 0) {
			// reactivate PLL
			write32(INTEL_DISPLAY_B_PLL, pll);
			read32(INTEL_DISPLAY_B_PLL);
			spin(150);
			write32(INTEL_DISPLAY_B_PLL, pll | DISPLAY_PLL_ENABLED);
			read32(INTEL_DISPLAY_B_PLL);
			spin(150);
			write32(INTEL_DISPLAY_B_PLL, pll | DISPLAY_PLL_ENABLED);
			read32(INTEL_DISPLAY_B_PLL);
			spin(150);
		}

		enable_display_pipe(true);
		enable_display_plane(true);
	}

	wait_for_vblank();

	switch (mode) {
		case B_DPMS_ON:
			monitorMode = DISPLAY_MONITOR_ON;
			break;
		case B_DPMS_SUSPEND:
			monitorMode = DISPLAY_MONITOR_SUSPEND;
			break;
		case B_DPMS_STAND_BY:
			monitorMode = DISPLAY_MONITOR_STAND_BY;
			break;
		case B_DPMS_OFF:
			monitorMode = DISPLAY_MONITOR_OFF;
			break;
	}

	if (gInfo->head_mode & HEAD_MODE_A_ANALOG) {
		write32(INTEL_DISPLAY_A_ANALOG_PORT,
			(read32(INTEL_DISPLAY_A_ANALOG_PORT)
				& ~(DISPLAY_MONITOR_MODE_MASK | DISPLAY_MONITOR_PORT_ENABLED))
			| monitorMode | (mode != B_DPMS_OFF ? DISPLAY_MONITOR_PORT_ENABLED : 0));
	}
	if (gInfo->head_mode & HEAD_MODE_B_DIGITAL) {
		write32(INTEL_DISPLAY_B_DIGITAL_PORT,
			(read32(INTEL_DISPLAY_B_DIGITAL_PORT)
				& ~(DISPLAY_MONITOR_MODE_MASK | DISPLAY_MONITOR_PORT_ENABLED))
			| (mode != B_DPMS_OFF ? DISPLAY_MONITOR_PORT_ENABLED : 0));
			// TODO: monitorMode?
	}

	if (mode != B_DPMS_ON) {
		enable_display_plane(false);
		wait_for_vblank();
		enable_display_pipe(false);
	}

	if (mode == B_DPMS_OFF) {
		write32(INTEL_DISPLAY_A_PLL, read32(INTEL_DISPLAY_A_PLL)
			| DISPLAY_PLL_ENABLED);
		write32(INTEL_DISPLAY_B_PLL, read32(INTEL_DISPLAY_B_PLL)
			| DISPLAY_PLL_ENABLED);

		read32(INTEL_DISPLAY_B_PLL);
			// flush the possibly cached PCI bus writes

		spin(150);
	}

	if ((gInfo->head_mode & HEAD_MODE_LVDS_PANEL) != 0)
		enable_lvds_panel(mode == B_DPMS_ON);

	read32(INTEL_DISPLAY_A_BASE);
		// flush the possibly cached PCI bus writes
}


//	#pragma mark -


uint32
intel_dpms_capabilities(void)
{
	TRACE(("intel_dpms_capabilities()\n"));
	return B_DPMS_ON | B_DPMS_SUSPEND | B_DPMS_STAND_BY | B_DPMS_OFF;
}


uint32
intel_dpms_mode(void)
{
	TRACE(("intel_dpms_mode()\n"));
	return gInfo->shared_info->dpms_mode;
}


status_t
intel_set_dpms_mode(uint32 mode)
{
	TRACE(("intel_set_dpms_mode()\n"));
	gInfo->shared_info->dpms_mode = mode;
	set_display_power_mode(mode);

	return B_OK;
}

