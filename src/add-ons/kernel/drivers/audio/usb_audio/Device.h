/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _USB_AUDIO_DEVICE_H_
#define _USB_AUDIO_DEVICE_H_


#include "Driver.h"
#include "USB_audio_spec.h"
#include "AudioControlInterface.h"
#include "AudioStreamingInterface.h"
#include "Stream.h"

// typedef	VectorMap<uint32, _AudioControl*>			AudioControlsMap;
// typedef	VectorMap<uint32, _AudioControl*>::Iterator	AudioControlsIterator;

typedef Vector<Stream*>				AudioStreamsVector;
typedef Vector<Stream*>::Iterator	AudioStreamsIterator;


class FeatureUnit;

class Device {
		friend	class FeatureUnit;
		friend	class Stream;
public:
							Device(usb_device device);
		virtual				~Device();

		status_t			InitCheck() { return fStatus; };

		status_t			Open(uint32 flags);
		bool				IsOpen() { return fOpen; };

		status_t			Close();
		status_t			Free();

		status_t			Read(uint8 *buffer, size_t *numBytes);
		status_t			Write(const uint8 *buffer, size_t *numBytes);
		status_t			Control(uint32 op, void *buffer, size_t length);

		void				Removed();
		bool				IsRemoved() { return fRemoved; };

		status_t			CompareAndReattach(usb_device device);
virtual	status_t			SetupDevice(bool deviceReplugged);
//		uint16				SpecReleaseNumber();
		
//		_AudioControl*		FindAudioControl(uint8 ID);
		usb_device			USBDevice() { return fDevice; }

		AudioControlInterface&	AudioControl() { return fAudioControl; }

private:
static	void				_ReadCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);
static	void				_WriteCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);
static	void				_NotifyCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);

		status_t			_SetupEndpoints();
//		void				ParseAudioControlInterface(usb_device device,
//									size_t interface, usb_interface_info *Interface);
//		void				ParseAudioStreamingInterface(usb_device device,
//									size_t interface, usb_interface_list *List);

protected:
virtual	status_t			StartDevice() { return B_OK; }
virtual	status_t			StopDevice();

		void				TraceMultiDescription(multi_description *Description,
										Vector<multi_channel_info>& Channels);
		void				TraceListMixControls(multi_mix_control_info *Info);
		// state tracking
		status_t			fStatus;
		bool				fOpen;
		bool				fRemoved;
		vint32				fInsideNotify;
		usb_device			fDevice;
		uint16				fVendorID;
		uint16				fProductID;
const	char *				fDescription;
		bool				fNonBlocking;

		AudioControlInterface	fAudioControl;
/*
		AudioControlHeader*	fAudioControlHeader;
		// map to store all controls and lookup by control ID
		AudioControlsMap	fAudioControls;
		// map to store output terminal and lookup them by source ID
		AudioControlsMap	fOutputTerminals;
		// map to store output terminal and lookup them by control ID
		AudioControlsMap	fInputTerminals;
*/
		// vector of audio streams
		AudioStreamsVector	fStreams;

protected:
		status_t			_MultiGetDescription(multi_description *Description);
		status_t			_MultiGetEnabledChannels(multi_channel_enable *Enable);
		status_t			_MultiSetEnabledChannels(multi_channel_enable *Enable);
		status_t			_MultiGetBuffers(multi_buffer_list* List);
		status_t			_MultiGetGlobalFormat(multi_format_info *Format);
		status_t			_MultiSetGlobalFormat(multi_format_info *Format);
		status_t			_MultiGetMix(multi_mix_value_info *Info);
		status_t			_MultiSetMix(multi_mix_value_info *Info);
		status_t			_MultiListMixControls(multi_mix_control_info* Info);
		status_t			_MultiBufferExchange(multi_buffer_info* Info);
		status_t			_MultiBufferForceStop();

		// interface and device infos
//		uint16				fFrameSize;

		// pipes for notifications and data io
		usb_pipe			fControlEndpoint;
		usb_pipe			fInStreamEndpoint;
		usb_pipe			fOutStreamEndpoint;

		// data stores for async usb transfers
		uint32				fActualLengthRead;
		uint32				fActualLengthWrite;
		int32				fStatusRead;
		int32				fStatusWrite;
		sem_id				fNotifyReadSem;
		sem_id				fNotifyWriteSem;

		uint8 *				fNotifyBuffer;
		uint32				fNotifyBufferLength;

		sem_id				fBuffersReadySem;
};


#endif // _USB_AUDIO_DEVICE_H_

