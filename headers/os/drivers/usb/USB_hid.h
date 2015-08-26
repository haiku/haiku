/*
 * Copyright 2004-2010, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_HID_H
#define _USB_HID_H


#include <SupportDefs.h>
#include <usb/USB_hid_page_alphanumeric_display.h>
#include <usb/USB_hid_page_arcade.h>
#include <usb/USB_hid_page_bar_code_scanner.h>
#include <usb/USB_hid_page_battery_system.h>
#include <usb/USB_hid_page_consumer.h>
#include <usb/USB_hid_page_digitizers.h>
#include <usb/USB_hid_page_game_controls.h>
#include <usb/USB_hid_page_generic_desktop.h>
#include <usb/USB_hid_page_generic_device_controls.h>
#include <usb/USB_hid_page_keyboard.h>
#include <usb/USB_hid_page_led.h>
#include <usb/USB_hid_page_magnetic_stripe_reader.h>
#include <usb/USB_hid_page_medical_instrument.h>
#include <usb/USB_hid_page_pid.h>
#include <usb/USB_hid_page_power_device.h>
#include <usb/USB_hid_page_simulation.h>
#include <usb/USB_hid_page_telephony.h>
#include <usb/USB_hid_page_usb_monitor.h>
#include <usb/USB_hid_page_vesa_virtual_controls.h>
#include <usb/USB_hid_page_vr_controls.h>
#include <usb/USB_hid_page_weighing_device.h>


/* References:
 *
 *		Device Class Definition for Human Interface Devices	(HID) Ver. 1.11
 *		http://www.usb.org/developers/devclass_docs/hid1_11.pdf
 *
 *		HID Usage Tables Ver. 1.12
 *		http://www.usb.org/developers/devclass_docs/Hut1_12.pdf
 *
 *		Device Class Definition for Physical Interface Deviced (PID) Ver. 1.0
 *		http://www.usb.org/developers/devclass_docs/pid1_01.pdf
 *
 *		Universal Serial Bus Usage Tables for HID Power Devices Ver. 1.0
 *		http://www.usb.org/developers/devclass_docs/pdcv10.pdf
 *
 *		HID Point of Sale Usage Tables Ver. 1.0
 *		http://www.usb.org/developers/devclass_docs/pos1_02.pdf
 *
 *		USB Monitor Control Class Specification, Rev. 1.0
 *		http://www.usb.org/developers/devclass_docs/usbmon10.pdf
 *
 *		Open Arcade Architecture Device (OAAD)
 *		Data Format Specification Rev. 1.100
 *		http://www.usb.org/developers/devclass_docs/oaaddataformatsv6.pdf
 */

#define USB_HID_DEVICE_CLASS 			0x03
#define USB_HID_CLASS_VERSION			0x0100

// HID Interface Subclasses
enum {
	B_USB_HID_INTERFACE_NO_SUBCLASS = 0x00,	//  No Subclass
	B_USB_HID_INTERFACE_BOOT_SUBCLASS			//	Boot Interface Subclass
};

// HID Class-Specific descriptor subtypes
enum {
	B_USB_HID_DESCRIPTOR_HID = 0x21,
	B_USB_HID_DESCRIPTOR_REPORT,
	B_USB_HID_DESCRIPTOR_PHYSICAL
};

// HID Class-specific requests
enum {
	B_USB_REQUEST_HID_GET_REPORT = 0x01,
	B_USB_REQUEST_HID_GET_IDLE,
	B_USB_REQUEST_HID_GET_PROTOCOL,

	B_USB_REQUEST_HID_SET_REPORT = 0x09,
	B_USB_REQUEST_HID_SET_IDLE,
	B_USB_REQUEST_HID_SET_PROTOCOL
};

// HID Class-specific requests report types
enum {
	B_USB_REQUEST_HID_INPUT_REPORT = 0x01,
	B_USB_REQUEST_HID_OUTPUT_REPORT,
	B_USB_REQUEST_HID_FEATURE_REPORT
};

// HID Usage Pages
enum {
	B_HID_USAGE_PAGE_GENERIC_DESKTOP = 0x1,
	B_HID_USAGE_PAGE_SIMULATION,
	B_HID_USAGE_PAGE_VR,
	B_HID_USAGE_PAGE_SPORT,
	B_HID_USAGE_PAGE_GAME,
	B_HID_USAGE_PAGE_GENERIC,
	B_HID_USAGE_PAGE_KEYBOARD,
	B_HID_USAGE_PAGE_LED,
	B_HID_USAGE_PAGE_BUTTON,
	B_HID_USAGE_PAGE_ORDINAL,
	B_HID_USAGE_PAGE_TELEPHONY,
	B_HID_USAGE_PAGE_CONSUMER,
	B_HID_USAGE_PAGE_DIGITIZER,
	
	B_HID_USAGE_PAGE_PID = 0xf,
	B_HID_USAGE_PAGE_UNICODE,
	B_HID_USAGE_PAGE_ALPHANUM_DISPLAY = 0x14,
	B_HID_USAGE_PAGE_MEDICAL = 0x40,

	B_HID_USAGE_PAGE_USB_MONITOR = 0x80,  	// alt. B_HID_USAGE_PAGE_MONITOR_0,
	B_HID_USAGE_PAGE_USB_ENUMERATED_VALUES,	// alt. B_HID_USAGE_PAGE_MONITOR_1,
	B_HID_USAGE_PAGE_VESA_VIRTUAL_CONTROLS,	// alt. B_HID_USAGE_PAGE_MONITOR_2,
	B_HID_USAGE_PAGE_MONITOR_3,
	
	B_HID_USAGE_PAGE_POWER_DEVICE = 0x84,	// alt. B_HID_USAGE_PAGE_POWER_0,
	B_HID_USAGE_PAGE_BATTERY_SYSTEM,		// alt. B_HID_USAGE_PAGE_POWER_1,
	B_HID_USAGE_PAGE_POWER_2,
	B_HID_USAGE_PAGE_POWER_3,
	
	B_HID_USAGE_PAGE_BAR_CODE_SCANNER = 0x8c,
	B_HID_USAGE_PAGE_WEIGHING_DEVICES,		// alt. B_HID_USAGE_PAGE_SCALE,
	B_HID_USAGE_PAGE_MAGNETIC_STRIPE_READER,
	B_HID_USAGE_PAGE_RESERVED_POS_PAGE,
	B_HID_USAGE_PAGE_CAMERA_CONTROL,
	B_HID_USAGE_PAGE_ARCADE,
	
	B_HID_USAGE_PAGE_MICROSOFT = 0xff00
};

typedef struct {
	uint8	length;
	uint8	descriptor_type;
	uint16	hid_version;
	uint8	country_code;
	uint8	num_descriptors;
	struct {
		uint8	descriptor_type;
		uint16	descriptor_length;
	} _PACKED descriptor_info[1];
} _PACKED usb_hid_descriptor;


#endif	// _USB_HID_H
