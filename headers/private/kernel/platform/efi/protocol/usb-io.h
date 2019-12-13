// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <efi/types.h>

#define EFI_USB_IO_PROTOCOL_GUID \
    {0x2b2f68d6, 0x0cd2, 0x44cf, {0x8e, 0x8b, 0xbb, 0xa2, 0x0b, 0x1b, 0x5b, 0x75}}
extern efi_guid UsbIoProtocol;

typedef enum {
    EfiUsbDataIn,
    EfiUsbDataOut,
    EfiUsbNoData
} efi_usb_data_direction;

#define EFI_USB_NOERROR        0x0000
#define EFI_USB_ERR_NOTEXECUTE 0x0001
#define EFI_USB_ERR_STALL      0x0002
#define EFI_USB_ERR_BUFFER     0x0004
#define EFI_USB_ERR_BABBLE     0x0008
#define EFI_USB_ERR_NAK        0x0010
#define EFI_USB_ERR_CRC        0x0020
#define EFI_USB_ERR_TIMEOUT    0x0040
#define EFI_USB_ERR_BITSTUFF   0x0080
#define EFI_USB_ERR_SYSTEM     0x0100

typedef struct {
    uint8_t  RequestType;
    uint8_t  Request;
    uint16_t Value;
    uint16_t Index;
    uint16_t Length;
} efi_usb_device_request;

typedef efi_status (*efi_async_usb_transfer_callback) (
        void* Data, size_t DataLength, void* Context, uint32_t Status) EFIAPI;

typedef struct {
    uint8_t  Length;
    uint8_t  DescriptorType;
    uint16_t BcdUSB;
    uint8_t  DeviceClass;
    uint8_t  DeviceSubClass;
    uint8_t  DeviceProtocol;
    uint8_t  MaxPacketSize0;
    uint16_t IdVendor;
    uint16_t IdProduct;
    uint16_t BcdDevice;
    uint8_t  StrManufacturer;
    uint8_t  StrProduct;
    uint8_t  StrSerialNumber;
    uint8_t  NumConfigurations;
} efi_usb_device_descriptor;

typedef struct {
    uint8_t  Length;
    uint8_t  DescriptorType;
    uint16_t TotalLength;
    uint8_t  NumInterfaces;
    uint8_t  ConfigurationValue;
    uint8_t  Configuration;
    uint8_t  Attributes;
    uint8_t  MaxPower;
} efi_usb_config_descriptor;

typedef struct {
    uint8_t Length;
    uint8_t DescriptorType;
    uint8_t InterfaceNumber;
    uint8_t AlternateSetting;
    uint8_t NumEndpoints;
    uint8_t InterfaceClass;
    uint8_t InterfaceSubClass;
    uint8_t InterfaceProtocol;
    uint8_t Interface;
} efi_usb_interface_descriptor;

typedef struct {
    uint8_t  Length;
    uint8_t  DescriptorType;
    uint8_t  EndpointAddress;
    uint8_t  Attributes;
    uint16_t MaxPacketSize;
    uint8_t  Interval;
} efi_usb_endpoint_descriptor;

typedef struct efi_usb_io_protocol {
    efi_status (*UsbControlTransfer) (struct efi_usb_io_protocol* self,
                                      efi_usb_device_request* request,
                                      efi_usb_data_direction direction,
                                      uint32_t timeout, void* data,
                                      size_t data_len, uint32_t* status) EFIAPI;

    efi_status (*UsbBulkTransfer) (struct efi_usb_io_protocol* self,
                                   uint8_t endpoint, void* data,
                                   size_t data_len, size_t timeout,
                                   uint32_t* status) EFIAPI;

    efi_status (*UsbAsyncInterruptTransfer) (struct efi_usb_io_protocol* self,
                                             uint8_t endpoint, bool is_new_transfer,
                                             size_t polling_interval,
                                             size_t data_len,
                                             efi_async_usb_transfer_callback interrupt_cb,
                                             void* context) EFIAPI;

    efi_status (*UsbSyncInterruptTransfer) (struct efi_usb_io_protocol* self,
                                            uint8_t endpoint, void* data,
                                            size_t* data_len, size_t timeout,
                                            uint32_t* status) EFIAPI;

    efi_status (*UsbIsochronousTransfer) (struct efi_usb_io_protocol* self,
                                          uint8_t endpoint,
                                          void* data, size_t data_len,
                                          uint32_t* status) EFIAPI;

    efi_status (*UsbAsyncIsochronousTransfer) (struct efi_usb_io_protocol* self,
                                               uint8_t endpoint,
                                               void* data, size_t data_len,
                                               efi_async_usb_transfer_callback isoc_cb,
                                               void* context) EFIAPI;

    efi_status (*UsbGetDeviceDescriptor) (struct efi_usb_io_protocol* self,
                                          efi_usb_device_descriptor* descriptor) EFIAPI;

    efi_status (*UsbGetConfigDescriptor) (struct efi_usb_io_protocol* self,
                                          efi_usb_config_descriptor* descriptor) EFIAPI;

    efi_status (*UsbGetInterfaceDescriptor) (struct efi_usb_io_protocol* self,
                                             efi_usb_interface_descriptor* descriptor) EFIAPI;

    efi_status (*UsbGetEndpointDescriptor) (struct efi_usb_io_protocol* self,
                                            uint8_t endpt_index,
                                            efi_usb_endpoint_descriptor* descriptor) EFIAPI;

    efi_status (*UsbGetStringDescriptor) (struct efi_usb_io_protocol* self,
                                          uint16_t langid, uint8_t stringid,
                                          char16_t** str) EFIAPI;

    efi_status (*UsbGetSupportedLanguages) (struct efi_usb_io_protocol* self,
                                            uint16_t** langid_table,
                                            uint16_t* table_size) EFIAPI;

    efi_status (*UsbPortReset) (struct efi_usb_io_protocol* self) EFIAPI;
} efi_usb_io_protocol;
