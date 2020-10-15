/*
	Author:
	Rudolf Cornelissen 7/2004-08/2009
*/

#define MODULE_BIT 0x04000000

#include "acc_std.h"

/* Get some info about the device */
status_t GET_ACCELERANT_DEVICE_INFO(accelerant_device_info * adi)
{
	LOG(4,("GET_ACCELERANT_DEVICE_INFO: returning info\n"));

	/* no info on version is provided, so presumably this is for my info */
	adi->version = 1;

	strcpy(adi->name, si->adi.name);
	strcpy(adi->chipset, si->adi.chipset);
	strcpy(adi->serial_no, "unknown");
	adi->memory = si->ps.memory_size;
	adi->dac_speed = si->ps.max_dac1_clock;

	return B_OK;
}

#ifdef __HAIKU__

/* Get the info fetched from the attached screen */
status_t GET_EDID_INFO(void* info, size_t size, uint32* _version)
{
	if (!si->ps.crtc1_screen.have_full_edid && !si->ps.crtc2_screen.have_full_edid) {
		LOG(4,("GET_EDID_INFO: EDID info not available\n"));
		return B_ERROR;
	}
	if (size < sizeof(struct edid1_info)) {
		LOG(4,("GET_EDID_INFO: not enough memory available\n"));
		return B_BUFFER_OVERFLOW;
	}

	LOG(4,("GET_EDID_INFO: returning info\n"));

	/* if we have two screens, make the best of it (go for the best compatible info) */
	if (si->ps.crtc1_screen.have_full_edid && si->ps.crtc2_screen.have_full_edid) {
		/* return info on screen with lowest aspect (4:3 takes precedence over ws)
		 * NOTE:
		 * allow 0.10 difference so 4:3 and 5:4 aspect screens are regarded as being the same. */
		if (si->ps.crtc1_screen.aspect < (si->ps.crtc2_screen.aspect - 0.10)) {
			memcpy(info, &si->ps.crtc1_screen.full_edid, sizeof(struct edid1_info));
		} else {
			if (si->ps.crtc1_screen.aspect > (si->ps.crtc2_screen.aspect + 0.10)) {
				memcpy(info, &si->ps.crtc2_screen.full_edid, sizeof(struct edid1_info));
			} else {
				/* both screens have the same aspect, return info on the one with
				 * the lowest native resolution */
				if (si->ps.crtc1_screen.timing.h_display < si->ps.crtc2_screen.timing.h_display)
					memcpy(info, &si->ps.crtc1_screen.full_edid, sizeof(struct edid1_info));
				if (si->ps.crtc1_screen.timing.h_display > si->ps.crtc2_screen.timing.h_display)
					memcpy(info, &si->ps.crtc2_screen.full_edid, sizeof(struct edid1_info));

				/* if both screens have the same resolution and aspect, return info on the 
				 * one used as main screen */
				if (si->ps.crtc1_screen.timing.h_display == si->ps.crtc2_screen.timing.h_display) {
					if (si->ps.crtc2_prim)
						memcpy(info, &si->ps.crtc2_screen.full_edid, sizeof(struct edid1_info));
					else
						memcpy(info, &si->ps.crtc1_screen.full_edid, sizeof(struct edid1_info));
				}
			}
		}
	} else {
		/* there's just one screen of which we have EDID information, return it */
		if (si->ps.crtc1_screen.have_full_edid)
			memcpy(info, &si->ps.crtc1_screen.full_edid, sizeof(struct edid1_info));
		else
			memcpy(info, &si->ps.crtc2_screen.full_edid, sizeof(struct edid1_info));
	}

	*_version = EDID_VERSION_1;
	return B_OK;
}

/* Return the optimum (native) mode for the attached screen */
status_t GET_PREFERRED_DISPLAY_MODE(display_mode* preferredMode)
{
	if (si->ps.crtc1_screen.have_full_edid || si->ps.crtc2_screen.have_full_edid) {
		LOG(4,("GET_PREFERRED_DISPLAY_MODE: full EDID known, aborting (fetch EDID instead).\n"));
		return B_ERROR;
	}

	if (!si->ps.crtc1_screen.have_native_edid && !si->ps.crtc2_screen.have_native_edid) {
		LOG(4,("GET_PREFERRED_DISPLAY_MODE: native mode(s) not known\n"));
		return B_ERROR;
	}

	/* if we got here then we're probably on a laptop, but DDC/EDID may have failed
	 * for another reason as well. */
	LOG(4,("GET_PREFERRED_DISPLAY_MODE: returning mode\n"));

	/* if we have two screens, make the best of it (go for the best compatible mode) */
	if (si->ps.crtc1_screen.have_native_edid && si->ps.crtc2_screen.have_native_edid) {
		/* return mode of screen with lowest aspect (4:3 takes precedence over ws)
		 * NOTE:
		 * allow 0.10 difference so 4:3 and 5:4 aspect screens are regarded as being the same. */
		if (si->ps.crtc1_screen.aspect < (si->ps.crtc2_screen.aspect - 0.10)) {
			get_crtc1_screen_native_mode(preferredMode);
		} else {
			if (si->ps.crtc1_screen.aspect > (si->ps.crtc2_screen.aspect + 0.10)) {
				get_crtc2_screen_native_mode(preferredMode);
			} else {
				/* both screens have the same aspect, return mode of the one with
				 * the lowest native resolution */
				if (si->ps.crtc1_screen.timing.h_display < si->ps.crtc2_screen.timing.h_display)
					get_crtc1_screen_native_mode(preferredMode);
				if (si->ps.crtc1_screen.timing.h_display > si->ps.crtc2_screen.timing.h_display)
					get_crtc2_screen_native_mode(preferredMode);

				/* if both screens have the same resolution and aspect, return mode of the 
				 * one used as main screen */
				if (si->ps.crtc1_screen.timing.h_display == si->ps.crtc2_screen.timing.h_display) {
					if (si->ps.crtc2_prim)
						get_crtc2_screen_native_mode(preferredMode);
					else
						get_crtc1_screen_native_mode(preferredMode);
				}
			}
		}
	} else {
		/* there's just one screen of which we have native mode information, return it's mode */
		if (si->ps.crtc1_screen.have_native_edid)
			get_crtc1_screen_native_mode(preferredMode);
		else
			get_crtc2_screen_native_mode(preferredMode);
	}

	return B_OK;
}

#endif	// __HAIKU__
