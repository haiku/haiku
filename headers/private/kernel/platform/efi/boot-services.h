// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>
#include <efi/protocol/device-path.h>
#include <stdbool.h>

#define EFI_BOOT_SERVICES_SIGNATURE 0x56524553544f4f42
#define EFI_BOOT_SERVICES_REVISION EFI_SPECIFICATION_VERSION

typedef size_t efi_tpl;

#define TPL_APPLICATION 4
#define TPL_CALLBACK    8
#define TPL_NOTIFY     16
#define TPL_HIGH_LEVEL 31

typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} efi_allocate_type;

typedef struct {
    uint32_t Type;
    efi_physical_addr PhysicalStart;
    efi_virtual_addr VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} efi_memory_descriptor;

#define EFI_MEMORY_UC 0x0000000000000001
#define EFI_MEMORY_WC 0x0000000000000002
#define EFI_MEMORY_WT 0x0000000000000004
#define EFI_MEMORY_WB 0x0000000000000008
#define EFI_MEMORY_UCE 0x0000000000000010
#define EFI_MEMORY_WP 0x0000000000001000
#define EFI_MEMORY_RP 0x0000000000002000
#define EFI_MEMORY_XP 0x0000000000004000
#define EFI_MEMORY_NV 0x0000000000008000
#define EFI_MEMORY_MORE_RELIABLE 0x0000000000010000
#define EFI_MEMORY_RO 0x0000000000020000
#define EFI_MEMORY_RUNTIME 0x8000000000000000

#define EFI_MEMORY_DESCRIPTOR_VERSION 1

typedef enum {
    EFI_NATIVE_INTERFACE
} efi_interface_type;

typedef enum {
    AllHandles,
    ByRegisterNotify,
    ByProtocol
} efi_locate_search_type;

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER           0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE           0x00000020

typedef struct {
    efi_handle agent_handle;
    efi_handle controller_handle;
    uint32_t attributes;
    uint32_t open_count;
} efi_open_protocol_information_entry;

#define EFI_HII_PACKAGE_LIST_PROTOCOL_GUID \
    {0x6a1ee763, 0xd47a, 0x43b4, {0xaa, 0xbe, 0xef, 0x1d, 0xe2, 0xab, 0x56, 0xfc}}

typedef struct efi_hii_package_list_header efi_hii_package_list_header;
typedef efi_hii_package_list_header* efi_hii_package_list_protocol;

// fwd declare efi_system_table to break circular dependencies
typedef struct efi_system_table efi_system_table;
typedef efi_status (*efi_image_entry_point) (efi_handle img, efi_system_table* sys) EFIAPI;

typedef struct {
    efi_table_header Hdr;

    efi_tpl (*RaiseTPL) (efi_tpl new_tpl) EFIAPI;

    void (*RestoreTPL) (efi_tpl old_tpl) EFIAPI;

    efi_status (*AllocatePages) (efi_allocate_type type, efi_memory_type memory_type,
                                 size_t pages, efi_physical_addr* memory) EFIAPI;

    efi_status (*FreePages) (efi_physical_addr memory, size_t pages) EFIAPI;

    efi_status (*GetMemoryMap) (size_t* memory_map_size, efi_memory_descriptor* memory_map,
                                size_t* map_key, size_t* desc_size, uint32_t* desc_version) EFIAPI;

    efi_status (*AllocatePool) (efi_memory_type pool_type, size_t size, void** buf) EFIAPI;

    efi_status (*FreePool) (void* buf) EFIAPI;

    efi_status (*CreateEvent) (uint32_t type, efi_tpl notify_tpl,
                               efi_event_notify notify_fn, void* notify_ctx,
                               efi_event* event) EFIAPI;

    efi_status (*SetTimer) (efi_event event, efi_timer_delay type, uint64_t trigger_time) EFIAPI;

    efi_status (*WaitForEvent) (size_t num_events, efi_event* event, size_t* index) EFIAPI;

    efi_status (*SignalEvent) (efi_event event) EFIAPI;

    efi_status (*CloseEvent) (efi_event event) EFIAPI;

    efi_status (*CheckEvent) (efi_event event) EFIAPI;

    efi_status (*InstallProtocolInterface) (efi_handle* handle, efi_guid* protocol,
                                            efi_interface_type intf_type, void* intf) EFIAPI;

    efi_status (*ReinstallProtocolInterface) (efi_handle hadle, efi_guid* protocol,
                                              void* old_intf, void* new_intf) EFIAPI;

    efi_status (*UninstallProtocolInterface) (efi_handle handle, efi_guid* protocol,
                                              void* intf) EFIAPI;

    efi_status (*HandleProtocol) (efi_handle handle, efi_guid* protocol, void** intf) EFIAPI;

    void* Reserved;

    efi_status (*RegisterProtocolNotify) (efi_guid* protocol, efi_event event,
                                          void** registration) EFIAPI;

    efi_status (*LocateHandle) (efi_locate_search_type search_type, efi_guid* protocol,
                                void* search_key, size_t* buf_size, efi_handle* buf) EFIAPI;

    efi_status (*LocateDevicePath) (efi_guid* protocol, efi_device_path_protocol** path,
                                    efi_handle* device) EFIAPI;

    efi_status (*InstallConfigurationTable) (efi_guid* guid, void* table) EFIAPI;

    efi_status (*LoadImage) (bool boot_policy, efi_handle parent_image_handle,
                             efi_device_path_protocol* path, void* src, size_t src_size,
                             efi_handle* image_handle) EFIAPI;

    efi_status (*StartImage) (efi_handle image_handle, size_t* exit_data_size,
                              char16_t** exit_data) EFIAPI;

    efi_status (*Exit) (efi_handle image_handle, efi_status exit_status,
                        size_t exit_data_size, char16_t* exit_data) EFIAPI;

    efi_status (*UnloadImage) (efi_handle image_handle) EFIAPI;

    efi_status (*ExitBootServices) (efi_handle image_handle, size_t map_key) EFIAPI;

    efi_status (*GetNextMonotonicCount) (uint64_t* count) EFIAPI;

    efi_status (*Stall) (size_t microseconds) EFIAPI;

    efi_status (*SetWatchdogTimer) (size_t timeout, uint64_t watchdog_code,
                                    size_t data_size, char16_t* watchdog_data) EFIAPI;

    efi_status (*ConnectController) (efi_handle controller_handle,
                                     efi_handle* driver_image_handle,
                                     efi_device_path_protocol* remaining_path,
                                     bool recursive) EFIAPI;

    efi_status (*DisconnectController) (efi_handle controller_handle,
                                        efi_handle driver_image_handle,
                                        efi_handle child_handle) EFIAPI;

    efi_status (*OpenProtocol) (efi_handle handle, efi_guid* protocol, void** intf,
                                efi_handle agent_handle, efi_handle controller_handle,
                                uint32_t attributes) EFIAPI;

    efi_status (*CloseProtocol) (efi_handle handle, efi_guid* protocol,
                                 efi_handle agent_handle, efi_handle controller_handle) EFIAPI;

    efi_status (*OpenProtocolInformation) (efi_handle handle, efi_guid* protocol,
                                           efi_open_protocol_information_entry** entry_buf,
                                           size_t* entry_count) EFIAPI;

    efi_status (*ProtocolsPerHandle) (efi_handle handle, efi_guid*** protocol_buf,
                                      size_t* protocol_buf_count) EFIAPI;

    efi_status (*LocateHandleBuffer) (efi_locate_search_type search_type,
                                      efi_guid* protocol, void* search_key,
                                      size_t* num_handles, efi_handle** buf) EFIAPI;

    efi_status (*LocateProtocol) (efi_guid* protocol, void* registration, void** intf) EFIAPI;

    efi_status (*InstallMultipleProtocolInterfaces) (efi_handle* handle, ...) EFIAPI;

    efi_status (*UninstallMultipleProtocolInterfaces) (efi_handle handle, ...) EFIAPI;

    efi_status (*CalculateCrc32) (void* data, size_t len, uint32_t* crc32) EFIAPI;

    void (*CopyMem) (void* dest, const void* src, size_t len) EFIAPI;

    void (*SetMem) (void* buf, size_t len, uint8_t val) EFIAPI;

    efi_status (*CreateEventEx) (uint32_t type, efi_tpl notify_tpl,
                                 efi_event_notify notify_fn, const void* notify_ctx,
                                 const efi_guid* event_group, efi_event* event) EFIAPI;
} efi_boot_services;
