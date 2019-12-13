// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>
#include <efi/protocol/simple-network.h>

#define EFI_MANAGED_NETWORK_PROTOCOL_GUID \
    {0x7ab33a91, 0xace5, 0x4326,{0xb5, 0x72, 0xe7, 0xee, 0x33, 0xd3, 0x9f, 0x16}}
extern efi_guid ManagedNetworkProtocol;

typedef struct {
    efi_time  Timestamp;
    efi_event RecycleEvent;
    uint32_t PacketLength;
    uint32_t HeaderLength;
    uint32_t AddressLength;
    uint32_t DataLength;
    bool BroadcastFlag;
    bool MulticastFlag;
    bool PromiscuousFlag;
    uint16_t  ProtocolType;
    void* DestinationAddress;
    void* SourceAddress;
    void* MediaHeader;
    void* PacketData;
} efi_managed_network_receive_data;

typedef struct {
    uint32_t FragmentLength;
    void* FragmentBuffer;
} efi_managed_network_fragment_data;

typedef struct {
    efi_mac_addr* DestinationAddress;
    efi_mac_addr* SourceAddress;
    uint16_t ProtocolType;
    uint32_t DataLength;
    uint16_t HeaderLength;
    uint16_t FragmentCount;
    efi_managed_network_fragment_data FragmentTable[1];
} efi_managed_network_transmit_data;

typedef struct {
    uint32_t ReceivedQueueTimeoutValue;
    uint32_t TransmitQueueTimeoutValue;
    uint16_t ProtocolTypeFilter;
    bool EnableUnicastReceive;
    bool EnableMulticastReceive;
    bool EnableBroadcastReceive;
    bool EnablePromiscuousReceive;
    bool FlushQueuesOnReset;
    bool EnableReceiveTimestamps;
    bool DisableBackgroundPolling;
} efi_managed_network_config_data;

typedef struct {
    efi_event Event;
    efi_status Status;
    union {
        efi_managed_network_receive_data* RxData;
        efi_managed_network_transmit_data* TxData;
    } Packet;
} efi_managed_network_sync_completion_token;

typedef struct efi_managed_network_protocol {
    efi_status (*GetModeData) (struct efi_managed_network_protocol* self,
                               efi_managed_network_config_data* mnp_config_data,
                               efi_simple_network_mode* snp_mode_data) EFIAPI;

    efi_status (*Configure) (struct efi_managed_network_protocol* self,
                             efi_managed_network_config_data* mnp_config_data) EFIAPI;

    efi_status (*McastIpToMac) (struct efi_managed_network_protocol* self,
                                bool ipv6_flag, efi_ip_addr* ip_addr, efi_mac_addr* mac_addr) EFIAPI;

    efi_status (*Groups) (struct efi_managed_network_protocol* self, bool join_flag,
                          efi_mac_addr* mac_addr) EFIAPI;

    efi_status (*Transmit) (struct efi_managed_network_protocol* self,
                            efi_managed_network_sync_completion_token* token) EFIAPI;

    efi_status (*Receive) (struct efi_managed_network_protocol* self,
                           efi_managed_network_sync_completion_token* token) EFIAPI;

    efi_status (*Cancel) (struct efi_managed_network_protocol* self,
                          efi_managed_network_sync_completion_token* token) EFIAPI;

    efi_status (*Poll) (struct efi_managed_network_protocol* self) EFIAPI;
} efi_managed_network_protocol;
