// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>
#include <efi/protocol/device-path.h>

#define EFI_DRIVER_BINDING_PROTOCOL_GUID \
    {0x18a031ab, 0xb443, 0x4d1a, {0xa5, 0xc0, 0x0c, 0x09, 0x26, 0x1e, 0x9f, 0x71}}
extern efi_guid DriverBindingProtocol;

typedef struct efi_driver_binding_protocol {
    efi_status (*Supported) (struct efi_driver_binding_protocol* self,
                             efi_handle controller_handle,
                             efi_device_path_protocol* remaining_path) EFIAPI;

    efi_status (*Start) (struct efi_driver_binding_protocol* self,
                         efi_handle controller_handle,
                         efi_device_path_protocol* remaining_path) EFIAPI;

    efi_status (*Stop) (struct efi_driver_binding_protocol* self,
                        efi_handle controller_handle,
                        size_t num_children, efi_handle* child_handle_buf) EFIAPI;

    uint32_t Version;
    efi_handle ImageHandle;
    efi_handle DriverBindingHandle;
} efi_driver_binding_protocol;
