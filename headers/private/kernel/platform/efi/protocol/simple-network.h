// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stdbool.h>
#include <efi/types.h>

#define EFI_SIMPLE_NETWORK_PROTOCOL_GUID \
    {0xa19832b9, 0xac25, 0x11d3, {0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}
extern efi_guid SimpleNetworkProtocol;

#define EFI_SIMPLE_NETWORK_PROTOCOL_REVISION 0x00010000

#define MAX_MCAST_FILTER_CNT 16
typedef struct {
    uint32_t State;
    uint32_t HwAddressSize;
    uint32_t MediaHeaderSize;
    uint32_t MaxPacketSize;
    uint32_t NvRamSize;
    uint32_t NvRamAccessSize;
    uint32_t ReceiveFilterMask;
    uint32_t ReceiveFilterSetting;
    uint32_t MaxMCastFilterCount;
    uint32_t MCastFilterCount;
    efi_mac_addr MCastFilter[MAX_MCAST_FILTER_CNT];
    efi_mac_addr CurrentAddress;
    efi_mac_addr BroadcastAddress;
    efi_mac_addr PermanentAddress;
    uint8_t IfType;
    bool MacAddressChangeable;
    bool MultipleTxSupported;
    bool MediaPresentSupported;
    bool MediaPresent;
} efi_simple_network_mode;

typedef enum {
    EfiSimpleNetworkStopped,
    EfiSimpleNetworkStarted,
    EfiSimpleNetworkInitialized,
    EfiSimpleNetworkMaxState
} efi_simple_network_state;

#define EFI_SIMPLE_NETWORK_RECEIVE_UNICAST               0x01
#define EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST             0x02
#define EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST             0x04
#define EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS           0x08
#define EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST 0x10

typedef struct {
    uint64_t RxTotalFrames;
    uint64_t RxGoodFrames;
    uint64_t RxUndersizeFrames;
    uint64_t RxOversizeFrames;
    uint64_t RxDroppedFrames;
    uint64_t RxUnicastFrames;
    uint64_t RxBroadcastFrames;
    uint64_t RxMulticastFrames;
    uint64_t RxCrcErrorFrames;
    uint64_t RxTotalBytes;
    uint64_t TxTotalFrames;
    uint64_t TxGoodFrames;
    uint64_t TxUndersizeFrames;
    uint64_t TxOversizeFrames;
    uint64_t TxDroppedFrames;
    uint64_t TxUnicastFrames;
    uint64_t TxBroadcastFrames;
    uint64_t TxMulticastFrames;
    uint64_t TxCrcErrorFrames;
    uint64_t TxTotalBytes;
    uint64_t Collisions;
    uint64_t UnsupportedProtocol;
    uint64_t RxDuplicatedFrames;
    uint64_t RxDecryptErrorFrames;
    uint64_t TxErrorFrames;
    uint64_t TxRetryFrames;
} efi_network_statistics;

#define EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT  0x01
#define EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT 0x02
#define EFI_SIMPLE_NETWORK_COMMAND_INTERRUPT  0x04
#define EFI_SIMPLE_NETWORK_SOFTWARE_INTERRUPT 0x08

typedef struct efi_simple_network_protocol {
    uint64_t Revision;

    efi_status (*Start) (struct efi_simple_network_protocol* self) EFIAPI;

    efi_status (*Stop) (struct efi_simple_network_protocol* self) EFIAPI;

    efi_status (*Initialize) (struct efi_simple_network_protocol* self,
                              size_t extra_rx_buf_size, size_t extra_tx_buf_size) EFIAPI;

    efi_status (*Reset) (struct efi_simple_network_protocol* self,
                         bool extended_verification) EFIAPI;

    efi_status (*Shutdown) (struct efi_simple_network_protocol* self) EFIAPI;

    efi_status (*ReceiveFilters) (struct efi_simple_network_protocol* self,
                                  uint32_t enable, uint32_t disable,
                                  bool reset_mcast_filter, size_t mcast_filter_count,
                                  efi_mac_addr* mcast_filter) EFIAPI;

    efi_status (*StationAddress) (struct efi_simple_network_protocol* self,
                                  bool reset, efi_mac_addr* new_addr) EFIAPI;

    efi_status (*Statistics) (struct efi_simple_network_protocol* self,
                              bool reset, size_t* stats_size,
                              efi_network_statistics* stats_table) EFIAPI;

    efi_status (*MCastIpToMac) (struct efi_simple_network_protocol* self,
                                bool ipv6, efi_ip_addr* ip, efi_mac_addr* mac) EFIAPI;

    efi_status (*NvData) (struct efi_simple_network_protocol* self,
                          bool read_write, size_t offset, size_t buf_size, void* buf) EFIAPI;

    efi_status (*GetStatus) (struct efi_simple_network_protocol* self,
                             uint32_t* interrupt_status, void** tx_buf) EFIAPI;

    efi_status (*Transmit) (struct efi_simple_network_protocol* self,
                            size_t header_size, size_t buf_size, void* buf,
                            efi_mac_addr* src, efi_mac_addr* dest, uint16_t* protocol) EFIAPI;

    efi_status (*Receive) (struct efi_simple_network_protocol* self,
                           size_t* header_size, size_t* buf_size, void* buf,
                           efi_mac_addr* src, efi_mac_addr* dest, uint16_t* protocol) EFIAPI;

    efi_event WaitForPacket;
    efi_simple_network_mode* Mode;
} efi_simple_network_protocol;
