/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_FB_CONSOLE_H
#define KERNEL_FB_CONSOLE_H

struct kernel_args;

int fb_console_dev_init(struct kernel_args *ka);

#endif	/* KERNEL_FB_CONSOLE_H */
