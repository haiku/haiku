/*
	Copyright (c) 2002, Thomas Kurschel

	Part of Radeon kernel driver

	AGP fix. Some motherboard BIOSes enable FastWrite even
	though the graphics card doesn't support it. Here, we'll
	fix that (hopefully it is generic enough).
*/


#include "radeon_driver.h"

static void agp_list_info(agp_info ai);
static void agp_list_active(uint32 cmd);


//! fix invalid AGP settings
void
Radeon_Set_AGP(device_info *di, bool enable_agp)
{
	uint8 agp_index = 0;
	agp_info nth_agp_info;
	bool found = false;
	uint32 agp_cmd;

	/* abort if no agp busmanager found */
	if (!sAGP) {
		SHOW_INFO0(1, "Busmanager not installed.\nWarning Card May hang if AGP Fastwrites are Enabled." );
		return;
	}

	/* contact driver and get a pointer to the registers and shared data */
	/* get nth AGP device info */
	while (sAGP->get_nth_agp_info(agp_index, &nth_agp_info) == B_OK) {
		/* see if we are this one */
		if (nth_agp_info.device_id == di->pcii.device_id
			&& nth_agp_info.vendor_id == di->pcii.vendor_id
			&& nth_agp_info.bus == di->pcii.bus
			&& nth_agp_info.device == di->pcii.device
			&& nth_agp_info.function == di->pcii.function) {
			SHOW_INFO0(1, "Found AGP capable device" );
			found = true;

			/* remember our info */
			di->agpi = nth_agp_info;

			/* log capabilities */
			agp_list_info(nth_agp_info);
			break;
		}

		agp_index++;
	}

	if (!found) {
		if (agp_index != 0) {
			SHOW_INFO0(1, "End of AGP capable devices list.");
		} else {
			SHOW_INFO0(1, "No AGP capable devices found.");
		}
		return;
	}

	if (di->settings.force_pci | !enable_agp) {
		SHOW_INFO0(1, "Disabling AGP mode...");

		/* we want zero agp features enabled */
		agp_cmd = sAGP->set_agp_mode(0);
	} else {
		/* activate AGP mode */
		SHOW_INFO0(1, "Activating AGP mode...");
		agp_cmd = 0xfffffff7;

		/* set agp 3 speed bit is agp is v3 */
		if ((nth_agp_info.interface.status & AGP_3_MODE) != 0)
			agp_cmd |= AGP_3_MODE;

		/* we want to perma disable fastwrites as they're evil, evil i say */
		agp_cmd &= ~AGP_FAST_WRITE;

		agp_cmd = sAGP->set_agp_mode(agp_cmd);
	}

	/* list mode now activated,
	 * make sure we have the correct speed scheme for logging */
	agp_list_active(agp_cmd | (nth_agp_info.interface.status & AGP_3_MODE));
}


static void
agp_list_info(agp_info ai)
{
	/*
		list device
	*/
	if (ai.class_base == PCI_display) {
		SHOW_INFO(4, "Device is a graphics card, subclass ID is $%02x", ai.class_sub);
	} else {
		SHOW_INFO(4, "Device is a hostbridge, subclass ID is $%02x", ai.class_sub);
	}

	SHOW_INFO(4, "Vendor ID $%04x", ai.vendor_id);
	SHOW_INFO(4, "Device ID $%04x", ai.device_id);
	SHOW_INFO(4, "Bus %d, device %d, function %d", ai.bus, ai.device, ai.function);

	/*
		list capabilities
	*/
	SHOW_INFO(4,
		"This device supports AGP specification %" B_PRIu32 ".%" B_PRIu32 ";",
		((ai.interface.capability_id & AGP_REV_MAJOR) >> AGP_REV_MAJOR_SHIFT),
		((ai.interface.capability_id & AGP_REV_MINOR) >> AGP_REV_MINOR_SHIFT));

	/* the AGP devices determine AGP speed scheme version used on power-up/reset */
	if ((ai.interface.status & AGP_3_MODE) == 0) {
		/* AGP 2.0 scheme applies */
		if (ai.interface.status & AGP_2_1x)
			SHOW_INFO0(4, "AGP 2.0 1x mode is available");
		if (ai.interface.status & AGP_2_2x)
			SHOW_INFO0(4, "AGP 2.0 2x mode is available");
		if (ai.interface.status & AGP_2_4x)
			SHOW_INFO0(41, "AGP 2.0 4x mode is available");
	} else {
		/* AGP 3.0 scheme applies */
		if (ai.interface.status & AGP_3_4x)
			SHOW_INFO0(4, "AGP 3.0 4x mode is available");
		if (ai.interface.status & AGP_3_8x)
			SHOW_INFO0(4, "AGP 3.0 8x mode is available");
	}
	
	if (ai.interface.status & AGP_FAST_WRITE)
		SHOW_INFO0(4, "Fast write transfers are supported");
	if (ai.interface.status & AGP_SBA)
		SHOW_INFO0(4, "Sideband adressing is supported");

	SHOW_INFO(1, "%" B_PRIu32 " queued AGP requests can be handled.",
		((ai.interface.status & AGP_REQUEST) >> AGP_REQUEST_SHIFT) + 1);

	/*
		list current settings,
		make sure we have the correct speed scheme for logging
	 */
	agp_list_active(ai.interface.command | (ai.interface.status & AGP_3_MODE));
}


static void
agp_list_active(uint32 cmd)
{
	SHOW_INFO0(4, "listing settings now in use:");
	if ((cmd & AGP_3_MODE) == 0) {
		/* AGP 2.0 scheme applies */
		if (cmd & AGP_2_1x)
			SHOW_INFO0(2,"AGP 2.0 1x mode is set");
		if (cmd & AGP_2_2x)
			SHOW_INFO0(2,"AGP 2.0 2x mode is set");
		if (cmd & AGP_2_4x)
			SHOW_INFO0(2,"AGP 2.0 4x mode is set");
	} else {
		/* AGP 3.0 scheme applies */
		if (cmd & AGP_3_4x)
			SHOW_INFO0(2,"AGP 3.0 4x mode is set");
		if (cmd & AGP_3_8x)
			SHOW_INFO0(2,"AGP 3.0 8x mode is set");
	}
	
	if (cmd & AGP_FAST_WRITE) {
		SHOW_INFO0(2, "Fast write transfers are enabled");
	} else {
		SHOW_INFO0(2, "Fast write transfers are disabled");
	}
	if (cmd & AGP_SBA)
		SHOW_INFO0(4, "Sideband adressing is enabled");

	SHOW_INFO(4, "Max. AGP queued request depth is set to %" B_PRIu32,
		(((cmd & AGP_REQUEST) >> AGP_REQUEST_SHIFT) + 1));

	if (cmd & AGP_ENABLE)
		SHOW_INFO0(2, "The AGP interface is enabled.");
	else
		SHOW_INFO0(2, "The AGP interface is disabled.");
}
