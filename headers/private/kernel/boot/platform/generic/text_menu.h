/*
 * Copyright 2011, Rene Gollent <rene@gollent.com>.
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef GENERIC_TEXT_MENU_H
#define GENERIC_TEXT_MENU_H

class Menu;
class MenuItem;

void platform_generic_update_text_menu_item(Menu* menu, MenuItem* item);
void platform_generic_run_text_menu(Menu* menu);
size_t platform_generic_get_user_input_text(Menu* menu, MenuItem* item,
	char* buffer, size_t bufferSize);

#endif	/* GENERIC_TEXT_MENU_H */
