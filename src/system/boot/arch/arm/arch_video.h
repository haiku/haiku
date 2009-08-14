/*
 * Copyright 2009, Haiku Inc.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_VIDEO_H
#define _ARCH_VIDEO_H

#include <SupportDefs.h>

/* try to detect current video mode and set gFrameBuffer accordingly */
extern status_t arch_probe_video_mode();
/* try to set a video mode */
extern status_t arch_set_video_mode(int width, int height, int depth);
extern status_t arch_set_default_video_mode();


#endif /* _ARCH_VIDEO_H */
