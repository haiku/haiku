/*
 * Copyright 2013, Fredrik Homlqvist, fredrik.holmqvist@gmail.com. All rights reserved.
 * Copyright 2019-2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef EFI_PLATFORM_H
#define EFI_PLATFORM_H


#include <efi/system-table.h>


#define EFI_TEXT_ATTR(f,b)  ((f) | ((b) << 4))

#define CHAR_NULL                       0x0000
#define CHAR_BACKSPACE                  0x0008
#define CHAR_TAB                        0x0009
#define CHAR_LINEFEED                   0x000A
#define CHAR_CARRIAGE_RETURN            0x000D

#define SCAN_NULL                       0x0000
#define SCAN_UP                         0x0001
#define SCAN_DOWN                       0x0002
#define SCAN_RIGHT                      0x0003
#define SCAN_LEFT                       0x0004
#define SCAN_HOME                       0x0005
#define SCAN_END                        0x0006
#define SCAN_INSERT                     0x0007
#define SCAN_DELETE                     0x0008
#define SCAN_PAGE_UP                    0x0009
#define SCAN_PAGE_DOWN                  0x000A
#define SCAN_F1                         0x000B
#define SCAN_F2                         0x000C
#define SCAN_F3                         0x000D
#define SCAN_F4                         0x000E
#define SCAN_F5                         0x000F
#define SCAN_F6                         0x0010
#define SCAN_F7                         0x0011
#define SCAN_F8                         0x0012
#define SCAN_F9                         0x0013
#define SCAN_F10                        0x0014
#define SCAN_F11                        0x0015
#define SCAN_F12                        0x0016
#define SCAN_ESC                        0x0017


extern const efi_system_table		*kSystemTable;
extern const efi_boot_services		*kBootServices;
extern const efi_runtime_services	*kRuntimeServices;
extern efi_handle kImage;

#endif	/* EFI_PLATFORM_H */
