/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2023, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUPPORTING_APPS_MENU_H
#define SUPPORTING_APPS_MENU_H


#include <SupportDefs.h>

class BHandler;
class BMenu;
class BMimeType;

extern const char* kApplicationSignature;

void update_supporting_apps_menu(BMenu* menu, BMimeType* type, uint32 what, BHandler* target);

#endif	// SUPPORTING_APPS_MENU_H
