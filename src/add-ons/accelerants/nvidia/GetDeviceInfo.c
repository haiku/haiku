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

	sprintf(adi->name, si->adi.name);
	sprintf(adi->chipset, si->adi.chipset);
	sprintf(adi->serial_no, "unknown");
	adi->memory = si->ps.memory_size;
	adi->dac_speed = si->ps.max_dac1_clock;

	return B_OK;
}

#ifdef __HAIKU__

status_t GET_EDID_INFO(void* info, size_t size, uint32* _version)
{
	if ((!si->ps.crtc1_screen.have_full_edid) && (!si->ps.crtc2_screen.have_full_edid)) {
		LOG(4,("GET_EDID_INFO: EDID info not available\n"));
		return B_ERROR;
	}
	if (size < sizeof(struct edid1_info)) {
		LOG(4,("GET_EDID_INFO: not enough memory available\n"));
		return B_BUFFER_OVERFLOW;
	}

	LOG(4,("GET_EDID_INFO: returning info\n"));

	/* if we have two screens, make the best of it (go for the best compatible info) */
	if ((si->ps.crtc1_screen.have_full_edid) && (si->ps.crtc2_screen.have_full_edid)) {
		/* return info on screen with lowest aspect (4:3 takes precedence over ws) */
		if (si->ps.crtc1_screen.aspect < si->ps.crtc2_screen.aspect)
			memcpy(info, &si->ps.crtc1_screen.full_edid, sizeof(struct edid1_info));
		if (si->ps.crtc1_screen.aspect > si->ps.crtc2_screen.aspect)
			memcpy(info, &si->ps.crtc2_screen.full_edid, sizeof(struct edid1_info));

		/* if both screens have the same aspect, return info on the one with
		 * the lowest native resolution */
		if (si->ps.crtc1_screen.aspect == si->ps.crtc2_screen.aspect) {
			if ((si->ps.crtc1_screen.timing.h_display) < (si->ps.crtc2_screen.timing.h_display))
				memcpy(info, &si->ps.crtc1_screen.full_edid, sizeof(struct edid1_info));
			if ((si->ps.crtc1_screen.timing.h_display) > (si->ps.crtc2_screen.timing.h_display))
				memcpy(info, &si->ps.crtc2_screen.full_edid, sizeof(struct edid1_info));

			/* if both screens have the same resolution and aspect, return info on the 
			 * one used as main screen */
			if ((si->ps.crtc1_screen.timing.h_display) == (si->ps.crtc2_screen.timing.h_display)) {
				if (si->ps.crtc2_prim)
					memcpy(info, &si->ps.crtc2_screen.full_edid, sizeof(struct edid1_info));
				else
					memcpy(info, &si->ps.crtc1_screen.full_edid, sizeof(struct edid1_info));
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

#endif	// __HAIKU__
