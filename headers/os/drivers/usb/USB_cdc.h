#ifndef USB_CDC_H
#define USB_CDC_H

// (Partial) USB Class Definitions for Communication Devices (CDC), version 1.1
// Reference: http://www.usb.org/developers/devclass_docs/usbcdc11.pdf

#define USB_COMMUNICATION_DEVICE_CLASS 			0x02

#define USB_CDC_COMMUNICATION_INTERFACE_CLASS	0x02
#define USB_CDC_CLASS_VERSION					0x0101

enum { // Communication Interface Subclasses
	USB_CDC_COMMUNICATION_INTERFACE_DLCM_SUBCLASS =	0x01,	// Direct Line Control Model
	USB_CDC_COMMUNICATION_INTERFACE_ACM_SUBCLASS,			// Abstract Control Model
	USB_CDC_COMMUNICATION_INTERFACE_TCM_SUBCLASS,			// Telephone Control Model
	USB_CDC_COMMUNICATION_INTERFACE_MCCM_SUBCLASS,			// Multi-Channel Control Model
	USB_CDC_COMMUNICATION_INTERFACE_CAPICM_SUBCLASS,    	// CAPI Control Model
	USB_CDC_COMMUNICATION_INTERFACE_ENCM_SUBCLASS,			// Ethernet Networking Control Model
	USB_CDC_COMMUNICATION_INTERFACE_ATMNCM_SUBCLASS			// ATM Networking Control Model
};

enum { // Communication Interface Class Control Protocols
	USB_CDC_COMMUNICATION_INTERFACE_NONE_PROTOCOL = 0x00,		// No class specific protocol required
	USB_CDC_COMMUNICATION_INTERFACE_V25TER_PROTOCOL = 0x01,		// Common AT commands (also knowns as "Hayes compatible")
	USB_CDC_COMMUNICATION_INTERFACE_SPECIFIC_PROTOCOL = 0xff	// Vendor-specific protocol
};


#define USB_CDC_DATA_INTERFACE_CLASS			0x0a

// Functional Descriptors (p32)

enum { // Functional Descriptors Subtypes
	USB_CDC_HEADER_FUNCTIONAL_DESCRIPTOR = 0x00,
	USB_CDC_CM_FUNCTIONAL_DESCRIPTOR,
	USB_CDC_ACM_FUNCTIONAL_DESCRIPTOR,
	USB_CDC_UNION_FUNCTIONAL_DESCRIPTOR = 0x06
};

typedef struct usb_cdc_header_function_descriptor {
	uint8	length;
	uint8	descriptor_type;
	uint8	descriptor_subtype;	// USB_CDC_HEADER_FUNCTIONAL_DESCRIPTOR
	uint16	cdc_version;
} _PACKED usb_cdc_header_function_descriptor;

typedef struct usb_cdc_cm_functional_descriptor {
	uint8	length;
	uint8	descriptor_type;
	uint8	descriptor_subtype;	// USB_CDC_CM_FUNCTIONAL_DESCRIPTOR
	uint8	capabilities;
	uint8	data_interface;
} _PACKED usb_cdc_cm_functional_descriptor;

enum { // Call Management Functional Descriptor capabilities bitmap
	USB_CDC_CM_DOES_CALL_MANAGEMENT = 0x01,
	USB_CDC_CM_OVER_DATA_INTERFACE	= 0x02
};

typedef struct usb_cdc_acm_functional_descriptor {
	uint8	length;
	uint8	descriptor_type;
	uint8	descriptor_subtype;	// USB_CDC_ACM_FUNCTIONAL_DESCRIPTOR
	uint8	capabilities;
} _PACKED usb_cdc_acm_functional_descriptor;

enum { // Abstract Control Model Functional Descriptor capabilities bitmap (p36)
	USB_CDC_ACM_HAS_COMM_FEATURES 		= 0x01,
	USB_CDC_ACM_HAS_LINE_CONTROL		= 0x02,
	USB_CDC_ACM_HAS_SEND_BREAK			= 0x04,
	USB_CDC_ACM_HAS_NETWORK_CONNECTION	= 0x08
};

typedef struct usb_cdc_union_functional_descriptor {
	uint8	length;
	uint8	descriptor_type;
	uint8	descriptor_subtype;	// USB_CDC_UNION_FUNCTIONAL_DESCRIPTOR
	uint8	master_interface;
	uint8	slave_interfaces[1];
} _PACKED usb_cdc_union_functional_descriptor;


enum { // Management Element Requests (p62)
	USB_CDC_SEND_ENCAPSULATED_COMMAND		= 0x00,
	USB_CDC_GET_ENCAPSULATED_RESPONSE,
	USB_CDC_SET_COMM_FEATURE,
	USB_CDC_GET_COMM_FEATURE,
	USB_CDC_CLEAR_COMM_FEATURE,
	// 0x05 -> 0x0F: reserved for future use
	USB_CDC_SET_AUX_LINE_STATE				= 0x10,
	USB_CDC_SET_HOOK_STATE,
	USB_CDC_PULSE_SETUP,
	USB_CDC_SEND_PULSE,
	USB_CDC_SET_PULSE_TIME,
	USB_CDC_RING_AUX_JACK,
	// 0x16 -> 0x1F: reserved for future use
	USB_CDC_SET_LINE_CODING					= 0x20,
	USB_CDC_GET_LINE_CODING,
	USB_CDC_SET_CONTROL_LINE_STATE,
	USB_CDC_SEND_BREAK,
	// 0x24 -> 0x2F: reserved for future use
	USB_CDC_SET_RINGER_PARMS				= 0x30,
	USB_CDC_GET_RINGER_PARMS,
	USB_CDC_SET_OPERATION_PARMS,
	USB_CDC_GET_OPERATION_PARMS,
	USB_CDC_SET_LINE_PARMS,
	USB_CDC_GET_LINE_PARMS,
	USB_CDC_DIAL_DIGITS,
	USB_CDC_SET_UNIT_PARAMETER,
	USB_CDC_GET_UNIT_PARAMETER,
	USB_CDC_CLEAR_UNIT_PARAMETER,
	USB_CDC_GET_PROFILE,
	// 0x3B -> 0x3F: reserved for future use
	USB_CDC_SET_ETHERNET_MULTICAST_FILTERS	= 0x40,
	USB_CDC_SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER,
	USB_CDC_GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER,
	USB_CDC_SET_ETHERNET_PACKET_FILTER,
	USB_CDC_GET_ETHERNET_STATISTIC,
	// 0x45 -> 0x4F: reserved for future use
	USB_CDC_SET_ATM_DATA_FORMAT				= 0x50,
	USB_CDC_GET_ATM_DEVICE_STATISTICS,
	USB_CDC_SET_ATM_DEFAULT_VC,
	USB_CDC_GET_ATM_VC_STATISTICS
	// 0x54 -> 0xFF: reserved for future use
};

enum { // Feature Selector (for USB_CDC_SET/GET/CLEAR_COMM_FEATURE (p65-66))
	USB_CDC_ABSTRACT_STATE	= 0x01,
	USB_CDC_COUNTRY_SETTING	= 0x02
};

enum { // ABSTRACT_STATE bitmap (for USB_CDC_ABSTRAT_STATE Feature Selector (p66))
	USB_CDC_ABSTRACT_STATE_IDLE				= 0x01,
	USB_CDC_ABSTRACT_STATE_DATA_MULTIPLEXED = 0x02
};

typedef struct {	// Line Coding Structure (for USB_CDC_SET/GET_LINE_CODING (p69))
	uint32 	speed;
	uint8 	stopbits;
	uint8 	parity;
	uint8 	databits;
} _PACKED usb_cdc_line_coding;

enum { // CDC stopbits values (for Line Coding Structure stopbits field)
	USB_CDC_LINE_CODING_1_STOPBIT	= 0,
	USB_CDC_LINE_CODING_1_5_STOPBITS,
	USB_CDC_LINE_CODING_2_STOPBITS
};

enum { // CDC parity values (for Line Coding Structure parity field)
	USB_CDC_LINE_CODING_NO_PARITY	= 0,
	USB_CDC_LINE_CODING_ODD_PARITY,
	USB_CDC_LINE_CODING_EVEN_PARITY,
	USB_CDC_LINE_CODING_MARK_PARITY,
	USB_CDC_LINE_CODING_SPACE_PARITY
};

enum { // CDC Control Signal bitmap (for CDC_SET_CONTROL_LINE_STATE request)
	USB_CDC_CONTROL_SIGNAL_STATE_DTR		= 0x01,
	USB_CDC_CONTROL_SIGNAL_STATE_RTS		= 0x02
};

enum {	// Notification Elements notifications (p84)
	USB_CDC_NETWORK_CONNECTION					= 0x00,
	USB_CDC_RESPONSE_AVAILABLE,
	// 0x02 -> 0x07: reserved for future use
	USB_CDC_AUX_JACK_HOOK_STATE					= 0x08,
	USB_CDC_RING_DETECT,
	// 0x0a -> 0x1f: reserved for future use
	USB_CDC_SERIAL_STATE						= 0x20,
	// 0x21 -> 0x27: reserved for future use
	USB_CDC_CALL_STATE_CHANGE					= 0x28,
	USB_CDC_LINE_STATE_CHANGE,
	USB_CDC_CONNECTION_SPEED_CHANGE
	// 0x2b -> 0xff: reserved for future use
};

enum { // CDC UART State bitmap (for USB_CDC_SERIAL_STATE notification)
	USB_CDC_UART_STATE_DCD				= 0x01,
	USB_CDC_UART_STATE_DSR				= 0x02,
	USB_CDC_UART_STATE_BREAK			= 0x04,
	USB_CDC_UART_STATE_RING				= 0x08,
	USB_CDC_UART_STATE_FRAMING_ERROR	= 0x10,
	USB_CDC_UART_STATE_PARITY_ERROR		= 0x20,
	USB_CDC_UART_STATE_OVERRUN_ERROR	= 0x40
};

#endif	// USB_CDC_H

