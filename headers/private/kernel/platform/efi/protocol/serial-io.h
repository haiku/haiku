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

#define EFI_SERIAL_TERMINAL_DEVICE_TYPE_GUID \
    {0x6ad9a60f, 0x5815, 0x4c7c, {0x8A, 0x10, 0x50, 0x53, 0xD2, 0xBF, 0x7A, 0x1B}}


#define EFI_SERIAL_IO_PROTOCOL_REVISION 0x00010000
#define EFI_SERIAL_IO_PROTOCOL_REVISION1p1 0x00010001

//
// Control bits.
//
#define EFI_SERIAL_CLEAR_TO_SEND 0x0010
#define EFI_SERIAL_DATA_SET_READY 0x0020
#define EFI_SERIAL_RING_INDICATE 0x0040
#define EFI_SERIAL_CARRIER_DETECT 0x0080
#define EFI_SERIAL_REQUEST_TO_SEND 0x0002
#define EFI_SERIAL_DATA_TERMINAL_READY 0x0001
#define EFI_SERIAL_INPUT_BUFFER_EMPTY 0x0100
#define EFI_SERIAL_OUTPUT_BUFFER_EMPTY 0x0200
#define EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE 0x1000
#define EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE 0x2000
#define EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE 0x4000

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
    const struct elf_guid* DeviceTypeGuid;  // Revision 1.1
} efi_serial_io_protocol;
