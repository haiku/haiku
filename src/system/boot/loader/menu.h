/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef MENU_H
#define MENU_H


#include <boot/vfs.h>


class PathBlacklist;


extern status_t user_menu(BootVolume& _bootVolume,
	PathBlacklist& _pathBlacklist);


#endif	/* MENU_H */
