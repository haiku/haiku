/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VIDEO_H
#define VIDEO_H


#include <SupportDefs.h>

class Menu;
class MenuItem;

bool video_mode_hook(Menu *menu, MenuItem *item);
Menu *video_mode_menu();

#endif	/* VIDEO_H */
