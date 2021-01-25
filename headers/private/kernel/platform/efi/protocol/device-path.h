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
    uint16_t Length;
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

#define DEVICE_PATH_MESSAGING_ATAPI	    0x01
#define DEVICE_PATH_MESSAGING_SCSI	    0x02
#define DEVICE_PATH_MESSAGING_FIBRE_CHANNEL 0x03
#define DEVICE_PATH_MESSAGING_1394	    0x04
#define DEVICE_PATH_MESSAGING_USB	    0x05
#define DEVICE_PATH_MESSAGING_I2O	    0x06
#define DEVICE_PATH_MESSAGING_INFINIBAND    0x09
#define DEVICE_PATH_MESSAGING_VENDOR	    0x0a
#define DEVICE_PATH_MESSAGING_MAC	    0x0b
#define DEVICE_PATH_MESSAGING_IPV4	    0x0c
#define DEVICE_PATH_MESSAGING_IPV6	    0x0d
#define DEVICE_PATH_MESSAGING_UART	    0x0e
#define DEVICE_PATH_MESSAGING_USB_CLASS	    0x0f
#define DEVICE_PATH_MESSAGING_USB_WWID	    0x10
#define DEVICE_PATH_MESSAGING_USB_LUN	    0x11
#define DEVICE_PATH_MESSAGING_SATA	    0x12
#define DEVICE_PATH_MESSAGING_VLAN	    0x14
#define DEVICE_PATH_MESSAGING_FIBRE_CHANNEL_EX 0x15

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
