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
#include "intel_extreme.h"

#include <new>


#undef TRACE
#define TRACE_PORTS
#ifdef TRACE_PORTS
#   define TRACE(x...) _sPrintf("intel_extreme:" x)
#else
#   define TRACE(x...)
#endif

#define ERROR(x...) _sPrintf("intel_extreme: " x)
#define CALLED(x...) TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


Port::Port(port_index index, const char* baseName)
	:
	fPortIndex(index),
	fPortName(NULL),
	fDisplayPipe(NULL),
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
Port::AssignPipe(pipe_index pipeIndex)
{
    CALLED();

    uint32 portRegister = _PortRegister();
	if (portRegister == 0) {
		ERROR("%s: Invalid PortRegister ((0x%" B_PRIx32 ") for %s\n", __func__,
			portRegister, PortName());
		return B_ERROR;
	}

	TRACE("%s: Assigning %s (0x%" B_PRIx32 ") to pipe %s\n", __func__,
		PortName(), portRegister, (pipeIndex == INTEL_PIPE_A) ? "A" : "B");

    uint32 portState = read32(portRegister);

    if (pipeIndex == INTEL_PIPE_A)
        write32(portRegister, portState & ~DISPLAY_MONITOR_PIPE_B);
    else
        write32(portRegister, portState | DISPLAY_MONITOR_PIPE_B);

	fDisplayPipe = new(std::nothrow) DisplayPipe(pipeIndex);

	if (fDisplayPipe == NULL)
		return B_NO_MEMORY;

	read32(portRegister);

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

		TRACE("%s: using register %" B_PRIxADDR "\n", PortName(), ddcRegister);

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
	TRACE("%s: %s-%d %dx%d\n", __func__, PortName(), PortIndex(),
		target->virtual_width, target->virtual_height);

	pll_divisors divisors;
	compute_pll_divisors(target, &divisors, false);

	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IGD)) {
		write32(INTEL_DISPLAY_A_PLL_DIVISOR_0,
			(((1 << divisors.n) << DISPLAY_PLL_N_DIVISOR_SHIFT)
				& DISPLAY_PLL_IGD_N_DIVISOR_MASK)
			| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT)
				& DISPLAY_PLL_IGD_M2_DIVISOR_MASK));
	} else {
		write32(INTEL_DISPLAY_A_PLL_DIVISOR_0,
			(((divisors.n - 2) << DISPLAY_PLL_N_DIVISOR_SHIFT)
				& DISPLAY_PLL_N_DIVISOR_MASK)
			| (((divisors.m1 - 2) << DISPLAY_PLL_M1_DIVISOR_SHIFT)
				& DISPLAY_PLL_M1_DIVISOR_MASK)
			| (((divisors.m2 - 2) << DISPLAY_PLL_M2_DIVISOR_SHIFT)
				& DISPLAY_PLL_M2_DIVISOR_MASK));
	}

	uint32 pll = DISPLAY_PLL_ENABLED | DISPLAY_PLL_NO_VGA_CONTROL;
	if (gInfo->shared_info->device_type.InFamily(INTEL_FAMILY_9xx)
		|| gInfo->shared_info->device_type.InFamily(INTEL_FAMILY_SER5)
		|| gInfo->shared_info->device_type.InFamily(INTEL_FAMILY_SOC0)) {
		if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IGD)) {
			pll |= ((1 << (divisors.post1 - 1))
					<< DISPLAY_PLL_IGD_POST1_DIVISOR_SHIFT)
				& DISPLAY_PLL_IGD_POST1_DIVISOR_MASK;
		} else {
			pll |= ((1 << (divisors.post1 - 1))
					<< DISPLAY_PLL_POST1_DIVISOR_SHIFT)
				& DISPLAY_PLL_9xx_POST1_DIVISOR_MASK;
//			pll |= ((divisors.post1 - 1) << DISPLAY_PLL_POST1_DIVISOR_SHIFT)
//				& DISPLAY_PLL_9xx_POST1_DIVISOR_MASK;
		}
		if (divisors.post2_high)
			pll |= DISPLAY_PLL_DIVIDE_HIGH;

		pll |= DISPLAY_PLL_MODE_ANALOG;

		if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_96x))
			pll |= 6 << DISPLAY_PLL_PULSE_PHASE_SHIFT;
	} else {
		if (!divisors.post2_high)
			pll |= DISPLAY_PLL_DIVIDE_4X;

		pll |= DISPLAY_PLL_2X_CLOCK;

		if (divisors.post1 > 2) {
			pll |= ((divisors.post1 - 2) << DISPLAY_PLL_POST1_DIVISOR_SHIFT)
				& DISPLAY_PLL_POST1_DIVISOR_MASK;
		} else
			pll |= DISPLAY_PLL_POST1_DIVIDE_2;
	}

	// Programmer's Ref says we must allow the DPLL to "warm up" before starting the plane
	// so mask its bit, wait, enable its bit
	write32(INTEL_DISPLAY_A_PLL, pll & ~DISPLAY_PLL_NO_VGA_CONTROL);
	read32(INTEL_DISPLAY_A_PLL);
	spin(150);
	write32(INTEL_DISPLAY_A_PLL, pll);
	read32(INTEL_DISPLAY_A_PLL);
	spin(150);

	// update timing parameters
	write32(INTEL_DISPLAY_A_HTOTAL,
		((uint32)(target->timing.h_total - 1) << 16)
		| ((uint32)target->timing.h_display - 1));
	write32(INTEL_DISPLAY_A_HBLANK,
		((uint32)(target->timing.h_total - 1) << 16)
		| ((uint32)target->timing.h_display - 1));
	write32(INTEL_DISPLAY_A_HSYNC,
		((uint32)(target->timing.h_sync_end - 1) << 16)
		| ((uint32)target->timing.h_sync_start - 1));

	write32(INTEL_DISPLAY_A_VTOTAL,
		((uint32)(target->timing.v_total - 1) << 16)
		| ((uint32)target->timing.v_display - 1));
	write32(INTEL_DISPLAY_A_VBLANK,
		((uint32)(target->timing.v_total - 1) << 16)
		| ((uint32)target->timing.v_display - 1));
	write32(INTEL_DISPLAY_A_VSYNC,
		((uint32)(target->timing.v_sync_end - 1) << 16)
		| ((uint32)target->timing.v_sync_start - 1));

	write32(INTEL_DISPLAY_A_IMAGE_SIZE,
		((uint32)(target->virtual_width - 1) << 16)
		| ((uint32)target->virtual_height - 1));

	write32(INTEL_ANALOG_PORT, (read32(INTEL_ANALOG_PORT)
			& ~(DISPLAY_MONITOR_POLARITY_MASK
				| DISPLAY_MONITOR_VGA_POLARITY))
		| ((target->timing.flags & B_POSITIVE_HSYNC) != 0
			? DISPLAY_MONITOR_POSITIVE_HSYNC : 0)
		| ((target->timing.flags & B_POSITIVE_VSYNC) != 0
			? DISPLAY_MONITOR_POSITIVE_VSYNC : 0));

	// TODO: verify the two comments below: the X driver doesn't seem to
	//		care about both of them!

	// These two have to be set for display B, too - this obviously means
	// that the second head always must adopt the color space of the first
	// head.
	write32(INTEL_DISPLAY_A_CONTROL, (read32(INTEL_DISPLAY_A_CONTROL)
			& ~(DISPLAY_CONTROL_COLOR_MASK | DISPLAY_CONTROL_GAMMA))
		| colorMode);

	if ((gInfo->head_mode & HEAD_MODE_B_DIGITAL) != 0) {
		write32(INTEL_DISPLAY_B_IMAGE_SIZE,
			((uint32)(target->virtual_width - 1) << 16)
			| ((uint32)target->virtual_height - 1));

		write32(INTEL_DISPLAY_B_CONTROL, (read32(INTEL_DISPLAY_B_CONTROL)
				& ~(DISPLAY_CONTROL_COLOR_MASK | DISPLAY_CONTROL_GAMMA))
			| colorMode);
	}

	// Set fCurrentMode to our set display mode
	memcpy(fCurrentMode, target, sizeof(display_mode));

	return B_OK;
}


// #pragma mark - LVDS Panel


LVDSPort::LVDSPort()
	:
	Port(INTEL_PORT_C, "LVDS")
{
	// Always unlock LVDS port as soon as we start messing with it.
	if (gInfo->shared_info->device_type.HasPlatformControlHub()) {
		write32(PCH_PANEL_CONTROL,
			read32(PCH_PANEL_CONTROL) | PANEL_REGISTER_UNLOCK);
	}
}


bool
LVDSPort::IsConnected()
{
	// Older generations don't have LVDS detection. If not mobile skip.
	if (gInfo->shared_info->device_type.Generation() <= 4) {
		if (!gInfo->shared_info->device_type.IsMobile()) {
			TRACE("LVDS: Skipping LVDS detection due to gen and not mobile\n");
			return false;
		}
	}

	uint32 registerValue = read32(INTEL_DIGITAL_LVDS_PORT);
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

	uint32 lvds = read32(INTEL_DIGITAL_LVDS_PORT) | LVDS_PORT_EN
		| LVDS_A0A2_CLKA_POWER_UP;

	lvds |= LVDS_18BIT_DITHER;
		// TODO: do not do this if the connected panel is 24-bit
		// (I don't know how to detect that)

	float referenceClock = gInfo->shared_info->pll_info.reference_frequency
		/ 1000.0f;

	// Set the B0-B3 data pairs corresponding to whether we're going to
	// set the DPLLs for dual-channel mode or not.
	if (divisors.post2 == LVDS_POST2_RATE_FAST)
		lvds |= LVDS_B0B3PAIRS_POWER_UP | LVDS_CLKB_POWER_UP;
	else
		lvds &= ~(LVDS_B0B3PAIRS_POWER_UP | LVDS_CLKB_POWER_UP);

	write32(INTEL_DIGITAL_LVDS_PORT, lvds);
	read32(INTEL_DIGITAL_LVDS_PORT);

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

	write32(INTEL_DISPLAY_B_PLL, dpll);
	read32(INTEL_DISPLAY_B_PLL);

	// Wait for the clocks to stabilize
	spin(150);

	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_96x)) {
		float adjusted = ((referenceClock * divisors.m) / divisors.n)
			/ divisors.post;
		uint32 pixelMultiply;
		if (needsScaling) {
			pixelMultiply = uint32(adjusted
				/ (hardwareTarget.timing.pixel_clock / 1000.0f));
		} else {
			pixelMultiply = uint32(adjusted
				/ (target->timing.pixel_clock / 1000.0f));
		}

		write32(INTEL_DISPLAY_B_PLL_MULTIPLIER_DIVISOR, (0 << 24)
			| ((pixelMultiply - 1) << 8));
	} else
		write32(INTEL_DISPLAY_B_PLL, dpll);

	read32(INTEL_DISPLAY_B_PLL);
	spin(150);

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

	write32(INTEL_DISPLAY_B_IMAGE_SIZE,
		((uint32)(target->virtual_width - 1) << 16)
		| ((uint32)target->virtual_height - 1));

	write32(INTEL_DISPLAY_B_POS, 0);
	write32(INTEL_DISPLAY_B_PIPE_SIZE,
		((uint32)(target->timing.v_display - 1) << 16)
		| ((uint32)target->timing.h_display - 1));

	write32(INTEL_DISPLAY_B_CONTROL, (read32(INTEL_DISPLAY_B_CONTROL)
			& ~(DISPLAY_CONTROL_COLOR_MASK | DISPLAY_CONTROL_GAMMA))
		| colorMode);

	write32(INTEL_DISPLAY_B_PIPE_CONTROL,
		read32(INTEL_DISPLAY_B_PIPE_CONTROL) | INTEL_PIPE_ENABLED);
	read32(INTEL_DISPLAY_B_PIPE_CONTROL);

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
	return 0;
}


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
	DigitalPort(index, baseName)
{
}


bool
DisplayPort::IsConnected()
{
	addr_t portRegister = _PortRegister();
	if (portRegister == 0)
		return false;

	if ((read32(portRegister) & DISPLAY_MONITOR_PORT_DETECTED) == 0)
		return false;

	return HasEDID();
}


addr_t
DisplayPort::_PortRegister()
{
	switch (PortIndex()) {
		case INTEL_PORT_A:
			return INTEL_DISPLAY_PORT_A;
		case INTEL_PORT_B:
			return INTEL_DISPLAY_PORT_B;
		case INTEL_PORT_C:
			return INTEL_DISPLAY_PORT_C;
		case INTEL_PORT_D:
			return INTEL_DISPLAY_PORT_D;
		default:
			return 0;
	}

	return 0;
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
