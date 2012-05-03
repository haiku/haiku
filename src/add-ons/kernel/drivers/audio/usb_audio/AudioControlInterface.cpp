/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the tems of the MIT license.
 *
 */

#include "AudioControlInterface.h"
#include "Settings.h"
#include "Device.h"
#include "audio.h"


enum TerminalTypes {
	// USB Terminal Types
	UndefinedUSB_IO		= 0x100,
	StreamingUSB_IO		= 0x101,
	VendorUSB_IO		= 0x1ff,
	// Input Terminal Types
	Undefined_In		= 0x200,
	Microphone_In		= 0x201,
	DesktopMic_In		= 0x202,
	PersonalMic_In		= 0x203,
	OmniMic_In			= 0x204,
	MicsArray_In		= 0x205,
	ProcMicsArray_In	= 0x206,
	// Output Terminal Types
	Undefined_Out		= 0x300,
	Speaker_Out			= 0x301,
	HeadPhones_Out		= 0x302,
	HMDAudio_Out		= 0x303,
	DesktopSpeaker		= 0x304,
	RoomSpeaker			= 0x305,
	CommSpeaker			= 0x306,
	LFESpeaker			= 0x307,
	// Bi-directional Terminal Types
	Undefined_IO		= 0x400,
	Handset_IO			= 0x401,
	Headset_IO			= 0x402,
	SpeakerPhone_IO		= 0x403,
	SpeakerPhoneES_IO	= 0x404,
	SpeakerPhoneEC_IO	= 0x405,
	// Telephony Terminal Types
	UndefTelephony_IO	= 0x500,
	PhoneLine_IO		= 0x501,
	Telephone_IO		= 0x502,
	DownLinePhone_IO	= 0x503,
	// External Terminal Types
	UndefinedExt_IO		= 0x600,
	AnalogConnector_IO	= 0x601,
	DAInterface_IO		= 0x602,
	LineConnector_IO	= 0x603,
	LegacyConnector_IO	= 0x604,
	SPDIFInterface_IO	= 0x605,
	DA1394Stream_IO		= 0x606,
	DV1394StreamSound_IO= 0x607,
	ADATLightpipe_IO	= 0x608,
	TDIF_IO				= 0x609,
	MADI_IO				= 0x60a,
	// Embedded Terminal Types
	UndefEmbedded_IO	= 0x700,
	LCNoiseSource_Out	= 0x701,
	EqualizationNoise_Out	= 0x702,
	CDPlayer_In			= 0x703,
	DAT_IO				= 0x704,
	DCC_IO				= 0x705,
	MiniDisk_IO			= 0x706,
	AnalogTape_IO		= 0x707,
	Phonograph_In		= 0x708,
	VCRAudio_In			= 0x709,
	VideoDiscAudio_In	= 0x70a,
	DVDAudio_In			= 0x70b,
	TVTunerAudio_In		= 0x70c,
	SatReceiverAudio_In	= 0x70d,
	CableTunerAudio_In	= 0x70e,
	DSSAudio_In			= 0x70f,
	Radio_Receiver_In	= 0x710,
	RadioTransmitter_In	= 0x711,
	MultiTrackRecorder_IO	= 0x712,
	Synthesizer_IO		= 0x713,
	Piano_IO			= 0x714,
	Guitar_IO			= 0x715,
	Drums_IO			= 0x716,
	Instrument_IO		= 0x717
};


const char*
GetTerminalDescription(uint16 TerminalType)
{
	switch(TerminalType) {
		// USB Terminal Types
		case UndefinedUSB_IO:		return "USB I/O";
		case StreamingUSB_IO:		return "USB Streaming I/O";
		case VendorUSB_IO:			return "Vendor USB I/O";
		// Input Terminal Types
		case Undefined_In:			return "Undefined Input";
		case Microphone_In:			return "Microphone";
		case DesktopMic_In:			return "Desktop Microphone";
		case PersonalMic_In:		return "Personal Microphone";
		case OmniMic_In:			return "Omni-directional Mic";
		case MicsArray_In:			return "Microphone Array";
		case ProcMicsArray_In:		return "Processing Mic Array";
		// Output Terminal Types
		case Undefined_Out:			return "Undefined Output";
		case Speaker_Out:			return "Speaker";
		case HeadPhones_Out:		return "Headphones";
		case HMDAudio_Out:			return "Head Mounted Disp.Audio";
		case DesktopSpeaker:		return "Desktop Speaker";
		case RoomSpeaker:			return "Room Speaker";
		case CommSpeaker:			return "Communication Speaker";
		case LFESpeaker:			return "LFE Speaker";
		// Bi-directional Terminal Types
		case Undefined_IO:			return "Undefined I/O";
		case Handset_IO:			return "Handset";
		case Headset_IO:			return "Headset";
		case SpeakerPhone_IO:		return "Speakerphone";
		case SpeakerPhoneES_IO:		return "Echo-supp Speakerphone";
		case SpeakerPhoneEC_IO:		return "Echo-cancel Speakerphone";
		// Telephony Terminal Types
		case UndefTelephony_IO:		return "Undefined Telephony";
		case PhoneLine_IO:			return "Phone Line";
		case Telephone_IO:			return "Telephone";
		case DownLinePhone_IO:		return "Down Line Phone";
		// External Terminal Types
		case UndefinedExt_IO:		return "Undefined External I/O";
		case AnalogConnector_IO:	return "Analog Connector";
		case DAInterface_IO:		return "Digital Audio Interface";
		case LineConnector_IO:		return "Line Connector";
		case LegacyConnector_IO:	return "LegacyAudioConnector";
		case SPDIFInterface_IO:		return "S/PDIF Interface";
		case DA1394Stream_IO:		return "1394 DA Stream";
		case DV1394StreamSound_IO:	return "1394 DV Stream Soundtrack";
		case ADATLightpipe_IO:		return "Alesis DAT Stream";
		case TDIF_IO:				return "Tascam Digital Interface";
		case MADI_IO:				return "AES Multi-channel interface";
		// Embedded Terminal Types
		case UndefEmbedded_IO:		return "Undefined Embedded I/O";
		case LCNoiseSource_Out:		return "Level Calibration Noise Source";
		case EqualizationNoise_Out:	return "Equalization Noise";
		case CDPlayer_In:			return "CD Player";
		case DAT_IO:				return "DAT";
		case DCC_IO:				return "DCC";
		case MiniDisk_IO:			return "Mini Disk";
		case AnalogTape_IO:			return "Analog Tape";
		case Phonograph_In:			return "Phonograph";
		case VCRAudio_In:			return "VCR Audio";
		case VideoDiscAudio_In:		return "Video Disc Audio";
		case DVDAudio_In:			return "DVD Audio";
		case TVTunerAudio_In:		return "TV Tuner Audio";
		case SatReceiverAudio_In:	return "Satellite Receiver Audio";
		case CableTunerAudio_In:	return "Cable Tuner Audio";
		case DSSAudio_In:			return "DSS Audio";
		case Radio_Receiver_In:		return "Radio Receiver";
		case RadioTransmitter_In:	return "Radio Transmitter";
		case MultiTrackRecorder_IO:	return "Multi-track Recorder";
		case Synthesizer_IO:		return "Synthesizer";
		case Piano_IO:				return "Piano";
		case Guitar_IO:				return "Guitar";
		case Drums_IO:				return "Percussion Instrument";
		case Instrument_IO:			return "Musical Instrument";
	}

	TRACE_ALWAYS("Unknown Terminal Type: %#06x", TerminalType);
	return "Unknown";
}


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
	if (SourceID() == 0 || fInterface == NULL) {
		return NULL;
	}

	_AudioControl* control = fInterface->Find(SourceID());
	if (control == NULL) {
		return NULL;
	}

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
	return GetTerminalDescription(fTerminalType);
}


bool
_Terminal::IsUSBIO()
{
	return (fTerminalType & 0xff00) == UndefinedUSB_IO;
}


InputTerminal::InputTerminal(AudioControlInterface*	interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioChannelCluster<_Terminal>(interface, Header)
{
	usb_input_terminal_descriptor_r1* Terminal
		= (usb_input_terminal_descriptor_r1*) Header;
	fID					= Terminal->terminal_id;
	fTerminalType		= Terminal->terminal_type;
	fAssociatedTerminal = Terminal->assoc_terminal;

	TRACE("Input Terminal ID:%d >>>\n",	fID);
	TRACE("Terminal type:%s (%#06x)\n",
				GetTerminalDescription(fTerminalType), fTerminalType);
	TRACE("Assoc.terminal:%d\n",	fAssociatedTerminal);

	if (fInterface->SpecReleaseNumber() < 0x200) {
		fOutChannelsNumber	= Terminal->num_channels;
		fChannelsConfig		= Terminal->channel_config;
		fChannelNames		= Terminal->channel_names;
		fStringIndex		= Terminal->terminal;
	} else {
		usb_input_terminal_descriptor* Terminal
			= (usb_input_terminal_descriptor*) Header;
		fClockSourceId		= Terminal->clock_source_id;
		fOutChannelsNumber	= Terminal->num_channels;
		fChannelsConfig		= Terminal->channel_config;
		fChannelNames		= Terminal->channel_names;
		fControlsBitmap		= Terminal->bm_controls;
		fStringIndex		= Terminal->terminal;

		TRACE("Clock Source ID:%d\n", fClockSourceId);
		TRACE("Controls Bitmap:%#04x\n", fControlsBitmap);
	}

	TRACE("Out.channels num:%d\n",	 fOutChannelsNumber);
	TRACE("Channels config:%#06x\n", fChannelsConfig);
	TRACE("Channels names:%d\n",	 fChannelNames);
	TRACE("StringIndex:%d\n",		 fStringIndex);

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
	usb_output_terminal_descriptor_r1* Terminal
		= (usb_output_terminal_descriptor_r1*) Header;

	fID					= Terminal->terminal_id;
	fTerminalType		= Terminal->terminal_type;
	fAssociatedTerminal	= Terminal->assoc_terminal;
	fSourceID			= Terminal->source_id;

	TRACE("Output Terminal ID:%d >>>\n",	fID);
	TRACE("Terminal type:%s (%#06x)\n",
				GetTerminalDescription(fTerminalType), fTerminalType);
	TRACE("Assoc.terminal:%d\n",		fAssociatedTerminal);
	TRACE("Source ID:%d\n",				fSourceID);

	if (fInterface->SpecReleaseNumber() < 0x200) {
		fStringIndex = Terminal->terminal;
	} else {
		usb_output_terminal_descriptor* Terminal
			= (usb_output_terminal_descriptor*) Header;

		fClockSourceId	= Terminal->clock_source_id;
		fControlsBitmap	= Terminal->bm_controls;
		fStringIndex	= Terminal->terminal;

		TRACE("Clock Source ID:%d\n", fClockSourceId);
		TRACE("Controls Bitmap:%#04x\n", fControlsBitmap);
	}

	TRACE("StringIndex:%d\n", fStringIndex);

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
	usb_mixer_unit_descriptor* Mixer
		= (usb_mixer_unit_descriptor*) Header;

	fID = Mixer->unit_id;
	TRACE("Mixer ID:%d >>>\n", fID);

	TRACE("Number of input pins:%d\n", Mixer->num_input_pins);
	for (size_t i = 0; i < Mixer->num_input_pins; i++) {
		fInputPins.PushBack(Mixer->input_pins[i]);
		TRACE("Input pin #%d:%d\n", i, fInputPins[i]);
	}

	uint8* mixerControlsData = NULL;
	uint8 mixerControlsSize = 0;

	if (fInterface->SpecReleaseNumber() < 0x200) {
		usb_output_channels_descriptor_r1* OutChannels
			= (usb_output_channels_descriptor_r1*)
				&Mixer->input_pins[Mixer->num_input_pins];

		fOutChannelsNumber	= OutChannels->num_output_pins;
		fChannelsConfig		= OutChannels->channel_config;
		fChannelNames		= OutChannels->channel_names;

		mixerControlsData = (uint8*) ++OutChannels;
		mixerControlsSize = Mixer->length - 10 - Mixer->num_input_pins;
		fStringIndex = *(mixerControlsData + mixerControlsSize);
	} else {
		usb_output_channels_descriptor* OutChannels
			= (usb_output_channels_descriptor*)
				&Mixer->input_pins[Mixer->num_input_pins];

		fOutChannelsNumber	= OutChannels->num_output_pins;
		fChannelsConfig		= OutChannels->channel_config;
		fChannelNames		= OutChannels->channel_names;

		mixerControlsData = (uint8*) ++OutChannels;
		mixerControlsSize = Mixer->length - 13 - Mixer->num_input_pins;
		fControlsBitmap = *(mixerControlsData + mixerControlsSize);
		fStringIndex = *(mixerControlsData + mixerControlsSize + 1);

		TRACE("Control Bitmap:%#04x\n", fControlsBitmap);
	}

	TRACE("Out channels number:%d\n",		fOutChannelsNumber);
	TRACE("Out channels config:%#06x\n",	fChannelsConfig);
	TRACE("Out channels names:%d\n",		fChannelNames);
	TRACE("Controls Size:%d\n", mixerControlsSize);

	for (size_t i = 0; i < mixerControlsSize; i++) {
		fProgrammableControls.PushBack(mixerControlsData[i]);
		TRACE("Controls Data[%d]:%#x\n", i, fProgrammableControls[i]);
	}

	TRACE("StringIndex:%d\n", fStringIndex);

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
	usb_selector_unit_descriptor* Selector
		= (usb_selector_unit_descriptor*) Header;

	fID = Selector->unit_id;
	TRACE("Selector ID:%d >>>\n", fID);

	TRACE("Number of input pins:%d\n", Selector->num_input_pins);
	for (size_t i = 0; i < Selector->num_input_pins; i++) {
		fInputPins.PushBack(Selector->input_pins[i]);
		TRACE("Input pin #%d:%d\n", i, fInputPins[i]);
	}

	if (fInterface->SpecReleaseNumber() < 0x200) {

		fStringIndex = Selector->input_pins[Selector->num_input_pins];
	} else {

		fControlsBitmap = Selector->input_pins[Selector->num_input_pins];
		fStringIndex = Selector->input_pins[Selector->num_input_pins + 1];

		TRACE("Controls Bitmap:%d\n", fControlsBitmap);
	}

	TRACE("StringIndex:%d\n", fStringIndex);

	fStatus = B_OK;
}


SelectorUnit::~SelectorUnit()
{
}


AudioChannelCluster*
SelectorUnit::OutCluster()
{
	if (fInterface == NULL) {
		return NULL;
	}

	for (int i = 0; i < fInputPins.Count(); i++) {
		_AudioControl* control = fInterface->Find(fInputPins[i]);
		if (control == NULL) {
			continue;
		}

		if (control->OutCluster() != NULL) {
			return control->OutCluster();
		}
	}

	return NULL;
}


FeatureUnit::FeatureUnit(AudioControlInterface*	interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(interface, Header)
{
	usb_feature_unit_descriptor* Feature
		= (usb_feature_unit_descriptor*) Header;

	fID = Feature->unit_id;
	TRACE("Feature ID:%d >>>\n", fID);

	fSourceID = Feature->source_id;
	TRACE("Source ID:%d\n", fSourceID);

	uint8 controlSize = 4;
	uint8 channelsCount = (Feature->length - 6) / controlSize;
	uint8 *ControlsBitmapPointer = (uint8*)&Feature->bma_controls[0];

	if (fInterface->SpecReleaseNumber() < 0x200) {
		usb_feature_unit_descriptor_r1* Feature
			= (usb_feature_unit_descriptor_r1*) Header;
		controlSize = Feature->control_size;
		channelsCount = (Feature->length - 7) / Feature->control_size;
		ControlsBitmapPointer = &Feature->bma_controls[0];
	}

	TRACE("Channel bitmap size:%d\n", controlSize);
	TRACE("Channels number:%d\n", channelsCount - 1); // not add master!

	for (size_t i = 0; i < channelsCount; i++) {
		uint8 *controlPointer = &ControlsBitmapPointer[i * controlSize];
		switch(controlSize) {
			case 1: fControlBitmaps.PushBack(*controlPointer); break;
			case 2: fControlBitmaps.PushBack(*(uint16*)controlPointer); break;
			case 4: fControlBitmaps.PushBack(*(uint32*)controlPointer); break;
			default:
				TRACE_ALWAYS("Feature control of unsupported size %d ignored\n",
														controlSize);
				continue;
		}

		NormalizeAndTraceChannel(i);
	}

	fStringIndex = ControlsBitmapPointer[channelsCount * controlSize];
	TRACE("StringIndex:%d\n", fStringIndex);

	fStatus = B_OK;
}


FeatureUnit::~FeatureUnit()
{
}


const char*
FeatureUnit::Name()
{
	// first check if source of this FU is an input terminal
	_AudioControl *control = fInterface->Find(fSourceID);
	while (control != 0) {

		if (control->SubType() != IDSInputTerminal)
			break;

		// USB I/O terminal is a not good candidate to use it's name
		if (static_cast<_Terminal*>(control)->IsUSBIO())
			break;

		// use the name of source input terminal as name of this FU
		return control->Name();
	}

	// check if output of this FU is connected to output terminal
	control = fInterface->FindOutputTerminal(fID);
	while (control != 0) {

		if (control->SubType() != IDSOutputTerminal)
			break;

		// USB I/O terminal is a not good candidate to use it's name
		if (static_cast<_Terminal*>(control)->IsUSBIO())
			break;

		// use the name of this output terminal as name of this FU
		return control->Name();
	}

	return "Unknown";
}


bool
FeatureUnit::HasControl(int32 Channel, uint32 Control)
{
	if (Channel >= fControlBitmaps.Count()) {
		TRACE_ALWAYS("Out of limits error of retrieving control %#010x "
								"for channel %d\n", Control, Channel);
		return false;
	}

	return (Control & fControlBitmaps[Channel]) != 0;
}


void
FeatureUnit::NormalizeAndTraceChannel(int32 Channel)
{
	if (Channel >= fControlBitmaps.Count()) {
		TRACE_ALWAYS("Out of limits error of tracing channel %d\n", Channel);
		return;
	}

	struct _RemapInfo {
		uint32	rev1Bits;
		uint32	rev2Bits;
		const char* name;
	} remapInfos[] = {
		{ MuteControl1,		MuteControl,		"Mute"		},
		{ VolumeControl1,	VolumeControl,		"Volume"	},
		{ BassControl1,		BassControl,		"Bass"		},
		{ MidControl1,		MidControl,			"Mid"		},
		{ TrebleControl1,	TrebleControl,		"Treble"	},
		{ GraphEqControl1,	GraphEqControl,		"Graphic Equalizer"	},
		{ AutoGainControl1,	AutoGainControl,	"Automatic Gain"	},
		{ DelayControl1,		DelayControl,		"Delay"			},
		{ BassBoostControl1,	BassBoostControl,	"Bass Boost"	},
		{ LoudnessControl1,		LoudnessControl,	"Loudness"		},
		{ 0,				InputGainControl,		"InputGain"		},
		{ 0,				InputGainPadControl,	"InputGainPad"	},
		{ 0,				PhaseInverterControl,	"PhaseInverter"	},
		{ 0,				UnderflowControl,		"Underflow"		},
		{ 0,				OverflowControl,		"Overflow"		}
	};

	if (Channel == 0)
		TRACE("Master channel bitmap:%#x\n", fControlBitmaps[Channel]);
	else
		TRACE("Channel %d bitmap:%#x\n", Channel, fControlBitmaps[Channel]);

	bool isRev1 = (fInterface->SpecReleaseNumber() < 0x200);

	uint32 remappedBitmap = 0;
	for (size_t i = 0; i < _countof(remapInfos); i++) {
		uint32 bits = isRev1 ? remapInfos[i].rev1Bits : remapInfos[i].rev2Bits;
		if ((fControlBitmaps[Channel] & bits) > 0) {
			if (isRev1) {
				remappedBitmap |= remapInfos[i].rev2Bits;
			}
			TRACE("\t%s\n", remapInfos[i].name);
		}
	}

	if (isRev1) {
		TRACE("\t%#08x -> %#08x.\n", fControlBitmaps[Channel], remappedBitmap);
		fControlBitmaps[Channel] = remappedBitmap;
	}
}


EffectUnit::EffectUnit(AudioControlInterface*	interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(interface, Header)
{
	usb_input_terminal_descriptor* D
		= (usb_input_terminal_descriptor*) Header;
	TRACE("Effect Unit:%d >>>\n",	D->terminal_id);
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
	usb_processing_unit_descriptor* Processing
		= (usb_processing_unit_descriptor*) Header;

	fID = Processing->unit_id;
	TRACE("Processing ID:%d >>>\n", fID);

	fProcessType = Processing->process_type;
	TRACE("Processing Type:%d\n", fProcessType);

	TRACE("Number of input pins:%d\n", Processing->num_input_pins);
	for (size_t i = 0; i < Processing->num_input_pins; i++) {
		fInputPins.PushBack(Processing->input_pins[i]);	
		TRACE("Input pin #%d:%d\n", i, fInputPins[i]);
	}

	if (fInterface->SpecReleaseNumber() < 0x200) {
		usb_output_channels_descriptor_r1* OutChannels
			= (usb_output_channels_descriptor_r1*)
				&Processing->input_pins[Processing->num_input_pins];

		fOutChannelsNumber	= OutChannels->num_output_pins;
		fChannelsConfig		= OutChannels->channel_config;
		fChannelNames		= OutChannels->channel_names;
	} else {
		usb_output_channels_descriptor* OutChannels
			= (usb_output_channels_descriptor*)
				&Processing->input_pins[Processing->num_input_pins];

		fOutChannelsNumber	= OutChannels->num_output_pins;
		fChannelsConfig		= OutChannels->channel_config;
		fChannelNames		= OutChannels->channel_names;
	}

	TRACE("Out channels number:%d\n",		fOutChannelsNumber);
	TRACE("Out channels config:%#06x\n",	fChannelsConfig);
	TRACE("Out channels names:%d\n",		fChannelNames);
	/*
	uint8 controlsSize = Processing->length - 10 - Processing->num_input_pins;
	TRACE("Controls Size:%d\n", controlsSize);

	uint8* controlsData = (uint8*) ++OutChannels;

	for (size_t i = 0; i < controlsSize; i++) {
		fProgrammableControls.PushBack(controlsData[i]);
		TRACE("Controls Data[%d]:%#x\n", i, controlsData[i]);
	}

	fStringIndex = *(controlsData + controlsSize);

	TRACE("StringIndex:%d\n", fStringIndex);
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
	usb_extension_unit_descriptor* Extension
		= (usb_extension_unit_descriptor*) Header;

	fID = Extension->unit_id;
	TRACE("Extension ID:%d >>>\n", fID);

	fExtensionCode = Extension->extension_code;
	TRACE("Extension Type:%d\n", fExtensionCode);

	TRACE("Number of input pins:%d\n", Extension->num_input_pins);
	for (size_t i = 0; i < Extension->num_input_pins; i++) {
		fInputPins.PushBack(Extension->input_pins[i]);
		TRACE("Input pin #%d:%d\n", i, fInputPins[i]);
	}

	if (fInterface->SpecReleaseNumber() < 0x200) {
		usb_output_channels_descriptor_r1* OutChannels
			= (usb_output_channels_descriptor_r1*)
				&Extension->input_pins[Extension->num_input_pins];

		fOutChannelsNumber	= OutChannels->num_output_pins;
		fChannelsConfig		= OutChannels->channel_config;
		fChannelNames		= OutChannels->channel_names;
	} else {
		usb_output_channels_descriptor* OutChannels
			= (usb_output_channels_descriptor*)
				&Extension->input_pins[Extension->num_input_pins];

		fOutChannelsNumber	= OutChannels->num_output_pins;
		fChannelsConfig		= OutChannels->channel_config;
		fChannelNames		= OutChannels->channel_names;
	}

	TRACE("Out channels number:%d\n",		fOutChannelsNumber);
	TRACE("Out channels config:%#06x\n",	fChannelsConfig);
	TRACE("Out channels names:%d\n",		fChannelNames);
	/*
	uint8 controlsSize = Processing->length - 10 - Processing->num_input_pins;
	TRACE("Controls Size:%d\n", controlsSize);

	uint8* controlsData = (uint8*) ++OutChannels;

	for (size_t i = 0; i < controlsSize; i++) {
		fProgrammableControls.PushBack(controlsData[i]);
		TRACE("Controls Data[%d]:%#x\n", i, controlsData[i]);
	}

	fStringIndex = *(controlsData + controlsSize);

	TRACE("StringIndex:%d\n", fStringIndex);
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
	usb_input_terminal_descriptor* D
		= (usb_input_terminal_descriptor*) Header;
	TRACE("Clock Source:%d >>>\n",	D->terminal_id);
}


ClockSource::~ClockSource()
{
}


ClockSelector::ClockSelector(AudioControlInterface*	interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(interface, Header)
{
	usb_input_terminal_descriptor* D
		= (usb_input_terminal_descriptor*) Header;
	TRACE("Clock Selector:%d >>>\n",	D->terminal_id);
}


ClockSelector::~ClockSelector()
{
}


ClockMultiplier::ClockMultiplier(AudioControlInterface*	interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(interface, Header)
{
	usb_input_terminal_descriptor* D
		= (usb_input_terminal_descriptor*) Header;
	TRACE("Clock Multiplier:%d >>>\n",	D->terminal_id);
}


ClockMultiplier::~ClockMultiplier()
{
}


SampleRateConverter::SampleRateConverter(AudioControlInterface*	interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(interface, Header)
{
	usb_input_terminal_descriptor* D
		= (usb_input_terminal_descriptor*) Header;
	TRACE("Sample Rate Converter:%d >>>\n",	D->terminal_id);
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
								I != fAudioControls.End(); I++) {
		delete I->Value();
	}

	fAudioControls.MakeEmpty();

	// object already freed. just purge the map
	fOutputTerminals.MakeEmpty();

	// object already freed. just purge the map
	fInputTerminals.MakeEmpty();
}


status_t
AudioControlInterface::Init(size_t interface, usb_interface_info *Interface)
{
	for (size_t i = 0; i < Interface->generic_count; i++) {
		usb_audiocontrol_header_descriptor *Header
			= (usb_audiocontrol_header_descriptor *)Interface->generic[i];

		if (Header->descriptor_type != AC_CS_INTERFACE) {
			TRACE_ALWAYS("Ignore Audio Control of "
				"unknown descriptor type %#04x.\n",	Header->descriptor_type);
			continue;
		}

		_AudioControl *control = NULL;

		switch(Header->descriptor_subtype) {
			default:
				TRACE_ALWAYS("Ignore Audio Control of unknown "
					"descriptor sub-type %#04x\n", Header->descriptor_subtype);
				break;
			case IDSUndefined:
				TRACE_ALWAYS("Ignore Audio Control of undefined sub-type\n");
				break;
			case IDSHeader:
				InitACHeader(interface, Header);
				break;
			case IDSInputTerminal:
				control = new InputTerminal(this, Header);
				break;
			case IDSOutputTerminal:
				control = new OutputTerminal(this, Header);
				break;
			case IDSMixerUnit:
				control = new MixerUnit(this, Header);
				break;
			case IDSSelectorUnit:
				control = new SelectorUnit(this, Header);
				break;
			case IDSFeatureUnit:
				control = new FeatureUnit(this, Header);
				break;
			case IDSEffectUnit:
				if (SpecReleaseNumber() < 200)
					control = new ProcessingUnit(this, Header);
				else
					control = new EffectUnit(this, Header);
				break;
			case IDSProcessingUnit:
				if (SpecReleaseNumber() < 200)
					control = new ExtensionUnit(this, Header);
				else
					control = new ProcessingUnit(this, Header);
				break;
			case IDSExtensionUnit:
				control = new ExtensionUnit(this, Header);
				break;
			case IDSClockSource:
				control = new ClockSource(this, Header);
				break;
			case IDSClockSelector:
				control = new ClockSelector(this, Header);
				break;
			case IDSClockMultiplier:
				control = new ClockMultiplier(this, Header);
				break;
			case IDSSampleRateConverter:
				control = new SampleRateConverter(this, Header);
				break;
		}

		if (control != 0 && control->InitCheck() == B_OK) {
			switch(control->SubType()) {
				case IDSOutputTerminal:
					fOutputTerminals.Put(control->SourceID(), control);
					break;
				case IDSInputTerminal:
					fInputTerminals.Put(control->ID(), control);
					break;
			}
			fAudioControls.Put(control->ID(), control);

		} else {
			delete control;
		}
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
	if (Header == NULL) {
		return fStatus = B_NO_INIT;
	}

	fInterface = interface;

	fADCSpecification = Header->bcd_release_no;
	TRACE("ADCSpecification:%#06x\n", fADCSpecification);

	if (fADCSpecification < 0x200) {
		usb_audiocontrol_header_descriptor_r1* Header1
			= (usb_audiocontrol_header_descriptor_r1*) Header;

		TRACE("InterfacesCount:%d\n",	Header1->in_collection);
		for (size_t i = 0; i < Header1->in_collection; i++) {
			fStreams.PushBack(Header1->interface_numbers[i]);
			TRACE("Interface[%d] number is %d\n", i, fStreams[i]);
		}
	} else {
		fFunctionCategory = Header->function_category;
		fControlsBitmap = Header->bm_controls;
		TRACE("Function Category:%#04x\n", fFunctionCategory);
		TRACE("Controls Bitmap:%#04x\n", fControlsBitmap);
	}

	return /*fStatus =*/ B_OK;
}


	uint32
AudioControlInterface::GetChannelsDescription(
						Vector<multi_channel_info>& Channels,
						multi_description *Description,
						AudioControlsVector &Terminals)
{
	uint32 addedChannels = 0;
//	multi_channel_info* Channels = Description->channels;

	for (int32 i = 0; i < Terminals.Count(); i++) {
		bool bIsInputTerminal = Terminals[i]->SubType() == IDSInputTerminal;

		AudioChannelCluster* cluster = Terminals[i]->OutCluster();
		if (cluster == 0 || cluster->ChannelsCount() <= 0) {
			TRACE_ALWAYS("Terminal #%d ignored due null "
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
						AudioChannelCluster* cluster, channel_kind kind,
						uint32 connectors)
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
					Vector<multi_channel_info>& Channels,
					multi_description *Description)
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
			TRACE_ALWAYS("Terminal #%d ignored due null "
					"channels cluster (%08x)\n", control->ID(), cluster);
			continue;
		}

		uint32 channels = GetTerminalChannels(Channels,
											cluster, B_MULTI_OUTPUT_BUS);

		Description->output_bus_channel_count += channels;
		addedChannels += channels;
	}

	// output channels should follow too
	for (AudioControlsIterator I = fInputTerminals.Begin();
										I != fInputTerminals.End(); I++) {

		_AudioControl* control = I->Value();
		if (static_cast<_Terminal*>(control)->IsUSBIO())
			continue;

		AudioChannelCluster* cluster = control->OutCluster();
		if (cluster == 0 || cluster->ChannelsCount() <= 0) {
			TRACE_ALWAYS("Terminal #%d ignored due null "
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
		TRACE_ALWAYS("Not processing due NULL root control.\n");
		return;
	}

	switch(rootControl->SubType()) {
		case IDSOutputTerminal:
			// _HarvestRecordFeatureUnits(Find(rootControl->SourceID()), Map);
			break;

		case IDSSelectorUnit:
			{
				SelectorUnit* unit = static_cast<SelectorUnit*>(rootControl);
				for (int i = 0; i < unit->fInputPins.Count(); i++) {
					_HarvestRecordFeatureUnits(Find(unit->fInputPins[i]), Map);
				}

				Map.Put(rootControl->ID(), rootControl);
			}
			break;

		case IDSFeatureUnit:
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
		{ UAS_GET_MIN, 0, Control.gain.min_gain },
		{ UAS_GET_MAX, 0, Control.gain.max_gain },
		{ UAS_GET_RES, 0, Control.gain.granularity }
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
			TRACE_ALWAYS("Request %d failed:%#08x; received %d of %d\n",
					i, status, actualLength, sizeof(gainInfos[i].data));
			continue;
		}

		gainInfos[i].value = static_cast<float>(gainInfos[i].data) / 256.;
	}

	TRACE_ALWAYS("Control %s: from %f to %f dB, step %f dB.\n", Control.name,
			Control.gain.min_gain, Control.gain.max_gain,
			Control.gain.granularity);
}


uint32
AudioControlInterface::_ListFeatureUnitOption(uint32 controlType,
								int32& index, int32 parentIndex,
								multi_mix_control_info* Info, FeatureUnit* unit,
								uint32 channel, uint32 channels)
{
	int32 startIndex = index;
	uint32 id = 0;
	uint32 flags = 0;
	strind_id string = S_null;
	const char* name = NULL;
	bool initGainLimits = false;

	switch(controlType) {
		case MuteControl:
			id = UAS_MUTE_CONTROL;
			flags = B_MULTI_MIX_ENABLE;
			string = S_MUTE;
			break;
		case VolumeControl:
			id = UAS_VOLUME_CONTROL;
			flags = B_MULTI_MIX_GAIN;
			string = S_GAIN;
			initGainLimits = true;
			break;
		case AutoGainControl:
			id = UAS_AUTOMATIC_GAIN_CONTROL;
			flags = B_MULTI_MIX_ENABLE;
			name = "Auto Gain";
			break;
		default:
			TRACE_ALWAYS("Unsupported type %#08x ignored.\n", controlType);
			return 0;
	}

	multi_mix_control* Controls = Info->controls;

	if (unit->HasControl(channel, controlType)) {
		uint32 masterIndex
			= Controls[index].id = CTL_ID(id, channel, unit->ID(), fInterface);
		Controls[index].flags	 = flags;
		Controls[index].parent	 = parentIndex;
		Controls[index].string	 = string;
		if (name != NULL)
			strncpy(Controls[index].name, name, sizeof(Controls[index].name));
		if (initGainLimits)
			_InitGainLimits(Controls[index]);

		index++;

		if (channels == 2) {
			Controls[index].id		= CTL_ID(id, channel + 1,
														unit->ID(), fInterface);
			Controls[index].flags	= flags;
			Controls[index].parent	= parentIndex;
			Controls[index].master	= masterIndex;
			Controls[index].string	= string;
			if (name != NULL)
				strncpy(Controls[index].name, name,
												sizeof(Controls[index].name));
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
		TRACE_ALWAYS("Feature Unit for null control ignored.\n");
		return 0;
	}

	AudioChannelCluster* cluster = unit->OutCluster();
	if (cluster == 0) {
		TRACE_ALWAYS("Control %s with null cluster ignored.\n", unit->Name());
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
			groupIndex
				= Controls[index].id	= index;
			Controls[index].flags		= B_MULTI_MIX_GROUP;
			Controls[index].parent		= parentIndex;
			snprintf(Controls[index].name, sizeof(Controls[index].name),
								"%s %s", unit->Name(), channelInfos[i].Name);
			index++;
		} else {
			groupIndex = masterIndex;
			masterIndex = 0;
		}

		// First list possible Mute controls
		_ListFeatureUnitOption(MuteControl, index, groupIndex, Info,
										unit, channel, channelInfos[i].channels);

		// Gain controls may be usefull too
		if (_ListFeatureUnitOption(VolumeControl, index, groupIndex, Info,
								unit, channel, channelInfos[i].channels) == 0)
		{
			masterIndex = (i == 0) ? groupIndex : 0 ;
			TRACE("channel:%d set master index to %d\n", channel, masterIndex);
		}

		// Auto Gain checkbox will be listed too
		_ListFeatureUnitOption(AutoGainControl, index, groupIndex, Info,
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

	if (channelsConfig > 0) {
		TRACE_ALWAYS("Following channels were "
								"not processed: %#08x.\n", channelsConfig);
	}

	// return last group index to stick possible selector unit to it. ;-)
	return groupIndex;
}


void
AudioControlInterface::_ListSelectorUnitControl(int32& index, int32 parentGroup,
						multi_mix_control_info* Info, _AudioControl* control)
{
	SelectorUnit* selector = static_cast<SelectorUnit*>(control);
	if (selector == 0 || selector->SubType() != IDSSelectorUnit) {
		return;
	}

	multi_mix_control* Controls = Info->controls;

	int32 recordMUX
		= Controls[index].id	= CTL_ID(0, 0, selector->ID(), fInterface);
	Controls[index].flags		= B_MULTI_MIX_MUX;
	Controls[index].parent		= parentGroup;
	Controls[index].string		= S_null;
	strncpy(Controls[index].name, "Source", sizeof(Controls[index].name));
	index++;

	for (int i = 0; i < selector->fInputPins.Count(); i++) {
		Controls[index].id		= CTL_ID(0, 1, selector->ID(), fInterface);
		Controls[index].flags	= B_MULTI_MIX_MUX_VALUE;
		Controls[index].master	= 0;
		Controls[index].string	= S_null;
		Controls[index].parent	= recordMUX;
		_AudioControl* control = Find(selector->fInputPins[i]);
		if (control != NULL) {
			strncpy(Controls[index].name,
					control->Name(), sizeof(Controls[index].name));
		} else {
			snprintf(Controls[index].name,
					sizeof(Controls[index].name), "Input #%d", i + 1);
		}
		index++;
	}

}


void
AudioControlInterface::_ListMixControlsPage(int32& index,
					multi_mix_control_info* Info,
					AudioControlsMap& Map, const char* Name)
{
	multi_mix_control* Controls = Info->controls;
	int32 groupIndex
		= Controls[index].id	= index | 0x10000;
	Controls[index].flags		= B_MULTI_MIX_GROUP;
	Controls[index].parent		= 0;
	strncpy(Controls[index].name, Name, sizeof(Controls[index].name));
	index++;

	int32 group = groupIndex;
	for (AudioControlsIterator I = Map.Begin(); I != Map.End(); I++) {
		TRACE("%s control %d listed.\n", Name, I->Value()->ID());
		switch(I->Value()->SubType()) {
			case IDSFeatureUnit:
				group = _ListFeatureUnitControl(index, groupIndex,
															Info, I->Value());
				break;
			case IDSSelectorUnit:
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
										I != fOutputTerminals.End(); I++)
	{
		_Terminal* terminal = static_cast<_Terminal*>(I->Value());
		if (terminal->IsUSBIO()) {
			_HarvestRecordFeatureUnits(terminal, RecordControlsMap);
		}
	}

	AudioControlsMap InputControlsMap;
	AudioControlsMap OutputControlsMap;

	for (AudioControlsIterator I = fAudioControls.Begin();
										I != fAudioControls.End(); I++)
	{
		_AudioControl* control = I->Value();
		// filter out feature units
		if (control->SubType() != IDSFeatureUnit) {
			continue;
		}

		// ignore controls that are already in the record controls map
		if (RecordControlsMap.Find(control->ID()) != RecordControlsMap.End()) {
			continue;
		}

		_AudioControl* sourceControl = Find(control->SourceID());
		if (sourceControl != 0 && sourceControl->SubType() == IDSInputTerminal) {
			InputControlsMap.Put(control->ID(), control);
		} else {
			OutputControlsMap.Put(control->ID(), control);
		}
	}

	int32 index = 0;
	if (InputControlsMap.Count() > 0) {
		_ListMixControlsPage(index, Info, InputControlsMap, "Input");
	}

	if (OutputControlsMap.Count() > 0) {
		_ListMixControlsPage(index, Info, OutputControlsMap, "Output");
	}

	if (RecordControlsMap.Count() > 0) {
		_ListMixControlsPage(index, Info, RecordControlsMap, "Record");
	}

	return B_OK;
}


status_t
AudioControlInterface::GetMix(multi_mix_value_info *Info)
{
	for (int32 i = 0; i < Info->item_count; i++) {

		uint16 length = 0;
		int16 data = 0;
		switch(CS_FROM_CTLID(Info->values[i].id)) {
			case UAS_VOLUME_CONTROL:
				length = 2;
				break;
			case 0: // Selector Unit
			case UAS_MUTE_CONTROL:
			case UAS_AUTOMATIC_GAIN_CONTROL:
				length = 1;
				break;
			default:
				TRACE_ALWAYS("Unsupported control type %#02x ignored.\n",
						CS_FROM_CTLID(Info->values[i].id));
				continue;
		}

		size_t actualLength = 0;
		status_t status = gUSBModule->send_request(fDevice->USBDevice(),
				USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_CLASS, UAS_GET_CUR,
				REQ_VALUE(Info->values[i].id), REQ_INDEX(Info->values[i].id),
				length, &data, &actualLength);

		if (status != B_OK || actualLength != length) {
			TRACE_ALWAYS("Request failed:%#08x; received %d of %d\n",
					status, actualLength, length);
			continue;
		}

		switch(CS_FROM_CTLID(Info->values[i].id)) {
			case UAS_VOLUME_CONTROL:
				Info->values[i].gain = static_cast<float>(data) / 256.;
				TRACE("Gain control %d; channel: %d; is %f dB.\n",
						ID_FROM_CTLID(Info->values[i].id),
						CN_FROM_CTLID(Info->values[i].id),
						Info->values[i].gain);
				break;
			case UAS_MUTE_CONTROL:
				Info->values[i].enable = data > 0;
				TRACE("Mute control %d; channel: %d; is %d.\n",
						ID_FROM_CTLID(Info->values[i].id),
						CN_FROM_CTLID(Info->values[i].id),
						Info->values[i].enable);
				break;
			case UAS_AUTOMATIC_GAIN_CONTROL:
				Info->values[i].enable = data > 0;
				TRACE("AGain control %d; channel: %d; is %d.\n",
						ID_FROM_CTLID(Info->values[i].id),
						CN_FROM_CTLID(Info->values[i].id),
						Info->values[i].enable);
				break;
			case 0: // Selector Unit
				Info->values[i].mux = data;
				TRACE("Selector control %d; is %d.\n",
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
AudioControlInterface::SetMix(multi_mix_value_info *Info)
{
	for (int32 i = 0; i < Info->item_count; i++) {

		uint16 length = 0;
		int16 data = 0;

		switch(CS_FROM_CTLID(Info->values[i].id)) {
			case UAS_VOLUME_CONTROL:
				data = static_cast<int16>(Info->values[i].gain * 256.);
				length = 2;
				TRACE("Gain control %d; channel: %d; about to set to %f dB.\n",
						ID_FROM_CTLID(Info->values[i].id),
						CN_FROM_CTLID(Info->values[i].id),
						Info->values[i].gain);
				break;
			case UAS_MUTE_CONTROL:
				data = (Info->values[i].enable ? 1 : 0);
				length = 1;
				TRACE("Mute control %d; channel: %d; about to set to %d.\n",
						ID_FROM_CTLID(Info->values[i].id),
						CN_FROM_CTLID(Info->values[i].id),
						Info->values[i].enable);
				break;
			case UAS_AUTOMATIC_GAIN_CONTROL:
				data = (Info->values[i].enable ? 1 : 0);
				length = 1;
				TRACE("AGain control %d; channel: %d; about to set to %d.\n",
						ID_FROM_CTLID(Info->values[i].id),
						CN_FROM_CTLID(Info->values[i].id),
						Info->values[i].enable);
				break;
			case 0: // Selector Unit
				data = Info->values[i].mux;
				length = 1;
				TRACE("Selector Control %d about to set to %d.\n",
						ID_FROM_CTLID(Info->values[i].id),
						Info->values[i].mux);
				break;
			default:
				TRACE_ALWAYS("Unsupported control type %#02x ignored.\n",
						CS_FROM_CTLID(Info->values[i].id));
				continue;
		}

		size_t actualLength = 0;
		status_t status = gUSBModule->send_request(fDevice->USBDevice(),
				USB_REQTYPE_INTERFACE_OUT | USB_REQTYPE_CLASS, UAS_SET_CUR,
				REQ_VALUE(Info->values[i].id), REQ_INDEX(Info->values[i].id),
				length, &data, &actualLength);

		if (status != B_OK || actualLength != length) {
			TRACE_ALWAYS("Request failed:%#08x; send %d of %d\n",
					status, actualLength, length);
			continue;
		}

		TRACE("Value set OK\n");
	}

	return B_OK;
}

