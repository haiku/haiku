/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _AUDIO_FUNCTION_H_
#define _AUDIO_FUNCTION_H_


#include <VectorMap.h>

#include "Driver.h"
#include "USB_audio_spec.h"


class Device;

//
// base class for all entities in Audio Function
//
//
class _AudioFunctionEntity {
public:
							_AudioFunctionEntity(Device* device, size_t interface);
							~_AudioFunctionEntity();

		status_t			InitCheck() { return fStatus; };

protected:
		// state tracking
		status_t			fStatus;
		Device*				fDevice;
		size_t				fInterface;
};


class _AudioChannelsCluster {
public:
							_AudioChannelsCluster();
		virtual				~_AudioChannelsCluster();

virtual _AudioChannelsCluster* OutputCluster();

protected:
		uint8				fOutChannelsNumber;
		uint32				fChannelsConfig;
		uint8				fChannelNames;
};


//
// base class for Audio Controls (Units and Terminals)
//
//
class _AudioControl : public _AudioFunctionEntity {
public:
							_AudioControl(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~_AudioControl();

		uint8				ID() { return fID; }
		uint8				SourceID() { return fSourceID; }
		uint8				SubType() { return fSubType; }
virtual	const char*			Name() { return ""; }
virtual _AudioChannelsCluster* OutputCluster();

protected:
		// state tracking
		uint8				fSubType;
		uint8				fID;
		uint8				fSourceID;
		uint8				fStringIndex;
};


class AudioControlHeader : public _AudioControl {
public:
							AudioControlHeader(Device* device, size_t interface, 
								usb_audiocontrol_header_descriptor* Header);
		virtual				~AudioControlHeader();

// protected:
		uint16				fADCSpecification;
		Vector<uint8>		fStreams;
		uint8				fFunctionCategory;
		uint8				fControlsBitmap;
};


class _Terminal : public _AudioControl {
public:
							_Terminal(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~_Terminal();

		uint16				TerminalType() { return fTerminalType; }
		bool				IsUSBIO();
virtual	const char*			Name();

protected:
		uint16				fTerminalType;
		uint8				fAssociatedTerminal;
		uint8				fClockSourceId;
		uint16				fControlsBitmap;
};


class InputTerminal : public _Terminal, public _AudioChannelsCluster {
public:
							InputTerminal(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~InputTerminal();

protected:
};


class OutputTerminal : public _Terminal {
public:
							OutputTerminal(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~OutputTerminal();

protected:
};


class MixerUnit : public _AudioControl, public _AudioChannelsCluster {
public:
							MixerUnit(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~MixerUnit();

protected:
		Vector<uint8>		fInputPins;
		Vector<uint8>		fProgrammableControls;
		uint8				fControlsBitmap;
};


class SelectorUnit : public _AudioControl {
public:
							SelectorUnit(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~SelectorUnit();

protected:
		Vector<uint8>		fInputPins;
		uint8				fControlsBitmap;
};


class FeatureUnit : public _AudioControl {
public:
							FeatureUnit(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~FeatureUnit();

virtual	const char*			Name();
		bool				HasControl(int32 Channel, uint32 Control);

protected:
		void				TraceChannel(Device* device, int32 Channel);

		Vector<uint32>		fControlBitmaps;
};


class EffectUnit : public _AudioControl/*, public _AudioChannelsCluster*/ {
public:
							EffectUnit(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~EffectUnit();

protected:
/*		uint16				fProcessType;
		Vector<uint8>		fInputPins;
		uint8				fControlsBitmap;
		Vector<uint16>		fModes;
*/};


class ProcessingUnit : public _AudioControl, public _AudioChannelsCluster {
public:
							ProcessingUnit(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~ProcessingUnit();

protected:
		uint16				fProcessType;
		Vector<uint8>		fInputPins;
		uint8				fControlsBitmap;
		Vector<uint16>		fModes;
};


class ExtensionUnit : public _AudioControl, public _AudioChannelsCluster {
public:
							ExtensionUnit(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~ExtensionUnit();

protected:
		uint16				fExtensionCode;
		Vector<uint8>		fInputPins;
		uint8				fControlsBitmap;
};


class ClockSource : public _AudioControl {
public:
							ClockSource(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~ClockSource();

protected:
};


class ClockSelector : public _AudioControl {
public:
							ClockSelector(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~ClockSelector();

protected:
};


class ClockMultiplier : public _AudioControl {
public:
							ClockMultiplier(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~ClockMultiplier();

protected:
};


class SampleRateConverter : public _AudioControl {
public:
							SampleRateConverter(Device* device, size_t interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~SampleRateConverter();
protected:
};


//
// Audio Streaming Interface information entities
//
//
class ASInterfaceDescriptor : public _AudioFunctionEntity {
public:
							ASInterfaceDescriptor(Device* device, size_t interface,
								usb_as_interface_descriptor_r1*	Descriptor);
							~ASInterfaceDescriptor();
// protected:
		uint8				fTerminalLink;
		uint8				fDelay;
		uint16				fFormatTag;
};


class ASEndpointDescriptor : public _AudioFunctionEntity {
public:
							ASEndpointDescriptor(Device* device, size_t interface,
								usb_as_cs_endpoint_descriptor* Descriptor);
							~ASEndpointDescriptor();
// protected:
		uint8				fAttributes;
		uint8				fLockDelayUnits;
		uint16				fLockDelay;
};


class _ASFormatDescriptor : public _AudioFunctionEntity {
public:
							_ASFormatDescriptor(Device* device, size_t interface);
		virtual				~_ASFormatDescriptor();

// protected:
		uint32				GetSamFreq(uint8* freq);
};


class TypeIFormatDescriptor : public _ASFormatDescriptor {
public:
							TypeIFormatDescriptor(Device* device, size_t interface,
								usb_type_I_format_descriptor* Descriptor);
		virtual				~TypeIFormatDescriptor();

		status_t			Init(usb_type_I_format_descriptor* Descriptor);

// protected:
		uint8				fNumChannels;
		uint8				fSubframeSize;
		uint8				fBitResolution;
		uint8				fSampleFrequencyType;
		Vector<uint32>		fSampleFrequencies;
};


class TypeIIFormatDescriptor : public _ASFormatDescriptor {
public:
							TypeIIFormatDescriptor(Device* device, size_t interface,
								usb_type_II_format_descriptor* Descriptor);
		virtual				~TypeIIFormatDescriptor();

// protected:
		uint16				fMaxBitRate;
		uint16				fSamplesPerFrame;
		uint8				fSampleFrequencyType;
		Vector<uint32>		fSampleFrequencies;
};


class TypeIIIFormatDescriptor : public TypeIFormatDescriptor {
public:
							TypeIIIFormatDescriptor(Device* device, size_t interface, 
								usb_type_III_format_descriptor* Descriptor);
		virtual				~TypeIIIFormatDescriptor();

// protected:
};


class AudioStreamAlternate {
public:
						AudioStreamAlternate(size_t alternate,
										ASInterfaceDescriptor* interface,
										ASEndpointDescriptor* endpoint,
										_ASFormatDescriptor* format);
						~AudioStreamAlternate();

		ASInterfaceDescriptor*	Interface()	{ return fInterface; }
		ASEndpointDescriptor*	Endpoint()	{ return fEndpoint;  }
		_ASFormatDescriptor*	Format()	{ return fFormat;	 }

protected:
		size_t					fAlternate;
		ASInterfaceDescriptor*	fInterface;
		ASEndpointDescriptor*	fEndpoint;
		_ASFormatDescriptor*	fFormat;
};

#endif // _AUDIO_FUNCTION_H_

