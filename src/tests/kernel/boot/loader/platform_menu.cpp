/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "RootFileSystem.h"

#include <boot/platform.h>
#include <boot/vfs.h>
#include <boot/menu.h>

#include <fcntl.h>
#include <stdio.h>


extern RootFileSystem *gRoot;


void
platform_add_menus(Menu *menu)
{
}


void
platform_update_menu_item(Menu *menu, MenuItem *item)
{
}


void
platform_run_menu(Menu *menu)
{
	puts("List of known root directories:");

	void *cookie;
	if (gRoot->Open(&cookie, O_RDONLY) == B_OK) {
		Directory *directory;
		while (gRoot->GetNextNode(cookie, (Node **)&directory) == B_OK) {
			char name[256];
			if (directory->GetName(name, sizeof(name)) == B_OK)
				printf(":: %s (%p)\n", name, directory);

			void *subCookie;
			if (directory->Open(&subCookie, O_RDONLY) == B_OK) {
				while (directory->GetNextEntry(subCookie, name, sizeof(name)) == B_OK) {
					printf("\t%s\n", name);
				}
				directory->Close(subCookie);
			}
		}
		gRoot->Close(cookie);
	}
}

