/*
 * Copyright 2009, Haiku Inc.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_VIDEO_H
#define _ARCH_VIDEO_H

#include <SupportDefs.h>

struct fb_description {
        uint8 *base;
        uint32 size;
        uint32 bytes_per_row;
        uint16 width;
        uint16 height;
        uint8 depth;
        bool enabled;
};

extern struct fb_description gFrameBuffer;

extern status_t arch_init_video();


#endif /* _ARCH_VIDEO_H */
