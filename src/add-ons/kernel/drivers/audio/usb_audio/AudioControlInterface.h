/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _AUDIO_CONTROL_INTERFACE_H_
#define _AUDIO_CONTROL_INTERFACE_H_


#include <util/VectorMap.h>

#include "Driver.h"
#include "USB_audio_spec.h"


class AudioControlInterface;


class AudioChannelCluster {
public:
				AudioChannelCluster();
	virtual		~AudioChannelCluster();

	uint8		ChannelsCount()		{ return fOutChannelsNumber; }
	uint32		ChannelsConfig()	{ return fChannelsConfig;	 }

protected:
	uint8		fOutChannelsNumber;
	uint32		fChannelsConfig;
	uint8		fChannelNames;
};


template< class __base_class_name >
class _AudioChannelCluster
		: public __base_class_name, public AudioChannelCluster {
public:
				_AudioChannelCluster(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header)
							:
							__base_class_name(interface, Header) {}
	virtual		~_AudioChannelCluster() {}

	virtual AudioChannelCluster*
				OutCluster() { return this; }

};


//
// base class for Audio Controls (Units and Terminals)
//
//
class _AudioControl {
public:
							_AudioControl(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~_AudioControl();

		uint8				ID() { return fID; }
		uint8				SourceID() { return fSourceID; }
		uint8				SubType() { return fSubType; }
		status_t			InitCheck() { return fStatus; };
virtual	const char*			Name() { return ""; }
virtual AudioChannelCluster* OutCluster();

protected:
		// state tracking
		status_t			fStatus;
		AudioControlInterface*	fInterface;
		uint8				fSubType;
		uint8				fID;
		uint8				fSourceID;
		uint8				fStringIndex;
};



class _Terminal : public _AudioControl {
public:
							_Terminal(AudioControlInterface* interface,
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

class InputTerminal : public _AudioChannelCluster<_Terminal> {
public:
							InputTerminal(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~InputTerminal();

protected:
};


class OutputTerminal : public _Terminal {
public:
							OutputTerminal(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~OutputTerminal();

protected:
};


class MixerUnit : public _AudioChannelCluster<_AudioControl> {
public:
							MixerUnit(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~MixerUnit();

protected:
		Vector<uint8>		fInputPins;
		Vector<uint8>		fProgrammableControls;
		uint8				fControlsBitmap;
};


class SelectorUnit : public _AudioControl {
public:
							SelectorUnit(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~SelectorUnit();

virtual AudioChannelCluster* OutCluster();
// protected:
		Vector<uint8>		fInputPins;
		uint8				fControlsBitmap;
};


class FeatureUnit : public _AudioControl {
public:
							FeatureUnit(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~FeatureUnit();

virtual	const char*			Name();
		bool				HasControl(int32 Channel, uint32 Control);

// protected:
		void				NormalizeAndTraceChannel(int32 Channel);

		Vector<uint32>		fControlBitmaps;
};


class EffectUnit : public _AudioControl/*, public _AudioChannelsCluster*/ {
public:
							EffectUnit(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~EffectUnit();

protected:
/*		uint16				fProcessType;
		Vector<uint8>		fInputPins;
		uint8				fControlsBitmap;
		Vector<uint16>		fModes;
*/};


class ProcessingUnit : public _AudioChannelCluster<_AudioControl> {
public:
							ProcessingUnit(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~ProcessingUnit();

protected:
		uint16				fProcessType;
		Vector<uint8>		fInputPins;
		uint8				fControlsBitmap;
		Vector<uint16>		fModes;
};


class ExtensionUnit : public _AudioChannelCluster<_AudioControl> {
public:
							ExtensionUnit(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~ExtensionUnit();

protected:
		uint16				fExtensionCode;
		Vector<uint8>		fInputPins;
		uint8				fControlsBitmap;
};


class ClockSource : public _AudioControl {
public:
							ClockSource(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~ClockSource();

protected:
};


class ClockSelector : public _AudioControl {
public:
							ClockSelector(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~ClockSelector();

protected:
};


class ClockMultiplier : public _AudioControl {
public:
							ClockMultiplier(AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~ClockMultiplier();

protected:
};


class SampleRateConverter : public _AudioControl {
public:
							SampleRateConverter(
								AudioControlInterface* interface,
								usb_audiocontrol_header_descriptor* Header);
		virtual				~SampleRateConverter();

protected:
};


typedef	VectorMap<uint32, _AudioControl*>			AudioControlsMap;
typedef	VectorMap<uint32, _AudioControl*>::Iterator	AudioControlsIterator;

typedef	Vector<_AudioControl*>			AudioControlsVector;
// typedef	Vector<_AudioControl*>::Iterator	AudioControlsIterator;

class Device;

class AudioControlInterface {
public:
						AudioControlInterface(Device* device);
						~AudioControlInterface();

		status_t		InitCheck() { return fStatus; }
		status_t		Init(size_t interface, usb_interface_info *Interface);

		_AudioControl*	Find(uint8 id);
		_AudioControl*	FindOutputTerminal(uint8 id);
		uint16			SpecReleaseNumber() { return fADCSpecification; }

		AudioControlsMap&	Controls() { return fAudioControls; }

		uint32			GetChannelsDescription(
								Vector<multi_channel_info>& Channels,
								multi_description *Description,
								AudioControlsVector &USBTerminals);
		uint32			GetBusChannelsDescription(
								Vector<multi_channel_info>& Channels,
								multi_description *Description);

		status_t		GetMix(multi_mix_value_info *Info);
		status_t		SetMix(multi_mix_value_info *Info);
		status_t		ListMixControls(multi_mix_control_info* Info);

protected:
		status_t		InitACHeader(size_t interface,
								usb_audiocontrol_header_descriptor* Header);

		uint32			GetTerminalChannels(
								Vector<multi_channel_info>& Channels,
								AudioChannelCluster* cluster,
								channel_kind kind, uint32 connectors = 0);

		void			_HarvestRecordFeatureUnits(_AudioControl* rootControl,
								AudioControlsMap& Map);
		void			_ListMixControlsPage(int32& index,
								multi_mix_control_info* Info,
								AudioControlsMap& Map, const char* Name);
		int32			_ListFeatureUnitControl(int32& index, int32 parentId,
								multi_mix_control_info* Info,
								_AudioControl* control);
		uint32			_ListFeatureUnitOption(uint32 controlType,
								int32& index, int32 parentIndex,
								multi_mix_control_info* Info, FeatureUnit* unit,
								uint32 channel, uint32 channels);
		void			_ListSelectorUnitControl(int32& index, int32 parentGroup,
								multi_mix_control_info* Info,
								_AudioControl* control);
		void			_InitGainLimits(multi_mix_control& Control);

		size_t			fInterface;
		status_t		fStatus;
		// part of AudioControl Header description
		uint16			fADCSpecification;
		Vector<uint8>	fStreams;
		uint8			fFunctionCategory;
		uint8			fControlsBitmap;
		Device*			fDevice;

		// map to store all controls and lookup by control ID
		AudioControlsMap	fAudioControls;
		// map to store output terminal and lookup them by source ID
		AudioControlsMap	fOutputTerminals;
		// map to store output terminal and lookup them by control ID
		AudioControlsMap	fInputTerminals;
};

#endif // _AUDIO_CONTROL_INTERFACE_H_

