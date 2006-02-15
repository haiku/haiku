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

#endif	// USB_MIDI_H
