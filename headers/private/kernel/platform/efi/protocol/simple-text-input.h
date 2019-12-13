// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>

#define EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID \
    {0x387477c1, 0x69c7, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}
extern efi_guid SimpleTextInputProtocol;

typedef struct {
    uint16_t ScanCode;
    char16_t UnicodeChar;
} efi_input_key;

typedef struct efi_simple_text_input_protocol {
    efi_status (*Reset) (struct efi_simple_text_input_protocol* self,
                         bool extendend_verification) EFIAPI;

    efi_status (*ReadKeyStroke) (struct efi_simple_text_input_protocol* self,
                                 efi_input_key* key) EFIAPI;

    efi_event WaitForKey;
} efi_simple_text_input_protocol;
