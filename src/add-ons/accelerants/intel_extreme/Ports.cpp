/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Michael Lotz, mmlr@mlotz.ch
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "Ports.h"

#include <ddc.h>
#include <stdlib.h>
#include <string.h>
#include <Debug.h>
#include <KernelExport.h>

#include "accelerant.h"
#include "accelerant_protos.h"
#include "FlexibleDisplayInterface.h"
#include "intel_extreme.h"

#include <new>


#undef TRACE
#define TRACE_PORTS
#ifdef TRACE_PORTS
#   define TRACE(x...) _sPrintf("intel_extreme: " x)
#else
#   define TRACE(x...)
#endif

#define ERROR(x...) _sPrintf("intel_extreme: " x)
#define CALLED(x...) TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


static bool
wait_for_set(addr_t address, uint32 mask, uint32 timeout)
{
	int interval = 50;
	uint32 i = 0;
	for(i = 0; i <= timeout; i += interval) {
		spin(interval);
		if ((read32(address) & mask) != 0)
			return true;
	}
	return false;
}


static bool
wait_for_clear(addr_t address, uint32 mask, uint32 timeout)
{
	int interval = 50;
	uint32 i = 0;
	for(i = 0; i <= timeout; i += interval) {
		spin(interval);
		if ((read32(address) & mask) == 0)
			return true;
	}
	return false;
}


Port::Port(port_index index, const char* baseName)
	:
	fPipe(NULL),
	fPortIndex(index),
	fPortName(NULL),
	fEDIDState(B_NO_INIT)
{
	char portID[2];
	portID[0] = 'A' + index - INTEL_PORT_A;
	portID[1] = 0;

	char buffer[32];
	buffer[0] = 0;

	strlcat(buffer, baseName, sizeof(buffer));
	strlcat(buffer, " ", sizeof(buffer));
	strlcat(buffer, portID, sizeof(buffer));
	fPortName = strdup(buffer);
}


Port::~Port()
{
	free(fPortName);
}


bool
Port::HasEDID()
{
	if (fEDIDState == B_NO_INIT)
		GetEDID(NULL);

	return fEDIDState == B_OK;
}


status_t
Port::SetPipe(Pipe* pipe)
{
    CALLED();

	if (pipe == NULL) {
		ERROR("%s: Invalid pipe provided!\n", __func__);
		return B_ERROR;
	}

    uint32 portRegister = _PortRegister();
	if (portRegister == 0) {
		ERROR("%s: Invalid PortRegister ((0x%" B_PRIx32 ") for %s\n", __func__,
			portRegister, PortName());
		return B_ERROR;
	}

	// TODO: UnAssignPipe?  This likely needs reworked a little
	if (fPipe != NULL) {
		ERROR("%s: Can't reassign display pipe (yet)\n", __func__);
		return B_ERROR;
	}

	TRACE("%s: Assigning %s (0x%" B_PRIx32 ") to pipe %s\n", __func__,
		PortName(), portRegister, (pipe->Index() == INTEL_PIPE_A) ? "A" : "B");

    uint32 portState = read32(portRegister);

    if (pipe->Index() == INTEL_PIPE_A)
        write32(portRegister, portState & ~DISPLAY_MONITOR_PIPE_B);
    else
        write32(portRegister, portState | DISPLAY_MONITOR_PIPE_B);

	fPipe = pipe;

	if (fPipe == NULL)
		return B_NO_MEMORY;

	// Disable display pipe until modesetting enables it
	if (fPipe->IsEnabled())
		fPipe->Enable(false);

	read32(portRegister);

	return B_OK;
}


status_t
Port::Power(bool enabled)
{
	fPipe->Enable(enabled);

	return B_OK;
}


status_t
Port::GetEDID(edid1_info* edid, bool forceRead)
{
	CALLED();

	if (fEDIDState == B_NO_INIT || forceRead) {
		TRACE("%s: trying to read EDID\n", PortName());

		addr_t ddcRegister = _DDCRegister();
		if (ddcRegister == 0) {
			TRACE("%s: no DDC register found\n", PortName());
			fEDIDState = B_ERROR;
			return fEDIDState;
		}

		TRACE("%s: using ddc @ 0x%" B_PRIxADDR "\n", PortName(), ddcRegister);

		i2c_bus bus;
		bus.cookie = (void*)ddcRegister;
		bus.set_signals = &_SetI2CSignals;
		bus.get_signals = &_GetI2CSignals;
		ddc2_init_timing(&bus);

		fEDIDState = ddc2_read_edid1(&bus, &fEDIDInfo, NULL, NULL);

		if (fEDIDState == B_OK) {
			TRACE("%s: found EDID information!\n", PortName());
			edid_dump(&fEDIDInfo);
		}
	}

	if (fEDIDState != B_OK) {
		TRACE("%s: no EDID information found.\n", PortName());
		return fEDIDState;
	}

	if (edid != NULL)
		memcpy(edid, &fEDIDInfo, sizeof(edid1_info));

	return B_OK;
}


status_t
Port::GetPLLLimits(pll_limits& limits)
{
	return B_ERROR;
}


status_t
Port::_GetI2CSignals(void* cookie, int* _clock, int* _data)
{
	addr_t ioRegister = (addr_t)cookie;
	uint32 value = read32(ioRegister);

	*_clock = (value & I2C_CLOCK_VALUE_IN) != 0;
	*_data = (value & I2C_DATA_VALUE_IN) != 0;

	return B_OK;
}


status_t
Port::_SetI2CSignals(void* cookie, int clock, int data)
{
	addr_t ioRegister = (addr_t)cookie;
	uint32 value;

	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_83x)) {
		// on these chips, the reserved values are fixed
		value = 0;
	} else {
		// on all others, we have to preserve them manually
		value = read32(ioRegister) & I2C_RESERVED;
	}

	if (data != 0)
		value |= I2C_DATA_DIRECTION_MASK;
	else {
		value |= I2C_DATA_DIRECTION_MASK | I2C_DATA_DIRECTION_OUT
			| I2C_DATA_VALUE_MASK;
	}

	if (clock != 0)
		value |= I2C_CLOCK_DIRECTION_MASK;
	else {
		value |= I2C_CLOCK_DIRECTION_MASK | I2C_CLOCK_DIRECTION_OUT
			| I2C_CLOCK_VALUE_MASK;
	}

	write32(ioRegister, value);
	read32(ioRegister);
		// make sure the PCI bus has flushed the write

	return B_OK;
}


// #pragma mark - Analog Port


AnalogPort::AnalogPort()
	:
	Port(INTEL_PORT_A, "Analog")
{
}


bool
AnalogPort::IsConnected()
{
	TRACE("%s: %s PortRegister: 0x%" B_PRIxADDR "\n", __func__, PortName(),
		_PortRegister());
	return HasEDID();
}


addr_t
AnalogPort::_DDCRegister()
{
	// always fixed
	return INTEL_I2C_IO_A;
}


addr_t
AnalogPort::_PortRegister()
{
	// always fixed
	return INTEL_ANALOG_PORT;
}


status_t
AnalogPort::SetDisplayMode(display_mode* target, uint32 colorMode)
{
	TRACE("%s: %s %dx%d\n", __func__, PortName(), target->virtual_width,
		target->virtual_height);

	if (fPipe == NULL) {
		ERROR("%s: Setting display mode without assigned pipe!\n", __func__);
		return B_ERROR;
	}

	// Train FDI if it exists
	FDILink* link = fPipe->FDI();
	if (link != NULL)
		link->Train(target);

	pll_divisors divisors;
	compute_pll_divisors(target, &divisors, false);

	uint32 extraPLLFlags = 0;
	if (gInfo->shared_info->device_type.Generation() >= 3)
		extraPLLFlags |= DISPLAY_PLL_MODE_NORMAL;

	// Program general pipe config
	fPipe->Configure(target);

	// Program pipe PLL's
	fPipe->ConfigureClocks(divisors, target->timing.pixel_clock, extraPLLFlags);

	write32(_PortRegister(), (read32(_PortRegister())
		& ~(DISPLAY_MONITOR_POLARITY_MASK | DISPLAY_MONITOR_VGA_POLARITY))
		| ((target->timing.flags & B_POSITIVE_HSYNC) != 0
			? DISPLAY_MONITOR_POSITIVE_HSYNC : 0)
		| ((target->timing.flags & B_POSITIVE_VSYNC) != 0
			? DISPLAY_MONITOR_POSITIVE_VSYNC : 0));

	// Program target display mode
	fPipe->ConfigureTimings(target);

	// Set fCurrentMode to our set display mode
	memcpy(&fCurrentMode, target, sizeof(display_mode));

	return B_OK;
}


// #pragma mark - LVDS Panel


LVDSPort::LVDSPort()
	:
	Port(INTEL_PORT_C, "LVDS")
{
	// Always unlock LVDS port as soon as we start messing with it.
	uint32 panelControl = INTEL_PANEL_CONTROL;
	if (gInfo->shared_info->device_type.HasPlatformControlHub())
		panelControl = PCH_PANEL_CONTROL;
	write32(panelControl, read32(panelControl) | PANEL_REGISTER_UNLOCK);
}


bool
LVDSPort::IsConnected()
{
	TRACE("%s: %s PortRegister: 0x%" B_PRIxADDR "\n", __func__, PortName(),
		_PortRegister());

	// Older generations don't have LVDS detection. If not mobile skip.
	if (gInfo->shared_info->device_type.Generation() <= 4) {
		if (!gInfo->shared_info->device_type.IsMobile()) {
			TRACE("LVDS: Skipping LVDS detection due to gen and not mobile\n");
			return false;
		}
	}

	uint32 registerValue = read32(_PortRegister());
	if (gInfo->shared_info->device_type.HasPlatformControlHub()) {
		// there's a detection bit we can use
		if ((registerValue & PCH_LVDS_DETECTED) == 0) {
			TRACE("LVDS: Not detected\n");
			return false;
		}
		// TODO: Skip if eDP support
	}

	// Try getting EDID, as the LVDS port doesn't overlap with anything else,
	// we don't run the risk of getting someone else's data.
	return HasEDID();
}


addr_t
LVDSPort::_DDCRegister()
{
	// always fixed
	return INTEL_I2C_IO_C;
}


addr_t
LVDSPort::_PortRegister()
{
	// always fixed
	return INTEL_DIGITAL_LVDS_PORT;
}


status_t
LVDSPort::SetDisplayMode(display_mode* target, uint32 colorMode)
{
    CALLED();
	if (target == NULL) {
		ERROR("%s: Invalid target mode passed!\n", __func__);
		return B_ERROR;
	}

	TRACE("%s: %s-%d %dx%d\n", __func__, PortName(), PortIndex(),
		target->virtual_width, target->virtual_height);

	if (fPipe == NULL) {
		ERROR("%s: Setting display mode without assigned pipe!\n", __func__);
		return B_ERROR;
	}

	addr_t panelControl = INTEL_PANEL_CONTROL;
	addr_t panelStatus = INTEL_PANEL_STATUS;
	if (gInfo->shared_info->device_type.HasPlatformControlHub()) {
		panelControl = PCH_PANEL_CONTROL;
		panelStatus = PCH_PANEL_STATUS;
	}

	// Power off Panel
	write32(panelControl, read32(panelControl) & ~PANEL_CONTROL_POWER_TARGET_ON);
	read32(panelControl);

	if (!wait_for_clear(panelStatus, PANEL_STATUS_POWER_ON, 1000))
		ERROR("%s: %s didn't power off within 1000ms!\n", __func__, PortName());

	// TODO: Fix software scaling?

	// Disable PanelFitter for now
	addr_t panelFitterControl = PCH_PANEL_FITTER_BASE_REGISTER
		+ PCH_PANEL_FITTER_CONTROL;
	if (fPipe->Index() == INTEL_PIPE_B)
		panelFitterControl += PCH_PANEL_FITTER_PIPE_OFFSET;
	write32(panelFitterControl, (read32(panelFitterControl) & ~PANEL_FITTER_ENABLED));
	read32(panelFitterControl);

#if 0
	// For LVDS panels, we actually always set the native mode in hardware
	// Then we use the panel fitter to scale the picture to that.
	display_mode hardwareTarget;
	bool needsScaling = false;
		// Try to get the panel preferred screen mode from EDID info
	if (gInfo->has_edid) {
		hardwareTarget.space = target->space;
		hardwareTarget.virtual_width
			= gInfo->edid_info.std_timing[0].h_size;
		hardwareTarget.virtual_height
			= gInfo->edid_info.std_timing[0].v_size;
		for (int i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; i++) {
			if (gInfo->edid_info.detailed_monitor[i].monitor_desc_type
					== EDID1_IS_DETAILED_TIMING) {
				hardwareTarget.virtual_width = gInfo->edid_info
					.detailed_monitor[i].data.detailed_timing.h_active;
				hardwareTarget.virtual_height = gInfo->edid_info
					.detailed_monitor[i].data.detailed_timing.v_active;
				break;
			}
		}
		TRACE("%s: hardware mode will actually be %dx%d\n", __func__,
			hardwareTarget.virtual_width, hardwareTarget.virtual_height);

		if ((hardwareTarget.virtual_width <= target->virtual_width
				&& hardwareTarget.virtual_height <= target->virtual_height
				&& hardwareTarget.space <= target->space)
			|| intel_propose_display_mode(&hardwareTarget, target, target)) {
			hardwareTarget = *target;
		} else
			needsScaling = true;
	} else {
		// We don't have EDID data, try to set the requested mode directly
		hardwareTarget = *target;
	}
	pll_divisors divisors;
	if (needsScaling)
		compute_pll_divisors(&hardwareTarget, &divisors, true);
	else
		compute_pll_divisors(target, &divisors, true);

	uint32 dpll = DISPLAY_PLL_NO_VGA_CONTROL | DISPLAY_PLL_ENABLED;
	if (gInfo->shared_info->device_type.Generation() >= 4) {
		// DPLL mode LVDS for i915+
		dpll |= LVDS_PLL_MODE_LVDS;
	}

	// Compute bitmask from p1 value
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IGD)) {
		dpll |= (1 << (divisors.post1 - 1))
			<< DISPLAY_PLL_IGD_POST1_DIVISOR_SHIFT;
	} else {
		dpll |= (1 << (divisors.post1 - 1))
			<< DISPLAY_PLL_POST1_DIVISOR_SHIFT;
	}
	switch (divisors.post2) {
		case 5:
		case 7:
			dpll |= DISPLAY_PLL_DIVIDE_HIGH;
			break;
	}

	// Disable panel fitting, but enable 8 to 6-bit dithering
	write32(INTEL_PANEL_FIT_CONTROL, 0x4);
		// TODO: do not do this if the connected panel is 24-bit
		// (I don't know how to detect that)

	if ((dpll & DISPLAY_PLL_ENABLED) != 0) {
		if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IGD)) {
			write32(INTEL_DISPLAY_B_PLL_DIVISOR_0,
				(((1 << divisors.n) << DISPLAY_PLL_N_DIVISOR_SHIFT)
					& DISPLAY_PLL_IGD_N_DIVISOR_MASK)
				| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT)
					& DISPLAY_PLL_IGD_M2_DIVISOR_MASK));
		} else {
			write32(INTEL_DISPLAY_B_PLL_DIVISOR_0,
				(((divisors.n - 2) << DISPLAY_PLL_N_DIVISOR_SHIFT)
					& DISPLAY_PLL_N_DIVISOR_MASK)
				| (((divisors.m1 - 2) << DISPLAY_PLL_M1_DIVISOR_SHIFT)
					& DISPLAY_PLL_M1_DIVISOR_MASK)
				| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT)
					& DISPLAY_PLL_M2_DIVISOR_MASK));
		}
		write32(INTEL_DISPLAY_B_PLL, dpll & ~DISPLAY_PLL_ENABLED);
		read32(INTEL_DISPLAY_B_PLL);
		spin(150);
	}
#endif


	pll_divisors divisors;
	compute_pll_divisors(target, &divisors, true);

	uint32 lvds = read32(_PortRegister())
		| LVDS_PORT_EN | LVDS_A0A2_CLKA_POWER_UP;

	if (gInfo->shared_info->device_type.Generation() == 4) {
		// LVDS_A3_POWER_UP == 24bpp
		// otherwise, 18bpp
		if ((lvds & LVDS_A3_POWER_MASK) != LVDS_A3_POWER_UP)
			lvds |= LVDS_18BIT_DITHER;
	}

	// Set the B0-B3 data pairs corresponding to whether we're going to
	// set the DPLLs for dual-channel mode or not.
	if (divisors.post2_high) {
		TRACE("LVDS: dual channel\n");
		lvds |= LVDS_B0B3_POWER_UP | LVDS_CLKB_POWER_UP;
	} else {
		TRACE("LVDS: single channel\n");
		lvds &= ~(LVDS_B0B3_POWER_UP | LVDS_CLKB_POWER_UP);
	}

	// LVDS port control moves polarity bits because Intel hates you.
	// Set LVDS sync polarity
	lvds &= ~(LVDS_HSYNC_POLARITY | LVDS_VSYNC_POLARITY);

	// set on - polarity.
	if ((target->timing.flags & B_POSITIVE_HSYNC) == 0)
		lvds |= LVDS_HSYNC_POLARITY;
	if ((target->timing.flags & B_POSITIVE_VSYNC) == 0)
		lvds |= LVDS_VSYNC_POLARITY;

	TRACE("%s: LVDS Write: 0x%" B_PRIx32 "\n", __func__, lvds);
	write32(_PortRegister(), lvds);
	read32(_PortRegister());

	uint32 extraPLLFlags = 0;

	// DPLL mode LVDS for i915+
	if (gInfo->shared_info->device_type.Generation() >= 3)
		extraPLLFlags |= DISPLAY_PLL_MODE_LVDS;

	// Program general pipe config
	fPipe->Configure(target);

	// Program pipe PLL's (pixel_clock is *always* the hardware pixel clock)
	fPipe->ConfigureClocks(divisors, target->timing.pixel_clock, extraPLLFlags);

	// Power on Panel
	write32(panelControl, read32(panelControl) | PANEL_CONTROL_POWER_TARGET_ON);
	read32(panelControl);

	if (!wait_for_set(panelStatus, PANEL_STATUS_POWER_ON, 1000))
		ERROR("%s: %s didn't power on within 1000ms!\n", __func__, PortName());

	// Program target display mode
	fPipe->ConfigureTimings(target);

#if 0

	// update timing parameters
	if (needsScaling) {
		// TODO: Alternatively, it should be possible to use the panel
		// fitter and scale the picture.

		// TODO: Perform some sanity check, for example if the target is
		// wider than the hardware mode we end up with negative borders and
		// broken timings
		uint32 borderWidth = hardwareTarget.timing.h_display
			- target->timing.h_display;

		uint32 syncWidth = hardwareTarget.timing.h_sync_end
			- hardwareTarget.timing.h_sync_start;

		uint32 syncCenter = target->timing.h_display
			+ (hardwareTarget.timing.h_total
			- target->timing.h_display) / 2;

		write32(INTEL_DISPLAY_B_HTOTAL,
			((uint32)(hardwareTarget.timing.h_total - 1) << 16)
			| ((uint32)target->timing.h_display - 1));
		write32(INTEL_DISPLAY_B_HBLANK,
			((uint32)(hardwareTarget.timing.h_total - borderWidth / 2 - 1)
				<< 16)
			| ((uint32)target->timing.h_display + borderWidth / 2 - 1));
		write32(INTEL_DISPLAY_B_HSYNC,
			((uint32)(syncCenter + syncWidth / 2 - 1) << 16)
			| ((uint32)syncCenter - syncWidth / 2 - 1));

		uint32 borderHeight = hardwareTarget.timing.v_display
			- target->timing.v_display;

		uint32 syncHeight = hardwareTarget.timing.v_sync_end
			- hardwareTarget.timing.v_sync_start;

		syncCenter = target->timing.v_display
			+ (hardwareTarget.timing.v_total
			- target->timing.v_display) / 2;

		write32(INTEL_DISPLAY_B_VTOTAL,
			((uint32)(hardwareTarget.timing.v_total - 1) << 16)
			| ((uint32)target->timing.v_display - 1));
		write32(INTEL_DISPLAY_B_VBLANK,
			((uint32)(hardwareTarget.timing.v_total - borderHeight / 2 - 1)
				<< 16)
			| ((uint32)target->timing.v_display
				+ borderHeight / 2 - 1));
		write32(INTEL_DISPLAY_B_VSYNC,
			((uint32)(syncCenter + syncHeight / 2 - 1) << 16)
			| ((uint32)syncCenter - syncHeight / 2 - 1));

		// This is useful for debugging: it sets the border to red, so you
		// can see what is border and what is porch (black area around the
		// sync)
		// write32(0x61020, 0x00FF0000);
	} else {
		write32(INTEL_DISPLAY_B_HTOTAL,
			((uint32)(target->timing.h_total - 1) << 16)
			| ((uint32)target->timing.h_display - 1));
		write32(INTEL_DISPLAY_B_HBLANK,
			((uint32)(target->timing.h_total - 1) << 16)
			| ((uint32)target->timing.h_display - 1));
		write32(INTEL_DISPLAY_B_HSYNC,
			((uint32)(target->timing.h_sync_end - 1) << 16)
			| ((uint32)target->timing.h_sync_start - 1));

		write32(INTEL_DISPLAY_B_VTOTAL,
			((uint32)(target->timing.v_total - 1) << 16)
			| ((uint32)target->timing.v_display - 1));
		write32(INTEL_DISPLAY_B_VBLANK,
			((uint32)(target->timing.v_total - 1) << 16)
			| ((uint32)target->timing.v_display - 1));
		write32(INTEL_DISPLAY_B_VSYNC, (
			(uint32)(target->timing.v_sync_end - 1) << 16)
			| ((uint32)target->timing.v_sync_start - 1));
	}
#endif

	// Set fCurrentMode to our set display mode
	memcpy(&fCurrentMode, target, sizeof(display_mode));

	return B_OK;
}


// #pragma mark - DVI/SDVO/generic


DigitalPort::DigitalPort(port_index index, const char* baseName)
	:
	Port(index, baseName)
{
}


bool
DigitalPort::IsConnected()
{
	TRACE("%s: %s PortRegister: 0x%" B_PRIxADDR "\n", __func__, PortName(),
		_PortRegister());

	// As this port overlaps with pretty much everything, this must be called
	// after having ruled out all other port types.
	return HasEDID();
}


addr_t
DigitalPort::_DDCRegister()
{
	//TODO: IS BROXTON, B = B, C = C, D = NIL
	switch (PortIndex()) {
		case INTEL_PORT_B:
			return INTEL_I2C_IO_E;
		case INTEL_PORT_C:
			return INTEL_I2C_IO_D;
		case INTEL_PORT_D:
			return INTEL_I2C_IO_F;
		default:
			return 0;
	}

	return 0;
}


addr_t
DigitalPort::_PortRegister()
{
	switch (PortIndex()) {
		case INTEL_PORT_A:
			return INTEL_DIGITAL_PORT_A;
		case INTEL_PORT_B:
			return INTEL_DIGITAL_PORT_B;
		case INTEL_PORT_C:
			return INTEL_DIGITAL_PORT_C;
		default:
			return 0;
	}
	return 0;
}


status_t
DigitalPort::SetDisplayMode(display_mode* target, uint32 colorMode)
{
	TRACE("%s: %s %dx%d\n", __func__, PortName(), target->virtual_width,
		target->virtual_height);

	if (fPipe == NULL) {
		ERROR("%s: Setting display mode without assigned pipe!\n", __func__);
		return B_ERROR;
	}

	// Train FDI if it exists
	FDILink* link = fPipe->FDI();
	if (link != NULL)
		link->Train(target);

	pll_divisors divisors;
	compute_pll_divisors(target, &divisors, false);

	uint32 extraPLLFlags = 0;
	if (gInfo->shared_info->device_type.Generation() >= 3)
		extraPLLFlags |= DISPLAY_PLL_MODE_NORMAL;

	// Program general pipe config
	fPipe->Configure(target);

	// Program pipe PLL's
	fPipe->ConfigureClocks(divisors, target->timing.pixel_clock, extraPLLFlags);

	// Program target display mode
	fPipe->ConfigureTimings(target);

	// Set fCurrentMode to our set display mode
	memcpy(&fCurrentMode, target, sizeof(display_mode));

	return B_OK;
}


// #pragma mark - LVDS Panel
// #pragma mark - HDMI


HDMIPort::HDMIPort(port_index index)
	:
	DigitalPort(index, "HDMI")
{
}


bool
HDMIPort::IsConnected()
{
	if (!gInfo->shared_info->device_type.SupportsHDMI())
		return false;

	addr_t portRegister = _PortRegister();
	TRACE("%s: %s PortRegister: 0x%" B_PRIxADDR "\n", __func__, PortName(),
		portRegister);

	if (portRegister == 0)
		return false;

	if (!gInfo->shared_info->device_type.HasPlatformControlHub()
		&& PortIndex() == INTEL_PORT_C) {
		// there's no detection bit on this port
	} else if ((read32(portRegister) & DISPLAY_MONITOR_PORT_DETECTED) == 0)
		return false;

	return HasEDID();
}


addr_t
HDMIPort::_PortRegister()
{
	// on PCH there's an additional port sandwiched in
	bool hasPCH = gInfo->shared_info->device_type.HasPlatformControlHub();
	bool fourthGen = gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV);

	switch (PortIndex()) {
		case INTEL_PORT_B:
			if (fourthGen)
				return GEN4_HDMI_PORT_B;
			return hasPCH ? PCH_HDMI_PORT_B : INTEL_HDMI_PORT_B;
		case INTEL_PORT_C:
			if (fourthGen)
				return GEN4_HDMI_PORT_C;
			return hasPCH ? PCH_HDMI_PORT_C : INTEL_HDMI_PORT_C;
		case INTEL_PORT_D:
			if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_CHV))
				return CHV_HDMI_PORT_D;
			return hasPCH ? PCH_HDMI_PORT_D : 0;
		default:
			return 0;
		}

	return 0;
}


// #pragma mark - DisplayPort


DisplayPort::DisplayPort(port_index index, const char* baseName)
	:
	Port(index, baseName)
{
}


bool
DisplayPort::IsConnected()
{
	addr_t portRegister = _PortRegister();

	TRACE("%s: %s PortRegister: 0x%" B_PRIxADDR "\n", __func__, PortName(),
		portRegister);

	if (portRegister == 0)
		return false;

	if ((read32(portRegister) & DISPLAY_MONITOR_PORT_DETECTED) == 0) {
		TRACE("%s: %s link not detected\n", __func__, PortName());
		return false;
	}

	return HasEDID();
}


addr_t
DisplayPort::_DDCRegister()
{
	// TODO: Do VLV + CHV use the VLV_DP_AUX_CTL_B + VLV_DP_AUX_CTL_C?
	switch (PortIndex()) {
		case INTEL_PORT_A:
			return INTEL_DP_AUX_CTL_A;
		case INTEL_PORT_B:
			if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV))
				return VLV_DP_AUX_CTL_B;
			return INTEL_DP_AUX_CTL_B;
		case INTEL_PORT_C:
			if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV))
				return VLV_DP_AUX_CTL_C;
			return INTEL_DP_AUX_CTL_C;
		case INTEL_PORT_D:
			if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_CHV))
				return CHV_DP_AUX_CTL_D;
			else if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV))
				return 0;
			return INTEL_DP_AUX_CTL_D;
		default:
			return 0;
	}

	return 0;
}


addr_t
DisplayPort::_PortRegister()
{
	// There are 6000 lines of intel linux code probing DP registers
	// to properly detect DP vs eDP to then in-turn properly figure out
	// what is DP and what is HDMI. It only takes 3 lines to
	// ignore DisplayPort on ValleyView / CherryView

	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV)
		|| gInfo->shared_info->device_type.InGroup(INTEL_GROUP_CHV)) {
		ERROR("TODO: DisplayPort on ValleyView / CherryView");
		return 0;
	}

	// Intel, are humans even involved anymore?
	// This is a lot more complex than this code makes it look. (see defines)
	// INTEL_DISPLAY_PORT_X moves around a lot based on PCH
	// except on ValleyView and CherryView.
	switch (PortIndex()) {
		case INTEL_PORT_A:
			return INTEL_DISPLAY_PORT_A;
		case INTEL_PORT_B:
			if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV))
				return VLV_DISPLAY_PORT_B;
			return INTEL_DISPLAY_PORT_B;
		case INTEL_PORT_C:
			if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV))
				return VLV_DISPLAY_PORT_C;
			return INTEL_DISPLAY_PORT_C;
		case INTEL_PORT_D:
			if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_CHV))
				return CHV_DISPLAY_PORT_D;
			else if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_VLV))
				return 0;
			return INTEL_DISPLAY_PORT_D;
		default:
			return 0;
	}

	return 0;
}


status_t
DisplayPort::SetDisplayMode(display_mode* target, uint32 colorMode)
{
	TRACE("%s: %s %dx%d\n", __func__, PortName(), target->virtual_width,
		target->virtual_height);

	ERROR("TODO: DisplayPort\n");
	return B_ERROR;
}


// #pragma mark - Embedded DisplayPort


EmbeddedDisplayPort::EmbeddedDisplayPort()
	:
	DisplayPort(INTEL_PORT_A, "Embedded DisplayPort")
{
}


bool
EmbeddedDisplayPort::IsConnected()
{
	if (!gInfo->shared_info->device_type.IsMobile())
		return false;

	return DisplayPort::IsConnected();
}
