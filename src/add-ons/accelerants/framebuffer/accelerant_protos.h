/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _ACCELERANT_PROTOS_H
#define _ACCELERANT_PROTOS_H


#include <Accelerant.h>
#include "video_overlay.h"


#ifdef __cplusplus
extern "C" {
#endif

// general
status_t framebuffer_init_accelerant(int fd);
ssize_t framebuffer_accelerant_clone_info_size(void);
void framebuffer_get_accelerant_clone_info(void *data);
status_t framebuffer_clone_accelerant(void *data);
void framebuffer_uninit_accelerant(void);
status_t framebuffer_get_accelerant_device_info(accelerant_device_info *adi);
sem_id framebuffer_accelerant_retrace_semaphore(void);

// modes & constraints
uint32 framebuffer_accelerant_mode_count(void);
status_t framebuffer_get_mode_list(display_mode *dm);
status_t framebuffer_set_display_mode(display_mode *modeToSet);
status_t framebuffer_get_display_mode(display_mode *currentMode);
status_t framebuffer_get_edid_info(void *info, size_t size, uint32 *_version);
status_t framebuffer_get_frame_buffer_config(frame_buffer_config *config);
status_t framebuffer_get_pixel_clock_limits(display_mode *dm, uint32 *low,
	uint32 *high);

#ifdef __cplusplus
}
#endif

#endif	/* _ACCELERANT_PROTOS_H */
