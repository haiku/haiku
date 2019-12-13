// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>

#define EFI_DEVICE_PATH_PROTOCOL_GUID \
    {0x09576e91, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}
extern efi_guid DevicePathProtocol;

typedef struct efi_device_path_protocol {
    uint8_t Type;
    uint8_t SubType;
    uint8_t Length[2];
} efi_device_path_protocol;

#define DEVICE_PATH_HARDWARE       0x01
#define DEVICE_PATH_ACPI           0x02
#define DEVICE_PATH_MESSAGING      0x03
#define DEVICE_PATH_MEDIA          0x04
#define DEVICE_PATH_BIOS_BOOT_SPEC 0x05
#define DEVICE_PATH_END            0x7f

#define DEVICE_PATH_INSTANCE_END 0x01
#define DEVICE_PATH_ENTIRE_END   0xff

#define DEVICE_PATH_HW_PCI        0x01
#define DEVICE_PATH_HW_PCCARD     0x02
#define DEVICE_PATH_HW_MEMMAP     0x03
#define DEVICE_PATH_HW_VENDOR     0x04
#define DEVICE_PATH_HW_CONTROLLER 0x05
#define DEVICE_PATH_HW_BMC        0x06

// TODO: sub-types for other types (ACPI, etc)

// DEVICE_PATH_MEDIA Types
#define MEDIA_HARDDRIVE_DP              0x01
#define MEDIA_CDROM_DP                  0x02
#define MEDIA_VENDOR_DP                 0x03
#define MEDIA_FILEPATH_DP               0x04
#define MEDIA_PROTOCOL_DP               0x05

// TODO: move this to another header? would break circular dependencies between
// boot-services.h and this header, for efi_memory_type
typedef struct {
    efi_device_path_protocol Header;
    efi_memory_type MemoryType;
    efi_physical_addr StartAddress;
    efi_physical_addr EndAddress;
} efi_device_path_hw_memmap;
