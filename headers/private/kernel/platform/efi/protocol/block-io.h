// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>
#include <efi/boot-services.h>
#include <efi/runtime-services.h>

#define EFI_BLOCK_IO_PROTOCOL_GUID \
    {0x964e5b21, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}

extern efi_guid BlockIoProtocol;

#define EFI_BLOCK_IO_PROTOCOL_REVISION2 0x00020001
#define EFI_BLOCK_IO_PROTOCOL_REVISION3 0x00020031

typedef struct efi_block_io_media efi_block_io_media;
typedef struct efi_block_io_protocol efi_block_io_protocol;

struct efi_block_io_protocol {
    uint64_t Revision;
    efi_block_io_media* Media;
    efi_status (*Reset)(efi_block_io_protocol* self,
                        bool ExtendedVerification) EFIAPI;
    efi_status (*ReadBlocks)(efi_block_io_protocol* self,
                             uint32_t MediaId, uint64_t LBA,
                             size_t BufferSize, void* Buffer) EFIAPI;
    efi_status (*WriteBlocks)(efi_block_io_protocol* self,
                              uint32_t MediaId, uint64_t LBA,
                              size_t BufferSize, const void* Buffer) EFIAPI;
    efi_status (*FlushBlocks)(efi_block_io_protocol* self);


};

struct efi_block_io_media {
    // present in rev1
    uint32_t MediaId;
    bool RemovableMedia;
    bool MediaPresent;
    bool LogicalPartition;
    bool ReadOnly;
    bool WriteCaching;
    uint32_t BlockSize;
    uint32_t IoAlign;
    uint64_t LastBlock;

    // present in rev2
    uint64_t LowestAlignedLba;
    uint32_t LogicalBlocksPerPhysicalBlock;

    // present in rev3
    uint32_t OptimalTransferLengthGranularity;
};
