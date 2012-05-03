/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the tems of the MIT license.
 *
 */

#include "AudioStreamingInterface.h"
#include "Settings.h"
#include "Device.h"
#include "audio.h"


//
//	Audio Stream information entities
//
//
ASInterfaceDescriptor::ASInterfaceDescriptor(/*Device* device, size_t interface,*/
					usb_as_interface_descriptor_r1*	Descriptor)
			:
//			_AudioFunctionEntity(device, interface),
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

	// fStatus = B_OK;
}


ASInterfaceDescriptor::~ASInterfaceDescriptor()
{

}


ASEndpointDescriptor::ASEndpointDescriptor(usb_endpoint_descriptor* Endpoint,
					usb_as_cs_endpoint_descriptor* Descriptor)
			:
			fAttributes(0),
			fLockDelayUnits(0),
			fLockDelay(0),
			fMaxPacketSize(0),
			fEndpointAddress(0)
{
//	usb_audiocontrol_header_descriptor *Header
//		= (usb_audiocontrol_header_descriptor *)Interface->generic[i];

	fAttributes = Descriptor->attributes;
	fLockDelayUnits = Descriptor->lock_delay_units;
	fLockDelay = Descriptor->lock_delay;

//	usb_endpoint_descriptor* endpoint = Interface->endpoint[0]->descr;
	fEndpointAddress = Endpoint->endpoint_address;
	fMaxPacketSize = Endpoint->max_packet_size;

	TRACE("fAttributes:%d\n", fAttributes);
	TRACE("fLockDelayUnits:%d\n", fLockDelayUnits);
	TRACE("fLockDelay:%d\n", fLockDelay);
	TRACE("fMaxPacketSize:%d\n", fMaxPacketSize);
	TRACE("fEndpointAddress:%#02x\n", fEndpointAddress);
}


ASEndpointDescriptor::~ASEndpointDescriptor()
{
}


_ASFormatDescriptor::_ASFormatDescriptor(usb_type_I_format_descriptor* Descriptor)
			:
			/*_AudioFunctionEntity(device, interface)*/
			fFormatType(UAF_FORMAT_TYPE_UNDEFINED)
{
	fFormatType = Descriptor->format_type;
}


_ASFormatDescriptor::~_ASFormatDescriptor()
{
}


uint32
_ASFormatDescriptor::GetSamFreq(uint8* freq)
{
	return freq[0] | freq[1] << 8 | freq[2] << 16;
}


TypeIFormatDescriptor::TypeIFormatDescriptor(/*Device* device, size_t interface,*/
					usb_type_I_format_descriptor* Descriptor)
			:
			_ASFormatDescriptor(Descriptor),
			fNumChannels(0),
			fSubframeSize(0),
			fBitResolution(0),
			fSampleFrequencyType(0)
{
	/*fStatus =*/ Init(Descriptor);
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


TypeIIFormatDescriptor::TypeIIFormatDescriptor(/*Device* device, size_t interface,*/ 
					usb_type_II_format_descriptor* Descriptor)
			:
			_ASFormatDescriptor((usb_type_I_format_descriptor*)Descriptor),
			fMaxBitRate(0),
			fSamplesPerFrame(0),
			fSampleFrequencyType(0),
			fSampleFrequencies(0)
{
}


TypeIIFormatDescriptor::~TypeIIFormatDescriptor()
{
}


TypeIIIFormatDescriptor::TypeIIIFormatDescriptor(/*Device* device, size_t interface,*/ 
					usb_type_III_format_descriptor* Descriptor)
			:
			TypeIFormatDescriptor(/*device, interface, */
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


AudioStreamingInterface::AudioStreamingInterface(
					AudioControlInterface*	controlInterface,
					size_t interface, usb_interface_list *List)
			:
			fInterface(interface),
			fControlInterface(controlInterface),
			fIsInput(false),
			fActiveAlternate(0)
{
	TRACE_ALWAYS("if[%d]:alt_count:%d\n", interface, List->alt_count);

	for (size_t alt = 0; alt < List->alt_count; alt++) {
		ASInterfaceDescriptor*	ASInterface	= NULL;
		ASEndpointDescriptor*	ASEndpoint	= NULL;
		_ASFormatDescriptor*	ASFormat	= NULL;

		usb_interface_info *Interface = &List->alt[alt];

		TRACE_ALWAYS("if[%d]:alt[%d]:descrs_count:%d\n",
									interface, alt, Interface->generic_count);
		for (size_t i = 0; i < Interface->generic_count; i++) {
			usb_audiocontrol_header_descriptor *Header
				= (usb_audiocontrol_header_descriptor *)Interface->generic[i];

			if (Header->descriptor_type == AC_CS_INTERFACE) {
				switch(Header->descriptor_subtype) {
					case UAS_AS_GENERAL:
						if (ASInterface == 0) {
							ASInterface = new ASInterfaceDescriptor(
									/*this, interface, */
									(usb_as_interface_descriptor_r1*) Header);
						} else
							TRACE_ALWAYS("Duplicate AStream interface ignored.\n");
						break;
					case UAS_FORMAT_TYPE:
						if (ASFormat == 0) {
							ASFormat = new TypeIFormatDescriptor(
										(usb_type_I_format_descriptor*) Header);
						} else
							TRACE_ALWAYS("Duplicate AStream format ignored.\n");
						break;
					default:
						TRACE_ALWAYS("Ignore AStream descr subtype %#04x\n",
										Header->descriptor_subtype);
						break;
				}
				continue;
			}

			if (Header->descriptor_type == AC_CS_ENDPOINT) {
				if (ASEndpoint == 0) {
					usb_endpoint_descriptor* Endpoint
							= Interface->endpoint[0].descr;
					ASEndpoint = new ASEndpointDescriptor(Endpoint,
							(usb_as_cs_endpoint_descriptor*)Header);
				} else
					TRACE_ALWAYS("Duplicate AStream endpoint ignored.\n");
				continue;
			}

			TRACE_ALWAYS("Ignore Audio Stream of "
				"unknown descriptor type %#04x.\n",	Header->descriptor_type);
		}

		fAlternates.Add(new AudioStreamAlternate(alt, ASInterface,
							ASEndpoint, ASFormat));
	}
}


AudioStreamingInterface::~AudioStreamingInterface()
{
		// alternates of the streams
//		StreamAlternatesVector	fAlternates;
//		size_t				fActiveAlternate;
	// we own stream header objects too, so free them
	for (StreamAlternatesIterator I = fAlternates.Begin();
								I != fAlternates.End(); I++) {
		delete *I;
	}
	fAlternates.MakeEmpty();
}


uint8
AudioStreamingInterface::TerminalLink()
{
	if (fAlternates[fActiveAlternate]->Interface() != 0) {
		return fAlternates[fActiveAlternate]->Interface()->fTerminalLink;
	}

	return 0;
}


AudioChannelCluster*
AudioStreamingInterface::ChannelCluster()
{
	_AudioControl* control = fControlInterface->Find(TerminalLink());
	if (control == 0) {
		TRACE_ALWAYS("Control was not found for terminal id:%d.\n",
			TerminalLink());
		return NULL;
	}

	return control->OutCluster();
}


/*
ASInterfaceDescriptor*
Stream::ASInterface()
{
	return fAlternates[fActiveAlternate]->Interface();
}


_ASFormatDescriptor*
Stream::ASFormat()
{
	return fAlternates[fActiveAlternate]->Format();
} */

void
AudioStreamingInterface::GetFormatsAndRates(multi_description *Description)
{
	// TODO: fIsInput ??????
	Description->interface_flags
		|= fIsInput ? B_MULTI_INTERFACE_RECORD : B_MULTI_INTERFACE_PLAYBACK;

	uint32 rates = 0;
	uint32 formats = 0;

	TypeIFormatDescriptor* format
		= static_cast<TypeIFormatDescriptor*>(
				fAlternates[fActiveAlternate]->Format());

	if (format == NULL || fAlternates[fActiveAlternate]->Interface() == NULL) {
		TRACE_ALWAYS("Ignore alternate %d due format "
				"%#08x or interface %#08x null.\n", fActiveAlternate, format,
				fAlternates[fActiveAlternate]->Interface());
	}

	if (format->fSampleFrequencyType == 0) { // continuous frequencies
		rates = B_SR_CVSR;
		Description->min_cvsr_rate = float(format->fSampleFrequencies[0]);
		Description->max_cvsr_rate = float(format->fSampleFrequencies[1]);
	} else {
		for (int i = 0; i < format->fSampleFrequencies.Count(); i++) {
			switch(format->fSampleFrequencies[i]) {
				case 8000:		rates |= B_SR_8000;		break;
				case 11025:		rates |= B_SR_11025;	break;
				case 12000:		rates |= B_SR_12000;	break;
				case 16000:		rates |= B_SR_16000;	break;
				case 22050:		rates |= B_SR_22050;	break;
				case 24000:		rates |= B_SR_24000;	break;
				case 32000:		rates |= B_SR_32000;	break;
				case 44100:		rates |= B_SR_44100;	break;
				case 48000:		rates |= B_SR_48000;	break;
				case 64000:		rates |= B_SR_64000;	break;
				case 88200:		rates |= B_SR_88200;	break;
				case 96000:		rates |= B_SR_96000;	break;
				case 176400:	rates |= B_SR_176400;	break;
				case 192000:	rates |= B_SR_192000;	break;
				case 384000:	rates |= B_SR_384000;	break;
				case 1536000:	rates |= B_SR_1536000;	break;
				default:
					TRACE_ALWAYS("Ignore unsupported "
						"sample rate %d for alternate %d.\n",
						format->fSampleFrequencies[i], fActiveAlternate);
					break;
			}
		}
	}

	switch (fAlternates[fActiveAlternate]->Interface()->fFormatTag) {
		case UAF_PCM8:			formats = B_FMT_8BIT_U;	break;
		case UAF_IEEE_FLOAT:	formats = B_FMT_FLOAT;	break;
		case UAF_PCM:
			switch(format->fBitResolution) {
				case 8: formats = B_FMT_8BIT_S; break;
				case 16: formats = B_FMT_16BIT; break;
				case 18: formats = B_FMT_18BIT; break;
				case 20: formats = B_FMT_20BIT; break;
				case 24: formats = B_FMT_24BIT; break;
				case 32: formats = B_FMT_32BIT; break;
					break;
				default:
					TRACE_ALWAYS("Ignore unsupported "
							"bit resolution %d for alternate %d.\n",
						format->fBitResolution, fActiveAlternate);
					break;
			}
			break;
	}

	if (fIsInput) {
		Description->input_rates = rates;
		Description->input_formats = formats;
	} else {
		Description->output_rates = rates;
		Description->output_formats = formats;
	}
}

