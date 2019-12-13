// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>
#include <efi/boot-services.h>

#define EFI_RUNTIME_SERVICES_SIGNATURE 0x56524553544e5552
#define EFI_RUNTIME_SERVICES_REVISION EFI_SPECIFICATION_VERSION

#define EFI_VARIABLE_NON_VOLATILE                          0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS                    0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS                        0x00000004
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD                 0x00000008
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS            0x00000010
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS 0x00000020
#define EFI_VARIABLE_APPEND_WRITE                          0x00000040

// TODO: implement the win_certificate structs if we need them
//typedef struct {
//    uint64_t MonotonicCount;
//    win_certificate_uefi_guid AuthInfo;
//} efi_variable_authentication;
//
//typedef struct {
//    efi_time TimeStamp;
//    win_certificate_uefi_guid AuthInfo;
//} efi_variable_authentication_2;

#define EFI_HARDWARE_ERROR_VARIABLE \
    {0x414e6bdd, 0xe47b, 0x47cc, {0xb2, 0x44, 0xbb, 0x61, 0x02, 0x0c, 0xf5, 0x16}}

typedef struct {
    uint16_t Year;
    uint8_t  Month;
    uint8_t  Day;
    uint8_t  Hour;
    uint8_t  Minute;
    uint8_t  Second;
    uint8_t  Pad1;
    uint32_t Nanosecond;
    int16_t  TimeZone;
    uint8_t  Daylight;
    uint8_t  Pad2;
} efi_time;

#define EFI_TIME_ADJUST_DAYLIGHT 0x01
#define EFI_TIME_IN_DAYLIGHT     0x02

#define EFI_UNSPECIFIED_TIMEZONE 0x07FF

typedef struct {
    uint32_t Resolution;
    uint32_t Accuracy;
    bool SetsToZero;
} efi_time_capabilities;

#define EFI_OPTIONAL_PTR 0x00000001

typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} efi_reset_type;

typedef struct {
    uint64_t Length;
    union {
        efi_physical_addr DataBlock;
        efi_physical_addr ContinuationPointer;
    } Union;
} efi_capsule_block_descriptor;

typedef struct {
    efi_guid CapsuleGuid;
    uint32_t HeaderSize;
    uint32_t Flags;
    uint32_t CapsuleImageSize;
} efi_capsule_header;

#define CAPSULE_FLAGS_PERSIST_ACROSS_RESET  0x00010000
#define CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE 0x00020000
#define CAPSULE_FLAGS_INITIATE_RESET        0x00040000

#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI                   0x0000000000000001
#define EFI_OS_INDICATIONS_TIMESTAMP_REVOCATION            0x0000000000000002
#define EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED 0x0000000000000004
#define EFI_OS_INDICATIONS_FMP_CAPSULE_SUPPORTED           0x0000000000000008
#define EFI_OS_INDICATIONS_CAPSULE_RESULT_VAR_SUPPORTED    0x0000000000000010
#define EFI_OS_INDICATIONS_START_OS_RECOVERY               0x0000000000000020
#define EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY         0x0000000000000040

#define EFI_CAPSULE_REPORT_GUID \
    {0x39b68c46, 0xf7fb, 0x441b, {0xb6, 0xec, 0x16, 0xb0, 0xf6, 0x98, 0x21, 0xf3}}

typedef struct {
    uint32_t VariableTotalSize;
    uint32_t Reserved;
    efi_guid CapsuleGuid;
    efi_time CapsuleProcessed;
    efi_status CapsuleStatus;
} efi_capsule_result_variable_header;

typedef struct {
    efi_table_header Hdr;

    efi_status (*GetTime) (efi_time* time, efi_time_capabilities* capabilities) EFIAPI;

    efi_status (*SetTime) (efi_time* time) EFIAPI;

    efi_status (*GetWakeupTime) (bool* enabled, bool* pending, efi_time* time) EFIAPI;

    efi_status (*SetWakeupTime) (bool enable, efi_time* time) EFIAPI;

    efi_status (*SetVirtualAddressMap) (size_t memory_map_size, size_t desc_size,
                                        uint32_t desc_version,
                                        efi_memory_descriptor* virtual_map) EFIAPI;

    efi_status (*ConvertPointer) (size_t debug_disposition, void** addr) EFIAPI;

    efi_status (*GetVariable) (char16_t* var_name, efi_guid* vendor_guid,
                               uint32_t* attributes, size_t* data_size, void* data) EFIAPI;

    efi_status (*GetNextVariableName) (size_t* var_name_size, char16_t* var_name,
                                       efi_guid* vendor_guid) EFIAPI;

    efi_status (*SetVariable) (char16_t* var_name, efi_guid* vendor_guid,
                               uint32_t attributes, size_t data_size, void* data) EFIAPI;

    efi_status (*GetNextHighMonotonicCount) (uint32_t* high_count) EFIAPI;

    efi_status (*ResetSystem) (efi_reset_type reset_type, efi_status reset_status,
                               size_t data_size, void* reset_data) EFIAPI;

    efi_status (*UpdateCapsule) (efi_capsule_header** capsule_header_array,
                                 size_t capsule_count,
                                 efi_physical_addr scatter_gather_list) EFIAPI;

    efi_status (*QueryCapsuleCapabilities) (efi_capsule_header** capsule_header_array,
                                            size_t capsule_count,
                                            uint64_t* max_capsule_size,
                                            efi_reset_type* reset_type) EFIAPI;

    efi_status (*QueryVariableInfo) (uint32_t attributes,
                                     uint64_t* max_var_storage_size,
                                     uint64_t* remaining_var_storage_size,
                                     uint64_t* max_var_size) EFIAPI;
} efi_runtime_services;
