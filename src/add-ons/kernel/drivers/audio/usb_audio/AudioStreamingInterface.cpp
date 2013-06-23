/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the tems of the MIT license.
 *
 */


#include "AudioStreamingInterface.h"


#include "audio.h"
#include "Driver.h"
#include "Settings.h"


static struct RatePair {
	uint32 rate;
	uint32 rateId;
} ratesMap[] = {
	{ 8000, B_SR_8000 },
	{ 11025, B_SR_11025 },
	{ 12000, B_SR_12000 },
	{ 16000, B_SR_16000 },
	{ 22050, B_SR_22050 },
	{ 24000, B_SR_24000 },
	{ 32000, B_SR_32000 },
	{ 44100, B_SR_44100 },
	{ 48000, B_SR_48000 },
	{ 64000, B_SR_64000 },
	{ 88200, B_SR_88200 },
	{ 96000, B_SR_96000 },
	{ 176400, B_SR_176400 },
	{ 192000, B_SR_192000 },
	{ 384000, B_SR_384000 },
	{ 1536000, B_SR_1536000 }
};


//
//	Audio Stream information entities
//
//
ASInterfaceDescriptor::ASInterfaceDescriptor(
		usb_as_interface_descriptor_r1* Descriptor)
	:
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
	fCSAttributes(0),
	fLockDelayUnits(0),
	fLockDelay(0),
	fMaxPacketSize(0),
	fEndpointAddress(0),
	fEndpointAttributes(0)
{
//	usb_audiocontrol_header_descriptor* Header
//		= (usb_audiocontrol_header_descriptor*)Interface->generic[i];

	fCSAttributes = Descriptor->attributes;
	fLockDelayUnits = Descriptor->lock_delay_units;
	fLockDelay = Descriptor->lock_delay;

//	usb_endpoint_descriptor* endpoint = Interface->endpoint[0]->descr;
	fEndpointAttributes = Endpoint->attributes;
	fEndpointAddress = Endpoint->endpoint_address;
	fMaxPacketSize = Endpoint->max_packet_size;

	TRACE("fCSAttributes:%d\n", fCSAttributes);
	TRACE("fLockDelayUnits:%d\n", fLockDelayUnits);
	TRACE("fLockDelay:%d\n", fLockDelay);
	TRACE("fMaxPacketSize:%d\n", fMaxPacketSize);
	TRACE("fEndpointAddress:%#02x\n", fEndpointAddress);
	TRACE("fEndpointAttributes:%d\n", fEndpointAttributes);
}


ASEndpointDescriptor::~ASEndpointDescriptor()
{
}


_ASFormatDescriptor::_ASFormatDescriptor(
		usb_type_I_format_descriptor* Descriptor)
	:
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


TypeIFormatDescriptor::TypeIFormatDescriptor(
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
	} else
		for (size_t i = 0; i < fSampleFrequencyType; i++)
			fSampleFrequencies.PushBack(
				GetSamFreq(Descriptor->sf.discr.sam_freq[i]));

	TRACE("fNumChannels:%d\n", fNumChannels);
	TRACE("fSubframeSize:%d\n", fSubframeSize);
	TRACE("fBitResolution:%d\n", fBitResolution);
	TRACE("fSampleFrequencyType:%d\n", fSampleFrequencyType);

	for (int32 i = 0; i < fSampleFrequencies.Count(); i++)
		TRACE("Frequency #%d: %d\n", i, fSampleFrequencies[i]);

	return B_OK;
}


TypeIIFormatDescriptor::TypeIIFormatDescriptor(
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


TypeIIIFormatDescriptor::TypeIIIFormatDescriptor(
		usb_type_III_format_descriptor* Descriptor)
	:
	TypeIFormatDescriptor((usb_type_I_format_descriptor*)Descriptor)
{
}


TypeIIIFormatDescriptor::~TypeIIIFormatDescriptor()
{
}


AudioStreamAlternate::AudioStreamAlternate(size_t alternate,
		ASInterfaceDescriptor* interface, ASEndpointDescriptor* endpoint,
		_ASFormatDescriptor* format)
	:
	fAlternate(alternate),
	fInterface(interface),
	fEndpoint(endpoint),
	fFormat(format),
	fSamplingRate(0)
{
}


AudioStreamAlternate::~AudioStreamAlternate()
{
	delete fInterface;
	delete fEndpoint;
	delete fFormat;
}


status_t
AudioStreamAlternate::SetSamplingRate(uint32 newRate)
{
	TypeIFormatDescriptor* format
		= static_cast<TypeIFormatDescriptor*>(Format());

	if (format == NULL) {
		TRACE_ALWAYS("Format not set for active alternate\n");
		return B_NO_INIT;
	}
	
	Vector<uint32>& frequencies = format->fSampleFrequencies;
	bool continuous = format->fSampleFrequencyType == 0;
	
	if (newRate == 0) { // by default select max available
		fSamplingRate = 0;
		if (continuous)
			fSamplingRate = max_c(frequencies[0], frequencies[1]);
		else
			for (int i = 0; i < frequencies.Count(); i++)
				fSamplingRate = max_c(fSamplingRate, frequencies[i]);
	} else {
		if (continuous) {
			uint32 min = min_c(frequencies[0], frequencies[1]);
			uint32 max = max_c(frequencies[0], frequencies[1]);
			if (newRate < min || newRate > max) {
				TRACE_ALWAYS("Rate %d outside of %d - %d ignored.\n",
					newRate, min, max);
				return B_BAD_INDEX;
			}
			fSamplingRate = newRate;
		} else {
			for (int i = 0; i < frequencies.Count(); i++)
				if (newRate == frequencies[i]) {
					fSamplingRate = newRate;
					return B_OK;
				}
				TRACE_ALWAYS("Rate %d not found - ignore it.\n", newRate);
				return B_BAD_INDEX;
		}
	}

	return B_OK;
}


uint32
AudioStreamAlternate::GetSamplingRateId(uint32 rate)
{
	if (rate == 0)
		rate = fSamplingRate;

	for (size_t i = 0; i < _countof(ratesMap); i++)
		if (ratesMap[i].rate == rate)
			return ratesMap[i].rateId;

	TRACE_ALWAYS("Ignore unsupported sample rate %d.\n", rate);
	return 0;
}


uint32
AudioStreamAlternate::GetSamplingRateIds()
{
	TypeIFormatDescriptor* format
		= static_cast<TypeIFormatDescriptor*>(Format());

	if (format == NULL) {
		TRACE_ALWAYS("Format not set for active alternate\n");
		return 0;
	}
	
	uint32 rates = 0;
	Vector<uint32>& frequencies = format->fSampleFrequencies;
	if (format->fSampleFrequencyType == 0) { // continuous frequencies
		uint32 min = min_c(frequencies[0], frequencies[1]);
		uint32 max = max_c(frequencies[0], frequencies[1]);

		for (int i = 0; i < frequencies.Count(); i++) {
			if (frequencies[i] < min || frequencies[i] > max)
				continue;
			rates |= GetSamplingRateId(frequencies[i]);
		}
	} else
		for (int i = 0; i < frequencies.Count(); i++)
			rates |= GetSamplingRateId(frequencies[i]);

	return rates;
}


uint32
AudioStreamAlternate::GetFormatId()
{
	TypeIFormatDescriptor* format
		= static_cast<TypeIFormatDescriptor*>(Format());

	if (format == NULL || Interface() == NULL) {
		TRACE_ALWAYS("Ignore alternate due format "
			"%#08x or interface %#08x null.\n", format, Interface());
		return 0;
	}

	uint32 formats = 0;
	switch (Interface()->fFormatTag) {
		case UAF_PCM8: formats = B_FMT_8BIT_U; break;
		case UAF_IEEE_FLOAT: formats = B_FMT_FLOAT; break;
		case UAF_PCM:
			switch(format->fBitResolution) {
				case 8: formats = B_FMT_8BIT_S; break;
				case 16: formats = B_FMT_16BIT; break;
				case 18: formats = B_FMT_18BIT; break;
				case 20: formats = B_FMT_20BIT; break;
				case 24: formats = B_FMT_24BIT; break;
				case 32: formats = B_FMT_32BIT; break;
				default:
					TRACE_ALWAYS("Ignore unsupported "
						"bit resolution %d for alternate.\n",
						format->fBitResolution);
					break;
			}
			break;
		default:
			TRACE_ALWAYS("Ignore unsupported "
				"format bit resolution %d for alternate.\n",
				Interface()->fFormatTag);
			break;
	}
	
	return formats;
}


uint32
AudioStreamAlternate::SamplingRateFromId(uint32 id)
{
	for (size_t i = 0; i < _countof(ratesMap); i++)
		if (ratesMap[i].rateId == id)
			return ratesMap[i].rate;

	TRACE_ALWAYS("Unknown sample rate id: %d.\n", id);
	return 0;
}


status_t
AudioStreamAlternate::SetSamplingRateById(uint32 newId)
{
	return SetSamplingRate(SamplingRateFromId(newId));
}


status_t
AudioStreamAlternate::SetFormatId(uint32 /*newFormatId*/)
{
	return B_OK; // TODO
}


AudioStreamingInterface::AudioStreamingInterface(
		AudioControlInterface*	controlInterface,
		size_t interface, usb_interface_list* List)
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

		usb_interface_info* Interface = &List->alt[alt];

		TRACE_ALWAYS("if[%d]:alt[%d]:descrs_count:%d\n",
			interface, alt, Interface->generic_count);
		for (size_t i = 0; i < Interface->generic_count; i++) {
			usb_audiocontrol_header_descriptor* Header
				= (usb_audiocontrol_header_descriptor*)Interface->generic[i];

			if (Header->descriptor_type == AC_CS_INTERFACE) {
				switch(Header->descriptor_subtype) {
					case UAS_AS_GENERAL:
						if (ASInterface == 0)
							ASInterface = new ASInterfaceDescriptor(
								(usb_as_interface_descriptor_r1*) Header);
						else
							TRACE_ALWAYS("Duplicate AStream interface ignored.\n");
						break;
					case UAS_FORMAT_TYPE:
						if (ASFormat == 0)
							ASFormat = new TypeIFormatDescriptor(
								(usb_type_I_format_descriptor*) Header);
						else
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
	// we own stream header objects too, so free them
	for (Vector<AudioStreamAlternate*>::Iterator I = fAlternates.Begin();
			I != fAlternates.End(); I++)
		delete *I;

	fAlternates.MakeEmpty();
}


uint8
AudioStreamingInterface::TerminalLink()
{
	if (fAlternates[fActiveAlternate]->Interface() != 0)
		return fAlternates[fActiveAlternate]->Interface()->fTerminalLink;
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


void
AudioStreamingInterface::GetFormatsAndRates(multi_description* Description)
{
	Description->interface_flags
		|= fIsInput ? B_MULTI_INTERFACE_RECORD : B_MULTI_INTERFACE_PLAYBACK;

	uint32 rates = fAlternates[fActiveAlternate]->GetSamplingRateIds();
	uint32 formats = fAlternates[fActiveAlternate]->GetFormatId();

	if (fIsInput) {
		Description->input_rates = rates;
		Description->input_formats = formats;
	} else {
		Description->output_rates = rates;
		Description->output_formats = formats;
	}
}

