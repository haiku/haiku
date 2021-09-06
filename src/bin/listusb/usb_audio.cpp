/*
 * Originally released under the Be Sample Code License.
 * Copyright 2000, Be Incorporated. All rights reserved.
 *
 * Modified for Haiku by François Revol and Michael Lotz.
 * Copyright 2007-2016, Haiku Inc. All rights reserved.
 */

#include <MediaDefs.h>
#include <USBKit.h>
#include <stdio.h>

#include <usb/USB_audio.h>
#include <usb/USB_midi.h>

#include "listusb.h"

void
DumpAudioCSInterfaceDescriptorHeader(
	const usb_audiocontrol_header_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Header)\n",
		descriptor->descriptor_subtype);
	printf("                    ADC Release ....... %d.%d\n",
		descriptor->bcd_release_no >> 8, descriptor->bcd_release_no & 0xFF);
	printf("                    Total Length ...... %u\n",
		descriptor->r1.total_length);
	printf("                    Interfaces ........ ");

	for (uint8 i = 0; i < descriptor->r1.in_collection; i++)
		printf("%u, ", descriptor->r1.interface_numbers[i]);
	printf("\n");
}


void
DumpChannelConfig(uint32 wChannelConfig)
{
	struct _Entry {
		const char* name;
		uint32 mask;
	} aClusters[] = {
		{ "Front .......... ", B_CHANNEL_LEFT | B_CHANNEL_RIGHT
			| B_CHANNEL_CENTER },
		{ "L.F.E .......... ", B_CHANNEL_SUB },
		{ "Back ........... ", B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT
			| B_CHANNEL_BACK_CENTER },
		{ "Center ......... ", B_CHANNEL_FRONT_LEFT_CENTER
			| B_CHANNEL_FRONT_RIGHT_CENTER },
		{ "Side ........... ", B_CHANNEL_SIDE_LEFT | B_CHANNEL_SIDE_RIGHT },
		{ "Top ............ ", B_CHANNEL_TOP_CENTER },
		{ "Top Front ...... ", B_CHANNEL_TOP_FRONT_LEFT
			| B_CHANNEL_TOP_FRONT_CENTER | B_CHANNEL_TOP_FRONT_RIGHT },
		{ "Top Back ....... ", B_CHANNEL_TOP_BACK_LEFT
			| B_CHANNEL_TOP_BACK_CENTER | B_CHANNEL_TOP_BACK_RIGHT }
	};

	struct _Entry aChannels[] = {
		{ "Left", B_CHANNEL_LEFT | B_CHANNEL_FRONT_LEFT_CENTER
			| B_CHANNEL_REARLEFT | B_CHANNEL_SIDE_LEFT | B_CHANNEL_TOP_BACK_LEFT
			| B_CHANNEL_TOP_FRONT_LEFT },
		{ "Right", B_CHANNEL_FRONT_RIGHT_CENTER | B_CHANNEL_TOP_FRONT_RIGHT
			| B_CHANNEL_REARRIGHT | B_CHANNEL_RIGHT | B_CHANNEL_SIDE_RIGHT
			| B_CHANNEL_TOP_BACK_RIGHT },
		{ "Center", B_CHANNEL_BACK_CENTER | B_CHANNEL_CENTER
			| B_CHANNEL_TOP_BACK_CENTER | B_CHANNEL_TOP_CENTER
			| B_CHANNEL_TOP_FRONT_CENTER },
		{ "L.F.E.", B_CHANNEL_SUB }
	};

	for (size_t i = 0; i < sizeof(aClusters) / sizeof(aClusters[0]); i++) {
		uint32 mask = aClusters[i].mask & wChannelConfig;
		if (mask != 0) {
			printf("                       %s", aClusters[i].name);
			for (size_t j = 0; j < sizeof(aChannels) / sizeof(aChannels[0]); j++)
				if ((aChannels[j].mask & mask) != 0)
					printf("%s ", aChannels[j].name);
			printf("\n");
		}
	}
}


static const char*
TerminalTypeName(uint16 terminalType)
{
	switch (terminalType) {
		case USB_AUDIO_UNDEFINED_USB_IO:
			return "USB Undefined";
		case USB_AUDIO_STREAMING_USB_IO:
			return "USB Streaming";
		case USB_AUDIO_VENDOR_USB_IO:
			return "USB vendor specific";

		case USB_AUDIO_UNDEFINED_IN:
			return "Undefined";
		case USB_AUDIO_MICROPHONE_IN:
			return "Microphone";
		case USB_AUDIO_DESKTOPMIC_IN:
			return "Desktop microphone";
		case USB_AUDIO_PERSONALMIC_IN:
			return "Personal microphone";
		case USB_AUDIO_OMNI_MIC_IN:
			return "Omni-directional microphone";
		case USB_AUDIO_MICS_ARRAY_IN:
			return "Microphone array";
		case USB_AUDIO_PROC_MICS_ARRAY_IN:
			return "Processing microphone array";
		case USB_AUDIO_LINE_CONNECTOR_IO:
			return "Line I/O";
		case USB_AUDIO_SPDIF_INTERFACE_IO:
			return "S/PDIF";

		case USB_AUDIO_UNDEFINED_OUT:
			return "Undefined";
		case USB_AUDIO_SPEAKER_OUT:
			return "Speaker";
		case USB_AUDIO_HEAD_PHONES_OUT:
			return "Headphones";
		case USB_AUDIO_HMD_AUDIO_OUT:
			return "Head Mounted Display Audio";
		case USB_AUDIO_DESKTOP_SPEAKER:
			return "Desktop speaker";
		case USB_AUDIO_ROOM_SPEAKER:
			return "Room speaker";
		case USB_AUDIO_COMM_SPEAKER:
			return "Communication speaker";
		case USB_AUDIO_LFE_SPEAKER:
			return "Low frequency effects speaker";

		default:
			return "Unknown";
	}
}


void
DumpAudioCSInterfaceDescriptorInputTerminal(
	const usb_audio_input_terminal_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Input Terminal)\n",
		descriptor->descriptor_subtype);
	printf("                    Terminal ID ....... %u\n",
		descriptor->terminal_id);
	printf("                    Terminal Type ..... 0x%04x (%s)\n",
		descriptor->terminal_type,
			TerminalTypeName(descriptor->terminal_type));
	printf("                    Associated Terminal %u\n",
		descriptor->assoc_terminal);
	printf("                    Nr Channels ....... %u\n",
		descriptor->r1.num_channels);
	printf("                    Channel Config .... 0x%x\n",
		descriptor->r1.channel_config);
	DumpChannelConfig(descriptor->r1.channel_config);

	printf("                    Channel Names ..... %u\n",
		descriptor->r1.channel_names);
	printf("                    Terminal .......... %u\n",
		descriptor->r1.terminal);
}


void
DumpAudioCSInterfaceDescriptorOutputTerminal(
	const usb_audio_output_terminal_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Output Terminal)\n",
		descriptor->descriptor_subtype);
	printf("                    Terminal ID ....... %u\n",
		descriptor->terminal_id);
	printf("                    Terminal Type ..... 0x%04x (%s)\n",
		descriptor->terminal_type,
			TerminalTypeName(descriptor->terminal_type));
	printf("                    Associated Terminal %u\n",
		descriptor->assoc_terminal);
	printf("                    Source ID ......... %u\n",
		descriptor->source_id);
	printf("                    Terminal .......... %u\n",
		descriptor->r1.terminal);
}


void
DumpAudioCSInterfaceDescriptorMixerUnit(
	const usb_audio_mixer_unit_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Mixer Unit)\n",
		descriptor->descriptor_subtype);
	printf("                    Unit ID ........... %u\n",
		descriptor->unit_id);

	printf("                    Source IDs ........ ");
	for (uint8 i = 0; i < descriptor->num_input_pins; i++)
		printf("%u, ", descriptor->input_pins[i]);
	printf("\n");

	usb_audio_output_channels_descriptor_r1* channels
		= (usb_audio_output_channels_descriptor_r1*)
			&descriptor->input_pins[descriptor->num_input_pins];

	printf("                    Channels .......... %u\n",
		channels->num_output_pins);
	printf("                    Channel Config .... 0x%x\n",
			channels->channel_config);
	DumpChannelConfig(channels->channel_config);
	printf("                    Channel Names ..... %u\n",
		channels->channel_names);

	usb_generic_descriptor* generic = (usb_generic_descriptor*)descriptor;
	uint8 idx = 7 + descriptor->num_input_pins;
		printf("                    Bitmap Control .... 0x");
	for (uint i = 1; idx < descriptor->length - 3; idx++, i++)
		printf("%02x ", (uint8)generic->data[idx]);
	printf("\n");

	printf("                    Mixer ............. %u\n",
			generic->data[generic->length - 3]);
}


void
DumpAudioCSInterfaceDescriptorSelectorUnit(
	const usb_audio_selector_unit_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Selector Unit)\n",
		descriptor->descriptor_subtype);
	printf("                    Unit ID ........... %u\n",
		descriptor->unit_id);

	printf("                    Source IDs ........ ");
	for (uint8 i = 0; i < descriptor->num_input_pins; i++)
		printf("%u, ", descriptor->input_pins[i]);
	printf("\n");

	usb_generic_descriptor* generic = (usb_generic_descriptor*)descriptor;
	printf("                    Selector .......... %u\n",
		(uint8)generic->data[descriptor->num_input_pins + 2]);
}


void
DumpBMAControl(uint8 channel, uint32 bma)
{
	const char* BMAControls[] = {
		"Mute",
		"Volume",
		"Bass",
		"Mid",
		"Treble",
		"Graphic Equalizer",
		"Automatic Gain",
		"Delay",
		"Bass Boost",
		"Loudness"
	};

	if (bma == 0)
		return;

	if (channel == 0)
		printf("                       Master Channel . ");
	else
		printf("                       Channel %u ...... ", channel);

	int mask = 1;
	for (uint8 i = 0;
			i < sizeof(BMAControls) / sizeof(BMAControls[0]); i++, mask <<= 1)
		if (bma & mask)
			printf("%s ", BMAControls[i]);
	printf("\n");
}


void
DumpAudioCSInterfaceDescriptorFeatureUnit(
	const usb_audio_feature_unit_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Feature Unit)\n",
		descriptor->descriptor_subtype);
	printf("                    Unit ID ........... %u\n",
			descriptor->unit_id);
	printf("                    Source ID ......... %u\n",
			descriptor->source_id);

	printf("                    Control Size ...... %u\n",
			descriptor->r1.control_size);


	uint8 channels = (descriptor->length - 6) / descriptor->r1.control_size;
	for (uint8 i = 0; i < channels; i++) {
		switch (descriptor->r1.control_size) {
			case 1:
				DumpBMAControl(i, descriptor->r1.bma_controls[i]);
				break;
			case 2:
				DumpBMAControl(i, *(uint16*)&descriptor->r1.bma_controls[i * 2]);
				break;
			case 4:
				DumpBMAControl(i, *(uint32*)&descriptor->r1.bma_controls[i * 4]);
				break;
			default:
				printf("                    BMA Channel %u ... ", i);
				for (uint8 j = 0; j < descriptor->r1.control_size; j++)
					printf("%02x ", descriptor->r1.bma_controls[i + j]);
				break;
		}
	}

	usb_generic_descriptor* generic = (usb_generic_descriptor*)descriptor;
	printf("                    Feature ........... %u\n",
			(uint8)generic->data[descriptor->length - 3]);
}


void
DumpAudioCSInterfaceDescriptorAssociated(
	const usb_generic_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Associate Interface)\n",
		(uint8)descriptor->data[0]);
	printf("                    Interface ......... %u\n",
		(uint8)descriptor->data[1]);

	printf("                    Data .............. ");
	for (uint8 i = 0; i < descriptor->length - 2; i++)
		printf("%02x ", descriptor->data[i]);
	printf("\n");
}


void
DumpAudioControlCSInterfaceDescriptor(const usb_generic_descriptor* descriptor)
{
	uint8 descriptorSubtype = descriptor->data[0];
	switch (descriptorSubtype) {
		case USB_AUDIO_AC_HEADER:
			DumpAudioCSInterfaceDescriptorHeader(
				(usb_audiocontrol_header_descriptor*)descriptor);
			break;
		case USB_AUDIO_AC_INPUT_TERMINAL:
			DumpAudioCSInterfaceDescriptorInputTerminal(
				(usb_audio_input_terminal_descriptor*)descriptor);
			break;
		case USB_AUDIO_AC_OUTPUT_TERMINAL:
			DumpAudioCSInterfaceDescriptorOutputTerminal(
				(usb_audio_output_terminal_descriptor*)descriptor);
			break;
		case USB_AUDIO_AC_MIXER_UNIT:
			DumpAudioCSInterfaceDescriptorMixerUnit(
				(usb_audio_mixer_unit_descriptor*)descriptor);
			break;
		case USB_AUDIO_AC_SELECTOR_UNIT:
			DumpAudioCSInterfaceDescriptorSelectorUnit(
				(usb_audio_selector_unit_descriptor*)descriptor);
			break;
		case USB_AUDIO_AC_FEATURE_UNIT:
			DumpAudioCSInterfaceDescriptorFeatureUnit(
				(usb_audio_feature_unit_descriptor*)descriptor);
			break;
		case USB_AUDIO_AC_EXTENSION_UNIT:
			DumpAudioCSInterfaceDescriptorAssociated(descriptor);
			break;
		default:
			DumpDescriptorData(descriptor);
	}
}


void
DumpGeneralASInterfaceDescriptor(
	const usb_audio_streaming_interface_descriptor* descriptor)
{
	printf("                    Subtype ........... %u (AS_GENERAL)\n",
		descriptor->descriptor_subtype);
	printf("                    Terminal link ..... %u\n",
		descriptor->terminal_link);
	printf("                    Delay ............. %u\n",
		descriptor->r1.delay);
	printf("                    Format tag ........ %u\n",
		descriptor->r1.format_tag);
}


uint32
GetSamplingFrequency(const usb_audio_sampling_freq& freq)
{
	return freq.bytes[0] | freq.bytes[1] << 8 | freq.bytes[2] << 16;
}


void
DumpSamplingFrequencies(uint8 type, const usb_audio_sampling_freq* freqs)
{
	if (type > 0) {
		printf("                    Sampling Freq ..... ");
		for (uint8 i = 0; i < type; i++)
			printf("%" B_PRIu32 ", ", GetSamplingFrequency(freqs[i]));
		printf("\n");
	} else {
		printf("                    Sampling Freq ..... %" B_PRIu32 " to %"
			B_PRIu32 "\n", GetSamplingFrequency(freqs[0]),
			GetSamplingFrequency(freqs[1]));
	}
}


void
DumpASFormatTypeI(const usb_audio_format_descriptor* descriptor)
{
	printf("                    Subtype ........... %u (FORMAT_TYPE)\n",
		descriptor->descriptor_subtype);
	printf("                    Format Type ....... %u (FORMAT_TYPE_I)\n",
		descriptor->format_type);
	printf("                    Channels .......... %u\n",
		descriptor->typeI.nr_channels);
	printf("                    Subframe size ..... %u\n",
		descriptor->typeI.subframe_size);
	printf("                    Bit resolution .... %u\n",
		descriptor->typeI.bit_resolution);

	DumpSamplingFrequencies(descriptor->typeI.sam_freq_type,
			descriptor->typeI.sam_freqs);
}


void
DumpASFormatTypeIII(const usb_audio_format_descriptor* descriptor)
{
	printf("                    Subtype ........... %u (FORMAT_TYPE)\n",
		descriptor->descriptor_subtype);
	printf("                    Format Type ....... %u (FORMAT_TYPE_III)\n",
		descriptor->format_type);
	printf("                    Channels .......... %u\n",
		descriptor->typeIII.nr_channels);
	printf("                    Subframe size ..... %u\n",
		descriptor->typeIII.subframe_size);
	printf("                    Bit resolution .... %u\n",
		descriptor->typeIII.bit_resolution);

	DumpSamplingFrequencies(descriptor->typeIII.sam_freq_type,
			descriptor->typeIII.sam_freqs);
}


void
DumpASFormatTypeII(const usb_audio_format_descriptor* descriptor)
{
	printf("                    Subtype ........... %u (FORMAT_TYPE)\n",
		descriptor->descriptor_subtype);
	printf("                    Format Type ....... %u (FORMAT_TYPE_II)\n",
		descriptor->format_type);
	printf("                    Max Bitrate ....... %u\n",
		descriptor->typeII.max_bit_rate);
	printf("                    Samples per Frame . %u\n",
		descriptor->typeII.samples_per_frame);

	DumpSamplingFrequencies(descriptor->typeII.sam_freq_type,
			descriptor->typeII.sam_freqs);
}


void
DumpASFmtType(const usb_audio_format_descriptor* descriptor)
{
	uint8 format = descriptor->format_type;
	switch (format) {
		case USB_AUDIO_FORMAT_TYPE_I:
			DumpASFormatTypeI(descriptor);
			break;
		case USB_AUDIO_FORMAT_TYPE_II:
			DumpASFormatTypeII(descriptor);
			break;
		case USB_AUDIO_FORMAT_TYPE_III:
			DumpASFormatTypeIII(descriptor);
			break;
		default:
			DumpDescriptorData((usb_generic_descriptor*)descriptor);
			break;
	}
}


void
DumpMPEGCapabilities(uint16 capabilities)
{
	const char* MPEGCapabilities[] = {
		"Layer I",
		"Layer II",
		"Layer III",

		"MPEG-1 only",
		"MPEG-1 dual-channel",
		"MPEG-2 second stereo",
		"MPEG-2 7.1 channel augumentation",
		"Adaptive multi-channel predicion"
	};

	uint16 mask = 1;
	for (uint8 i = 0;
			i < sizeof(MPEGCapabilities) / sizeof(MPEGCapabilities[0]); i++) {
		if (capabilities & mask)
			printf("                         %s\n", MPEGCapabilities[i]);
		mask <<= 1;
	}

	mask = 0x300; // bits 8 and 9
	uint16 multilingualSupport = (capabilities & mask) >> 8;
	switch (multilingualSupport) {
		case 0:
			printf("                         No Multilingual support\n");
			break;
		case 1:
			printf("                         Supported at Fs\n");
			break;
		case 3:
			printf("                         Supported at Fs and 1/2Fs\n");
			break;
		default:
			break;
	}
}


void
DumpMPEGFeatures(uint8 features)
{
	uint8 mask = 0x30; // bits 4 and 5
	uint8 dynRangeControl = (features & mask) >> 4;
	switch (dynRangeControl) {
		case 0:
			printf("                         Not supported\n");
			break;
		case 1:
			printf("                         Supported, not scalable\n");
			break;
		case 2:
			printf("                         Scalable, common boost, "
				"cut scaling value\n");
			break;
		case 3:
			printf("                         Scalable, separate boost, "
				"cut scaling value\n");
		default:
			break;
	}
}


void
DumpASFmtSpecificMPEG(const usb_generic_descriptor* descriptor)
{
	printf("                    Subtype ........... %u (FORMAT_SPECIFIC)\n",
		descriptor->data[0]);
	printf("                    Format Tag ........ %u\n",
		*(uint16*)&descriptor->data[1]);
	printf("                    MPEG Capabilities . %u\n",
		*(uint16*)&descriptor->data[3]);
	DumpMPEGCapabilities(*(uint16*)&descriptor->data[3]);
	printf("                    MPEG Features ..... %u\n",
		descriptor->data[5]);
	DumpMPEGFeatures(descriptor->data[5]);
}


void
DumpAC_3Features(uint8 features)
{
	const char* featuresStr[] = {
		"RF mode",
		"Line mode",
		"Custom0 mode",
		"Custom1 mode"
	};

	uint8 mask = 1;
	for (uint8 i = 0; i < sizeof(featuresStr) / sizeof(const char*); i++) {
		if (features & mask)
			printf("                         %s\n", featuresStr[i]);
		mask <<= 1;
	}

	mask = 0x30; // bits 4 and 5
	uint8 dynRangeControl = (features & mask) >> 4;
	switch (dynRangeControl) {
		case 0:
			printf("                         Not supported\n");
			break;
		case 1:
			printf("                         Supported, not scalable\n");
			break;
		case 2:
			printf("                         Scalable, common boost, "
				"cut scaling value\n");
			break;
		case 3:
			printf("                         Scalable, separate boost, "
				"cut scaling value\n");
		default:
			break;
	}
}


void
DumpASFmtSpecificAC_3(const usb_generic_descriptor* descriptor)
{
	printf("                    Subtype ........... %u (FORMAT_TYPE)\n",
		descriptor->data[0]);
	printf("                    Format Tag ........ %u\n",
		*(uint16*)&descriptor->data[1]);
	printf("                    BSID .............. %" B_PRIx32 "\n",
		*(uint32*)&descriptor->data[2]);
	printf("                    AC3 Features ...... %u\n",
		descriptor->data[6]);
	DumpAC_3Features(descriptor->data[6]);
}


void
DumpASFmtSpecific(const usb_generic_descriptor* descriptor)
{
	enum {
		TYPE_II_UNDEFINED = 0x1000,
		MPEG =				0x1001,
		AC_3 =				0x1002
	};

	uint16 formatTag = *(uint16*)&descriptor->data[1];
	switch (formatTag) {
		case MPEG:
			DumpASFmtSpecificMPEG(descriptor);
			break;
		case AC_3:
			DumpASFmtSpecificAC_3(descriptor);
			break;
		default:
			DumpDescriptorData(descriptor);
			break;
	}
}


void
DumpAudioStreamCSInterfaceDescriptor(const usb_generic_descriptor* descriptor)
{
	uint8 subtype = descriptor->data[0];
	switch (subtype) {
		case USB_AUDIO_AS_GENERAL:
			DumpGeneralASInterfaceDescriptor(
				(usb_audio_streaming_interface_descriptor*)descriptor);
			break;
		case USB_AUDIO_AS_FORMAT_TYPE:
			DumpASFmtType(
				(usb_audio_format_descriptor*)descriptor);
			break;
		case USB_AUDIO_AS_FORMAT_SPECIFIC:
			DumpASFmtSpecific(descriptor);
			break;
		default:
			DumpDescriptorData(descriptor);
			break;
	}
}


void
DumpAudioStreamCSEndpointDescriptor(
	const usb_audio_streaming_endpoint_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x (CS_ENDPOINT)\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (EP_GENERAL)\n",
		descriptor->descriptor_subtype);
	printf("                    Attributes ........ 0x%02x ",
		descriptor->attributes);

	const char* attributes[] = {
		"Sampling Frequency",
		"Pitch",
		"", "", "", "", "",
		"Max Packet Only"
	};

	uint8 mask = 1;
	for (uint8 i = 0; i < sizeof(attributes) / sizeof(attributes[0]); i++) {
		if ((descriptor->attributes & mask) != 0)
			printf("%s ", attributes[i]);
		mask <<= 1;
	}
	printf("\n");

	const char* aUnits[] = {
		"Undefined",
		"Milliseconds",
		"Decoded PCM samples",
		"Unknown (%u)"
	};

	const char* units = descriptor->lock_delay_units >= 4
		? aUnits[3] : aUnits[descriptor->lock_delay_units];

	printf("                    Lock Delay Units .. %u (%s)\n",
		descriptor->lock_delay_units, units);
	printf("                    Lock Delay ........ %u\n",
		descriptor->lock_delay);
}


void
DumpMidiInterfaceHeaderDescriptor(
	const usb_midi_interface_header_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x (CS_ENDPOINT)\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (MS_HEADER)\n",
		descriptor->descriptor_subtype);
	printf("                    MSC Version ....... 0x%04x\n",
		descriptor->ms_version);
	printf("                    Length ............ 0x%04x\n",
		descriptor->total_length);
}


void
DumpMidiInJackDescriptor(
	const usb_midi_in_jack_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x (CS_INTERFACE)\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (MIDI_IN_JACK)\n",
		descriptor->descriptor_subtype);
	printf("                    Jack ID ........... 0x%02x\n",
		descriptor->id);
	// TODO can we get the string?
	printf("                    String ............ 0x%02x\n",
		descriptor->string_descriptor);

	switch (descriptor->type) {
		case USB_MIDI_EMBEDDED_JACK:
			printf("                    Jack Type ......... Embedded\n");
			break;
		case USB_MIDI_EXTERNAL_JACK:
			printf("                    Jack Type ......... External\n");
			break;
		default:
			printf("                    Jack Type ......... 0x%02x (unknown)\n",
				descriptor->type);
			break;
	}
}


void
DumpMidiOutJackDescriptor(
	const usb_midi_out_jack_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x (CS_INTERFACE)\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (MIDI_OUT_JACK)\n",
		descriptor->descriptor_subtype);
	printf("                    Jack ID ........... 0x%02x\n",
		descriptor->id);

	switch (descriptor->type) {
		case USB_MIDI_EMBEDDED_JACK:
			printf("                    Jack Type ......... Embedded\n");
			break;
		case USB_MIDI_EXTERNAL_JACK:
			printf("                    Jack Type ......... External\n");
			break;
		default:
			printf("                    Jack Type ......... 0x%02x (unknown)\n",
				descriptor->type);
			break;
	}

	for (int i = 0; i < descriptor->inputs_count; i++) {
		printf("                    Pin %02d ............ (%d,%d)\n", i,
			descriptor->input_source[i].source_id,
			descriptor->input_source[i].source_pin);
	}
}


void
DumpMidiStreamCSInterfaceDescriptor(const usb_generic_descriptor* descriptor)
{
	uint8 subtype = descriptor->data[0];
	switch (subtype) {
		case USB_MS_HEADER_DESCRIPTOR:
			DumpMidiInterfaceHeaderDescriptor(
				(usb_midi_interface_header_descriptor*)descriptor);
			break;
		case USB_MS_MIDI_IN_JACK_DESCRIPTOR:
			DumpMidiInJackDescriptor(
				(usb_midi_in_jack_descriptor*)descriptor);
			break;
		case USB_MS_MIDI_OUT_JACK_DESCRIPTOR:
			DumpMidiOutJackDescriptor(
				(usb_midi_out_jack_descriptor*)descriptor);
			break;
		case USB_MS_ELEMENT_DESCRIPTOR:
			// TODO
			DumpDescriptorData(descriptor);
			break;
		default:
			DumpDescriptorData(descriptor);
			break;
	}
}


void
DumpMidiStreamCSEndpointDescriptor(
	const usb_midi_endpoint_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x (CS_ENDPOINT)\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (MS_GENERAL)\n",
		descriptor->descriptor_subtype);
	printf("                    Jacks ............. ");

	for (int i = 0; i < descriptor->jacks_count; i++)
		printf("%d, ", descriptor->jacks_id[i]);

	printf("\n");
}


void
DumpAudioStreamInterfaceDescriptor(const usb_interface_descriptor* descriptor)
{
	printf("                    Type .............. %u (INTERFACE)\n",
		descriptor->descriptor_type);
	printf("                    Interface ........... %u\n",
		descriptor->interface_number);
	printf("                    Alternate setting ... %u\n",
		descriptor->alternate_setting);
	printf("                    Endpoints ........... %u\n",
		descriptor->num_endpoints);
	printf("                    Interface class ..... %u (AUDIO)\n",
		descriptor->interface_class);
	printf("                    Interface subclass .. %u (AUDIO_STREAMING)\n",
		descriptor->interface_subclass);
	printf("                    Interface ........... %u\n",
		descriptor->interface);
}


void
DumpAudioDescriptor(const usb_generic_descriptor* descriptor, int subclass)
{
	const uint8 USB_AUDIO_INTERFACE = 0x04;

	switch (subclass) {
		case USB_AUDIO_INTERFACE_AUDIOCONTROL_SUBCLASS:
			switch (descriptor->descriptor_type) {
				case USB_AUDIO_CS_INTERFACE:
					DumpAudioControlCSInterfaceDescriptor(descriptor);
					break;
				default:
					DumpDescriptorData(descriptor);
					break;
			}
			break;
		case USB_AUDIO_INTERFACE_AUDIOSTREAMING_SUBCLASS:
			switch (descriptor->descriptor_type) {
				case USB_AUDIO_INTERFACE:
					DumpAudioStreamInterfaceDescriptor(
						(const usb_interface_descriptor*)descriptor);
					break;
				case USB_AUDIO_CS_INTERFACE:
					DumpAudioStreamCSInterfaceDescriptor(descriptor);
					break;
				case USB_AUDIO_CS_ENDPOINT:
					DumpAudioStreamCSEndpointDescriptor(
						(const usb_audio_streaming_endpoint_descriptor*)descriptor);
					break;
				default:
					DumpDescriptorData(descriptor);
					break;
			}
			break;
		case USB_AUDIO_INTERFACE_MIDISTREAMING_SUBCLASS:
			switch (descriptor->descriptor_type) {
				case USB_AUDIO_CS_INTERFACE:
					DumpMidiStreamCSInterfaceDescriptor(descriptor);
					break;
				case USB_AUDIO_CS_ENDPOINT:
					DumpMidiStreamCSEndpointDescriptor(
						(const usb_midi_endpoint_descriptor*)descriptor);
					break;
				default:
					DumpDescriptorData(descriptor);
					break;
			}
			break;
		default:
			DumpDescriptorData(descriptor);
			break;
	}
}
