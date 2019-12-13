// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>
#include <efi/protocol/file.h>

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    {0x0964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}
extern efi_guid SimpleFileSystemProtocol;

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION 0x00010000

typedef struct efi_simple_file_system_protocol {
    uint64_t Revision;

    efi_status (*OpenVolume) (struct efi_simple_file_system_protocol* self,
                              efi_file_protocol** root) EFIAPI;
} efi_simple_file_system_protocol;
