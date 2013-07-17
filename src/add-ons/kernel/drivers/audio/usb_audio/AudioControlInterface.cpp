/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009-13 S.Zharski <imker@gmx.li>
 *	Distributed under the tems of the MIT license.
 *
 */

#include "AudioControlInterface.h"

#include <usb/USB_audio.h>

#include "Device.h"
#include "Driver.h"
#include "Settings.h"


// control id is encoded in following way
//	CS	CN	ID	IF where:
//	CS				- control selector
//		CN			- channel
//			ID		- id of this feature unit
//				IF	- interface

#define CTL_ID(_CS, _CN, _ID, _IF) \
	(((_CS) << 24) | ((_CN) << 16) | ((_ID) << 8) | (_IF))

#define CS_FROM_CTLID(_ID) (0xff & ((_ID) >> 24))
#define CN_FROM_CTLID(_ID) (0xff & ((_ID) >> 16))
#define ID_FROM_CTLID(_ID) (0xff & ((_ID) >> 8))

#define REQ_VALUE(_ID) (0xffff & ((_ID) >> 16))
#define REQ_INDEX(_ID) (0xffff & (_ID))


_AudioControl::_AudioControl(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	fStatus(B_NO_INIT),
	fInterface(interface),
	fSubType(Header->descriptor_subtype),
	fID(0),
	fSourceID(0),
	fStringIndex(0)
{
}


_AudioControl::~_AudioControl()
{
}


AudioChannelCluster*
_AudioControl::OutCluster()
{
	if (SourceID() == 0 || fInterface == NULL)
		return NULL;

	_AudioControl* control = fInterface->Find(SourceID());
	if (control == NULL)
		return NULL;

	return control->OutCluster();
}


AudioChannelCluster::AudioChannelCluster()
	:
	fOutChannelsNumber(0),
	fChannelsConfig(0),
	fChannelNames(0)
{
}


AudioChannelCluster::~AudioChannelCluster()
{
}


_Terminal::_Terminal(AudioControlInterface*	interface,
	usb_audiocontrol_header_descriptor* Header)
	:
	_AudioControl(interface, Header),
	fTerminalType(0),
	fAssociatedTerminal(0),
	fClockSourceId(0),
	fControlsBitmap(0)
{
}


_Terminal::~_Terminal()
{
}


const char*
_Terminal::Name()
{
	return _GetTerminalDescription(fTerminalType);
}


bool
_Terminal::IsUSBIO()
{
	return (fTerminalType & 0xff00) == USB_AUDIO_UNDEFINED_USB_IO;
}


const char*
_Terminal::_GetTerminalDescription(uint16 TerminalType)
{
	static struct _pair {
		uint16 type;
		const char* description;
	} termInfoPairs[] = {
		// USB Terminal Types
		{ USB_AUDIO_UNDEFINED_USB_IO,		"USB I/O" },
		{ USB_AUDIO_STREAMING_USB_IO,		"USB Streaming I/O" },
		{ USB_AUDIO_VENDOR_USB_IO,			"Vendor USB I/O" },
		// Input Terminal Types
		{ USB_AUDIO_UNDEFINED_IN,			"Undefined Input" },
		{ USB_AUDIO_MICROPHONE_IN,			"Microphone" },
		{ USB_AUDIO_DESKTOPMIC_IN,			"Desktop Microphone" },
		{ USB_AUDIO_PERSONALMIC_IN,			"Personal Microphone" },
		{ USB_AUDIO_OMNI_MIC_IN,			"Omni-directional Mic" },
		{ USB_AUDIO_MICS_ARRAY_IN,			"Microphone Array" },
		{ USB_AUDIO_PROC_MICS_ARRAY_IN,		"Processing Mic Array" },
		// Output Terminal Types
		{ USB_AUDIO_UNDEFINED_OUT,			"Undefined Output" },
		{ USB_AUDIO_SPEAKER_OUT,			"Speaker" },
		{ USB_AUDIO_HEAD_PHONES_OUT,		"Headphones" },
		{ USB_AUDIO_HMD_AUDIO_OUT,			"Head Mounted Disp.Audio" },
		{ USB_AUDIO_DESKTOP_SPEAKER,		"Desktop Speaker" },
		{ USB_AUDIO_ROOM_SPEAKER,			"Room Speaker" },
		{ USB_AUDIO_COMM_SPEAKER,			"Communication Speaker" },
		{ USB_AUDIO_LFE_SPEAKER,			"LFE Speaker" },
		// Bi-directional Terminal Types
		{ USB_AUDIO_UNDEFINED_IO,			"Undefined I/O" },
		{ USB_AUDIO_HANDSET_IO,				"Handset" },
		{ USB_AUDIO_HEADSET_IO,				"Headset" },
		{ USB_AUDIO_SPEAKER_PHONE_IO,		"Speakerphone" },
		{ USB_AUDIO_SPEAKER_PHONEES_IO,		"Echo-supp Speakerphone" },
		{ USB_AUDIO_SPEAKER_PHONEEC_IO,		"Echo-cancel Speakerphone" },
		// Telephony Terminal Types
		{ USB_AUDIO_UNDEF_TELEPHONY_IO,		"Undefined Telephony" },
		{ USB_AUDIO_PHONE_LINE_IO,			"Phone Line" },
		{ USB_AUDIO_TELEPHONE_IO,			"Telephone" },
		{ USB_AUDIO_DOWNLINE_PHONE_IO,		"Down Line Phone" },
		// External Terminal Types
		{ USB_AUDIO_UNDEFINEDEXT_IO,		"Undefined External I/O" },
		{ USB_AUDIO_ANALOG_CONNECTOR_IO,	"Analog Connector" },
		{ USB_AUDIO_DAINTERFACE_IO,			"Digital Audio Interface" },
		{ USB_AUDIO_LINE_CONNECTOR_IO,		"Line Connector" },
		{ USB_AUDIO_LEGACY_CONNECTOR_IO,	"LegacyAudioConnector" },
		{ USB_AUDIO_SPDIF_INTERFACE_IO,		"S/PDIF Interface" },
		{ USB_AUDIO_DA1394_STREAM_IO,		"1394 DA Stream" },
		{ USB_AUDIO_DV1394_STREAMSOUND_IO,	"1394 DV Stream Soundtrack" },
		{ USB_AUDIO_ADAT_LIGHTPIPE_IO,		"Alesis DAT Stream" },
		{ USB_AUDIO_TDIF_IO,				"Tascam Digital Interface" },
		{ USB_AUDIO_MADI_IO,				"AES Multi-channel interface" },
		// Embedded Terminal Types
		{ USB_AUDIO_UNDEF_EMBEDDED_IO,		"Undefined Embedded I/O" },
		{ USB_AUDIO_LC_NOISE_SOURCE_OUT,	"Level Calibration Noise Source" },
		{ USB_AUDIO_EQUALIZATION_NOISE_OUT,	"Equalization Noise" },
		{ USB_AUDIO_CDPLAYER_IN,			"CD Player" },
		{ USB_AUDIO_DAT_IO,					"DAT" },
		{ USB_AUDIO_DCC_IO,					"DCC" },
		{ USB_AUDIO_MINI_DISK_IO,			"Mini Disk" },
		{ USB_AUDIO_ANALOG_TAPE_IO,			"Analog Tape" },
		{ USB_AUDIO_PHONOGRAPH_IN,			"Phonograph" },
		{ USB_AUDIO_VCR_AUDIO_IN,			"VCR Audio" },
		{ USB_AUDIO_VIDEO_DISC_AUDIO_IN,	"Video Disc Audio" },
		{ USB_AUDIO_DVD_AUDIO_IN,			"DVD Audio" },
		{ USB_AUDIO_TV_TUNER_AUDIO_IN,		"TV Tuner Audio" },
		{ USB_AUDIO_SAT_RECEIVER_AUDIO_IN,	"Satellite Receiver Audio" },
		{ USB_AUDIO_CABLE_TUNER_AUDIO_IN,	"Cable Tuner Audio" },
		{ USB_AUDIO_DSS_AUDIO_IN,			"DSS Audio" },
		{ USB_AUDIO_RADIO_RECEIVER_IN,		"Radio Receiver" },
		{ USB_AUDIO_RADIO_TRANSMITTER_IN,	"Radio Transmitter" },
		{ USB_AUDIO_MULTI_TRACK_RECORDER_IO,"Multi-track Recorder" },
		{ USB_AUDIO_SYNTHESIZER_IO,			"Synthesizer" },
		{ USB_AUDIO_PIANO_IO,				"Piano" },
		{ USB_AUDIO_GUITAR_IO,				"Guitar" },
		{ USB_AUDIO_DRUMS_IO,				"Percussion Instrument" },
		{ USB_AUDIO_INSTRUMENT_IO,			"Musical Instrument" }
	};

	for (size_t i = 0; _countof(termInfoPairs); i++)
		if (termInfoPairs[i].type == TerminalType)
			return termInfoPairs[i].description;

	TRACE(ERR, "Unknown Terminal Type: %#06x", TerminalType);
	return "Unknown";
}


InputTerminal::InputTerminal(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	_AudioChannelCluster<_Terminal>(interface, Header)
{
	usb_audio_input_terminal_descriptor* Terminal
		= (usb_audio_input_terminal_descriptor*) Header;
	fID					= Terminal->terminal_id;
	fTerminalType		= Terminal->terminal_type;
	fAssociatedTerminal = Terminal->assoc_terminal;

	TRACE(UAC, "Input Terminal ID:%d >>>\n", fID);
	TRACE(UAC, "Terminal type:%s (%#06x)\n",
		_GetTerminalDescription(fTerminalType), fTerminalType);
	TRACE(UAC, "Assoc.terminal:%d\n",	fAssociatedTerminal);

	if (fInterface->SpecReleaseNumber() < 0x200) {
		fOutChannelsNumber	= Terminal->r1.num_channels;
		fChannelsConfig		= Terminal->r1.channel_config;
		fChannelNames		= Terminal->r1.channel_names;
		fStringIndex		= Terminal->r1.terminal;
	} else {
		fClockSourceId		= Terminal->r2.clock_source_id;
		fOutChannelsNumber	= Terminal->r2.num_channels;
		fChannelsConfig		= Terminal->r2.channel_config;
		fChannelNames		= Terminal->r2.channel_names;
		fControlsBitmap		= Terminal->r2.bm_controls;
		fStringIndex		= Terminal->r2.terminal;

		TRACE(UAC, "Clock Source ID:%d\n", fClockSourceId);
		TRACE(UAC, "Controls Bitmap:%#04x\n", fControlsBitmap);
	}

	TRACE(UAC, "Out.channels num:%d\n",	 fOutChannelsNumber);
	TRACE(UAC, "Channels config:%#06x\n", fChannelsConfig);
	TRACE(UAC, "Channels names:%d\n",	 fChannelNames);
	TRACE(UAC, "StringIndex:%d\n",		 fStringIndex);

	fStatus = B_OK;
}


InputTerminal::~InputTerminal()
{
}


OutputTerminal::OutputTerminal(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	_Terminal(interface, Header)
{
	usb_audio_output_terminal_descriptor* Terminal
		= (usb_audio_output_terminal_descriptor*) Header;

	fID					= Terminal->terminal_id;
	fTerminalType		= Terminal->terminal_type;
	fAssociatedTerminal	= Terminal->assoc_terminal;
	fSourceID			= Terminal->source_id;

	TRACE(UAC, "Output Terminal ID:%d >>>\n",	fID);
	TRACE(UAC, "Terminal type:%s (%#06x)\n",
		_GetTerminalDescription(fTerminalType), fTerminalType);
	TRACE(UAC, "Assoc.terminal:%d\n",		fAssociatedTerminal);
	TRACE(UAC, "Source ID:%d\n",				fSourceID);

	if (fInterface->SpecReleaseNumber() < 0x200) {
		fStringIndex = Terminal->r1.terminal;
	} else {
		fClockSourceId	= Terminal->r2.clock_source_id;
		fControlsBitmap	= Terminal->r2.bm_controls;
		fStringIndex	= Terminal->r2.terminal;

		TRACE(UAC, "Clock Source ID:%d\n", fClockSourceId);
		TRACE(UAC, "Controls Bitmap:%#04x\n", fControlsBitmap);
	}

	TRACE(UAC, "StringIndex:%d\n", fStringIndex);

	fStatus = B_OK;
}


OutputTerminal::~OutputTerminal()
{
}


MixerUnit::MixerUnit(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	_AudioChannelCluster<_AudioControl>(interface, Header),
	fControlsBitmap(0)
{
	usb_audio_mixer_unit_descriptor* Mixer
		= (usb_audio_mixer_unit_descriptor*) Header;

	fID = Mixer->unit_id;
	TRACE(UAC, "Mixer ID:%d >>>\n", fID);

	TRACE(UAC, "Number of input pins:%d\n", Mixer->num_input_pins);
	for (size_t i = 0; i < Mixer->num_input_pins; i++) {
		fInputPins.PushBack(Mixer->input_pins[i]);
		TRACE(UAC, "Input pin #%d:%d\n", i, fInputPins[i]);
	}

	uint8* mixerControlsData = NULL;
	uint8 mixerControlsSize = 0;

	if (fInterface->SpecReleaseNumber() < 0x200) {
		usb_audio_output_channels_descriptor_r1* OutChannels
			= (usb_audio_output_channels_descriptor_r1*)
			&Mixer->input_pins[Mixer->num_input_pins];

		fOutChannelsNumber	= OutChannels->num_output_pins;
		fChannelsConfig		= OutChannels->channel_config;
		fChannelNames		= OutChannels->channel_names;

		mixerControlsData = (uint8*) ++OutChannels;
		mixerControlsSize = Mixer->length - 10 - Mixer->num_input_pins;
		fStringIndex = *(mixerControlsData + mixerControlsSize);
	} else {
		usb_audio_output_channels_descriptor* OutChannels
			= (usb_audio_output_channels_descriptor*)
			&Mixer->input_pins[Mixer->num_input_pins];

		fOutChannelsNumber	= OutChannels->num_output_pins;
		fChannelsConfig		= OutChannels->channel_config;
		fChannelNames		= OutChannels->channel_names;

		mixerControlsData = (uint8*) ++OutChannels;
		mixerControlsSize = Mixer->length - 13 - Mixer->num_input_pins;
		fControlsBitmap = *(mixerControlsData + mixerControlsSize);
		fStringIndex = *(mixerControlsData + mixerControlsSize + 1);

		TRACE(UAC, "Control Bitmap:%#04x\n", fControlsBitmap);
	}

	TRACE(UAC, "Out channels number:%d\n",		fOutChannelsNumber);
	TRACE(UAC, "Out channels config:%#06x\n",	fChannelsConfig);
	TRACE(UAC, "Out channels names:%d\n",		fChannelNames);
	TRACE(UAC, "Controls Size:%d\n", mixerControlsSize);

	for (size_t i = 0; i < mixerControlsSize; i++) {
		fProgrammableControls.PushBack(mixerControlsData[i]);
		TRACE(UAC, "Controls Data[%d]:%#x\n", i, fProgrammableControls[i]);
	}

	TRACE(UAC, "StringIndex:%d\n", fStringIndex);

	fStatus = B_OK;
}


MixerUnit::~MixerUnit()
{
}


SelectorUnit::SelectorUnit(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	_AudioControl(interface, Header),
	fControlsBitmap(0)
{
	usb_audio_selector_unit_descriptor* Selector
		= (usb_audio_selector_unit_descriptor*) Header;

	fID = Selector->unit_id;
	TRACE(UAC, "Selector ID:%d >>>\n", fID);

	TRACE(UAC, "Number of input pins:%d\n", Selector->num_input_pins);
	for (size_t i = 0; i < Selector->num_input_pins; i++) {
		fInputPins.PushBack(Selector->input_pins[i]);
		TRACE(UAC, "Input pin #%d:%d\n", i, fInputPins[i]);
	}

	if (fInterface->SpecReleaseNumber() < 0x200) {
		fStringIndex = Selector->input_pins[Selector->num_input_pins];
	} else {
		fControlsBitmap = Selector->input_pins[Selector->num_input_pins];
		fStringIndex = Selector->input_pins[Selector->num_input_pins + 1];

		TRACE(UAC, "Controls Bitmap:%d\n", fControlsBitmap);
	}

	TRACE(UAC, "StringIndex:%d\n", fStringIndex);

	fStatus = B_OK;
}


SelectorUnit::~SelectorUnit()
{
}


AudioChannelCluster*
SelectorUnit::OutCluster()
{
	if (fInterface == NULL)
		return NULL;

	for (int i = 0; i < fInputPins.Count(); i++) {
		_AudioControl* control = fInterface->Find(fInputPins[i]);
		if (control == NULL)
			continue;

		if (control->OutCluster() != NULL)
			return control->OutCluster();
	}

	return NULL;
}


FeatureUnit::FeatureUnit(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	_AudioControl(interface, Header)
{
	usb_audio_feature_unit_descriptor* Feature
		= (usb_audio_feature_unit_descriptor*) Header;

	fID = Feature->unit_id;
	TRACE(UAC, "Feature ID:%d >>>\n", fID);

	fSourceID = Feature->source_id;
	TRACE(UAC, "Source ID:%d\n", fSourceID);

	uint8 controlSize = 4;
	uint8 channelsCount = (Feature->length - 6) / controlSize;
	uint8* ControlsBitmapPointer = (uint8*)&Feature->r2.bma_controls[0];

	if (fInterface->SpecReleaseNumber() < 0x200) {
		controlSize = Feature->r1.control_size;
		channelsCount = (Feature->length - 7) / Feature->r1.control_size;
		ControlsBitmapPointer = &Feature->r1.bma_controls[0];
	}

	TRACE(UAC, "Channel bitmap size:%d\n", controlSize);
	TRACE(UAC, "Channels number:%d\n", channelsCount - 1); // not add master!

	for (size_t i = 0; i < channelsCount; i++) {
		uint8* controlPointer = &ControlsBitmapPointer[i* controlSize];
		switch(controlSize) {
			case 1: fControlBitmaps.PushBack(*controlPointer); break;
			case 2: fControlBitmaps.PushBack(*(uint16*)controlPointer); break;
			case 4: fControlBitmaps.PushBack(*(uint32*)controlPointer); break;
			default:
				TRACE(ERR, "Feature control of unsupported size %d ignored\n",
														controlSize);
				continue;
		}

		NormalizeAndTraceChannel(i);
	}

	fStringIndex = ControlsBitmapPointer[channelsCount* controlSize];
	TRACE(UAC, "StringIndex:%d\n", fStringIndex);

	fStatus = B_OK;
}


FeatureUnit::~FeatureUnit()
{
}


const char*
FeatureUnit::Name()
{
	// first check if source of this FU is an input terminal
	_AudioControl* control = fInterface->Find(fSourceID);
	while (control != NULL) {
		if (control->SubType() != USB_AUDIO_AC_INPUT_TERMINAL)
			break;

		// USB I/O terminal is a not good candidate to use it's name
		if (static_cast<_Terminal*>(control)->IsUSBIO())
			break;

		// use the name of source input terminal as name of this FU
		return control->Name();
	}

	// check if output of this FU is connected to output terminal
	control = fInterface->FindOutputTerminal(fID);
	while (control != NULL) {
		if (control->SubType() != USB_AUDIO_AC_OUTPUT_TERMINAL)
			break;

		// USB I/O terminal is a not good candidate to use it's name
		if (static_cast<_Terminal*>(control)->IsUSBIO())
			break;

		// use the name of this output terminal as name of this FU
		return control->Name();
	}

	// otherwise get the generic name of this FU's source
	control = fInterface->Find(fSourceID);
	if (control != NULL && control->Name() != NULL
			&& strlen(control->Name()) > 0)
		return control->Name();

	// I have no more ideas, have you one?
	return "Unknown";
}


bool
FeatureUnit::HasControl(int32 Channel, uint32 Control)
{
	if (Channel >= fControlBitmaps.Count()) {
		TRACE(ERR, "Out of limits error of retrieving control %#010x "
			"for channel %d\n", Control, Channel);
		return false;
	}

	return (Control & fControlBitmaps[Channel]) != 0;
}


void
FeatureUnit::NormalizeAndTraceChannel(int32 Channel)
{
	if (Channel >= fControlBitmaps.Count()) {
		TRACE(ERR, "Out of limits error of tracing channel %d\n", Channel);
		return;
	}

	struct _RemapInfo {
		uint32	rev1Bits;
		uint32	rev2Bits;
		const char* name;
	} remapInfos[] = {
		{ BMA_CTL_MUTE_R1,		BMA_CTL_MUTE,			"Mute"		},
		{ BMA_CTL_VOLUME_R1,	BMA_CTL_VOLUME,			"Volume"	},
		{ BMA_CTL_BASS_R1,		BMA_CTL_BASS,			"Bass"		},
		{ BMA_CTL_MID_R1,		BMA_CTL_MID,			"Mid"		},
		{ BMA_CTL_TREBLE_R1,	BMA_CTL_TREBLE,			"Treble"	},
		{ BMA_CTL_GRAPHEQ_R1,	BMA_CTL_GRAPHEQ,		"Graphic Equalizer"	},
		{ BMA_CTL_AUTOGAIN_R1,	BMA_CTL_AUTOGAIN,		"Automatic Gain"},
		{ BMA_CTL_DELAY_R1,		BMA_CTL_DELAY,			"Delay"			},
		{ BMA_CTL_BASSBOOST_R1,	BMA_CTL_BASSBOOST,		"Bass Boost"	},
		{ BMA_CTL_LOUDNESS_R1,	BMA_CTL_LOUDNESS,		"Loudness"		},
		{ 0,					BMA_CTL_INPUTGAIN,		"InputGain"		},
		{ 0,					BMA_CTL_INPUTGAINPAD,	"InputGainPad"	},
		{ 0,					BMA_CTL_PHASEINVERTER,	"PhaseInverter"	},
		{ 0,					BMA_CTL_UNDERFLOW,		"Underflow"		},
		{ 0,					BMA_CTL_OVERFLOW,		"Overflow"		}
	};

	if (Channel == 0)
		TRACE(UAC, "Master channel bitmap:%#x\n", fControlBitmaps[Channel]);
	else
		TRACE(UAC, "Channel %d bitmap:%#x\n", Channel, fControlBitmaps[Channel]);

	bool isRev1 = (fInterface->SpecReleaseNumber() < 0x200);

	uint32 remappedBitmap = 0;
	for (size_t i = 0; i < _countof(remapInfos); i++) {
		uint32 bits = isRev1 ? remapInfos[i].rev1Bits : remapInfos[i].rev2Bits;
		if ((fControlBitmaps[Channel] & bits) > 0) {
			if (isRev1)
				remappedBitmap |= remapInfos[i].rev2Bits;
			TRACE(UAC, "\t%s\n", remapInfos[i].name);
		}
	}

	if (isRev1) {
		TRACE(UAC, "\t%#08x -> %#08x.\n",
			fControlBitmaps[Channel], remappedBitmap);
		fControlBitmaps[Channel] = remappedBitmap;
	}
}


EffectUnit::EffectUnit(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	_AudioControl(interface, Header)
{
	usb_audio_input_terminal_descriptor* descriptor
		= (usb_audio_input_terminal_descriptor*) Header;
	TRACE(UAC, "Effect Unit:%d >>>\n", descriptor->terminal_id);
}


EffectUnit::~EffectUnit()
{
}


ProcessingUnit::ProcessingUnit(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	_AudioChannelCluster<_AudioControl>(interface, Header),
	fProcessType(0),
	fControlsBitmap(0)
{
	usb_audio_processing_unit_descriptor* Processing
		= (usb_audio_processing_unit_descriptor*) Header;

	fID = Processing->unit_id;
	TRACE(UAC, "Processing ID:%d >>>\n", fID);

	fProcessType = Processing->process_type;
	TRACE(UAC, "Processing Type:%d\n", fProcessType);

	TRACE(UAC, "Number of input pins:%d\n", Processing->num_input_pins);
	for (size_t i = 0; i < Processing->num_input_pins; i++) {
		fInputPins.PushBack(Processing->input_pins[i]);	
		TRACE(UAC, "Input pin #%d:%d\n", i, fInputPins[i]);
	}

	if (fInterface->SpecReleaseNumber() < 0x200) {
		usb_audio_output_channels_descriptor_r1* OutChannels
			= (usb_audio_output_channels_descriptor_r1*)
			&Processing->input_pins[Processing->num_input_pins];

		fOutChannelsNumber	= OutChannels->num_output_pins;
		fChannelsConfig		= OutChannels->channel_config;
		fChannelNames		= OutChannels->channel_names;
	} else {
		usb_audio_output_channels_descriptor* OutChannels
			= (usb_audio_output_channels_descriptor*)
			&Processing->input_pins[Processing->num_input_pins];

		fOutChannelsNumber	= OutChannels->num_output_pins;
		fChannelsConfig		= OutChannels->channel_config;
		fChannelNames		= OutChannels->channel_names;
	}

	TRACE(UAC, "Out channels number:%d\n",		fOutChannelsNumber);
	TRACE(UAC, "Out channels config:%#06x\n",	fChannelsConfig);
	TRACE(UAC, "Out channels names:%d\n",		fChannelNames);
	/*
	uint8 controlsSize = Processing->length - 10 - Processing->num_input_pins;
	TRACE(UAC, "Controls Size:%d\n", controlsSize);

	uint8* controlsData = (uint8*) ++OutChannels;

	for (size_t i = 0; i < controlsSize; i++) {
		fProgrammableControls.PushBack(controlsData[i]);
		TRACE(UAC, "Controls Data[%d]:%#x\n", i, controlsData[i]);
	}

	fStringIndex = *(controlsData + controlsSize);

	TRACE(UAC, "StringIndex:%d\n", fStringIndex);
*/
	fStatus = B_OK;
}


ProcessingUnit::~ProcessingUnit()
{
}


ExtensionUnit::ExtensionUnit(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	_AudioChannelCluster<_AudioControl>(interface, Header),
	fExtensionCode(0),
	fControlsBitmap(0)
{
	usb_audio_extension_unit_descriptor* Extension
		= (usb_audio_extension_unit_descriptor*) Header;

	fID = Extension->unit_id;
	TRACE(UAC, "Extension ID:%d >>>\n", fID);

	fExtensionCode = Extension->extension_code;
	TRACE(UAC, "Extension Type:%d\n", fExtensionCode);

	TRACE(UAC, "Number of input pins:%d\n", Extension->num_input_pins);
	for (size_t i = 0; i < Extension->num_input_pins; i++) {
		fInputPins.PushBack(Extension->input_pins[i]);
		TRACE(UAC, "Input pin #%d:%d\n", i, fInputPins[i]);
	}

	if (fInterface->SpecReleaseNumber() < 0x200) {
		usb_audio_output_channels_descriptor_r1* OutChannels
			= (usb_audio_output_channels_descriptor_r1*)
			&Extension->input_pins[Extension->num_input_pins];

		fOutChannelsNumber	= OutChannels->num_output_pins;
		fChannelsConfig		= OutChannels->channel_config;
		fChannelNames		= OutChannels->channel_names;
	} else {
		usb_audio_output_channels_descriptor* OutChannels
			= (usb_audio_output_channels_descriptor*)
			&Extension->input_pins[Extension->num_input_pins];

		fOutChannelsNumber	= OutChannels->num_output_pins;
		fChannelsConfig		= OutChannels->channel_config;
		fChannelNames		= OutChannels->channel_names;
	}

	TRACE(UAC, "Out channels number:%d\n",		fOutChannelsNumber);
	TRACE(UAC, "Out channels config:%#06x\n",	fChannelsConfig);
	TRACE(UAC, "Out channels names:%d\n",		fChannelNames);
	/*
	uint8 controlsSize = Processing->length - 10 - Processing->num_input_pins;
	TRACE(UAC, "Controls Size:%d\n", controlsSize);

	uint8* controlsData = (uint8*) ++OutChannels;

	for (size_t i = 0; i < controlsSize; i++) {
		fProgrammableControls.PushBack(controlsData[i]);
		TRACE(UAC, "Controls Data[%d]:%#x\n", i, controlsData[i]);
	}

	fStringIndex = *(controlsData + controlsSize);

	TRACE(UAC, "StringIndex:%d\n", fStringIndex);
*/
	fStatus = B_OK;
}


ExtensionUnit::~ExtensionUnit()
{
}


ClockSource::ClockSource(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	_AudioControl(interface, Header)
{
	usb_audio_input_terminal_descriptor* descriptor
		= (usb_audio_input_terminal_descriptor*) Header;
	TRACE(UAC, "Clock Source:%d >>>\n",	descriptor->terminal_id);
}


ClockSource::~ClockSource()
{
}


ClockSelector::ClockSelector(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	_AudioControl(interface, Header)
{
	usb_audio_input_terminal_descriptor* descriptor
		= (usb_audio_input_terminal_descriptor*) Header;
	TRACE(UAC, "Clock Selector:%d >>>\n", descriptor->terminal_id);
}


ClockSelector::~ClockSelector()
{
}


ClockMultiplier::ClockMultiplier(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	_AudioControl(interface, Header)
{
	usb_audio_input_terminal_descriptor* descriptor
		= (usb_audio_input_terminal_descriptor*) Header;
	TRACE(UAC, "Clock Multiplier:%d >>>\n",	descriptor->terminal_id);
}


ClockMultiplier::~ClockMultiplier()
{
}


SampleRateConverter::SampleRateConverter(AudioControlInterface*	interface,
		usb_audiocontrol_header_descriptor* Header)
	:
	_AudioControl(interface, Header)
{
	usb_audio_input_terminal_descriptor* descriptor
		= (usb_audio_input_terminal_descriptor*) Header;
	TRACE(UAC, "Sample Rate Converter:%d >>>\n", descriptor->terminal_id);
}


SampleRateConverter::~SampleRateConverter()
{
}


AudioControlInterface::AudioControlInterface(Device* device)
	:
	fInterface(0),
	fStatus(B_NO_INIT),
	fADCSpecification(0),
	fFunctionCategory(0),
	fControlsBitmap(0),
	fDevice(device)
{
}


AudioControlInterface::~AudioControlInterface()
{
	for (AudioControlsIterator I = fAudioControls.Begin();
			I != fAudioControls.End(); I++)
		delete I->Value();

	fAudioControls.MakeEmpty();

	// object already freed. just purge the map
	fOutputTerminals.MakeEmpty();

	// object already freed. just purge the map
	fInputTerminals.MakeEmpty();
}


status_t
AudioControlInterface::Init(size_t interface, usb_interface_info* Interface)
{
	for (size_t i = 0; i < Interface->generic_count; i++) {
		usb_audiocontrol_header_descriptor* Header
			= (usb_audiocontrol_header_descriptor* )Interface->generic[i];

		if (Header->descriptor_type != USB_AUDIO_CS_INTERFACE) {
			TRACE(ERR, "Ignore Audio Control of "
				"unknown descriptor type %#04x.\n",	Header->descriptor_type);
			continue;
		}

		_AudioControl* control = NULL;

		switch(Header->descriptor_subtype) {
			default:
				TRACE(ERR, "Ignore Audio Control of unknown "
					"descriptor sub-type %#04x\n", Header->descriptor_subtype);
				break;
			case USB_AUDIO_AC_DESCRIPTOR_UNDEFINED:
				TRACE(ERR, "Ignore Audio Control of undefined sub-type\n");
				break;
			case USB_AUDIO_AC_HEADER:
				InitACHeader(interface, Header);
				break;
			case USB_AUDIO_AC_INPUT_TERMINAL:
				control = new InputTerminal(this, Header);
				break;
			case USB_AUDIO_AC_OUTPUT_TERMINAL:
				control = new OutputTerminal(this, Header);
				break;
			case USB_AUDIO_AC_MIXER_UNIT:
				control = new MixerUnit(this, Header);
				break;
			case USB_AUDIO_AC_SELECTOR_UNIT:
				control = new SelectorUnit(this, Header);
				break;
			case USB_AUDIO_AC_FEATURE_UNIT:
				control = new FeatureUnit(this, Header);
				break;
			case USB_AUDIO_AC_PROCESSING_UNIT:
				if (SpecReleaseNumber() < 200)
					control = new ProcessingUnit(this, Header);
				else
					control = new EffectUnit(this, Header);
				break;
			case USB_AUDIO_AC_EXTENSION_UNIT:
				if (SpecReleaseNumber() < 200)
					control = new ExtensionUnit(this, Header);
				else
					control = new ProcessingUnit(this, Header);
				break;
			case USB_AUDIO_AC_EXTENSION_UNIT_R2:
				control = new ExtensionUnit(this, Header);
				break;
			case USB_AUDIO_AC_CLOCK_SOURCE_R2:
				control = new ClockSource(this, Header);
				break;
			case USB_AUDIO_AC_CLOCK_SELECTOR_R2:
				control = new ClockSelector(this, Header);
				break;
			case USB_AUDIO_AC_CLOCK_MULTIPLIER_R2:
				control = new ClockMultiplier(this, Header);
				break;
			case USB_AUDIO_AC_SAMPLE_RATE_CONVERTER_R2:
				control = new SampleRateConverter(this, Header);
				break;
		}

		if (control != 0 && control->InitCheck() == B_OK) {
			switch(control->SubType()) {
				case USB_AUDIO_AC_OUTPUT_TERMINAL:
					fOutputTerminals.Put(control->SourceID(), control);
					break;
				case USB_AUDIO_AC_INPUT_TERMINAL:
					fInputTerminals.Put(control->ID(), control);
					break;
			}
			fAudioControls.Put(control->ID(), control);

		} else
			delete control;
	}

	return fStatus = B_OK;
}


_AudioControl*
AudioControlInterface::Find(uint8 id)
{
	return fAudioControls.Get(id);
}


_AudioControl*
AudioControlInterface::FindOutputTerminal(uint8 id)
{
	return fOutputTerminals.Get(id);
}


status_t
AudioControlInterface::InitACHeader(size_t interface,
		usb_audiocontrol_header_descriptor* Header)
{
	if (Header == NULL)
		return fStatus = B_NO_INIT;

	fInterface = interface;

	fADCSpecification = Header->bcd_release_no;
	TRACE(UAC, "ADCSpecification:%#06x\n", fADCSpecification);

	if (fADCSpecification < 0x200) {
		TRACE(UAC, "InterfacesCount:%d\n",	Header->r1.in_collection);
		for (size_t i = 0; i < Header->r1.in_collection; i++) {
			fStreams.PushBack(Header->r1.interface_numbers[i]);
			TRACE(UAC, "Interface[%d] number is %d\n", i, fStreams[i]);
		}
	} else {
		fFunctionCategory = Header->r2.function_category;
		fControlsBitmap = Header->r2.bm_controls;
		TRACE(UAC, "Function Category:%#04x\n", fFunctionCategory);
		TRACE(UAC, "Controls Bitmap:%#04x\n", fControlsBitmap);
	}

	return /*fStatus =*/ B_OK;
}


uint32
AudioControlInterface::GetChannelsDescription(
		Vector<multi_channel_info>& Channels, multi_description* Description,
		Vector<_AudioControl*>&Terminals)
{
	uint32 addedChannels = 0;
//	multi_channel_info* Channels = Description->channels;

	for (int32 i = 0; i < Terminals.Count(); i++) {
		bool bIsInputTerminal
			= Terminals[i]->SubType() == USB_AUDIO_AC_INPUT_TERMINAL;

		AudioChannelCluster* cluster = Terminals[i]->OutCluster();
		if (cluster == 0 || cluster->ChannelsCount() <= 0) {
			TRACE(ERR, "Terminal #%d ignored due null "
				"channels cluster (%08x)\n", Terminals[i]->ID(), cluster);
			continue;
		}

		uint32 channels = GetTerminalChannels(Channels, cluster, // index,
			bIsInputTerminal ? B_MULTI_INPUT_CHANNEL : B_MULTI_OUTPUT_CHANNEL);

		if (bIsInputTerminal)
			Description->input_channel_count += channels;
		else
			Description->output_channel_count += channels;

		addedChannels += channels;
	}

	return addedChannels;
}


uint32
AudioControlInterface::GetTerminalChannels(Vector<multi_channel_info>& Channels,
		AudioChannelCluster* cluster, channel_kind kind, uint32 connectors)
{
	if (cluster->ChannelsCount() < 2) { // mono channel
		multi_channel_info info;
		info.channel_id	= Channels.Count();
		info.kind		= kind;
		info.designations= B_CHANNEL_MONO_BUS;
		info.connectors	= connectors;
		Channels.PushBack(info);

		return 1;
	}

	uint32 startCount = Channels.Count();

	const uint32 locations = 18;

	struct _Designation {
		uint32 ch;
		uint32 bus;
	} Designations[locations] = {
		{ B_CHANNEL_LEFT,				B_CHANNEL_STEREO_BUS	},
		{ B_CHANNEL_RIGHT,				B_CHANNEL_STEREO_BUS	},
		{ B_CHANNEL_CENTER,				B_CHANNEL_SURROUND_BUS	},
		{ B_CHANNEL_SUB,				B_CHANNEL_SURROUND_BUS	},
		{ B_CHANNEL_REARLEFT,			B_CHANNEL_STEREO_BUS	},
		{ B_CHANNEL_REARRIGHT,			B_CHANNEL_STEREO_BUS	},
		{ B_CHANNEL_FRONT_LEFT_CENTER,	B_CHANNEL_STEREO_BUS	},
		{ B_CHANNEL_FRONT_RIGHT_CENTER,	B_CHANNEL_STEREO_BUS	},
		{ B_CHANNEL_BACK_CENTER,		B_CHANNEL_SURROUND_BUS	},
		{ B_CHANNEL_SIDE_LEFT,			B_CHANNEL_STEREO_BUS	},
		{ B_CHANNEL_SIDE_RIGHT,			B_CHANNEL_STEREO_BUS	},
		{ B_CHANNEL_TOP_CENTER,			B_CHANNEL_SURROUND_BUS	},
		{ B_CHANNEL_TOP_FRONT_LEFT,		B_CHANNEL_STEREO_BUS	},
		{ B_CHANNEL_TOP_FRONT_CENTER,	B_CHANNEL_STEREO_BUS	},
		{ B_CHANNEL_TOP_FRONT_RIGHT,	B_CHANNEL_STEREO_BUS	},
		{ B_CHANNEL_TOP_BACK_LEFT,		B_CHANNEL_STEREO_BUS	},
		{ B_CHANNEL_TOP_BACK_CENTER,	B_CHANNEL_SURROUND_BUS	},
		{ B_CHANNEL_TOP_BACK_RIGHT,		B_CHANNEL_STEREO_BUS	}
	};

	// Haiku multi-aduio designations have the same bits
	// as USB Audio 2.0 cluster spatial locations :-)
	for (size_t i = 0; i < locations; i++) {
		uint32 designation = 1 << i;
		if ((cluster->ChannelsConfig() & designation) == designation) {
			multi_channel_info info;
			info.channel_id	= Channels.Count();
			info.kind		= kind;
			info.designations= Designations[i].ch | Designations[i].bus;
			info.connectors	= connectors;
			Channels.PushBack(info);
		}
	}

	return Channels.Count() - startCount;
}


uint32
AudioControlInterface::GetBusChannelsDescription(
		Vector<multi_channel_info>& Channels, multi_description* Description)
{
	uint32 addedChannels = 0;
//	multi_channel_info* Channels = Description->channels;

	// first iterate output channels
	for (AudioControlsIterator I = fOutputTerminals.Begin();
			I != fOutputTerminals.End(); I++) {
		_AudioControl* control = I->Value();
		if (static_cast<_Terminal*>(control)->IsUSBIO())
			continue;

		AudioChannelCluster* cluster = control->OutCluster();
		if (cluster == 0 || cluster->ChannelsCount() <= 0) {
			TRACE(ERR, "Terminal #%d ignored due null "
				"channels cluster (%08x)\n", control->ID(), cluster);
			continue;
		}

		uint32 channels = GetTerminalChannels(Channels,
			cluster, B_MULTI_OUTPUT_BUS);

		Description->output_bus_channel_count += channels;
		addedChannels += channels;
	}

	// input channels should follow too
	for (AudioControlsIterator I = fInputTerminals.Begin();
			I != fInputTerminals.End(); I++) {
		_AudioControl* control = I->Value();
		if (static_cast<_Terminal*>(control)->IsUSBIO())
			continue;

		AudioChannelCluster* cluster = control->OutCluster();
		if (cluster == 0 || cluster->ChannelsCount() <= 0) {
			TRACE(ERR, "Terminal #%d ignored due null "
				"channels cluster (%08x)\n", control->ID(), cluster);
			continue;
		}

		uint32 channels = GetTerminalChannels(Channels,
			cluster, B_MULTI_INPUT_BUS);

		Description->input_bus_channel_count += channels;
		addedChannels += channels;
	}

	return addedChannels;
}


void
AudioControlInterface::_HarvestRecordFeatureUnits(_AudioControl* rootControl,
		AudioControlsMap& Map)
{
	if (rootControl == 0) {
		TRACE(ERR, "Not processing due NULL root control.\n");
		return;
	}

	switch(rootControl->SubType()) {
		case USB_AUDIO_AC_OUTPUT_TERMINAL:
			_HarvestRecordFeatureUnits(Find(rootControl->SourceID()), Map);
			break;

		case USB_AUDIO_AC_SELECTOR_UNIT:
			{
				SelectorUnit* unit = static_cast<SelectorUnit*>(rootControl);
				for (int i = 0; i < unit->fInputPins.Count(); i++)
					_HarvestRecordFeatureUnits(Find(unit->fInputPins[i]), Map);
				Map.Put(rootControl->ID(), rootControl);
			}
			break;

		case USB_AUDIO_AC_FEATURE_UNIT:
			Map.Put(rootControl->ID(), rootControl);
			break;
	}
}


void
AudioControlInterface::_InitGainLimits(multi_mix_control& Control)
{
	struct _GainInfo {
		uint8	request;
		int16	data;
		float&	value;
	} gainInfos[] = {
		{ USB_AUDIO_GET_MIN, 0, Control.gain.min_gain },
		{ USB_AUDIO_GET_MAX, 0, Control.gain.max_gain },
		{ USB_AUDIO_GET_RES, 0, Control.gain.granularity }
	};

	Control.gain.min_gain = 0.;
	Control.gain.max_gain = 100.;
	Control.gain.granularity = 1.;

	size_t actualLength = 0;
	for (size_t i = 0; i < _countof(gainInfos); i++) {
		status_t status = gUSBModule->send_request(fDevice->USBDevice(),
			USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_CLASS,
			gainInfos[i].request, REQ_VALUE(Control.id),
			REQ_INDEX(Control.id), sizeof(gainInfos[i].data),
			&gainInfos[i].data, &actualLength);

		if (status != B_OK || actualLength != sizeof(gainInfos[i].data)) {
			TRACE(ERR, "Request %d failed:%#08x; received %d of %d\n",
				i, status, actualLength, sizeof(gainInfos[i].data));
			continue;
		}

		gainInfos[i].value = static_cast<float>(gainInfos[i].data) / 256.;
	}

	TRACE(ERR, "Control %s: from %f to %f dB, step %f dB.\n", Control.name,
		Control.gain.min_gain, Control.gain.max_gain, Control.gain.granularity);
}


uint32
AudioControlInterface::_ListFeatureUnitOption(uint32 controlType,
		int32& index, int32 parentIndex, multi_mix_control_info* Info,
		FeatureUnit* unit, uint32 channel, uint32 channels)
{
	int32 startIndex = index;
	uint32 id = 0;
	uint32 flags = 0;
	strind_id string = S_null;
	const char* name = NULL;
	bool initGainLimits = false;

	switch(controlType) {
		case BMA_CTL_MUTE:
			id = USB_AUDIO_MUTE_CONTROL;
			flags = B_MULTI_MIX_ENABLE;
			string = S_MUTE;
			break;
		case BMA_CTL_VOLUME:
			id = USB_AUDIO_VOLUME_CONTROL;
			flags = B_MULTI_MIX_GAIN;
			string = S_GAIN;
			initGainLimits = true;
			break;
		case BMA_CTL_AUTOGAIN:
			id = USB_AUDIO_AUTOMATIC_GAIN_CONTROL;
			flags = B_MULTI_MIX_ENABLE;
			name = "Auto Gain";
			break;
		default:
			TRACE(ERR, "Unsupported type %#08x ignored.\n", controlType);
			return 0;
	}

	multi_mix_control* Controls = Info->controls;

	if (unit->HasControl(channel, controlType)) {
		uint32 masterIndex = CTL_ID(id, channel, unit->ID(), fInterface);
		Controls[index].id  = masterIndex;
		Controls[index].flags = flags;
		Controls[index].parent = parentIndex;
		Controls[index].string = string;
		if (name != NULL)
			strlcpy(Controls[index].name, name, sizeof(Controls[index].name));
		if (initGainLimits)
			_InitGainLimits(Controls[index]);

		index++;

		if (channels == 2) {
			Controls[index].id = CTL_ID(id, channel + 1, unit->ID(), fInterface);
			Controls[index].flags = flags;
			Controls[index].parent = parentIndex;
			Controls[index].master = masterIndex;
			Controls[index].string = string;
			if (name != NULL)
				strlcpy(Controls[index].name, name, sizeof(Controls[index].name));
			if (initGainLimits)
				_InitGainLimits(Controls[index]);
			index++;
		}
	}

	return index - startIndex;
}


int32
AudioControlInterface::_ListFeatureUnitControl(int32& index, int32 parentIndex,
		multi_mix_control_info* Info, _AudioControl* control)
{
	FeatureUnit* unit = static_cast<FeatureUnit*>(control);
	if (unit == 0) {
		TRACE(ERR, "Feature Unit for null control ignored.\n");
		return 0;
	}

	AudioChannelCluster* cluster = unit->OutCluster();
	if (cluster == 0) {
		TRACE(ERR, "Control %s with null cluster ignored.\n", unit->Name());
		return 0;
	}

	struct _ChannelInfo {
		const char*	Name;
		uint8		channels;
		uint32		Mask;
	} channelInfos[] = {
		{ "",			1, 0 },	// Master channel entry - no bitmask
		{ "",			2, B_CHANNEL_LEFT | B_CHANNEL_RIGHT },
		{ "Left",		1, B_CHANNEL_LEFT	},
		{ "Right",		1, B_CHANNEL_RIGHT	},
		{ "Center",		1, B_CHANNEL_CENTER },
		{ "L.F.E.",		1, B_CHANNEL_SUB	},
		{ "Back",		2, B_CHANNEL_REARLEFT | B_CHANNEL_REARRIGHT },
		{ "Back Left",	1, B_CHANNEL_REARLEFT	},
		{ "Back Right",	1, B_CHANNEL_REARRIGHT	},
		{ "Front of Center",		2, B_CHANNEL_FRONT_LEFT_CENTER
										| B_CHANNEL_FRONT_RIGHT_CENTER },
		{ "Front Left of Center",	1, B_CHANNEL_FRONT_LEFT_CENTER	},
		{ "Front Right of Center",	1, B_CHANNEL_FRONT_RIGHT_CENTER	},
		{ "Back Center",		1, B_CHANNEL_BACK_CENTER },
		{ "Side",				2, B_CHANNEL_SIDE_LEFT | B_CHANNEL_SIDE_RIGHT },
		{ "Side Left",			1, B_CHANNEL_SIDE_LEFT },
		{ "Side Right",			1, B_CHANNEL_SIDE_RIGHT },
		{ "Top Center",			1, B_CHANNEL_TOP_CENTER },
		{ "Top Front Left",		1, B_CHANNEL_TOP_FRONT_LEFT },
		{ "Top Front Center",	1, B_CHANNEL_TOP_FRONT_CENTER },
		{ "Top Front Right",	1, B_CHANNEL_TOP_FRONT_RIGHT },
		{ "Top Back Left",		1, B_CHANNEL_TOP_BACK_LEFT },
		{ "Top Back Center",	1, B_CHANNEL_TOP_BACK_CENTER },
		{ "Top Back Right",		1, B_CHANNEL_TOP_BACK_RIGHT }
	};

	multi_mix_control* Controls = Info->controls;

	uint32 channelsConfig = cluster->ChannelsConfig();
	int32 groupIndex = 0;
	int32 channel = 0;
	int32 masterIndex = 0;	// in case master channel has no volume
							// control - add following "L+R" channels into it

	for (size_t i = 0; i < _countof(channelInfos) /*&& channelsConfig > 0*/; i++) {
		if ((channelsConfig & channelInfos[i].Mask) != channelInfos[i].Mask) {
			// ignore non-listed and possibly non-paired stereo channels.
			//	note that master channel with zero mask pass this check! ;-)
			continue;
		}

		if (masterIndex == 0) {
			groupIndex = index;
			Controls[index].id = groupIndex;
			Controls[index].flags = B_MULTI_MIX_GROUP;
			Controls[index].parent = parentIndex;
			snprintf(Controls[index].name, sizeof(Controls[index].name),
				"%s %s", unit->Name(), channelInfos[i].Name);
			index++;
		} else {
			groupIndex = masterIndex;
			masterIndex = 0;
		}

		// First list possible Mute controls
		_ListFeatureUnitOption(BMA_CTL_MUTE, index, groupIndex, Info,
				unit, channel, channelInfos[i].channels);

		// Gain controls may be usefull too
		if (_ListFeatureUnitOption(BMA_CTL_VOLUME, index, groupIndex, Info,
				unit, channel, channelInfos[i].channels) == 0) {
			masterIndex = (i == 0) ? groupIndex : 0 ;
			TRACE(UAC, "channel:%d set master index to %d\n",
				channel, masterIndex);
		}

		// Auto Gain checkbox will be listed too
		_ListFeatureUnitOption(BMA_CTL_AUTOGAIN, index, groupIndex, Info,
			unit, channel, channelInfos[i].channels);

		// Now check if the group filled with something usefull.
		// In case no controls were added into it - "remove" it
		if (Controls[index - 1].flags == B_MULTI_MIX_GROUP) {
			Controls[index - 1].id = 0;
			index--;

			masterIndex = 0;
		}

		channel += channelInfos[i].channels;

		// remove bits for already processed channels - this prevent from
		// duplication of the stereo channels as "orphaned" ones and optimize
		// exit from this iterations after all channels are processed.
		channelsConfig &= ~channelInfos[i].Mask;

		if (0 == channelsConfig)
			break;
	}

	if (channelsConfig > 0)
		TRACE(ERR, "Following channels were not processed: %#08x.\n",
			channelsConfig);

	// return last group index to stick possible selector unit to it. ;-)
	return groupIndex;
}


void
AudioControlInterface::_ListSelectorUnitControl(int32& index, int32 parentGroup,
		multi_mix_control_info* Info, _AudioControl* control)
{
	SelectorUnit* selector = static_cast<SelectorUnit*>(control);
	if (selector == 0 || selector->SubType() != USB_AUDIO_AC_SELECTOR_UNIT)
		return;

	multi_mix_control* Controls = Info->controls;

	int32 recordMUX = CTL_ID(0, 0, selector->ID(), fInterface);
	Controls[index].id	= recordMUX;
	Controls[index].flags = B_MULTI_MIX_MUX;
	Controls[index].parent = parentGroup;
	Controls[index].string = S_null;
	strlcpy(Controls[index].name, "Source", sizeof(Controls[index].name));
	index++;

	for (int i = 0; i < selector->fInputPins.Count(); i++) {
		Controls[index].id = CTL_ID(0, 1, selector->ID(), fInterface);
		Controls[index].flags = B_MULTI_MIX_MUX_VALUE;
		Controls[index].master = 0;
		Controls[index].string = S_null;
		Controls[index].parent = recordMUX;
		_AudioControl* control = Find(selector->fInputPins[i]);
		if (control != NULL)
			strlcpy(Controls[index].name,
				control->Name(), sizeof(Controls[index].name));
		else
			snprintf(Controls[index].name,
				sizeof(Controls[index].name), "Input #%d", i + 1);
		index++;
	}
}


void
AudioControlInterface::_ListMixControlsPage(int32& index,
		multi_mix_control_info* Info, AudioControlsMap& Map, const char* Name)
{
	multi_mix_control* Controls = Info->controls;
	int32 groupIndex = index | 0x10000;
	Controls[index].id	= groupIndex;
	Controls[index].flags = B_MULTI_MIX_GROUP;
	Controls[index].parent = 0;
	strlcpy(Controls[index].name, Name, sizeof(Controls[index].name));
	index++;

	int32 group = groupIndex;
	for (AudioControlsIterator I = Map.Begin(); I != Map.End(); I++) {
		TRACE(UAC, "%s control %d listed.\n", Name, I->Value()->ID());
		switch(I->Value()->SubType()) {
			case USB_AUDIO_AC_FEATURE_UNIT:
				group = _ListFeatureUnitControl(index, groupIndex,
					Info, I->Value());
				break;
			case USB_AUDIO_AC_SELECTOR_UNIT:
				_ListSelectorUnitControl(index, group, Info, I->Value());
				break;
		}
	}
}


status_t
AudioControlInterface::ListMixControls(multi_mix_control_info* Info)
{
	// first harvest feature units that assigned to output USB I/O terminal(s)
	AudioControlsMap RecordControlsMap;

	for (AudioControlsIterator I = fOutputTerminals.Begin();
			I != fOutputTerminals.End(); I++) {
		_Terminal* terminal = static_cast<_Terminal*>(I->Value());
		if (terminal->IsUSBIO())
			_HarvestRecordFeatureUnits(terminal, RecordControlsMap);
	}

	AudioControlsMap InputControlsMap;
	AudioControlsMap OutputControlsMap;

	for (AudioControlsIterator I = fAudioControls.Begin();
			I != fAudioControls.End(); I++) {
		_AudioControl* control = I->Value();
		// filter out feature units
		if (control->SubType() != USB_AUDIO_AC_FEATURE_UNIT)
			continue;

		// ignore controls that are already in the record controls map
		if (RecordControlsMap.Find(control->ID()) != RecordControlsMap.End())
			continue;

		_AudioControl* sourceControl = Find(control->SourceID());
		if (sourceControl != 0
				&& sourceControl->SubType() == USB_AUDIO_AC_INPUT_TERMINAL)
			InputControlsMap.Put(control->ID(), control);
		else
			OutputControlsMap.Put(control->ID(), control);
	}

	int32 index = 0;
	if (InputControlsMap.Count() > 0)
		_ListMixControlsPage(index, Info, InputControlsMap, "Input");

	if (OutputControlsMap.Count() > 0)
		_ListMixControlsPage(index, Info, OutputControlsMap, "Output");

	if (RecordControlsMap.Count() > 0)
		_ListMixControlsPage(index, Info, RecordControlsMap, "Record");

	return B_OK;
}


status_t
AudioControlInterface::GetMix(multi_mix_value_info* Info)
{
	for (int32 i = 0; i < Info->item_count; i++) {
		uint16 length = 0;
		int16 data = 0;
		switch(CS_FROM_CTLID(Info->values[i].id)) {
			case USB_AUDIO_VOLUME_CONTROL:
				length = 2;
				break;
			case 0: // Selector Unit
			case USB_AUDIO_MUTE_CONTROL:
			case USB_AUDIO_AUTOMATIC_GAIN_CONTROL:
				length = 1;
				break;
			default:
				TRACE(ERR, "Unsupported control type %#02x ignored.\n",
					CS_FROM_CTLID(Info->values[i].id));
				continue;
		}

		size_t actualLength = 0;
		status_t status = gUSBModule->send_request(fDevice->USBDevice(),
			USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_CLASS, USB_AUDIO_GET_CUR,
			REQ_VALUE(Info->values[i].id), REQ_INDEX(Info->values[i].id),
			length, &data, &actualLength);

		if (status != B_OK || actualLength != length) {
			TRACE(ERR, "Request failed:%#08x; received %d of %d\n",
				status, actualLength, length);
			continue;
		}

		switch(CS_FROM_CTLID(Info->values[i].id)) {
			case USB_AUDIO_VOLUME_CONTROL:
				Info->values[i].gain = static_cast<float>(data) / 256.;
				TRACE(MIX, "Gain control %d; channel: %d; is %f dB.\n",
					ID_FROM_CTLID(Info->values[i].id),
					CN_FROM_CTLID(Info->values[i].id),
					Info->values[i].gain);
				break;
			case USB_AUDIO_MUTE_CONTROL:
				Info->values[i].enable = data > 0;
				TRACE(MIX, "Mute control %d; channel: %d; is %d.\n",
					ID_FROM_CTLID(Info->values[i].id),
					CN_FROM_CTLID(Info->values[i].id),
					Info->values[i].enable);
				break;
			case USB_AUDIO_AUTOMATIC_GAIN_CONTROL:
				Info->values[i].enable = data > 0;
				TRACE(MIX, "AGain control %d; channel: %d; is %d.\n",
					ID_FROM_CTLID(Info->values[i].id),
					CN_FROM_CTLID(Info->values[i].id),
					Info->values[i].enable);
				break;
			case 0: // Selector Unit
				Info->values[i].mux = data - 1;
				TRACE(MIX, "Selector control %d; is %d.\n",
					ID_FROM_CTLID(Info->values[i].id),
					Info->values[i].mux);
				break;
			default:
				break;
		}
	}

	return B_OK;
}


status_t
AudioControlInterface::SetMix(multi_mix_value_info* Info)
{
	for (int32 i = 0; i < Info->item_count; i++) {
		uint16 length = 0;
		int16 data = 0;

		switch(CS_FROM_CTLID(Info->values[i].id)) {
			case USB_AUDIO_VOLUME_CONTROL:
				data = static_cast<int16>(Info->values[i].gain * 256.);
				length = 2;
				TRACE(MIX, "Gain control %d; channel: %d; about to set to %f dB.\n",
					ID_FROM_CTLID(Info->values[i].id),
					CN_FROM_CTLID(Info->values[i].id),
					Info->values[i].gain);
				break;
			case USB_AUDIO_MUTE_CONTROL:
				data = (Info->values[i].enable ? 1 : 0);
				length = 1;
				TRACE(MIX, "Mute control %d; channel: %d; about to set to %d.\n",
					ID_FROM_CTLID(Info->values[i].id),
					CN_FROM_CTLID(Info->values[i].id),
					Info->values[i].enable);
				break;
			case USB_AUDIO_AUTOMATIC_GAIN_CONTROL:
				data = (Info->values[i].enable ? 1 : 0);
				length = 1;
				TRACE(MIX, "AGain control %d; channel: %d; about to set to %d.\n",
					ID_FROM_CTLID(Info->values[i].id),
					CN_FROM_CTLID(Info->values[i].id),
					Info->values[i].enable);
				break;
			case 0: // Selector Unit
				data = Info->values[i].mux + 1;
				length = 1;
				TRACE(MIX, "Selector Control %d about to set to %d.\n",
					ID_FROM_CTLID(Info->values[i].id),
					Info->values[i].mux);
				break;
			default:
				TRACE(ERR, "Unsupported control type %#02x ignored.\n",
					CS_FROM_CTLID(Info->values[i].id));
				continue;
		}

		size_t actualLength = 0;
		status_t status = gUSBModule->send_request(fDevice->USBDevice(),
			USB_REQTYPE_INTERFACE_OUT | USB_REQTYPE_CLASS, USB_AUDIO_SET_CUR,
			REQ_VALUE(Info->values[i].id), REQ_INDEX(Info->values[i].id),
			length, &data, &actualLength);

		if (status != B_OK || actualLength != length) {
			TRACE(ERR, "Request failed:%#08x; send %d of %d\n",
				status, actualLength, length);
			continue;
		}

		TRACE(MIX, "Value set OK\n");
	}

	return B_OK;
}

