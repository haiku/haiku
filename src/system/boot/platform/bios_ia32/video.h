/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef VIDEO_H
#define VIDEO_H


#include <SupportDefs.h>


class Menu;
class MenuItem;

bool video_mode_hook(Menu *menu, MenuItem *item);
Menu *video_mode_menu();

void video_move_text_cursor(int x, int y);
void video_show_text_cursor(void);
void video_hide_text_cursor(void);

#endif	/* VIDEO_H */
