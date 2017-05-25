/*
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "smp.h"
#include "video.h"

#include <boot/menu.h>
#include <boot/platform/generic/text_menu.h>
#include <safemode.h>


static void
warm_reboot(char key)
{
	asm("	cli						;"
		"	movl	$0x11, %eax		;"
		"	movl	%eax, %cr0		;"
		"	movl	$0x0, %eax		;"
		"	movl	%eax, %cr3		;"
		"	movl	$0x200, %eax	;"
		"	movl	%eax, %cr4		;"
		"	lidt	saved_idt		;"
		"	call	switch_to_real_mode	;"
		"	int	$0x19				;"
		"	.p2align 4				;"
		"saved_idt:					;"
		"	.short	0x7ff			;"
		"	.long	0x0				;"
		"	.long	0x0				;");
}


void
platform_add_menus(Menu* menu)
{
	MenuItem* item;

	switch (menu->Type()) {
		case MAIN_MENU:
			item = new(std::nothrow) MenuItem("Select screen resolution",
				video_mode_menu());
			if (item != NULL) {
				menu->AddItem(item);
				item->SetTarget(video_mode_hook);
				item->SetShortcut('v');
			}

			menu->AddShortcut('w', warm_reboot);
			break;

		case SAFE_MODE_MENU:
			item = new(std::nothrow) MenuItem("Use fail-safe graphics driver");
			if (item != NULL) {
				menu->AddItem(item);
				item->SetType(MENU_ITEM_MARKABLE);
				item->SetData(B_SAFEMODE_FAIL_SAFE_VIDEO_MODE);
				item->SetHelpText("The system will use VESA mode "
					"and won't try to use any video graphics drivers.");
			}

			smp_add_safemode_menus(menu);

			item = new(std::nothrow) MenuItem("Don't call the BIOS");
			if (item != NULL) {
				menu->AddItem(item);
				item->SetHelpText("Stops the system from calling BIOS "
					"functions.");
				item->SetType(MENU_ITEM_MARKABLE);
			}

			item = new(std::nothrow) MenuItem("Disable APM");
			if (item != NULL) {
				menu->AddItem(item);
				item->SetType(MENU_ITEM_MARKABLE);
				item->SetData(B_SAFEMODE_DISABLE_APM);
				item->SetHelpText("Disables Advanced Power Management hardware "
					"support, overriding the APM setting in the kernel "
					"settings file.");
			}

			item = new(std::nothrow) MenuItem("Disable ACPI");
			if (item != NULL) {
				menu->AddItem(item);
				item->SetType(MENU_ITEM_MARKABLE);
				item->SetData(B_SAFEMODE_DISABLE_ACPI);
				item->SetHelpText("Disables Advanced Configuration and Power "
					"Interface hardware support, overriding the ACPI setting "
					"in the kernel settings file.");
			}
			break;

		default:
			break;
	}
}


void
platform_update_menu_item(Menu* menu, MenuItem* item)
{
	platform_generic_update_text_menu_item(menu, item);
}


void
platform_run_menu(Menu* menu)
{
	platform_generic_run_text_menu(menu);
}


size_t
platform_get_user_input_text(Menu* menu, MenuItem* item, char* buffer,
	size_t bufferSize)
{
	return platform_generic_get_user_input_text(menu, item, buffer,
		bufferSize);
}
