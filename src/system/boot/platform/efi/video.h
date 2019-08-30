/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef VIDEO_H
#define VIDEO_H


class Menu;
class MenuItem;

bool video_mode_hook(Menu *menu, MenuItem *item);
Menu *video_mode_menu();

#endif	/* VIDEO_H */
