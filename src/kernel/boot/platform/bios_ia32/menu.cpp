/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "keyboard.h"
#include "console.h"
#include "bios.h"
#include "video.h"

#include <boot/platform.h>
#include <boot/menu.h>

#include <string.h>


// position
static const int32 kFirstLine = 8;
static const int32 kOffsetX = 10;
static const int32 kHelpLines = 2;

// colors
static const console_color kItemColor = SILVER;
static const console_color kSelectedItemColor = WHITE;
static const console_color kBackgroundColor = BLACK;
static const console_color kSelectedBackgroundColor = SILVER;
static const console_color kDisabledColor = GREY;

static int32 sMenuOffset = 0;


static int32
menu_height()
{
	return console_height() - kFirstLine - 1 - kHelpLines;
}


static void
print_spacing(int32 count)
{
	for (int32 i = 0; i < count; i++)
		putchar(' ');
}


static void
print_centered(int32 line, const char *text)
{
	console_set_cursor(console_width() / 2 - strlen(text) / 2, line);
	printf("%s", text);

	console_set_cursor(0, 0);
		// this avoids unwanted line feeds
}


static void
print_item_at(int32 line, MenuItem *item, bool clearHelp = true)
{
	bool selected = item->IsSelected();

	line -= sMenuOffset;
	if (line < 0 || line >= menu_height())
		return;

	console_color background = selected ? kSelectedBackgroundColor : kBackgroundColor;
	console_color foreground = selected ? kSelectedItemColor : kItemColor;

	if (!item->IsEnabled())
		foreground = kDisabledColor;

	console_set_cursor(kOffsetX, line + kFirstLine);
	console_set_color(foreground, background);

	size_t length = strlen(item->Label()) + 1;

	if (item->Type() == MENU_ITEM_MARKABLE) {
		console_set_color(GREY, background);
		printf(" [");
		console_set_color(foreground, background);
		printf("%c", item->IsMarked() ? 'x' : ' ');
		console_set_color(GREY, background);
		printf("] ");
		console_set_color(foreground, background);

		length += 4;
	} else
		printf(" ");

	printf(item->Label());
	
	if (item->Submenu() && item->Submenu()->Type() == CHOICE_MENU) {
		// show the current choice (if any)
		Menu *subMenu = item->Submenu();
		MenuItem *subItem = NULL;

		for (int32 i = subMenu->CountItems(); i-- > 0; ) {
			subItem = subMenu->ItemAt(i);
			if (subItem != NULL && subItem->IsMarked())
				break;
		}

		const char *text = " (Current: ";
		printf(text);
		length += strlen(text);

		text = subItem != NULL ? subItem->Label() : "None";
		length += strlen(text);

		console_set_color(selected ? GREY : WHITE, background);

		printf(text);

		console_set_color(foreground, background);
		putchar(')');
		length++;
	}

	print_spacing(console_width() - length - 2*kOffsetX);

	if (!selected)
		return;

	console_set_cursor(0, console_height() - kHelpLines);
	console_set_color(WHITE, BLACK);

	if (clearHelp) {
		// clear help text area
		for (int32 i = 0; i < console_width() * 2 - 1; i++)
			putchar(' ');

		console_set_cursor(0, console_height() - kHelpLines);
	}

	if (item->HelpText() != NULL) {
		// show help text at the bottom of the screen,
		// center it, and wrap it correctly

		const char *text = item->HelpText();
		int32 width = console_width() - 2 * kOffsetX;
		int32 length = strlen(text);

		if (length > width * 2)
			width += 2 * kOffsetX - 1;

		char buffer[width + 1];
		buffer[width] = '\0';
			// make sure the buffer is always terminated

		int32 row = 0;

		for (int32 i = 0; i < length && row < 2; i++) {
			while (text[i] == ' ')
				i++;

			// copy as much bytes as possible
			int32 bytes = width;
			if (bytes > length - i)
				bytes = length - i;

			memcpy(buffer, text + i, bytes);
			buffer[bytes] = '\0';

			char *pos = strchr(buffer, '\n');
			if (pos != NULL)
				i = pos - buffer;
			else if (bytes < length - i) {
				// search for possible line breaks
				pos = strrchr(buffer, ' ');
				if (pos != NULL)
					i = pos - buffer;
				else {
					// no wrapping possible
					i += bytes;
				}
			} else
				i += bytes;

			buffer[i] = '\0';
			print_centered(console_height() - kHelpLines + row, buffer);
			row++;
		}
	}
}


static void
draw_menu(Menu *menu)
{
	console_clear_screen();

	console_set_color(WHITE, BLACK);
	print_centered(1, "Welcome To The");

	console_set_color(WHITE, BLACK);
	print_centered(2, "Haiku Boot Loader");

	console_set_color(GREEN, BLACK);
	print_centered(4, "Copyright 2004 Haiku Inc.");

	if (menu->Title()) {
		console_set_cursor(kOffsetX, kFirstLine - 2);
		console_set_color(WHITE, GREY);

		printf(" %s", menu->Title());
		print_spacing(console_width() - 1 - strlen(menu->Title()) - 2*kOffsetX);
	}

	MenuItemIterator iterator = menu->ItemIterator();
	MenuItem *item;
	int32 i = 0;

	while ((item = iterator.Next()) != NULL) {
		if (item->Type() == MENU_ITEM_SEPARATOR) {
			putchar('\n');
			i++;
			continue;
		}

		print_item_at(i++, item, false);
	}

	int32 height = menu_height();
	if (menu->CountItems() >= height) {
		int32 x = console_width() - kOffsetX;
		console_set_cursor(x, kFirstLine);
		console_set_color(YELLOW, BLACK);
		putchar(30/*24*/);
		height--;

		int32 start = sMenuOffset * height / menu->CountItems();
		int32 end = (sMenuOffset + height) * height / menu->CountItems();

		for (i = 1; i < height; i++) {
			console_set_cursor(x, kFirstLine + i);
			if (i >= start && i <= end)
				console_set_color(WHITE, YELLOW);
			else
				console_set_color(WHITE, GREY);

			putchar(' ');
		}

		console_set_cursor(x, kFirstLine + i);
		console_set_color(YELLOW, BLACK);
		putchar(31/*25*/);
	}
}


static void
run_menu(Menu *menu)
{
	sMenuOffset = 0;
	menu->Show();

	draw_menu(menu);

	// Get selected entry, or select the last one, if there is none
	int32 selected;
	MenuItem *item = menu->FindSelected(&selected);
	if (item == NULL) {
		selected = menu->CountItems() - 1;
		item = menu->ItemAt(selected);
		if (item != NULL)
			item->Select(true);
	}

	while (true) {
		union key key = wait_for_key();

		item = menu->ItemAt(selected);

		if (key.code.ascii == 0) {
			int32 oldSelected = selected;

			switch (key.code.bios) {
				case BIOS_KEY_UP:
					while ((item = menu->ItemAt(--selected)) != NULL) {
						if (item->IsEnabled() && item->Type() != MENU_ITEM_SEPARATOR)
							break;
					}
					if (selected < 0)
						selected = menu->CountItems() - 1;
					break;
				case BIOS_KEY_DOWN:
					while ((item = menu->ItemAt(++selected)) != NULL) {
						if (item->IsEnabled() && item->Type() != MENU_ITEM_SEPARATOR)
							break;
					}
					if (selected >= menu->CountItems())
						selected = 0;
					break;
				case BIOS_KEY_PAGE_UP:
					selected -= menu_height() - 1;

					while ((item = menu->ItemAt(selected)) != NULL) {
						if (item->IsEnabled() && item->Type() != MENU_ITEM_SEPARATOR)
							break;

						selected--;
					}

					if (selected < 0)
						selected = 0;
					break;
				case BIOS_KEY_PAGE_DOWN:
					selected += menu_height() - 1;

					while ((item = menu->ItemAt(selected)) != NULL) {
						if (item->IsEnabled() && item->Type() != MENU_ITEM_SEPARATOR)
							break;

						selected++;
					}

					if (selected >= menu->CountItems())
						selected = menu->CountItems() - 1;
					break;
				case BIOS_KEY_HOME:
					selected = 0;
					break;
				case BIOS_KEY_END:
					selected = menu->CountItems() - 1;
					break;
			}

			// check if selected has changed
			if (selected != oldSelected) {
				MenuItem *item = menu->ItemAt(selected);
				if (item != NULL)
					item->Select(true);
				
				// make sure that the new selected entry is visible
				if (sMenuOffset > selected
					|| sMenuOffset + menu_height() <= selected) {
					if (sMenuOffset > selected)
						sMenuOffset = selected;
					else
						sMenuOffset = selected + 1 - menu_height();

					draw_menu(menu);
				}
			}
		} else if (key.code.ascii == 0xd || key.code.ascii == ' ') {
			// leave the menu
			if (item->Submenu() != NULL) {
				int32 offset = sMenuOffset;
				menu->Hide();

				run_menu(item->Submenu());
				if (item->Target() != NULL)
					(*item->Target())(menu, item);

				// restore current menu
				sMenuOffset = offset;
				menu->Show();
				draw_menu(menu);
			} else if (item->Type() == MENU_ITEM_MARKABLE) {
				// toggle state
				item->SetMarked(!item->IsMarked());
				print_item_at(selected, item);

				if (item->Target() != NULL)
					(*item->Target())(menu, item);
			} else if (key.code.ascii == 0xd) {
				// the space key does not exit the menu

				if (menu->Type() == CHOICE_MENU
					&& item->Type() != MENU_ITEM_NO_CHOICE
					&& item->Type() != MENU_ITEM_TITLE)
					item->SetMarked(true);

				if (item->Target() != NULL)
					(*item->Target())(menu, item);

				break;
			}
		} else if (key.code.ascii == 0x1b && menu->Type() != MAIN_MENU)
			// escape key was hit
			break;
	}

	menu->Hide();
}


//	#pragma mark -


void
platform_add_menus(Menu *menu)
{
	MenuItem *item;

	switch (menu->Type()) {
		case MAIN_MENU:
			menu->AddItem(item = new MenuItem("Select fail-safe video mode", video_mode_menu()));
			item->SetTarget(video_mode_hook);
			break;
		case SAFE_MODE_MENU:
			menu->AddItem(item = new MenuItem("Don't call the BIOS"));
			item->SetType(MENU_ITEM_MARKABLE);
			break;
		default:
			break;
	}
}


void 
platform_update_menu_item(Menu *menu, MenuItem *item)
{
	if (menu->IsHidden())
		return;

	int32 index = menu->IndexOf(item);
	if (index == -1)
		return;

	print_item_at(index, item);
}


void
platform_run_menu(Menu *menu)
{
	platform_switch_to_text_mode();

	run_menu(menu);

	platform_switch_to_logo();
}

