/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "accelerant.h"


#undef TRACE
//#define TRACE_DPMS
#ifdef TRACE_DPMS
#	define TRACE(x...) _sPrintf("intel_extreme: " x)
#else
#	define TRACE(x...)
#endif

#define ERROR(x...) _sPrintf("intel_extreme: " x)
#define CALLED(x...) TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


static void
enable_all_pipes(bool enable)
{
	// Go over each port and enable pipe/plane
	for (uint32 i = 0; i < gInfo->port_count; i++) {
		if (gInfo->ports[i] == NULL)
			continue;
		if (!gInfo->ports[i]->IsConnected())
			continue;
		if (gInfo->ports[i]->GetPipe() == NULL)
			continue;

		gInfo->ports[i]->Power(enable);
	}

	read32(INTEL_DISPLAY_A_BASE);
		// flush the possibly cached PCI bus writes

	set_frame_buffer_base();
}


static void
enable_lvds_panel(bool enable)
{
	bool hasPCH = (gInfo->shared_info->pch_info != INTEL_PCH_NONE);

	int controlRegister = hasPCH ? PCH_PANEL_CONTROL : INTEL_PANEL_CONTROL;
	int statusRegister = hasPCH ? PCH_PANEL_STATUS : INTEL_PANEL_STATUS;

	uint32 control = read32(controlRegister);
	uint32 panelStatus;

	if (enable) {
		if ((control & PANEL_CONTROL_POWER_TARGET_ON) == 0) {
			write32(controlRegister, control | PANEL_CONTROL_POWER_TARGET_ON
				/*| (hasPCH ? PANEL_REGISTER_UNLOCK : 0)*/);
		}

		if (!hasPCH) {
			do {
				panelStatus = read32(statusRegister);
			} while ((panelStatus & PANEL_STATUS_POWER_ON) == 0);
		}
	} else {
		if ((control & PANEL_CONTROL_POWER_TARGET_ON) != 0) {
			write32(controlRegister, (control & ~PANEL_CONTROL_POWER_TARGET_ON)
				/*| (hasPCH ? PANEL_REGISTER_UNLOCK : 0)*/);
		}

		if (!hasPCH)
		{
			do {
				panelStatus = read32(statusRegister);
			} while ((panelStatus & PANEL_STATUS_POWER_ON) != 0);
		}
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

		enable_all_pipes(true);
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

	//fixme: port DPMS programming should better be in Ports.cpp. Resetup..
	if (gInfo->head_mode & HEAD_MODE_A_ANALOG) {
		write32(INTEL_ANALOG_PORT, (read32(INTEL_ANALOG_PORT)
				& ~(DISPLAY_MONITOR_MODE_MASK | DISPLAY_MONITOR_PORT_ENABLED))
			| monitorMode
			| (mode != B_DPMS_OFF ? DISPLAY_MONITOR_PORT_ENABLED : 0));
	}

	//fixme: port DPMS programming should better be in Ports.cpp and is faulty. Resetup..
	if (false) {//gInfo->head_mode & HEAD_MODE_B_DIGITAL) {
		write32(INTEL_DIGITAL_PORT_B, (read32(INTEL_DIGITAL_PORT_B)
				& ~(/*DISPLAY_MONITOR_MODE_MASK |*/ DISPLAY_MONITOR_PORT_ENABLED))
			| (mode != B_DPMS_OFF ? DISPLAY_MONITOR_PORT_ENABLED : 0));
			// TODO: monitorMode?
	}

	if (mode != B_DPMS_ON)
		enable_all_pipes(false);

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
	CALLED();
	return B_DPMS_ON | B_DPMS_SUSPEND | B_DPMS_STAND_BY | B_DPMS_OFF;
}


uint32
intel_dpms_mode(void)
{
	CALLED();
	return gInfo->shared_info->dpms_mode;
}


status_t
intel_set_dpms_mode(uint32 mode)
{
	CALLED();
	gInfo->shared_info->dpms_mode = mode;
	set_display_power_mode(mode);

	return B_OK;
}

