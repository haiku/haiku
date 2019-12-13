// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>
#include <efi/boot-services.h>
#include <efi/runtime-services.h>

#define EFI_FILE_PROTOCOL_REVISION        0x00010000
#define EFI_FILE_PROTOCOL_REVISION2       0x00020000
#define EFI_FILE_PROTOCOL_LATEST_REVISION EFI_FILE_PROTOCOL_REVISION2

#define EFI_FILE_MODE_READ   0x0000000000000001
#define EFI_FILE_MODE_WRITE  0x0000000000000002
#define EFI_FILE_MODE_CREATE 0x8000000000000000

#define EFI_FILE_READ_ONLY  0x0000000000000001
#define EFI_FILE_HIDDEN     0x0000000000000002
#define EFI_FILE_SYSTEM     0x0000000000000004
#define EFI_FILE_RESERVED   0x0000000000000008
#define EFI_FILE_DIRECTORY  0x0000000000000010
#define EFI_FILE_ARCHIVE    0x0000000000000020
#define EFI_FILE_VALID_ATTR 0x0000000000000037

typedef struct {
    efi_event Event;
    efi_status Status;
    size_t BufferSize;
    void* Buffer;
} efi_file_io_token;

#define EFI_FILE_INFO_GUID \
    {0x09576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}
extern efi_guid FileInfoGuid;

typedef struct {
    uint64_t Size;
    uint64_t FileSize;
    uint64_t PhysicalSize;
    efi_time CreateTime;
    efi_time LastAccessTime;
    efi_time ModificationTime;
    uint64_t Attribute;
    char16_t FileName[];
} efi_file_info;

#define EFI_FILE_SYSTEM_INFO_GUID \
    {0x09576e93, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}
extern efi_guid FileSystemInfoGuid;

typedef struct {
    uint64_t Size;
    bool ReadOnly;
    uint64_t VolumeSize;
    uint64_t FreeSpace;
    uint32_t BlockSize;
    char16_t* VolumeLabel[];
} efi_file_system_info;

typedef struct efi_file_protocol {
    uint64_t Revision;

    efi_status (*Open) (struct efi_file_protocol* self, struct efi_file_protocol** new_handle,
                        const char16_t* filename, uint64_t open_mode, uint64_t attributes) EFIAPI;

    efi_status (*Close) (struct efi_file_protocol* self) EFIAPI;

    efi_status (*Delete) (struct efi_file_protocol* self) EFIAPI;

    efi_status (*Read) (struct efi_file_protocol* self, size_t* len, void* buf) EFIAPI;

    efi_status (*Write) (struct efi_file_protocol* self, size_t* len, void* buf) EFIAPI;

    efi_status (*GetPosition) (struct efi_file_protocol* self, uint64_t* position) EFIAPI;

    efi_status (*SetPosition) (struct efi_file_protocol* self, uint64_t position) EFIAPI;

    efi_status (*GetInfo) (struct efi_file_protocol* self, efi_guid* info_type,
                           size_t* buf_size, void* buf) EFIAPI;

    efi_status (*SetInfo) (struct efi_file_protocol* self, efi_guid* info_type,
                           size_t buf_size, void* buf) EFIAPI;

    efi_status (*Flush) (struct efi_file_protocol* self) EFIAPI;

    efi_status (*OpenEx) (struct efi_file_protocol* self, struct efi_file_protocol* new_handle,
                          char16_t* filename, uint64_t open_mode, uint64_t attributes,
                          efi_file_io_token* token) EFIAPI;

    efi_status (*ReadEx) (struct efi_file_protocol* self, efi_file_io_token* token) EFIAPI;

    efi_status (*WriteEx) (struct efi_file_protocol* self, efi_file_io_token* token) EFIAPI;

    efi_status (*FlushEx) (struct efi_file_protocol* self, efi_file_io_token* token) EFIAPI;
} efi_file_protocol;
