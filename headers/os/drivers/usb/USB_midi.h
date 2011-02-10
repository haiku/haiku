#ifndef USB_MIDI_H
#define USB_MIDI_H

#include <usb/USB_audio.h>

// (Partial) USB Class Definitions for MIDI Devices, version 1.0
// Reference: http://www.usb.org/developers/devclass_docs/midi10.pdf

#define USB_MIDI_CLASS_VERSION		0x0100	// Class specification version 1.0

// USB MIDI Event Packet

// ... as clean structure:
typedef struct {	// USB MIDI Event Packet
	uint8	cin:4;	// Code Index Number
	uint8	cn:4;	// Cable Number
	uint8	midi[3];
} _PACKED usb_midi_event_packet;


// MIDIStreaming (ms) interface descriptors (p20)

enum { // MIDI Streaming descriptors subtypes
	USB_MS_HEADER_DESCRIPTOR = 0x01,
	USB_MS_MIDI_IN_JACK_DESCRIPTOR,
	USB_MS_MIDI_OUT_JACK_DESCRIPTOR,
	USB_MS_ELEMENT_DESCRIPTOR
};

typedef struct usb_midi_interface_header_descriptor {
	uint8	length;
	uint8	descriptor_type;
	uint8	descriptor_subtype;	// USB_MS_HEADER_DESCRIPTOR
	uint16	ms_version;
	uint16	total_length;
} _PACKED usb_midi_interface_header_descriptor;


enum {
	USB_MIDI_EMBEDDED_JACK = 0x01,
	USB_MIDI_EXTERNAL_JACK
};

typedef struct usb_midi_in_jack_descriptor {
	uint8	length;
	uint8	descriptor_type;
	uint8	descriptor_subtype;	// USB_MS_MIDI_IN_JACK_DESCRIPTOR
	uint8	type;	// USB_MIDI_{EMBEDDED | EXTERNAL}_JACK
	uint8	id;
	uint8	string_descriptor;
} _PACKED usb_ms_midi_in_jack_descriptor;


typedef struct usb_midi_source {
	uint8	source_id;
	uint8	source_pin;
} _PACKED usb_midi_source;

typedef struct usb_midi_out_jack_descriptor {
	uint8	length;
	uint8	descriptor_type;
	uint8	descriptor_subtype;	// USB_MS_MIDI_OUT_JACK_DESCRIPTOR
	uint8	type;	// USB_MIDI_{EMBEDDED | EXTERNAL}_JACK
	uint8	id;
	uint8	inputs_count;
	usb_midi_source input_source[0];	// inputs_count times
	// uint8	string_descriptor;
} _PACKED usb_midi_out_jack_descriptor;


enum { // USB Element Capabilities bitmap (p23,25)
	USB_MS_ELEMENT_CUSTOM_UNDEFINED		= 0x0001,
	USB_MS_ELEMENT_MIDI_CLOCK			= 0x0002,
	USB_MS_ELEMENT_MIDI_TIME_CODE		= 0x0004,
	USB_MS_ELEMENT_MTC	= USB_MS_ELEMENT_MIDI_TIME_CODE,
	USB_MS_ELEMENT_MIDI_MACHINE_CONTROL	= 0x0008,
	USB_MS_ELEMENT_MMC	= USB_MS_ELEMENT_MIDI_MACHINE_CONTROL,
	// General MIDI System Level 1 compatible
	USB_MS_ELEMENT_GM1					= 0x0010,
	// General MIDI System Level 2 compatible
	USB_MS_ELEMENT_GM2					= 0x0020,
	// GS Format compatible (Roland)
	USB_MS_ELEMENT_GS					= 0x0040,
	// XG compatible (Yamaha)
	USB_MS_ELEMENT_XG					= 0x0080,
	USB_MS_ELEMENT_EFX					= 0x0100,
	// internal MIDI Patcher or Router
	USB_MS_ELEMENT_MIDI_PATCH_BAY		= 0x0200,
	// Downloadable Sounds Standards Level 1 compatible
	USB_MS_ELEMENT_DLS1					= 0x0400,
	// Downloadable Sounds Standards Level 2 compatible
	USB_MS_ELEMENT_DLS2					= 0x0800
};

typedef struct usb_midi_element_descriptor {
	uint8	length;
	uint8	descriptor_type;
	uint8	descriptor_subtype;	// USB_MS_ELEMENT_DESCRIPTOR
	uint8	id;
	uint8	inputs_count;
	usb_midi_source input_source[0];	// inputs_count times
	// uint8	outputs_count;
	// uint8	input_terminal_id;
	// uint8	output_terminal_id;
	// uint8	capabilities_size;
	// uint8	capabilities[0];	// capabilities_size bytes
	// uint8	string_descriptor;	// see USB_MS_ELEMENT_* enum above
} _PACKED usb_midi_element_descriptor;

// Class-Specific MIDIStream Bulk Data Endpoint descriptor (p26)

#define USB_MS_GENERAL_DESCRIPTOR	0x01

typedef struct usb_midi_endpoint_descriptor {
	uint8	length;
	uint8	descriptor_type;	// USB_DESCRIPTOR_CS_ENDPOINT
	uint8	descriptor_subtype;	// USB_MS_GENERAL_DESCRIPTOR
	uint8	jacks_count;
	uint8	jacks_id[0];		// jacks_count times
} _PACKED usb_midi_endpoint_descriptor;


#endif	// USB_MIDI_H
