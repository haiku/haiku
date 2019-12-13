// Copyright 2016-2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>
#include <efi/boot-services.h>
#include <efi/runtime-services.h>

#define EFI_SERIAL_IO_PROTOCOL_GUID \
    {0xBB25CF6F, 0xF1D4, 0x11D2, {0x9A, 0x0C, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0xFD}}

extern efi_guid SerialIoProtocol;

#define EFI_SERIAL_IO_PROTOCOL_REVISION 0x00010000

typedef struct efi_serial_io_protocol efi_serial_io_protocol;

typedef struct {
    uint32_t ControlMask;
    
    // current Attributes
    uint32_t Timeout;
    uint64_t BaudRate;
    uint32_t ReceiveFifoDepth;
    uint32_t DataBits;
    uint32_t Parity;
    uint32_t StopBits;
} efi_serial_io_mode;

typedef enum {
    DefaultParity,
    NoParity,
    EvenParity,
    OddParity,
    MarkParity,
    SpaceParity
} efi_parity_type;

typedef enum {
    DefaultStopBits,
    OneStopBit, 
    OneFiveStopBits,
    TwoStopBits
} efi_stop_bits_type;


typedef struct efi_serial_io_protocol {
    uint32_t Revision;

    efi_status (*Reset)(efi_serial_io_protocol* self) EFIAPI;
    efi_status (*SetAttributes)(efi_serial_io_protocol* self,
                                uint64_t BaudRate, uint32_t ReceiveFifoDepth,
                                uint32_t Timeout, efi_parity_type Parity,
                                uint8_t DataBits, efi_stop_bits_type StopBits) EFIAPI;
    efi_status (*SetControl)(efi_serial_io_protocol* self,
                             uint32_t Control) EFIAPI;
    efi_status (*GetControl)(efi_serial_io_protocol* self,
                             uint32_t* Control) EFIAPI;
    efi_status (*Write)(efi_serial_io_protocol* self,
                        size_t* BufferSize, void* Buffer) EFIAPI;
    efi_status (*Read)(efi_serial_io_protocol* self,
                       size_t* BufferSize, void* Buffer) EFIAPI;

    efi_serial_io_mode* Mode;
} efi_serial_io_protocol;
