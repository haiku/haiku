// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/boot-services.h>
#include <efi/types.h>

#define EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_GUID \
    {0x2f707ebb, 0x4a1a, 0x11D4, {0x9a, 0x38, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}
extern efi_guid PciRootBridgeIoProtocol;

typedef enum {
    EfiPciWidthUint8,
    EfiPciWidthUint16,
    EfiPciWidthUint32,
    EfiPciWidthUint64,
    EfiPciWidthFifoUint8,
    EfiPciWidthFifoUint16,
    EfiPciWidthFifoUint32,
    EfiPciWidthFifoUint64,
    EfiPciWidthFillUint8,
    EfiPciWidthFillUint16,
    EfiPciWidthFillUint32,
    EfiPciWidthFillUint64,
    EfiPciWidthMaximum,
} efi_pci_root_bridge_io_width;

struct efi_pci_root_bridge_io_protocol;

typedef struct {
    efi_status (*Read) (struct efi_pci_root_bridge_io_protocol* self,
                        efi_pci_root_bridge_io_width   width,
                        uint64_t addr, size_t count, void* buffer) EFIAPI;
    efi_status (*Write) (struct efi_pci_root_bridge_io_protocol* self,
                         efi_pci_root_bridge_io_width   width,
                         uint64_t addr, size_t count, void* buffer) EFIAPI;
} efi_pci_root_bridge_io_access;

#define EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO 0x0001
#define EFI_PCI_ATTRIBUTE_ISA_IO 0x0002
#define EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO 0x0004
#define EFI_PCI_ATTRIBUTE_VGA_MEMORY 0x0008
#define EFI_PCI_ATTRIBUTE_VGA_IO 0x0010
#define EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO 0x0020
#define EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO 0x0040
#define EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE 0x0080
#define EFI_PCI_ATTRIBUTE_MEMORY_CACHED 0x0800
#define EFI_PCI_ATTRIBUTE_MEMORY_DISABLE 0x1000
#define EFI_PCI_ATTRIBUTE_DUAL_ADDRESS_CYCLE 0x8000
#define EFI_PCI_ATTRIBUTE_ISA_IO_16 0x10000
#define EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16 0x20000
#define EFI_PCI_ATTRIBUTE_VGA_IO_16 0x40000

typedef enum {
    EfiPciOperationBusMasterRead,
    EfiPciOperationBusMasterWrite,
    EfiPciOperationBusMasterCommonBuffer,
    EfiPciOperationBusMasterRead64,
    EfiPciOperationBusMasterWrite64,
    EfiPciOperationBusMasterCommonBuffer64,
    EfiPciOperationMaximum,
} efi_pci_root_bridge_io_operation;

typedef struct efi_pci_root_bridge_io_protocol {
    efi_handle ParentHandle;

    efi_status (*PollMem) (struct efi_pci_root_bridge_io_protocol *self,
                           efi_pci_root_bridge_io_width width,
                           uint64_t addr, uint64_t mask, uint64_t value, uint64_t delay,
                           uint64_t* result) EFIAPI;

    efi_status (*PollIo) (struct efi_pci_root_bridge_io_protocol *self,
                          efi_pci_root_bridge_io_width width,
                          uint64_t addr, uint64_t mask, uint64_t value, uint64_t delay,
                          uint64_t* result) EFIAPI;

    efi_pci_root_bridge_io_access Mem;
    efi_pci_root_bridge_io_access Io;
    efi_pci_root_bridge_io_access Pci;

    efi_status (*CopyMem) (struct efi_pci_root_bridge_io_protocol* self,
                           efi_pci_root_bridge_io_width width,
                           uint64_t dest_addr, uint64_t src_addr, size_t count) EFIAPI;

    efi_status (*Map) (struct efi_pci_root_bridge_io_protocol* self,
                       efi_pci_root_bridge_io_operation operation,
                       void* host_addr, size_t* num_bytes,
                       efi_physical_addr* device_addr, void** mapping) EFIAPI;

    efi_status (*Unmap) (struct efi_pci_root_bridge_io_protocol* self,
                         void* mapping) EFIAPI;

    efi_status (*AllocateBuffer) (struct efi_pci_root_bridge_io_protocol* self,
                                  efi_allocate_type type, efi_memory_type memory_type,
                                  size_t pages, void** host_addr, uint64_t attributes) EFIAPI;

    efi_status (*FreeBuffer) (struct efi_pci_root_bridge_io_protocol* self,
                              size_t pages, void* host_addr) EFIAPI;

    efi_status (*Flush) (struct efi_pci_root_bridge_io_protocol* self) EFIAPI;

    efi_status (*GetAttributes) (struct efi_pci_root_bridge_io_protocol* self,
                                 uint64_t* supports, uint64_t* attributes) EFIAPI;

    efi_status (*SetAttributes) (struct efi_pci_root_bridge_io_protocol* self,
                                 uint64_t attributes, uint64_t* resource_base,
                                 uint64_t* resource_len) EFIAPI;

    efi_status (*Configuration) (struct efi_pci_root_bridge_io_protocol* self,
                                 void** resources) EFIAPI;

    uint32_t SegmentNumber;
} efi_pci_root_bridge_io_protocol;
