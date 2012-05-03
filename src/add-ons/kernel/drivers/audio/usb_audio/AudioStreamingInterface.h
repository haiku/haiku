/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _AUDIO_STREAMING_INTERFACE_H_
#define _AUDIO_STREAMING_INTERFACE_H_


#include "Driver.h"
#include "AudioControlInterface.h"

#include <util/VectorMap.h>
#include "USB_audio_spec.h"


//
// Audio Streaming Interface information entities
//
//
class ASInterfaceDescriptor /*: public _AudioFunctionEntity*/ {
public:
							ASInterfaceDescriptor(/*Device* device, size_t interface,*/
								usb_as_interface_descriptor_r1*	Descriptor);
							~ASInterfaceDescriptor();

// protected:
		uint8				fTerminalLink;
		uint8				fDelay;
		uint16				fFormatTag;
};


class ASEndpointDescriptor /*: public _AudioFunctionEntity*/ {
public:
							ASEndpointDescriptor(
								usb_endpoint_descriptor* Endpoint,
								usb_as_cs_endpoint_descriptor* Descriptor);
							~ASEndpointDescriptor();

// protected:
		uint8				fAttributes;
		uint8				fLockDelayUnits;
		uint16				fLockDelay;
		uint16				fMaxPacketSize;
		uint8				fEndpointAddress;
};


class _ASFormatDescriptor /*: public _AudioFunctionEntity*/ {
public:
							_ASFormatDescriptor(
									usb_type_I_format_descriptor* Descriptor);
		virtual				~_ASFormatDescriptor();

// protected:
		uint8				fFormatType;
		uint32				GetSamFreq(uint8* freq);
};


class TypeIFormatDescriptor : public _ASFormatDescriptor {
public:
							TypeIFormatDescriptor(/*Device* device, size_t interface,*/ 
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
							TypeIIFormatDescriptor(/*Device* device, size_t interface,*/ 
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
							TypeIIIFormatDescriptor(/*Device* device, size_t interface, */
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
		ASEndpointDescriptor*	Endpoint()	{ return fEndpoint;	 }
		_ASFormatDescriptor*	Format()	{ return fFormat;	 }

protected:
		size_t					fAlternate;
		ASInterfaceDescriptor*	fInterface;
		ASEndpointDescriptor*	fEndpoint;
		_ASFormatDescriptor*	fFormat;
};


typedef	Vector<AudioStreamAlternate*>			StreamAlternatesVector;
typedef	Vector<AudioStreamAlternate*>::Iterator	StreamAlternatesIterator;


class AudioStreamingInterface {
public:
						AudioStreamingInterface(
								AudioControlInterface*	controlInterface,
								size_t interface, usb_interface_list *List);
						~AudioStreamingInterface();

	//	status_t		InitCheck() { return fStatus; }
		uint8			TerminalLink();
		bool			IsInput() { return fIsInput; }

		AudioChannelCluster* ChannelCluster();

		void GetFormatsAndRates(multi_description *Description);

protected:
		size_t				fInterface;
		AudioControlInterface*	fControlInterface;

	//	status_t			fStatus;
		bool			fIsInput;
		// alternates of the streams
		StreamAlternatesVector	fAlternates;
		size_t				fActiveAlternate;
};


#endif // _AUDIO_STREAMING_INTERFACE_H_

