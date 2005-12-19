/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the Haiku License.
 */

#ifndef GENERIC_TEXT_CONSOLE_H
#define GENERIC_TEXT_CONSOLE_H

class Menu;
class MenuItem;

void platform_generic_update_text_menu_item(Menu *menu, MenuItem *item);
void platform_generic_run_text_menu(Menu *menu);

#endif	/* GENERIC_TEXT_CONSOLE_H */
