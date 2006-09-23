/*
 * Copyright 2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#ifndef _USB_RAW_H_
#define _USB_RAW_H_


#include <lock.h>
#include <USB3.h>


typedef enum {
	RAW_COMMAND_GET_VERSION = 0x1000,

	RAW_COMMAND_GET_DEVICE_DESCRIPTOR = 0x2000,
	RAW_COMMAND_GET_CONFIGURATION_DESCRIPTOR,
	RAW_COMMAND_GET_INTERFACE_DESCRIPTOR,
	RAW_COMMAND_GET_ENDPOINT_DESCRIPTOR,
	RAW_COMMAND_GET_STRING_DESCRIPTOR,
	RAW_COMMAND_GET_GENERIC_DESCRIPTOR,

	RAW_COMMAND_SET_CONFIGURATION = 0x3000,
	RAW_COMMAND_SET_FEATURE,
	RAW_COMMAND_CLEAR_FEATURE,
	RAW_COMMAND_GET_STATUS,
	RAW_COMMAND_GET_DESCRIPTOR,

	RAW_COMMAND_CONTROL_TRANSFER = 0x4000,
	RAW_COMMAND_INTERRUPT_TRANSFER,
	RAW_COMMAND_BULK_TRANSFER
} raw_command_id;


typedef enum {
	RAW_STATUS_SUCCESS = 0,

	RAW_STATUS_FAILED,
	RAW_STATUS_ABORTED,
	RAW_STATUS_STALLED,
	RAW_STATUS_CRC_ERROR,
	RAW_STATUS_TIMEOUT,

	RAW_STATUS_INVALID_CONFIGURATION,
	RAW_STATUS_INVALID_INTERFACE,
	RAW_STATUS_INVALID_ENDPOINT,
	RAW_STATUS_INVALID_STRING,

	RAW_STATUS_NO_MEMORY
} raw_command_status;


typedef union {
	struct {
		status_t						status;
	} version;

	struct {
		status_t						status;
		usb_device_descriptor			*descriptor;
	} device;

	struct {
		status_t						status;
		usb_configuration_descriptor	*descriptor;
		uint32							config_index;
	} config;

	struct {
		status_t						status;
		usb_interface_descriptor		*descriptor;
		uint32							config_index;
		uint32							interface_index;
	} interface;

	struct {
		status_t						status;
		usb_endpoint_descriptor			*descriptor;
		uint32							config_index;
		uint32							interface_index;
		uint32							endpoint_index;
	} endpoint;

	struct {
		status_t						status;
		usb_string_descriptor			*descriptor;
		uint32							string_index;
		size_t							length;
	} string;

	struct {
		status_t						status;
		usb_descriptor					*descriptor;
		uint32							config_index;
		uint32							interface_index;
		uint32							generic_index;
		size_t							length;
	} generic;

	struct {
		status_t						status;
		uint8							type;
		uint8							index;
		uint16							language_id;
		void							*data;
		size_t							length;
	} descriptor;

	struct {
		status_t						status;
		uint8							request_type;
		uint8							request;
		uint16							value;
		uint16							index;
		uint16							length;
		void							*data;
	} control;

	struct {
		status_t						status;
		uint32							interface;
		uint32							endpoint;
		void							*data;
		size_t							length;
	} transfer;
} raw_command;


typedef struct {
	usb_device			device;
	benaphore			lock;
	uint32				reference_count;

	char				name[32];
	void				*link;

	sem_id				notify;
	status_t			status;
	size_t				actual_length;

	uint8				buffer[B_PAGE_SIZE];
} raw_device;

#endif // _USB_RAW_H_
