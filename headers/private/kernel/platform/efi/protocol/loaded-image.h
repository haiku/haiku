// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>
#include <efi/system-table.h>
#include <efi/protocol/device-path.h>

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5b1b31a1, 0x9562, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}
extern efi_guid LoadedImageProtocol;

#define EFI_LOADED_IMAGE_PROTOCOL_REVISION 0x1000

typedef struct {
    uint32_t Revision;
    efi_handle ParentHandle;
    efi_system_table* SystemTable;
    efi_handle DeviceHandle;
    efi_device_path_protocol* FilePath;
    void* Reserved;
    uint32_t LoadOptionsSize;
    void* LoadOptions;
    void* ImageBase;
    uint64_t ImageSize;
    efi_memory_type ImageCodeType;
    efi_memory_type ImageDataType;

    efi_status (*Unload) (efi_handle img) EFIAPI;
} efi_loaded_image_protocol;
