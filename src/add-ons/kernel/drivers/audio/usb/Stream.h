/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009-13 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _USB_AUDIO_STREAM_H_
#define _USB_AUDIO_STREAM_H_


#include "AudioStreamingInterface.h"


class Device;

class Stream : public AudioStreamingInterface {
	friend	class			Device;
public:
							Stream(Device* device, size_t interface,
								usb_interface_list* List);
							~Stream();

			status_t		Init();
			status_t		InitCheck() { return fStatus; }

			status_t		Start();
			status_t		Stop();
			bool			IsRunning() { return fIsRunning; }
			void			OnRemove();

			status_t		GetBuffers(multi_buffer_list* List);

			status_t		OnSetConfiguration(usb_device device,
							const usb_configuration_info* config);

			bool			ExchangeBuffer(multi_buffer_info* Info);
			status_t		GetEnabledChannels(uint32& offset,
								multi_channel_enable* Enable);
			status_t		SetEnabledChannels(uint32& offset,
								multi_channel_enable* Enable);
			status_t		GetGlobalFormat(multi_format_info* Format);
			status_t		SetGlobalFormat(multi_format_info* Format);

protected:

			Device*			fDevice;
			status_t		fStatus;

			uint8			fTerminalID;
			usb_pipe		fStreamEndpoint;
			bool			fIsRunning;
			area_id			fArea;
			size_t			fAreaSize;
			usb_iso_packet_descriptor*	fDescriptors;
			size_t			fDescriptorsCount;
			size_t			fCurrentBuffer;
			uint32			fStartingFrame;
			size_t			fSamplesCount;
			size_t			fPacketSize;
			int32			fProcessedBuffers;
			int32			fInsideNotify;

private:
			status_t		_ChooseAlternate();
			status_t		_SetupBuffers();
			status_t		_QueueNextTransfer(size_t buffer, bool start);
	static	void			_TransferCallback(void* cookie, status_t status,
								void* data, size_t actualLength);
			void			_DumpDescriptors();
};


#endif // _USB_AUDIO_STREAM_H_

