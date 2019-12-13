// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    {0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}}
extern efi_guid GraphicsOutputProtocol;

typedef struct {
    uint32_t RedMask;
    uint32_t GreenMask;
    uint32_t BlueMask;
    uint32_t ReservedMask;
} efi_pixel_bitmask;

typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} efi_graphics_pixel_format;

typedef struct {
    uint32_t Version;
    uint32_t HorizontalResolution;
    uint32_t VerticalResolution;
    efi_graphics_pixel_format PixelFormat;
    efi_pixel_bitmask PixelInformation;
    uint32_t PixelsPerScanLine;
} efi_graphics_output_mode_information;

typedef struct {
    uint32_t MaxMode;
    uint32_t Mode;
    efi_graphics_output_mode_information* Info;
    size_t SizeOfInfo;
    efi_physical_addr FrameBufferBase;
    size_t FrameBufferSize;
} efi_graphics_output_mode;

typedef struct {
    uint8_t Blue;
    uint8_t Green;
    uint8_t Red;
    uint8_t Reserved;
} efi_graphics_output_blt_pixel;

typedef enum {
    EfiBltVideoFill,
    EfiBltVideoToBltBuffer,
    EfiBltBufferToVideo,
    EfiBltVideoToVideo,
    EfiGraphicsOutputBltOperationMax
} efi_graphics_output_blt_operation;

typedef struct efi_graphics_output_protocol {
    efi_status (*QueryMode) (struct efi_graphics_output_protocol* self,
                             uint32_t mode_num, size_t* info_len,
                             efi_graphics_output_mode_information** info) EFIAPI;

    efi_status (*SetMode) (struct efi_graphics_output_protocol* self,
                           uint32_t mode_num) EFIAPI;

    efi_status (*Blt) (struct efi_graphics_output_protocol* self,
                       efi_graphics_output_blt_pixel* blt_buf,
                       efi_graphics_output_blt_operation blt_operation,
                       size_t src_x, size_t src_y, size_t dest_x, size_t dest_y,
                       size_t width, size_t height, size_t delta) EFIAPI;

    efi_graphics_output_mode* Mode;
} efi_graphics_output_protocol;
