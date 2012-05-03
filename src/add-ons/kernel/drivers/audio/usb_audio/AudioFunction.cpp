/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the tems of the MIT license.
 *
 */

#include "AudioFunction.h"
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


_AudioFunctionEntity::_AudioFunctionEntity(Device* device, size_t interface)
			:
			fStatus(B_NO_INIT),
			fDevice(device),
			fInterface(interface)
{
}


_AudioFunctionEntity::~_AudioFunctionEntity()
{

}


_AudioControl::_AudioControl(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioFunctionEntity(device, interface),
			fSubType(Header->descriptor_subtype),
			fID(0),
			fSourceID(0),
			fStringIndex(0)
{
}


_AudioControl::~_AudioControl()
{
}


_AudioChannelsCluster*
_AudioControl::OutputCluster()
{
	if (SourceID() == 0 || fDevice == 0) {
		return NULL;
	}

	_AudioControl* control = fDevice->FindAudioControl(SourceID());
	if (control == 0) {
		return NULL;
	}

	return control->OutputCluster();
}


AudioControlHeader::AudioControlHeader(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(device, interface, Header),
			fADCSpecification(0),
			fFunctionCategory(0),
			fControlsBitmap(0)
{
	if (Header == NULL) {
		return;
	}

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

	fStatus = B_OK;
}


AudioControlHeader::~AudioControlHeader()
{

}


_AudioChannelsCluster::_AudioChannelsCluster()
			:
			fOutChannelsNumber(0),
			fChannelsConfig(0),
			fChannelNames(0)
{

}


_AudioChannelsCluster::~_AudioChannelsCluster()
{

}


_AudioChannelsCluster*
_AudioChannelsCluster::OutputCluster()
{
	return this;
}


_Terminal::_Terminal(Device* device, size_t interface,
						usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(device, interface, Header),
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
	return (fTerminalType && 0xff00) == UndefinedUSB_IO;
}


InputTerminal::InputTerminal(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_Terminal(device, interface, Header)
{
	usb_input_terminal_descriptor_r1* Terminal
		= (usb_input_terminal_descriptor_r1*) Header;
	fID					= Terminal->terminal_id;
	fTerminalType		= Terminal->terminal_type;
	fAssociatedTerminal = Terminal->assoc_terminal;

	TRACE("Input Terminal ID:%d\n",	fID);
	TRACE("Terminal type:%s (%#06x)\n",
				GetTerminalDescription(fTerminalType), fTerminalType);
	TRACE("Assoc.terminal:%d\n",	fAssociatedTerminal);

	if (device->SpecReleaseNumber() < 0x200) {
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


OutputTerminal::OutputTerminal(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_Terminal(device, interface, Header)
{
	usb_output_terminal_descriptor_r1* Terminal
			= (usb_output_terminal_descriptor_r1*) Header;

	fID					= Terminal->terminal_id;
	fTerminalType		= Terminal->terminal_type;
	fAssociatedTerminal	= Terminal->assoc_terminal;
	fSourceID			= Terminal->source_id;

	TRACE("Output Terminal ID:%d\n",	fID);
	TRACE("Terminal type:%s (%#06x)\n",
				GetTerminalDescription(fTerminalType), fTerminalType);
	TRACE("Assoc.terminal:%d\n",		fAssociatedTerminal);
	TRACE("Source ID:%d\n",				fSourceID);

	if (device->SpecReleaseNumber() < 0x200) {
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


MixerUnit::MixerUnit(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(device, interface, Header),
			fControlsBitmap(0)
{
	usb_mixer_unit_descriptor* Mixer
		= (usb_mixer_unit_descriptor*) Header;

	fID = Mixer->unit_id;
	TRACE("Mixer ID:%d\n", fID);

	TRACE("Number of input pins:%d\n", Mixer->num_input_pins);
	for (size_t i = 0; i < Mixer->num_input_pins; i++) {
		fInputPins.PushBack(Mixer->input_pins[i]);
		TRACE("Input pin #%d:%d\n", i, fInputPins[i]);
	}

	uint8* mixerControlsData = NULL;
	uint8 mixerControlsSize = 0;

	if (device->SpecReleaseNumber() < 0x200) {
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


SelectorUnit::SelectorUnit(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(device, interface, Header),
			fControlsBitmap(0)
{
	usb_selector_unit_descriptor* Selector
			= (usb_selector_unit_descriptor*) Header;

	fID = Selector->unit_id;
	TRACE("Selector ID:%d\n", fID);

	TRACE("Number of input pins:%d\n", Selector->num_input_pins);
	for (size_t i = 0; i < Selector->num_input_pins; i++) {
		fInputPins.PushBack(Selector->input_pins[i]);
		TRACE("Input pin #%d:%d\n", i, fInputPins[i]);
	}

	if (device->SpecReleaseNumber() < 0x200) {

		fStringIndex =  Selector->input_pins[Selector->num_input_pins];
	} else {

		fControlsBitmap = Selector->input_pins[Selector->num_input_pins];
		fStringIndex =  Selector->input_pins[Selector->num_input_pins + 1];

		TRACE("Controls Bitmap:%d\n", fControlsBitmap);
	}

	TRACE("StringIndex:%d\n", fStringIndex);

	fStatus = B_OK;
}


SelectorUnit::~SelectorUnit()
{
}


FeatureUnit::FeatureUnit(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(device, interface, Header)
{
	usb_feature_unit_descriptor* Feature
		= (usb_feature_unit_descriptor*) Header;

	fID = Feature->unit_id;
	TRACE("Feature ID:%d\n", fID);

	fSourceID = Feature->source_id;
	TRACE("Source ID:%d\n", fSourceID);

	uint8 controlSize = 4;
	uint8 channelsCount = (Feature->length - 6) / controlSize;
	uint8 *ControlsBitmapPointer = (uint8*)&Feature->bma_controls[0];

	if (device->SpecReleaseNumber() < 0x200) {
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

		TraceChannel(device, i);
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
	AudioControlsIterator it = fDevice->fAudioControls.Find(fSourceID);
	while (it != fDevice->fAudioControls.End()) {
		_AudioControl *control = it->Value();
		if (control != 0)
			break;

		if (control->SubType() != IDSInputTerminal)
			break;

		// USB I/O terminal is a not good candidate to use it's name
		if (static_cast<_Terminal*>(control)->IsUSBIO())
			break;

		// use the name of source input terminal as name of this FU
		return control->Name();
	}

	// check if output of this FU is connected to output terminal
	it = fDevice->fOutputTerminals.Find(fID);
	while (it != fDevice->fOutputTerminals.End()) {
		_AudioControl *control = it->Value();
		if (control != 0)
			break;

		if (control->SubType() != IDSOutputTerminal)
			break;

		// USB I/O terminal is a not good candidate to use it's name
		uint16 Type = static_cast<OutputTerminal*>(control)->TerminalType();
		if ((Type && 0xff00) == UndefinedUSB_IO)
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
FeatureUnit::TraceChannel(Device* device, int32 Channel)
{
	if (Channel >= fControlBitmaps.Count()) {
		TRACE_ALWAYS("Out of limits error of tracing channel %d\n", Channel);
		return;
	}

	if (Channel == 0)
		TRACE("Master channel bitmap:%#x\n", fControlBitmaps[Channel]);
	else
		TRACE("Channel %d bitmap:%#x\n", Channel, fControlBitmaps[Channel]);

	if (device->SpecReleaseNumber() < 0x200) {
		if (HasControl(Channel, MuteControl1))
			TRACE("\tMute\n");
		if (HasControl(Channel, VolumeControl1))
			TRACE("\tVolume\n");
		if (HasControl(Channel, BassControl1))
			TRACE("\tBass\n");
		if (HasControl(Channel, MidControl1))
			TRACE("\tMid\n");
		if (HasControl(Channel, TrebleControl1))
			TRACE("\tTreble\n");
		if (HasControl(Channel, GraphEqControl1))
			TRACE("\tGraphic Equalizer\n");
		if (HasControl(Channel, AutoGainControl1))
			TRACE("\tAutomatic Gain\n");
		if (HasControl(Channel, DelayControl1))
			TRACE("\tDelay\n");
		if (HasControl(Channel, BassBoostControl1))
			TRACE("\tBass Boost\n");
		if (HasControl(Channel, LoudnessControl1))
			TRACE("\tLoudness\n");
	} else {
		if (HasControl(Channel, MuteControl))
			TRACE("\tMute\n");
		if (HasControl(Channel, VolumeControl))
			TRACE("\tVolume\n");
		if (HasControl(Channel, BassControl))
			TRACE("\tBass\n");
		if (HasControl(Channel, MidControl))
			TRACE("\tMid\n");
		if (HasControl(Channel, TrebleControl))
			TRACE("\tTreble\n");
		if (HasControl(Channel, GraphEqControl))
			TRACE("\tGraphic Equalizer\n");
		if (HasControl(Channel, AutoGainControl))
			TRACE("\tAutomatic Gain\n");
		if (HasControl(Channel, DelayControl))
			TRACE("\tDelay\n");
		if (HasControl(Channel, BassBoostControl))
			TRACE("\tBass Boost\n");
		if (HasControl(Channel, LoudnessControl))
			TRACE("\tLoudness\n");
		if (HasControl(Channel, InputGainControl))
			TRACE("\tInputGain\n");
		if (HasControl(Channel, InputGainPadControl))
			TRACE("\tInputGainPad\n");
		if (HasControl(Channel, PhaseInverterControl))
			TRACE("\tPhaseInverter\n");
		if (HasControl(Channel, UnderflowControl))
			TRACE("\tUnderflow\n");
		if (HasControl(Channel, OverflowControl))
			TRACE("\tOverflow\n");
	}
}


EffectUnit::EffectUnit(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(device, interface, Header)
{
	usb_input_terminal_descriptor* D
		= (usb_input_terminal_descriptor*) Header;
	TRACE("Effect Unit:%d\n",	D->terminal_id);
}


EffectUnit::~EffectUnit()
{
}


ProcessingUnit::ProcessingUnit(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(device, interface, Header),
			fProcessType(0),
			fControlsBitmap(0)
{
	usb_processing_unit_descriptor* Processing
			= (usb_processing_unit_descriptor*) Header;

	fID = Processing->unit_id;
	TRACE("Processing ID:%d\n", fID);

	fProcessType = Processing->process_type;
	TRACE("Processing Type:%d\n", fProcessType);

	TRACE("Number of input pins:%d\n", Processing->num_input_pins);
	for (size_t i = 0; i < Processing->num_input_pins; i++) {
		fInputPins.PushBack(Processing->input_pins[i]);
		TRACE("Input pin #%d:%d\n", i, fInputPins[i]);
	}

	if (device->SpecReleaseNumber() < 0x200) {
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


ExtensionUnit::ExtensionUnit(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(device, interface, Header),
			fExtensionCode(0),
			fControlsBitmap(0)
{
	usb_extension_unit_descriptor* Extension
			= (usb_extension_unit_descriptor*) Header;

	fID = Extension->unit_id;
	TRACE("Extension ID:%d\n", fID);

	fExtensionCode = Extension->extension_code;
	TRACE("Extension Type:%d\n", fExtensionCode);

	TRACE("Number of input pins:%d\n", Extension->num_input_pins);
	for (size_t i = 0; i < Extension->num_input_pins; i++) {
		fInputPins.PushBack(Extension->input_pins[i]);
		TRACE("Input pin #%d:%d\n", i, fInputPins[i]);
	}


	if (device->SpecReleaseNumber() < 0x200) {
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


ClockSource::ClockSource(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
				:
				_AudioControl(device, interface, Header)
{
	usb_input_terminal_descriptor* D
			= (usb_input_terminal_descriptor*) Header;
	TRACE("Clock Source:%d\n",	D->terminal_id);
}


ClockSource::~ClockSource()
{
}


ClockSelector::ClockSelector(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(device, interface, Header)
{
	usb_input_terminal_descriptor* D
			= (usb_input_terminal_descriptor*) Header;
	TRACE("Clock Selector:%d\n",	D->terminal_id);
}


ClockSelector::~ClockSelector()
{
}


ClockMultiplier::ClockMultiplier(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(device, interface, Header)
{
	usb_input_terminal_descriptor* D
			= (usb_input_terminal_descriptor*) Header;
	TRACE("Clock Multiplier:%d\n",	D->terminal_id);
}


ClockMultiplier::~ClockMultiplier()
{
}


SampleRateConverter::SampleRateConverter(Device* device, size_t interface,
					usb_audiocontrol_header_descriptor* Header)
			:
			_AudioControl(device, interface, Header)
{
	usb_input_terminal_descriptor* D
			= (usb_input_terminal_descriptor*) Header;
	TRACE("Sample Rate Converter:%d\n",	D->terminal_id);
}


SampleRateConverter::~SampleRateConverter()
{
}


//
// Audio Stream information entities
//
//
ASInterfaceDescriptor::ASInterfaceDescriptor(Device* device, size_t interface,
					usb_as_interface_descriptor_r1*	Descriptor)
			:
			_AudioFunctionEntity(device, interface),
			fTerminalLink(0),
			fDelay(0),
			fFormatTag(0)
{
	fTerminalLink = Descriptor->terminal_link;
	fDelay = Descriptor->delay;
	fFormatTag = Descriptor->format_tag;

	TRACE("fTerminalLink:%d\n", fTerminalLink);
	TRACE("fDelay:%d\n", fDelay);
	TRACE("fFormatTag:%#06x\n", fFormatTag);

	fStatus = B_OK;
}


ASInterfaceDescriptor::~ASInterfaceDescriptor()
{

}


ASEndpointDescriptor::ASEndpointDescriptor(Device* device, size_t interface,
					usb_as_cs_endpoint_descriptor* Descriptor)
			:
			_AudioFunctionEntity(device, interface),
			fAttributes(0),
			fLockDelayUnits(0),
			fLockDelay(0)
{

	fAttributes = Descriptor->attributes;
	fLockDelayUnits = Descriptor->lock_delay_units;
	fLockDelay = Descriptor->lock_delay;

	TRACE("fAttributes:%d\n", fAttributes);
	TRACE("fLockDelayUnits:%d\n", fLockDelayUnits);
	TRACE("fLockDelay:%d\n", fLockDelay);

	fStatus = B_OK;
}


ASEndpointDescriptor::~ASEndpointDescriptor()
{
}


_ASFormatDescriptor::_ASFormatDescriptor(Device* device, size_t interface)
			:
			_AudioFunctionEntity(device, interface)
{
}


_ASFormatDescriptor::~_ASFormatDescriptor()
{
}


uint32
_ASFormatDescriptor::GetSamFreq(uint8* freq)
{
	return freq[0] | freq[1] << 8 | freq[2] << 16;
}


TypeIFormatDescriptor::TypeIFormatDescriptor(Device* device, size_t interface,
					usb_type_I_format_descriptor* Descriptor)
			:
			_ASFormatDescriptor(device, interface),
			fNumChannels(0),
			fSubframeSize(0),
			fBitResolution(0),
			fSampleFrequencyType(0)
{
	fStatus = Init(Descriptor);
}


TypeIFormatDescriptor::~TypeIFormatDescriptor()
{
}


status_t
TypeIFormatDescriptor::Init(usb_type_I_format_descriptor* Descriptor)
{
	fNumChannels = Descriptor->nr_channels;
	fSubframeSize = Descriptor->subframe_size;
	fBitResolution = Descriptor->bit_resolution;
	fSampleFrequencyType = Descriptor->sam_freq_type;

	if (fSampleFrequencyType == 0) {
		fSampleFrequencies.PushBack(
				GetSamFreq(Descriptor->sf.cont.lower_sam_freq));
		fSampleFrequencies.PushBack(
				GetSamFreq(Descriptor->sf.cont.upper_sam_freq));
	} else {
		for (size_t i = 0; i < fSampleFrequencyType; i++) {
			fSampleFrequencies.PushBack(
				GetSamFreq(Descriptor->sf.discr.sam_freq[i]));
		}
	}

	TRACE("fNumChannels:%d\n", fNumChannels);
	TRACE("fSubframeSize:%d\n", fSubframeSize);
	TRACE("fBitResolution:%d\n", fBitResolution);
	TRACE("fSampleFrequencyType:%d\n", fSampleFrequencyType);

	for (int32 i = 0; i < fSampleFrequencies.Count(); i++) {
		TRACE("Frequency #%d: %d\n", i, fSampleFrequencies[i]);
	}

	return B_OK;
}


TypeIIFormatDescriptor::TypeIIFormatDescriptor(Device* device, size_t interface,
							usb_type_II_format_descriptor* Descriptor)
			:
			_ASFormatDescriptor(device, interface),
			fMaxBitRate(0),
			fSamplesPerFrame(0),
			fSampleFrequencyType(0),
			fSampleFrequencies(0)
{
}


TypeIIFormatDescriptor::~TypeIIFormatDescriptor()
{
}


TypeIIIFormatDescriptor::TypeIIIFormatDescriptor(Device* device, size_t interface,
							usb_type_III_format_descriptor* Descriptor)
			:
			TypeIFormatDescriptor(device, interface,
			(usb_type_I_format_descriptor*) Descriptor)
{

}


TypeIIIFormatDescriptor::~TypeIIIFormatDescriptor()
{
}


AudioStreamAlternate::AudioStreamAlternate(size_t alternate,
					ASInterfaceDescriptor* interface,
					ASEndpointDescriptor* endpoint,
					_ASFormatDescriptor* format)
			:
			fAlternate(alternate),
			fInterface(interface),
			fEndpoint(endpoint),
			fFormat(format)
{
}


AudioStreamAlternate::~AudioStreamAlternate()
{
	delete fInterface;
	delete fEndpoint;
	delete fFormat;
}

