/*
 * Copyright 2005-2015, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>
#include <string.h>

#include <compute_display_timing.h>
#include <create_display_modes.h>

#include "accelerant_protos.h"
#include "accelerant.h"


#define TRACE_MODE
#ifdef TRACE_MODE
extern "C" void _sPrintf(const char* format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


bool
operator==(const display_mode &lhs, const display_mode &rhs)
{
	return lhs.space == rhs.space
		&& lhs.virtual_width == rhs.virtual_width
		&& lhs.virtual_height == rhs.virtual_height
		&& lhs.h_display_start == rhs.h_display_start
		&& lhs.v_display_start == rhs.v_display_start;
}


/*!	Checks if the specified \a mode can be set using VESA. */
static bool
is_mode_supported(display_mode* mode)
{
	return (mode != NULL) && (*mode == gInfo->shared_info->current_mode);
}


/*!	Creates the initial mode list of the primary accelerant.
	It's called from vesa_init_accelerant().
*/
status_t
create_mode_list(void)
{
	const color_space colorspace[] = {
		(color_space)gInfo->shared_info->current_mode.space
	};

	if (!gInfo->shared_info->has_edid) {
		display_mode mode = gInfo->shared_info->current_mode;

		compute_display_timing(mode.virtual_width, mode.virtual_height, 60, false,
			&mode.timing);
		fill_display_mode(mode.virtual_width, mode.virtual_height, &mode);

		gInfo->mode_list_area = create_display_modes("virtio_gpu modes",
			NULL, &mode, 1, colorspace, 1, is_mode_supported, &gInfo->mode_list,
			&gInfo->shared_info->mode_count);
	} else {
		edid1_info edidInfo;
		edid_decode(&edidInfo, &gInfo->shared_info->edid_raw);
		gInfo->mode_list_area = create_display_modes("virtio_gpu modes",
			&edidInfo, NULL, 0, colorspace, 1, NULL, &gInfo->mode_list,
			&gInfo->shared_info->mode_count);
	}
	if (gInfo->mode_list_area < 0)
		return gInfo->mode_list_area;

	gInfo->shared_info->mode_list_area = gInfo->mode_list_area;
	return B_OK;
}


//	#pragma mark -


uint32
virtio_gpu_accelerant_mode_count(void)
{
	TRACE(("virtio_gpu_accelerant_mode_count() = %d\n",
		gInfo->shared_info->mode_count));
	return gInfo->shared_info->mode_count;
}


status_t
virtio_gpu_get_mode_list(display_mode* modeList)
{
	TRACE(("virtio_gpu_get_mode_info()\n"));
	memcpy(modeList, gInfo->mode_list,
		gInfo->shared_info->mode_count * sizeof(display_mode));
	return B_OK;
}


status_t
virtio_gpu_get_preferred_mode(display_mode* _mode)
{
	TRACE(("virtio_gpu_get_preferred_mode()\n"));
	*_mode = gInfo->shared_info->current_mode;

	return B_OK;
}


status_t
virtio_gpu_set_display_mode(display_mode* _mode)
{
	TRACE(("virtio_gpu_set_display_mode()\n"));
	if (_mode != NULL && *_mode == gInfo->shared_info->current_mode)
		return B_OK;

	return ioctl(gInfo->device, VIRTIO_GPU_SET_DISPLAY_MODE,
		_mode, sizeof(display_mode));
}


status_t
virtio_gpu_get_display_mode(display_mode* _currentMode)
{
	TRACE(("virtio_gpu_get_display_mode()\n"));
	*_currentMode = gInfo->shared_info->current_mode;
	return B_OK;
}


status_t
virtio_gpu_get_edid_info(void* info, size_t size, uint32* _version)
{
	TRACE(("virtio_gpu_get_edid_info()\n"));

	if (!gInfo->shared_info->has_edid)
		return B_ERROR;
	if (size < sizeof(struct edid1_info))
		return B_BUFFER_OVERFLOW;

	edid_decode((edid1_info*)info, &gInfo->shared_info->edid_raw);
	*_version = EDID_VERSION_1;
	edid_dump((edid1_info*)info);
	return B_OK;
}


status_t
virtio_gpu_get_frame_buffer_config(frame_buffer_config* config)
{
	TRACE(("virtio_gpu_get_frame_buffer_config()\n"));

	config->frame_buffer = gInfo->shared_info->frame_buffer;
	TRACE(("virtio_gpu_get_frame_buffer_config() = %" B_PRIxADDR "\n",
		config->frame_buffer));
	//config->frame_buffer_dma = gInfo->shared_info->physical_frame_buffer;
	config->bytes_per_row = gInfo->shared_info->bytes_per_row;
	TRACE(("virtio_gpu_get_frame_buffer_config() %p\n", config->frame_buffer));
	return B_OK;
}


status_t
virtio_gpu_get_pixel_clock_limits(display_mode* mode, uint32* _low, uint32* _high)
{
	TRACE(("virtio_gpu_get_pixel_clock_limits()\n"));

	// TODO: do some real stuff here (taken from radeon driver)
	uint32 totalPixel = (uint32)mode->timing.h_total
		* (uint32)mode->timing.v_total;
	uint32 clockLimit = 2000000;

	// lower limit of about 48Hz vertical refresh
	*_low = totalPixel * 48L / 1000L;
	if (*_low > clockLimit)
		return B_ERROR;

	*_high = clockLimit;
	return B_OK;
}

