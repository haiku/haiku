/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VIDEO_H
#define VIDEO_H


#include <SupportDefs.h>

/* 4CCs for custom colorspaces */
// XXX: move to kernel/app_server header ?
#define ATARI_IBP1 'IBP1'
#define ATARI_IBP2 'IBP2'
#define ATARI_IBP4 'IBP4'
#define ATARI_IBP8 'IBP8'


class Menu;
class MenuItem;

bool video_mode_hook(Menu *menu, MenuItem *item);
Menu *video_mode_menu();

#endif	/* VIDEO_H */
