/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "menu.h"
#include "loader.h"
#include "RootFileSystem.h"

#include <OS.h>

#include <boot/stage2.h>
#include <boot/vfs.h>
#include <boot/platform.h>
#include <boot/stdio.h>


status_t
user_menu(Directory **_bootVolume)
{
	platform_switch_to_text_mode();

	int32 index = 1;
	void *cookie;

	if (gRoot->Open(&cookie, O_RDONLY) == B_OK) {
		Directory *volume;
		while (gRoot->GetNextNode(cookie, (Node **)&volume) == B_OK) {
			// only list bootable volumes
			if (!is_bootable(volume))
				continue;

			char name[B_FILE_NAME_LENGTH];
			if (volume->GetName(name, sizeof(name)) == B_OK)
				printf("%ld) /%s\n", index++, name);

			// ToDo: for now, just choose the first volume as boot volume
			if (*_bootVolume == NULL)
				*_bootVolume = volume;
		}
		gRoot->Close(cookie);
	}
	puts("(it always tries to boot from the first for now...)");

	platform_switch_to_logo();
	return B_OK;
}

