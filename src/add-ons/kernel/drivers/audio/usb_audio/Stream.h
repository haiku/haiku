/*
 *	Driver for USB Audio Device Class devices.
 *	Copyright (c) 2009,10,12 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _STREAM_H_
#define _STREAM_H_


#include <OS.h>

#include "AudioStreamingInterface.h"

class Device;

// typedef	Vector<AudioStreamAlternate*>			StreamAlternatesVector;
// typedef	Vector<AudioStreamAlternate*>::Iterator	StreamAlternatesIterator;

class Stream : public AudioStreamingInterface {
		friend		class	Device;
public:
							Stream(Device* device, size_t interface,
								usb_interface_list *List
								/*, bool isInput, uint32 HWChannel*/);
							~Stream();
		status_t			Init();
		status_t			InitCheck() { return fStatus; }

		status_t			Start();
		status_t			Stop();
		bool				IsRunning() { return fIsRunning; }

		status_t			GetBuffers(multi_buffer_list* List);

		status_t			OnSetConfiguration(usb_device device,
								const usb_configuration_info *config);

		bool				ExchangeBuffer(multi_buffer_info* Info);
/*
		int32				InterruptHandler(uint32 SignaledChannelsMask);
*/
//		ASInterfaceDescriptor*	ASInterface();
//		_ASFormatDescriptor*	ASFormat();
		status_t			GetEnabledChannels(uint32& offset,
												multi_channel_enable *Enable);
		status_t			SetEnabledChannels(uint32& offset,
												multi_channel_enable *Enable);
		status_t			GetGlobalFormat(multi_format_info *Format);
		status_t			SetGlobalFormat(multi_format_info *Format);


protected:

		Device*				fDevice;
		status_t			fStatus;

		// alternates of the streams
//		StreamAlternatesVector	fAlternates;
//		size_t				fActiveAlternate;

		uint8				fTerminalID;
		usb_pipe			fStreamEndpoint;
		bool				fIsRunning;
/*		uint32				fHWChannel;*/
		area_id				fArea;
		usb_iso_packet_descriptor*	fDescriptors;
		size_t						fDescriptorsCount;
		size_t				fCurrentBuffer;
		uint32				fStartingFrame;
		size_t				fSamplesCount;
		int32				fProcessedBuffers;
//		void*				fBuffersPhysAddress;
/*		bigtime_t			fRealTime;
		bigtime_t			fFramesCount;
		int32				fBufferCycle;
public:
		uint32				fCSP; */
private:
		status_t			_QueueNextTransfer(size_t buffer);
static	void				_TransferCallback(void *cookie, int32 status,
								void *data, uint32 actualLength);
		void				_DumpDescriptors();
};

/*
class RecordStream : public Stream {
public:
							RecordStream(Device* device, uint32 HWChannel);
							~RecordStream();
		status_t			Start();
};
*/

#endif // _STREAM_H_

