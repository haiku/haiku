/*
 *	SiS 7018, Trident 4D Wave DX/NX, Acer Lab M5451 Sound Driver.
 *	Copyright (c) 2002, 2008-2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _SiS7018_STREAM_H_
#define _SiS7018_STREAM_H_


#include "hmulti_audio.h"


class Device;

class Stream {
public:
							Stream(Device* device, bool isInput);
							~Stream();

		status_t			Init();
		void				Free();
		status_t			InitCheck() { return fStatus; }

		status_t			Start();
		status_t			Stop();
		bool				IsActive() { return fIsActive; }

		void				GetBuffers(uint32& Flags,
										int32& BuffersCount,
										int32& ChannelsCount,
										uint32& BufferSize,
										buffer_desc** Buffers);

		void				ExchangeBuffers(bigtime_t& RealTime,
											bigtime_t& FramesCount,
											int32& BufferCycle);

		void				GetFormat(multi_format_info *Format);
		status_t			SetFormat(_multi_format& format,
											uint32 formats, uint32 rates);

		bool				InterruptHandler();

private:

		uint32				_DecodeRate(uint32 rate);
inline	bool				_UseBankB() { return (fHWChannel & 0x20) == 0x20; }
inline	uint32				_HWVoice() { return fHWChannel & 0x1f; }
inline	uint32				_HWId();

		Device*				fDevice;
		status_t			fStatus;
		bool				fIsInput;
		bool				fIsActive;
		uint32				fHWChannel;

		area_id				fBuffersArea;
		size_t				fBuffersAreaSize;
		size_t				fBufferSamplesCount;
		void*				fBuffersAddress;
		phys_addr_t			fBuffersPhysAddress;

		_multi_format		fFormat;
		bigtime_t			fRealTime;
		bigtime_t			fFramesCount;
		int32				fBufferCycle;
};


#endif // _SiS7018_STREAM_H_

